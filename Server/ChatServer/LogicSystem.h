#pragma once
#include "Singleton.h"
#include <functional>
#include <map>
#include "CSession.h"

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
};