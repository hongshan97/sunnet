#pragma once

#include <sys/epoll.h>
#include <memory>
#include <Conn.h>

class SocketWorker {
private:
    int epollFd;

    void OnEvent(epoll_event ev);
    void OnAccept(std::shared_ptr<Conn> conn);
    void OnRW(std::shared_ptr<Conn> conn, bool r, bool w);

public:
    void Init();
    void operator()();

    void AddEvent(int fd);
    void RemoveEvent(int fd);
    void ModifyEvent(int fd, bool epollOut); // 参数epollOut代表是否监听它的写事件。如果 epollOut为真，则epoll对象将会同时监听可读（EPOLLIN）和可写 （EPOLLOUT）事件，否则就只监听可读事件
};