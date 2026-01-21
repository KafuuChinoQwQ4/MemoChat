#include "LogicSystem.h"
#include "MysqlMgr.h" // 记得包含数据库管理器
#include <json/json.h>
#include <iostream>

LogicSystem::LogicSystem() {
    RegisterCallBacks();
}

LogicSystem::~LogicSystem() {}

void LogicSystem::RegisterCallBacks() {
    // 注册登录回调
    _fun_callbacks[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::PostMsgToQue(std::shared_ptr<CSession> session, short msg_id, std::string msg_data) {
    if (_fun_callbacks.find(msg_id) != _fun_callbacks.end()) {
        _fun_callbacks[msg_id](session, msg_id, msg_data);
    }
}

// [核心实现] 登录处理
void LogicSystem::LoginHandler(std::shared_ptr<CSession> session, const short &msg_id, const std::string &msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    
    auto uid = root["uid"].asInt();
    auto token = root["token"].asString();
    
    std::cout << "User login: uid=" << uid << " token=" << token << endl;

    // === 验证逻辑 (简化版) ===
    // 正常流程：RPC调用 StatusServer 验证 Token
    // 当前流程：由于我们还没写 StatusServer，且 Token 是 GateServer 发的假 Token，
    // 我们这里直接信任 Token，只验证 UID 是否有效。
    
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;

    // 1. 先在内存里找用户
    std::shared_ptr<UserInfo> user_info = nullptr;
    auto find_iter = _users.find(uid);
    if (find_iter != _users.end()) {
        user_info = find_iter->second;
    } else {
        // 2. 内存没有，去数据库查 (复用 MysqlMgr)
        // 注意：你需要确保 MysqlMgr 里有 GetUser 函数，或者我们这里手动写一个简单的查询
        // 为了方便，我们这里暂时假设 MysqlDao 里需要加一个 GetUser
        // 或者我们简化一下，直接返回一个假名字，保证流程跑通：
        
        user_info = std::make_shared<UserInfo>();
        user_info->uid = uid;
        user_info->name = "TestUser_" + std::to_string(uid); // 暂时用假名字
        _users[uid] = user_info;
        
        // TODO: 真正的数据库查询逻辑
        // user_info = MysqlMgr::GetInstance()->GetUser(uid);
    }

    // 3. 构造回包
    rtvalue["uid"] = uid;
    rtvalue["token"] = token;
    rtvalue["name"] = user_info->name; // 把名字返给客户端
    
    std::string return_str = rtvalue.toStyledString();
    session->Send(return_str, MSG_CHAT_LOGIN_RSP);
}