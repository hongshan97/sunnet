#pragma once

#include <queue>
#include <thread>
#include <stdint-gcc.h>

#include "Msg.h"
extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"
}

class Service {
public:
    uint32_t id; // 服务id
    std::shared_ptr<std::string> type; // 服务类型
    bool isExiting = false; //服务是否正在退出
    std::queue<std::shared_ptr<BaseMsg>> msgQueue; // 消息队列
    pthread_spinlock_t queueLock; // 消息队列的锁

    Service();
    ~Service();

    //回调，服务逻辑
    void OnInit();

    void OnMsg(std::shared_ptr<BaseMsg> msg);
        void OnServiceMsg(std::shared_ptr<ServiceMsg> msg);
        void OnAcceptMsg(std::shared_ptr<SocketAcceptMsg> msg);
        void OnRWMsg(std::shared_ptr<SocketRWMsg> msg);
            void OnSocketData(int fd, const char* buff, int len);
            void OnSocketWritable(int fd);
            void OnSocketClose(int fd);
            
    void OnExit();

    void PushMsg(std::shared_ptr<BaseMsg> msg); // 插入消息

    // 执行消息
    bool ProcessMsg();
    void ProcessMsgs(int max);

    // 服务是否在全局消息队列flag
    bool inGlobal = false;
    pthread_spinlock_t inGlobalLock;
    void SetInGlobal(bool isIn);

private:
    std::shared_ptr<BaseMsg> PopMsg();

    lua_State* luaState;
};