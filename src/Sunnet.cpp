#include <iostream>

#include "Sunnet.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

// 饿汉单例
_Sunnet* Sunnet::inst = new _Sunnet();

void _Sunnet::Start() {
    std::cout << "Hello Sunnet!" << std::endl;

    pthread_rwlock_init(&servicesLock, NULL);

    pthread_spin_init(&globalLock, PTHREAD_PROCESS_PRIVATE);

    pthread_mutex_init(&sleepMtx, NULL);
    pthread_cond_init(&sleepCond, NULL);

    pthread_rwlock_init(&connsLock, NULL);

    StartWorker();
    StartScoket();
}

void _Sunnet::StartWorker() {
    for(int i = 0; i < WORKER_NUM; ++i) {
        std::cout << "Start worker thread : " << '\t' << i << std:: endl;
        // 创建worker
        Worker* worker = new Worker();
        worker->id = i;
        // eachNum小的worker相同时间可以处理更多服务，服务延迟低。eachNum大的worker相同时间可以处理更多消息，但这些消息大部分来自少数服务，服务调度不积极，延迟高。
        worker->eachNum = 2 << i; 
        // 创建线程
        std::thread *wt = new std::thread(*worker);
        
        workers.push_back(worker);
        workerThreads.push_back(wt);
    }
}

void _Sunnet::StartScoket() {
    socketWorker = new SocketWorker();
    socketWorker->Init();
    socketThread = new std::thread(*socketWorker);
}

void _Sunnet::Wait() {
    if(workerThreads[0]) {
        workerThreads[0]->join();
    }
}

uint32_t _Sunnet::NewService(std::shared_ptr<std::string> type) {
    std::shared_ptr<Service> srv = std::make_shared<Service>();
    srv->type = type;
    
    pthread_rwlock_wrlock(&servicesLock);

    srv->id = maxId++;
    services.emplace(srv->id, srv);

    pthread_rwlock_unlock(&servicesLock);

    srv->OnInit();
    return srv->id;
}

std::shared_ptr<Service> _Sunnet::GetService(uint32_t id) {
    std::shared_ptr<Service> srv = nullptr;
    
    pthread_rwlock_rdlock(&servicesLock);

    std::unordered_map<uint32_t, std::shared_ptr<Service>>::iterator iter = services.find(id);
    if(iter != services.end()) 
        srv = iter->second;

    pthread_rwlock_unlock(&servicesLock);

    return srv;
}

void _Sunnet::KillService(uint32_t id) {
    std::shared_ptr<Service> srv = GetService(id);
    if(!srv)
        return;
    
    srv->OnExit();
    srv->isExiting = true;

    pthread_rwlock_wrlock(&servicesLock);

    services.erase(id);

    pthread_rwlock_unlock(&servicesLock);
}

std::shared_ptr<Service> _Sunnet::PopGlobalQueue() {
    std::shared_ptr<Service> srv = nullptr;

    pthread_spin_lock(&globalLock);

    if(!globalQueue.empty()) {
        srv = globalQueue.front();
        globalQueue.pop();
        --globalLen;
    }

    pthread_spin_unlock(&globalLock);

    return srv;
}

void _Sunnet::PushGlobalQueue(std::shared_ptr<Service> srv) {
    pthread_spin_lock(&globalLock);

    globalQueue.push(srv);
    ++globalLen;

    pthread_spin_unlock(&globalLock);
}

void _Sunnet::Send(uint32_t toId, std::shared_ptr<BaseMsg> msg) {
    std::shared_ptr<Service> toSrv = GetService(toId);
    if(!toSrv) {
        std::cout << "Send failed, Service " << toId << " not exist" << std::endl;
        return; 
    }

    toSrv->PushMsg(msg);

    bool hasPush = false;
    
    pthread_spin_lock(&toSrv->inGlobalLock); // 局部的inGlobalLock加锁

    if(!toSrv->inGlobal) {
        PushGlobalQueue(toSrv); // 全局的globalLock加锁
        toSrv->inGlobal = true;
        hasPush = true;
    }

    pthread_spin_unlock(&toSrv->inGlobalLock);

    // 唤起
    if(hasPush) // haspush只有在GlobalQueue长度增加后才为true，如果msg被send到在GlobalQueue的srv中，则不必唤醒其他worker线程
        CheckAndWakeup();
}

void _Sunnet::WorkerWait() {
    pthread_mutex_lock(&sleepMtx);

    ++sleepCount;
    pthread_cond_wait(&sleepCond, &sleepMtx);
    --sleepCount;

    pthread_mutex_unlock(&sleepMtx);
}

void _Sunnet::CheckAndWakeup() {
    if(sleepCount == 0)
        return;
    
    if(WORKER_NUM - sleepCount <= globalLen) {
        pthread_cond_signal(&sleepCond);
    }
}

int _Sunnet::AddConn(int fd, uint32_t id, Conn::TYPE type) {
    std::shared_ptr<Conn> conn = std::make_shared<Conn>();
    conn->fd = fd;
    conn->serviceId = id;
    conn->type = type;

    pthread_rwlock_wrlock(&connsLock);
    
    conns.emplace(fd, conn);

    pthread_rwlock_unlock(&connsLock);

    return fd;
}

std::shared_ptr<Conn> _Sunnet::GetConn(int fd) {
    std::shared_ptr<Conn> conn = nullptr;

    pthread_rwlock_rdlock(&connsLock);

    std::unordered_map<int, std::shared_ptr<Conn>>::iterator iter = conns.find(fd);
    if(iter != conns.end()) 
        conn = iter->second;

    pthread_rwlock_unlock(&connsLock);

    return conn;
}

bool _Sunnet::RemoveConn(int fd) {
    int res = 0;
    
    pthread_rwlock_wrlock(&connsLock);

    res = conns.erase(fd);

    pthread_rwlock_unlock(&connsLock);

    return 1 == res;
}

int _Sunnet::Listen(uint32_t port, uint32_t serviceId) {
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd <= 0) {
        std::cout << "socket error" << std::endl;
        return -1;
    }

    fcntl(listenFd, F_SETFL, O_NONBLOCK); // 设置listen的fd为非阻塞

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int res = bind(listenFd, (struct sockaddr*)&addr, sizeof(addr));
    if(res == -1) {
        std::cout << "bind error: " << strerror(errno) << std::endl;
        return -1;
    }

    res = listen(listenFd, 64);
    if(res < 0) {
        std::cout << "listen error" << std::endl;
        return -1;
    }

std::cout << "listen on " << port << std::endl;

    AddConn(listenFd, serviceId, Conn::TYPE::LISTEN);

    socketWorker->AddEvent(listenFd);
    return listenFd;
}

void _Sunnet::CloseConn(uint32_t fd) {
    bool succ = RemoveConn(fd);

    close(fd);
    if(succ)
        socketWorker->RemoveEvent(fd);
}