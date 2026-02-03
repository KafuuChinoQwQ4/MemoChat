#include "LogicSystem.h"
#include "MysqlMgr.h" // 记得包含数据库管理器
#include <json/json.h>
#include <iostream>
#include "UserMgr.h" 
#include "const.h"  // 确保里面有 LOGIN_COUNT 定义
#include "ConfigMgr.h"
#include "RedisMgr.h"  // [新增] 必须包含
#include "ConfigMgr.h"

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

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    
    // 使用 Defer 确保函数退出时发送回包 (如果没有 Defer 类，请直接在所有 return 前发送)
    // 假设你参考的代码里有 Defer，如果没有可以去掉，在最后手动 Send
    bool verified = false;

    // ==========================================================
    // 1. 验证 Token (分布式架构核心)
    // ==========================================================
    // 从 Redis 获取 StatusServer 生成的 Token
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::string token_value = "";
    
    bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
    
    // 如果 Redis 里没有 Token (过期或未登录 StatusServer)，或者 Token 不匹配
    if (!success) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, MSG_CHAT_LOGIN_RSP);
        return;
    }

    if (token_value != token) {
        rtvalue["error"] = ErrorCodes::TokenInvalid;
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, MSG_CHAT_LOGIN_RSP);
        return;
    }

    // ==========================================================
    // 2. 获取用户信息 (查找内存 -> 查库)
    // ==========================================================
    std::shared_ptr<UserInfo> user_info = nullptr;
    auto find_iter = _users.find(uid);
    if (find_iter != _users.end()) {
        user_info = find_iter->second;
    } else {
        // 这里的 MysqlMgr::GetInstance()->GetUser(uid) 需要你自己实现
        // 如果暂时没写，可以用下面的临时代码代替：
        // user_info = std::make_shared<UserInfo>();
        // user_info->uid = uid;
        // user_info->name = "TestUser_" + uid_str;
        // user_info->nick = "Nick_" + uid_str;
        // _users[uid] = user_info;

        // 推荐做法：从数据库获取
         user_info = MysqlMgr::GetInstance()->GetUser(uid);
         if (user_info == nullptr) {
             rtvalue["error"] = ErrorCodes::UidInvalid;
             std::string return_str = rtvalue.toStyledString();
             session->Send(return_str, MSG_CHAT_LOGIN_RSP);
             return;
         }
         _users[uid] = user_info;
    }

    // ==========================================================
    // 3. 更新 Redis 状态 (StatusServer 负载均衡需要)
    // ==========================================================
    auto server_name = ConfigMgr::Inst().GetValue("SelfServer", "Name");
    
    // 3.1 增加本服务器的登录计数
    // HGet 返回的是 string，需要转 int 再转回 string 存入
    std::string count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server_name);
    int count = 0;
    if (!count_str.empty()) {
        count = std::stoi(count_str);
    }
    count++;
    RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, std::to_string(count));

    // 3.2 记录用户当前所在的服务器 (用于后续聊天路由)
    // Key: uip_UID, Value: chatserver1
    std::string ipkey = USERIPPREFIX + uid_str;
    RedisMgr::GetInstance()->Set(ipkey, server_name);

    // ==========================================================
    // 4. Session 绑定与管理
    // ==========================================================
    // 在 Session 中记录 UID，方便断开连接时清理
    session->SetUserId(uid);
    
    // 在内存管理器中注册，方便通过 UID 找到 Session
    UserMgr::GetInstance()->SetUserSession(uid, session);

    // ==========================================================
    // 5. 构造回包
    // ==========================================================
    rtvalue["uid"] = uid;
    rtvalue["token"] = token;
    rtvalue["name"] = user_info->name;
    // rtvalue["icon"] = user_info->icon; // 如果有头像字段

    std::string return_str = rtvalue.toStyledString();
    session->Send(return_str, MSG_CHAT_LOGIN_RSP);
}