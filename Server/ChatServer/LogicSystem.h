#pragma once
#include "Singleton.h"
#include <functional>
#include <map>
#include <string>
#include <json/json.h> // 确保包含 json
#include "CSession.h"
#include <vector>
#include "const.h"
#include "MsgNode.h"

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
    
    // 业务处理函数
    void LoginHandler(std::shared_ptr<CSession> session, const short &msg_id, const std::string &msg_data);
    void SearchInfo(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    void AddFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    void AuthFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    void DealChatTextMsg(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);

    // 辅助函数
    bool isPureDigit(const std::string& str);
    void GetUserByUid(const std::string& uid_str, Json::Value& rtvalue);
    void GetUserByName(const std::string& name_str, Json::Value& rtvalue);
    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& user_info);
    bool GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list);
    bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list);
    
    // 内存用户缓存
    std::map<int, std::shared_ptr<UserInfo>> _users;
};