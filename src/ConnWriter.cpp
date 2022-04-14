#include "ConnWriter.h"

#include <unistd.h>
#include <Sunnet.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>

void ConnWriter::EntireWrite(std::shared_ptr<char> buff, std::streamsize len) {
    if(isClosing) {
        std::cout << "EntireWrite fail, isClosing" << std::endl;
        return;
    }
    if(objs.empty()) // 前面缓冲区没有没写完的数据，先尝试写入
        EntireWriteWhenEmpty(buff, len);
    else  // 前面缓冲区有没写完的数据，buff数据添加到这些带写入数据后面
        EntireWriteWhenNotEmpty(buff, len);
}

void ConnWriter::EntireWriteWhenEmpty(std::shared_ptr<char> buff, std::streamsize len) {
    char* s = buff.get();
    std::streamsize n = write(fd, s, len);
    
    if(n < 0 && errno == EINTR) {} // ？？？

    std::cout << "EntireWriteWhenEmpty write n = " << n << std::endl;

    if(n >= 0 && n == len) // 全部写完，不用应用层缓冲
        return;
    
    if((n > 0 && n < len) || (n < 0 && errno == EAGAIN)) { // 写入了一部分，或者都没写入，加到缓冲区
        std::shared_ptr<WriteObject> obj = std::make_shared<WriteObject>();
        obj->start = n;
        obj->buff = buff;
        obj->len = len;
        objs.push_back(obj);
        Sunnet::inst->ModifyEvent(fd, true); // 激活epoll_event对可写的实践监督
        return;
    }

    // 写入发生错误
    std::cout << "EntireWrite write error: " << strerror(errno) << std::endl;
}

void ConnWriter::EntireWriteWhenNotEmpty(std::shared_ptr<char> buff, std::streamsize len) {
    std::shared_ptr<WriteObject> obj = std::make_shared<WriteObject>();
    obj->start = 0;
    obj->buff = buff;
    obj->len = len;
    objs.push_back(obj);
}


// 可写时
void ConnWriter::OnWritable() {
    std::shared_ptr<Conn> conn = Sunnet::inst->GetConn(fd);
    if(conn == nullptr) // 连接已经关闭
        return;
    
    while(WriteFrontObj()) {

    }

    if(objs.empty()) { // 如果不空，则还有东西没写完，就不去改变epoll_event
        Sunnet::inst->ModifyEvent(fd, false); // 停止监听可写事件

        if(isClosing) { // ？？？？什么延迟关闭做法
            std::cout << "linger close conn" << std::endl;
            shutdown(fd, SHUT_RD);
            auto msg= std::make_shared<SocketRWMsg>();
            msg->type = BaseMsg::TYPE::SOCKET_RW;
            msg->fd = conn->fd;
            msg->isRead = true;
            Sunnet::inst->Send(conn->serviceId, msg);
        }
    }
}

bool ConnWriter::WriteFrontObj() {
    if(objs.empty()) // 缓冲区空，没有要写入的数据
        return false;

    std::shared_ptr<WriteObject> obj = objs.front();

    char* s = obj->buff.get() + obj->start;
    int len = obj->len - obj->start;
    int n = write(fd, s, len);
    std::cout << "WriteFrontObj write n = " << n << std::endl;
    
    if(n < 0 && errno == EINTR) {} // ？？？
    
    if(n >= 0 && n == len) { // obj全写完了，返回ture继续WriteFrontObj
        objs.pop_front();
        return true;
    }

    if((n > 0 && n < len) || (n < 0 && errno == EAGAIN)) {
        obj->start += n;
        return false; 
        // 写了一半，可能socket缓冲区又满了，直接不写了。return后的逻辑没改epoll_event，再次等待下次可写事件
    }

    std::cout << "EntireWrite write error: " << strerror(errno) << std::endl;
    return false;
}

void ConnWriter::LingerClose(){
    if(isClosing){
        return;
    }
    isClosing = true;
    if(objs.empty()) {
        Sunnet::inst->CloseConn(fd);
        return;
    }
}
