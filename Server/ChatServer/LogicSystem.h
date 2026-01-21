#pragma once
#include "Singleton.h"
#include <functional>
#include <map>
#include "CSession.h"

// 用户信息结构
struct UserInfo {
    std::string name;
    std::string pwd;
    int uid;
    std::string email;
    // ... 其他字段
};

typedef std::function<void(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data)> FunCallBack;

class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    void RegisterCallBacks();
    void PostMsgToQue(std::shared_ptr<CSession> session, short msg_id, std::string msg_data);

private:
    LogicSystem();
    std::map<short, FunCallBack> _fun_callbacks;
    
    // [新增] 登录处理函数
    void LoginHandler(std::shared_ptr<CSession> session, const short &msg_id, const std::string &msg_data);
    
    // [新增] 内存中缓存的用户信息
    std::map<int, std::shared_ptr<UserInfo>> _users;
};