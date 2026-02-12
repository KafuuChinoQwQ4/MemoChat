#include "UserMgr.h"
#include <QJsonArray>
#include <QJsonObject>

UserMgr::UserMgr() : _uid(0) {}

UserMgr::~UserMgr() {}

void UserMgr::SetName(QString name) {
    _name = name;
}

void UserMgr::SetUid(int uid) {
    _uid = uid;
}

void UserMgr::SetToken(QString token) {
    _token = token;
}

QString UserMgr::GetName() {
    return _name;
}

int UserMgr::GetUid() {
    return _uid;
}

QString UserMgr::GetToken() {
    return _token;
}

void UserMgr::AddApplyList(std::shared_ptr<AddFriendApply> app) {
    _apply_list.push_back(app);
}

bool UserMgr::AlreadyApply(int uid) {
    for (auto& app : _apply_list) {
        if (app->_from_uid == uid)
            return true;
    }
    return false;
}

void UserMgr::AddFriend(std::shared_ptr<AuthRsp> auth_rsp) {
    auto user_info = std::make_shared<UserInfo>(auth_rsp);
    _friend_map[user_info->_uid] = user_info;
}

void UserMgr::AddFriend(std::shared_ptr<AuthInfo> auth_info) {
    auto user_info = std::make_shared<UserInfo>(auth_info);
    _friend_map[user_info->_uid] = user_info;
}

bool UserMgr::CheckFriendById(int uid) {
    auto iter = _friend_map.find(uid);
    return iter != _friend_map.end();
}

void UserMgr::SetUserInfo(std::shared_ptr<UserInfo> user_info) {
    _user_info = user_info;
    _uid = user_info->_uid;
    _name = user_info->_name;
}

void UserMgr::AppendApplyList(QJsonArray array) {
    for (const QJsonValue &value : array) {
        QJsonObject obj = value.toObject();
        int uid = obj["uid"].toInt();
        QString name = obj["name"].toString();
        QString desc = obj["desc"].toString();
        QString icon = obj["icon"].toString();
        QString nick = obj["nick"].toString();
        int sex = obj["sex"].toInt();
        int status = obj["status"].toInt();

        auto apply = std::make_shared<AddFriendApply>(uid, name, desc, icon, nick, sex, status);
        AddApplyList(apply);
    }
}

void UserMgr::AppendFriendList(QJsonArray array) {
    for (const QJsonValue &value : array) {
        QJsonObject obj = value.toObject();
        int uid = obj["uid"].toInt();
        QString name = obj["name"].toString();
        QString desc = obj["desc"].toString();
        QString icon = obj["icon"].toString();
        QString nick = obj["nick"].toString();
        int sex = obj["sex"].toInt();
        int back = obj["back"].toInt();

        auto user_info = std::make_shared<UserInfo>(uid, name, nick, icon, sex);
        _friend_map[uid] = user_info;
    }
}

std::shared_ptr<UserInfo> UserMgr::GetUserInfo() {
    return _user_info;
}

std::shared_ptr<UserInfo> UserMgr::GetFriendById(int uid) {
    auto iter = _friend_map.find(uid);
    if (iter == _friend_map.end()) {
        return nullptr;
    }
    return iter->second;
}

void UserMgr::AppendFriendChatMsg(int friend_id, std::vector<std::shared_ptr<TextChatData>> msgs) {
    auto iter = _friend_chat_msgs.find(friend_id);
    if (iter == _friend_chat_msgs.end()) {
        _friend_chat_msgs[friend_id] = msgs;
        return;
    }
    iter->second.insert(iter->second.end(), msgs.begin(), msgs.end());
}