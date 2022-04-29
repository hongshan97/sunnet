#include "Service.h"
#include "Sunnet.h"
#include "LuaAPI.h"

#include <iostream>
#include <unistd.h>
#include <string.h>

Service::Service() {
    pthread_spin_init(&queueLock, PTHREAD_PROCESS_PRIVATE); // 初始化锁
    pthread_spin_init(&inGlobalLock, PTHREAD_PROCESS_PRIVATE);
}

Service::~Service() {
    pthread_spin_destroy(&queueLock);
    pthread_spin_destroy(&inGlobalLock);
}

// 插入消息
void Service::PushMsg(std::shared_ptr<BaseMsg> msg) {
    pthread_spin_lock(&queueLock); // 加锁

    msgQueue.push(msg);

    pthread_spin_unlock(&queueLock); // 解锁
}

// 取出消息
std::shared_ptr<BaseMsg> Service::PopMsg() {
    std::shared_ptr<BaseMsg> msg = nullptr;
    
    pthread_spin_lock(&queueLock);

    if(!msgQueue.empty()) {
        msg = msgQueue.front();
        msgQueue.pop();
    }

    pthread_spin_unlock(&queueLock);

    return msg;
}

// 创建服务后触发
void Service::OnInit() {
    std::cout << '[' << id << ']' << " OnInit" << std::endl;

    // 新建lua虚拟机
    luaState = luaL_newstate();
    luaL_openlibs(luaState);

    // 注册sunnet系统api到lua虚拟机
    LuaAPI::Register(luaState);

    // 执行lua文件
    std::string filename = "../service/" + *type + "/init.lua";
    int isok = luaL_dofile(luaState, filename.data());
    if(isok == 0)
        std::cout << "run service " << *type << " init  success" << std::endl;
    else 
        std::cout << "run service " << *type << " init  fail" << lua_tostring(luaState, -1) << std::endl;
    
    // 调用lua函数
    lua_getglobal(luaState, "OnInit");
    lua_pushinteger(luaState, id);
    isok = lua_pcall(luaState, 1, 0, 0);
    if(isok == 0)
        std::cout << "call service " << *type << " init  success" << std::endl;
    else 
        std::cout << "call service " << *type << " init  fail" << lua_tostring(luaState, -1) << std::endl;
    
}

// 收到消息后触发
void Service::OnMsg(std::shared_ptr<BaseMsg> msg) {
    // if(msg->type == BaseMsg::TYPE::SERVICE) { // 收到其他服务发来的消息
    //     std::shared_ptr<ServiceMsg> m = std::dynamic_pointer_cast<ServiceMsg>(msg);
    //     std::cout << '[' << id << "] OnMsg   " << m->buff << std::endl;

    //     std::shared_ptr<BaseMsg> msgRet = Sunnet::inst->MakeMsg(id, new char[9999999]{'w', 'o', '\0'}, 9999999);

    //     Sunnet::inst->Send(m->source, msgRet);
    // }
    // else if(msg->type == BaseMsg::TYPE::SOCKET_ACCEPT) { // 收到客户端建立连接请求的消息
    //     std::shared_ptr<SocketAcceptMsg> m = std::dynamic_pointer_cast<SocketAcceptMsg>(msg);
    //     std::cout << "new conn" << m->clientFd << std::endl; // 这里本服务可以存储这个fd，有需要了话可以主动给客户端发消息
    // }
    // else if(msg->type == BaseMsg::TYPE::SOCKET_RW) { // 收到已建立连接conn的可读可写消息
    //     std::shared_ptr<SocketRWMsg> m = std::dynamic_pointer_cast<SocketRWMsg>(msg);
    //     if(m->isRead) { // 可读
    //         char buff[512];
    //         int len = read(m->fd, &buff, 512);
    //         if(len > 0) {
    //             write(m->fd, &buff, len);
    //         }
    //         else {
    //             std::cout << "close " << m->fd << " errno: " << strerror(errno) << std::endl;
    //         }
    //     }
    // }
    // else {
    //     std::cout << '[' << id << ']' << " OnMsg" << std::endl;
    // }
    if(msg->type == BaseMsg::TYPE::SERVICE) { // 收到其他服务发来的消息
        std::shared_ptr<ServiceMsg> m = std::dynamic_pointer_cast<ServiceMsg>(msg);
        OnServiceMsg(m);
    }
    else if(msg->type == BaseMsg::TYPE::SOCKET_ACCEPT) { // 收到客户端建立连接请求的消息
        std::shared_ptr<SocketAcceptMsg> m = std::dynamic_pointer_cast<SocketAcceptMsg>(msg);
        OnAcceptMsg(m);
    }
    else if(msg->type == BaseMsg::TYPE::SOCKET_RW) { // 收到已建立连接conn的可读可写消息
        std::shared_ptr<SocketRWMsg> m = std::dynamic_pointer_cast<SocketRWMsg>(msg);
        OnRWMsg(m);
    }
    else {
        std::cout << '[' << id << ']' << " OnMsg" << std::endl;
    }
}

void Service::OnServiceMsg(std::shared_ptr<ServiceMsg> msg) {
    std::cout << "OnServiceMsg" << std::endl;

    // 调用lua的OnServiceMsg
    lua_getglobal(luaState, "OnServiceMsg");
    lua_pushinteger(luaState, msg->source);
    lua_pushlstring(luaState, msg->buff.get(), msg->size);
    int isok = lua_pcall(luaState, 2, 0, 0);
    if(isok != 0)
        std::cout << "call lua OnServiceMsg fail" << lua_tostring(luaState, -1) << std::endl;
}

void Service::OnAcceptMsg(std::shared_ptr<SocketAcceptMsg> msg) {
    std::cout << "OnAcceptMsg" << std::endl;
}

void Service::OnRWMsg(std::shared_ptr<SocketRWMsg> msg) {
    std::cout << "OnRWMsg" << std::endl;
    int fd = msg->fd;
    if(msg->isRead) {
        const int BUFFSIZE = 512;
        char buff[BUFFSIZE];
        int len = 0;
        do {
            len = read(fd, buff, BUFFSIZE);
            if(len > 0)
                OnSocketData(fd, buff, len);
        } while(len == BUFFSIZE);

        if(len <= 0 && errno != EAGAIN) {
            if(Sunnet::inst->GetConn(fd)) { // 防止多次释放
                OnSocketClose(fd);
                Sunnet::inst->CloseConn(fd);
            }
        }
    }
    
    if(msg->isWrite) {
        if(Sunnet::inst->GetConn(fd)) // 如果在write以上的逻辑中关闭了连接或有其他错误，则导致无法写入，这里需要判断连接存在否再读取
            OnSocketWritable(fd);
    }
}

void Service::OnSocketData(int fd, const char* buff, int len) {
    std::cout << "OnSocketData " << fd << " buff: " << buff << std::endl;

    /*sleep(10); // 在此sleep期间退出客户端连接，RST，产生PIPE信号
    std::cout << "发送" << std::endl;
    if(len > 0)
        write(fd, buff, len);
    sleep(2);
    std::cout << "发送" << strerror(errno) << std::endl;*/ // cilent关闭后第一次write，ser端的socket还没销毁，所以write成功

    // ECHO
    // if(len > 0)
    //     write(fd, buff, len);
    // std::cout << "发送" << strerror(errno) << std::endl; // client 关闭第二次write，收到Broken PIPE
}

void Service::OnSocketWritable(int fd) {
    std::cout << "OnSocketWritable " << fd << std::endl;
}

void Service::OnSocketClose(int fd) {
    std::cout << "OnSocketClose " << fd << std::endl;
}

// 退出服务时触发
void Service::OnExit() {
    std::cout << '[' << id << ']' << " OnExit" << std::endl;

    // 调用lua函数
    lua_getglobal(luaState, "OnExit");
    int isok = lua_pcall(luaState, 0, 0, 0);
    if(isok == 0)
        std::cout << "call lua OnExit success" << std::endl;
    else 
        std::cout << "call lua OnExit fail" << lua_tostring(luaState, -1) << std::endl;
    
    // 关闭lua虚拟机
    lua_close(luaState);
}

// 处理一条消息，
bool Service::ProcessMsg() {
    std::shared_ptr<BaseMsg> msg = PopMsg();
    if(msg) {
        OnMsg(msg);
        return true;
    }
    else {
        return false;
    }
}

// 处理N条消息
void Service::ProcessMsgs(int max) {
    for(int i = 0; i < max; ++i) {
        bool succ = ProcessMsg();
        if(!succ)
            break;
    }
}

void Service::SetInGlobal(bool isIn) {
    pthread_spin_lock(&inGlobalLock);
    
    inGlobal = isIn;

    pthread_spin_unlock(&inGlobalLock);
}