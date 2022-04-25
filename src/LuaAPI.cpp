#include "LuaAPI.h"
#include "Sunnet.h"

#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <iostream>


// 注册lua模块
void LuaAPI::Register(lua_State* luaState) {

    static luaL_Reg lualibs[] = {
        {"NewService", NewService},
        {"KillService", KillService},
        {"Send", Send},
        // {"Listen", Listen},
        // {"ClostConn", ClostConn},
        // {"Write", Write},
        {NULL, NULL}
    };

    luaL_newlib(luaState, lualibs);
    lua_setglobal(luaState, "sunnet");
}

// 开启新服务
int LuaAPI::NewService(lua_State* luaState) {
    int num = lua_gettop(luaState); // 返回栈顶元素索引（参数个数），获取lua调用时传入的参数个数
    if(lua_isstring(luaState, 1) == 0 || num != 1) { //若第一个参数不是string
        std::cout << "NewService arg err" << std::endl;
        lua_pushinteger(luaState, -1);
        return -1;
    }
    size_t len = 0;
    const char *type = lua_tolstring(luaState, 1, &len);
    char* newstr = new char[len + 1];
    newstr[len] = '\0';
    memcpy(newstr, type, len);
    std::shared_ptr<std::string> t = std::make_shared<std::string>(newstr);
    uint32_t id = Sunnet::inst->NewService(t);
    lua_pushinteger(luaState, id);
    std::cout << "NewService success" << std::endl;
    return 1;
}

int LuaAPI::KillService(lua_State* luaState) {
    int num = lua_gettop(luaState);
    if(lua_isinteger(luaState, 1) == 0 || num != 1) {
        std::cout << "KillService arg err" << std::endl;
        return 0;
    }
    const int id = lua_tointeger(luaState, 1);
    if(id < 0) {
        std::cout << "KillService id err" << std::endl;
        return 0;
    }
    Sunnet::inst->KillService(uint32_t(id));
    std::cout << "KillService success" << std::endl;
    return 0;
}

int LuaAPI::Send(lua_State* luaState) {
    int num = lua_gettop(luaState);
    if(lua_isinteger(luaState, 1) == 0 || lua_isinteger(luaState, 2) == 0 || lua_isstring(luaState, 3) == 0 || num != 3) {
        std::cout << "Send arg err" << std::endl;
        return 0;
    }
    const int srcId = lua_tointeger(luaState, 1);
    const int toId = lua_tointeger(luaState, 2);
    size_t len = 0;
    const char* buff = lua_tolstring(luaState, 3, &len);
    
    std::shared_ptr<ServiceMsg> msg = std::make_shared<ServiceMsg>();
    msg->buff = std::make_shared<char>(*(new char[len]));
    memcpy(msg->buff.get(), buff, len);
    msg->source = srcId;
    msg->size = len;
    msg->type = BaseMsg::TYPE::SERVICE;
    
    Sunnet::inst->Send(toId, msg);

    std::cout << "Send success" << std::endl;
    return 0;
}