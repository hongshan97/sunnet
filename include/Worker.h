#pragma once

#include <thread>
#include "Service.h"
#include "Sunnet.h"

// 一个worker封装一个线程来工作
class Worker {
public:
    int id; // 编号
    int eachNum; // 每次处理消息数
    void operator()(); // 线程函数

    void CheckAndPutGlobal(std::shared_ptr<Service> srv);
};