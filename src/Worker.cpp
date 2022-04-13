#include <iostream> 
#include <unistd.h> 

#include "Worker.h"

void Worker::operator()() {
    while(true) { 
        // std::cout << "working id:" << '\t' <<  id << std::endl; 
        // usleep(100000); //0.1s 
        std::shared_ptr<Service> srv = Sunnet::inst->PopGlobalQueue();
        if(!srv) { // 全局服务队列空，无有需要处理消息的服务
            Sunnet::inst->WorkerWait();
        }
        else {
            srv->ProcessMsgs(eachNum);
            CheckAndPutGlobal(srv);
        }
    } 
}

void Worker::CheckAndPutGlobal(std::shared_ptr<Service> srv) {
    if(srv->isExiting)
        return;

    pthread_spin_lock(&srv->queueLock);

    if(!srv->msgQueue.empty())
        Sunnet::inst->PushGlobalQueue(srv);
    else
        srv->SetInGlobal(false);

    pthread_spin_unlock(&srv->queueLock);
}