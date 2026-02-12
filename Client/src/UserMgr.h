#pragma once
#include <QObject>
#include <memory>
#include "Singleton.h"
#include "userdata.h"
#include <vector>
#include <QJsonArray>

class UserMgr : public QObject, public Singleton<UserMgr>,
                public std::enable_shared_from_this<UserMgr>
{
    Q_OBJECT
public:
    friend class Singleton<UserMgr>;
    ~UserMgr();
    
    void SetName(QString name);
    void SetUid(int uid);
    void SetToken(QString token);
    
    QString GetName();
    int GetUid();
    QString GetToken();
    
    void AddApplyList(std::shared_ptr<AddFriendApply> apply);
    bool AlreadyApply(int uid);
    void AddFriend(std::shared_ptr<AuthRsp> auth_rsp);
    void AddFriend(std::shared_ptr<AuthInfo> auth_info);
    bool CheckFriendById(int uid);
    void SetUserInfo(std::shared_ptr<UserInfo> user_info);
    void AppendApplyList(QJsonArray array);
    void AppendFriendList(QJsonArray array);
    std::shared_ptr<UserInfo> GetUserInfo();
    std::shared_ptr<UserInfo> GetFriendById(int uid);
    void AppendFriendChatMsg(int friend_id, std::vector<std::shared_ptr<TextChatData>> msgs);

private:
    UserMgr();
    QString _name;
    QString _token;
    int _uid;
    std::vector<std::shared_ptr<AddFriendApply>> _apply_list;
    std::unordered_map<int, std::shared_ptr<UserInfo>> _friend_map;
    std::shared_ptr<UserInfo> _user_info;
    std::unordered_map<int, std::vector<std::shared_ptr<TextChatData>>> _friend_chat_msgs;
};