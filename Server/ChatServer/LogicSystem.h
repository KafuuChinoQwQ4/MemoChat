#pragma once
#include "Singleton.h"
#include <functional>
#include <map>
#include "CSession.h"
#include "const.h" // 确保包含这个，UserInfo 就在这里面
#include "CSession.h"
#include "MsgNode.h"

typedef std::function<void(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data)> FunCallBack;

class LogicNode {
public:
    LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode)
        : _session(session), _recvnode(recvnode) {}
    std::shared_ptr<CSession> _session;
    std::shared_ptr<RecvNode> _recvnode;
};

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