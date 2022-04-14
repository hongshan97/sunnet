#include "SocketWorker.h"

#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "Sunnet.h"


void SocketWorker::Init() {
    std::cout << "SocketWorker Init" << std::endl;

    epollFd = epoll_create(9527);
    std::cout << "Create epoll, fd: " << epollFd << std::endl;
}

void SocketWorker::operator()() {
    for(;;) {
        // std::cout << "SocketWorker";
        // std::cout << "working" << std::endl;
        // usleep(1000);
        const int EVENT_SIZE = 64;
        struct epoll_event events[EVENT_SIZE];
        int eventCount = epoll_wait(epollFd, events, EVENT_SIZE, -1);

        for(int i = 0; i < eventCount; ++i) {
            epoll_event ev = events[i];
            OnEvent(ev);
        }
    }
}

void SocketWorker::AddEvent(int fd) {
    std::cout << "AddEvent, fd: " << fd << std::endl;
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // 边缘触发模式监听可读事件
    ev.data.fd = fd;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == -1) 
        std::cout <<"AddEvent epoll_ctl fail: " << strerror(errno) << std::endl;
}

void SocketWorker::RemoveEvent(int fd) {
    std::cout << "RemoveEvent, fd: " << fd << std::endl;
    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
}

void SocketWorker::ModifyEvent(int fd, bool epollOut) {
    std::cout << "ModifyEvent, fd: " << fd << std::endl;
    struct epoll_event ev;
    ev.data.fd = fd;

    if(epollOut)
        ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
    else
        ev.events = EPOLLIN | EPOLLET;

    epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev);
}

void SocketWorker::OnEvent(epoll_event ev) {
    std::cout << "OnEvent" << std::endl;

    int fd = ev.data.fd;
    std::shared_ptr<Conn> conn = Sunnet::inst->GetConn(fd);
    if(!conn) {
        std::cout << "OnEvent error, conn is null" << std::endl;
        return;
    }
    bool isRead = ev.events & EPOLLIN;
    bool isWrite = ev.events & EPOLLOUT;
    bool isError = ev.events & EPOLLERR;

    if(conn->type == Conn::TYPE::LISTEN) {
        if(isRead)
            OnAccept(conn);
else
std::cout << "OnEevent 1" << std::endl;
    }
    else {
        if(isRead || isWrite) 
            OnRW(conn, isRead, isWrite);
        if(isError)
            std::cout << "OnError fd: " << conn->fd << std::endl;
if(!(isRead || isWrite || isError))
std::cout << "OnEevent 2" << std::endl;
    }
}

// 传递有新连接给对应srv
void SocketWorker::OnAccept(std::shared_ptr<Conn> conn) {
    std::cout << "OnAccept fd: " << conn->fd << std::endl;
    int clientFd = accept(conn->fd, NULL, NULL);
    if(clientFd < 0) {
        std::cout << "accept error" << std::endl;
        return;
    }
    fcntl(clientFd, F_SETFL, O_NONBLOCK); // 设为非阻塞IO

    unsigned long buffSize = 4294967295;
    if(setsockopt(clientFd, SOL_SOCKET, SO_SNDBUFFORCE, &buffSize, sizeof(buffSize)) < 0) 
        std::cout << "OnAccept setsocketopt fail: " << strerror(errno) << std::endl;

    Sunnet::inst->AddConn(clientFd, conn->serviceId, Conn::TYPE::CLIENT);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // 注册这个连接的可读消息
    ev.data.fd = clientFd;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &ev) == -1) {
        std::cout << "epoll_ctl fail" << std::endl;
        return;
    }
    std::shared_ptr<SocketAcceptMsg> msg = std::make_shared<SocketAcceptMsg>();
    msg->type = BaseMsg::TYPE::SOCKET_ACCEPT;
    msg->listenFd = conn->fd;
    msg->clientFd = clientFd;
    Sunnet::inst->Send(conn->serviceId, msg); // 发给相关的srv（gateway）去处理clientfd
}

// 传递可读可写消息给对应srv
void SocketWorker::OnRW(std::shared_ptr<Conn> conn, bool r, bool w) {
    std::cout << "OnRW fd: " << conn->fd << std::endl;
    std::shared_ptr<SocketRWMsg> msg = std::make_shared<SocketRWMsg>();
    msg->type = BaseMsg::TYPE::SOCKET_RW;
    msg->fd = conn->fd;
    msg->isRead = r;
    msg->isWrite = w;
    Sunnet::inst->Send(conn->serviceId, msg);
}