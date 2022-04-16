#include <iostream>

#include "Sunnet.h"

#include <unistd.h>

void PingPongTest() {
    std::shared_ptr<std::string> pingType = std::make_shared<std::string>("ping");
    uint32_t ping1 = Sunnet::inst->NewService(pingType);
    uint32_t ping2 = Sunnet::inst->NewService(pingType);
    uint32_t pong = Sunnet::inst->NewService(pingType);
usleep(10000);
    auto msg1 = Sunnet::inst->MakeMsg(ping1, new char[3]{'h', 'i', '\0'}, 3);
    auto msg2 = Sunnet::inst->MakeMsg(ping2, new char[3]{'0', 'o', '\0'}, 3);

    Sunnet::inst->Send(pong, msg1);
    Sunnet::inst->Send(pong, msg2);
}

void TestSocket() {
    int fd = Sunnet::inst->Listen(9526, 1);
    sleep(10);
    Sunnet::inst->CloseConn(fd);
}

void TestEcho() {
    auto t = std::make_shared<std::string>("gateway");
    uint32_t gateway = Sunnet::inst->NewService(t);
}

int main() {
    Sunnet::inst->Start();
    
    std::shared_ptr<std::string> maniSrvType = std::make_shared<std::string>("main"); // 默认服务逻辑从开启main服务开始
    Sunnet::inst->NewService(maniSrvType);
    
    Sunnet::inst->Wait();

    std::cout << "主线程return" << std::endl;
    return 0;
}