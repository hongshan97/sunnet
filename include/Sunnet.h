#pragma once

#include <vector>
#include <unordered_map>
#include <stdint-gcc.h>

#include "Service.h"
#include "Worker.h"
#include "SocketWorker.h"
#include "Conn.h"

class Worker;

class _Sunnet {
public:
    void Start(); // 初始化并开始
    void Wait(); // 等待运行

    std::unordered_map<uint32_t, std::shared_ptr<Service>> services; // 服务列表
    uint32_t maxId = 0; // 服务最大ID
    pthread_rwlock_t servicesLock; // 消息列表读写锁

    uint32_t NewService(std::shared_ptr<std::string> type); // 新建服务
    void KillService(uint32_t id); // 销毁服务（服务自己调用）

    std::queue<std::shared_ptr<Service>> globalQueue; // 全局消息队列
    int globalLen = 0;
    pthread_spinlock_t globalLock; //

    // 全局消息操作
    void Send(uint32_t toId, std::shared_ptr<BaseMsg> msg); // 发消息
    std::shared_ptr<Service> PopGlobalQueue();
    void PushGlobalQueue(std::shared_ptr<Service> srv);

    std::shared_ptr<BaseMsg> MakeMsg(uint32_t src, char* buff, int len) {
        std::shared_ptr<ServiceMsg> msg = std::make_shared<ServiceMsg>();
        msg->type = BaseMsg::TYPE::SERVICE;
        msg->source = src;
        msg->buff = std::shared_ptr<char>(buff);
        msg->size = len;
        return msg;
    }

    void CheckAndWakeup(); // 唤醒工作线程
    void WorkerWait(); // 让工作线程等待（仅工作线程调用）

    // 增删查Conn
    int AddConn(int fd, uint32_t id, Conn::TYPE type);
    std::shared_ptr<Conn> GetConn(int fd);
    bool RemoveConn(int fd);

    int Listen(uint32_t port, uint32_t serviceId);
    void CloseConn(uint32_t fd);

private:
    int WORKER_NUM = 3; // 工作的worker数量
    std::vector<Worker*> workers; // worker
    std::vector<std::thread*> workerThreads; // 线程

    void StartWorker();

    // socket线程
    SocketWorker* socketWorker;
    std::thread* socketThread;
    void StartScoket();

    std::shared_ptr<Service> GetService(uint32_t id); // 获取服务

    pthread_mutex_t sleepMtx;
    pthread_cond_t sleepCond;
    int sleepCount = 0; // 休眠工作线程数量

    // Conn列表
    std::unordered_map<int, std::shared_ptr<Conn>> conns;
    pthread_rwlock_t connsLock;
};


// 单例Sunnet内含单个的_Sunnet对象
class Sunnet {
    Sunnet() = delete;
    ~Sunnet() = delete;
public:
    static _Sunnet* inst;
};


