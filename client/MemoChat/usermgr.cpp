#include "usermgr.h"
#include <QJsonArray>
#include "tcpmgr.h"

UserMgr::~UserMgr()
{

}

void UserMgr::SetUserInfo(std::shared_ptr<UserInfo> user_info) {
    _user_info = user_info;
}

void UserMgr::SetToken(QString token)
{
    _token = token;
}

int UserMgr::GetUid()
{
    if (!_user_info) {
        return 0;
    }
    return _user_info->_uid;
}

QString UserMgr::GetName()
{
    if (!_user_info) {
        return {};
    }
    return _user_info->_name;
}

QString UserMgr::GetNick()
{
    if (!_user_info) {
        return {};
    }
    return _user_info->_nick;
}

QString UserMgr::GetIcon()
{
    if (!_user_info) {
        return {};
    }
    return _user_info->_icon;
}

QString UserMgr::GetDesc()
{
    if (!_user_info) {
        return {};
    }
    return _user_info->_desc;
}

void UserMgr::UpdateNickAndDesc(const QString &nick, const QString &desc)
{
    if (!_user_info) {
        return;
    }
    _user_info->_nick = nick;
    _user_info->_desc = desc;
}

void UserMgr::UpdateIcon(const QString &icon)
{
    if (!_user_info) {
        return;
    }
    _user_info->_icon = icon;
}

std::shared_ptr<UserInfo> UserMgr::GetUserInfo()
{
    return _user_info;
}

void UserMgr::ResetSession()
{
    _user_info.reset();
    _apply_list.clear();
    _friend_list.clear();
    _friend_map.clear();
    _group_list.clear();
    _group_map.clear();
    _token.clear();
    _chat_loaded = 0;
    _contact_loaded = 0;
    _group_loaded = 0;
}

void UserMgr::AppendApplyList(QJsonArray array)
{
    _apply_list.clear();
    // 遍历 QJsonArray 并输出每个元素
    for (const QJsonValue &value : array) {
        auto name = value["name"].toString();
        auto desc = value["desc"].toString();
        auto icon = value["icon"].toString();
        auto nick = value["nick"].toString();
        auto sex = value["sex"].toInt();
        auto uid = value["uid"].toInt();
        auto status = value["status"].toInt();
        auto info = std::make_shared<ApplyInfo>(uid, name,
                           desc, icon, nick, sex, status);
        _apply_list.push_back(info);
    }
}

void UserMgr::AppendFriendList(QJsonArray array) {
    _friend_list.clear();
    _friend_map.clear();
    _chat_loaded = 0;
    _contact_loaded = 0;
    // 遍历 QJsonArray 并输出每个元素
    for (const QJsonValue& value : array) {
        auto name = value["name"].toString();
        auto desc = value["desc"].toString();
        auto icon = value["icon"].toString();
        auto nick = value["nick"].toString();
        auto sex = value["sex"].toInt();
        auto uid = value["uid"].toInt();
        auto back = value["back"].toString();

        auto info = std::make_shared<FriendInfo>(uid, name,
            nick, icon, sex, desc, back);
        _friend_list.push_back(info);
        _friend_map.insert(uid, info);
    }
}

void UserMgr::SetGroupList(const QJsonArray &array)
{
    _group_list.clear();
    _group_map.clear();
    _group_loaded = 0;
    for (const QJsonValue &value : array) {
        auto info = std::make_shared<GroupInfoData>();
        info->_group_id = value["groupid"].toVariant().toLongLong();
        info->_name = value["name"].toString();
        info->_announcement = value["announcement"].toString();
        info->_owner_uid = value["owner_uid"].toInt();
        info->_member_limit = value["member_limit"].toInt(200);
        info->_member_count = value["member_count"].toInt(0);
        info->_role = value["role"].toInt(1);
        info->_is_all_muted = value["is_all_muted"].toInt(0);
        if (info->_group_id <= 0) {
            continue;
        }
        _group_list.push_back(info);
        _group_map.insert(info->_group_id, info);
    }
}

std::vector<std::shared_ptr<ApplyInfo> > UserMgr::GetApplyList()
{
    return _apply_list;
}

std::vector<std::shared_ptr<ApplyInfo> > UserMgr::GetApplyListSnapshot() const
{
    return _apply_list;
}

void UserMgr::AddApplyList(std::shared_ptr<ApplyInfo> app)
{
    if (!app) {
        return;
    }

    for (const auto &item : _apply_list) {
        if (!item) {
            continue;
        }
        if (item->_uid == app->_uid) {
            item->_name = app->_name;
            item->_nick = app->_nick;
            item->_desc = app->_desc;
            item->_icon = app->_icon;
            item->_sex = app->_sex;
            item->_status = app->_status;
            return;
        }
    }

    _apply_list.push_back(app);
}

bool UserMgr::AlreadyApply(int uid)
{
    for(auto& apply: _apply_list){
        if(apply->_uid == uid){
            return true;
        }
    }

    return false;
}

void UserMgr::MarkApplyStatus(int uid, int status)
{
    for (const auto &apply : _apply_list) {
        if (!apply) {
            continue;
        }
        if (apply->_uid == uid) {
            apply->_status = status;
            return;
        }
    }
}

std::vector<std::shared_ptr<FriendInfo>> UserMgr::GetChatListPerPage() {
    
    std::vector<std::shared_ptr<FriendInfo>> friend_list;
    int begin = _chat_loaded;
    int end = begin + CHAT_COUNT_PER_PAGE;

    if (begin >= _friend_list.size()) {
        return friend_list;
    }

    if (end > _friend_list.size()) {
        friend_list = std::vector<std::shared_ptr<FriendInfo>>(_friend_list.begin() + begin, _friend_list.end());
        return friend_list;
    }


    friend_list = std::vector<std::shared_ptr<FriendInfo>>(_friend_list.begin() + begin, _friend_list.begin()+ end);
    return friend_list;
}


std::vector<std::shared_ptr<FriendInfo>> UserMgr::GetConListPerPage() {
    std::vector<std::shared_ptr<FriendInfo>> friend_list;
    int begin = _contact_loaded;
    int end = begin + CHAT_COUNT_PER_PAGE;

    if (begin >= _friend_list.size()) {
        return friend_list;
    }

    if (end > _friend_list.size()) {
        friend_list = std::vector<std::shared_ptr<FriendInfo>>(_friend_list.begin() + begin, _friend_list.end());
        return friend_list;
    }


    friend_list = std::vector<std::shared_ptr<FriendInfo>>(_friend_list.begin() + begin, _friend_list.begin() + end);
    return friend_list;
}


UserMgr::UserMgr():_user_info(nullptr), _chat_loaded(0),_contact_loaded(0), _group_loaded(0)
{

}

void UserMgr::SlotAddFriendRsp(std::shared_ptr<AuthRsp> rsp)
{
    AddFriend(rsp);
}

void UserMgr::SlotAddFriendAuth(std::shared_ptr<AuthInfo> auth)
{
    AddFriend(auth);
}

bool UserMgr::IsLoadChatFin() {
    if (_chat_loaded >= _friend_list.size()) {
        return true;
    }

    return false;
}

void UserMgr::UpdateChatLoadedCount() {
    int begin = _chat_loaded;
    int end = begin + CHAT_COUNT_PER_PAGE;

    if (begin >= _friend_list.size()) {
        return ;
    }

    if (end > _friend_list.size()) {
        _chat_loaded = _friend_list.size();
        return ;
    }

    _chat_loaded = end;
}

void UserMgr::UpdateContactLoadedCount() {
    int begin = _contact_loaded;
    int end = begin + CHAT_COUNT_PER_PAGE;

    if (begin >= _friend_list.size()) {
        return;
    }

    if (end > _friend_list.size()) {
        _contact_loaded = _friend_list.size();
        return;
    }

    _contact_loaded = end;
}

bool UserMgr::IsLoadConFin()
{
    if (_contact_loaded >= _friend_list.size()) {
        return true;
    }

    return false;
}

bool UserMgr::CheckFriendById(int uid)
{
    auto iter = _friend_map.find(uid);
    if(iter == _friend_map.end()){
        return false;
    }

    return true;
}

void UserMgr::AddFriend(std::shared_ptr<AuthRsp> auth_rsp)
{
    auto friend_info = std::make_shared<FriendInfo>(auth_rsp);
    auto iter = _friend_map.find(friend_info->_uid);
    if (iter == _friend_map.end()) {
        _friend_list.push_back(friend_info);
    }
    _friend_map[friend_info->_uid] = friend_info;
}

void UserMgr::AddFriend(std::shared_ptr<AuthInfo> auth_info)
{
    auto friend_info = std::make_shared<FriendInfo>(auth_info);
    auto iter = _friend_map.find(friend_info->_uid);
    if (iter == _friend_map.end()) {
        _friend_list.push_back(friend_info);
    }
    _friend_map[friend_info->_uid] = friend_info;
}

std::shared_ptr<FriendInfo> UserMgr::GetFriendById(int uid)
{
    auto find_it = _friend_map.find(uid);
    if(find_it == _friend_map.end()){
        return nullptr;
    }

    return *find_it;
}

std::vector<std::shared_ptr<FriendInfo> > UserMgr::GetFriendListSnapshot() const
{
    return _friend_list;
}

void UserMgr::AppendFriendChatMsg(int friend_id,std::vector<std::shared_ptr<TextChatData> > msgs)
{
    auto find_iter = _friend_map.find(friend_id);
    if(find_iter == _friend_map.end()){
        qDebug()<<"append friend uid  " << friend_id << " not found";
        return;
    }

    find_iter.value()->AppendChatMsgs(msgs);
}

std::vector<std::shared_ptr<GroupInfoData>> UserMgr::GetGroupListPerPage()
{
    std::vector<std::shared_ptr<GroupInfoData>> groups;
    int begin = _group_loaded;
    int end = begin + CHAT_COUNT_PER_PAGE;
    if (begin >= _group_list.size()) {
        return groups;
    }
    if (end > _group_list.size()) {
        groups = std::vector<std::shared_ptr<GroupInfoData>>(_group_list.begin() + begin, _group_list.end());
        return groups;
    }
    groups = std::vector<std::shared_ptr<GroupInfoData>>(_group_list.begin() + begin, _group_list.begin() + end);
    return groups;
}

bool UserMgr::IsLoadGroupFin()
{
    return _group_loaded >= _group_list.size();
}

void UserMgr::UpdateGroupLoadedCount()
{
    int begin = _group_loaded;
    int end = begin + CHAT_COUNT_PER_PAGE;
    if (begin >= _group_list.size()) {
        return;
    }
    if (end > _group_list.size()) {
        _group_loaded = static_cast<int>(_group_list.size());
        return;
    }
    _group_loaded = end;
}

std::shared_ptr<GroupInfoData> UserMgr::GetGroupById(qint64 groupId)
{
    auto iter = _group_map.find(groupId);
    if (iter == _group_map.end()) {
        return nullptr;
    }
    return iter.value();
}

bool UserMgr::CheckGroupById(qint64 groupId)
{
    return _group_map.find(groupId) != _group_map.end();
}

void UserMgr::UpsertGroup(const std::shared_ptr<GroupInfoData> &groupInfo)
{
    if (!groupInfo || groupInfo->_group_id <= 0) {
        return;
    }
    auto iter = _group_map.find(groupInfo->_group_id);
    if (iter == _group_map.end()) {
        _group_list.push_back(groupInfo);
        _group_map.insert(groupInfo->_group_id, groupInfo);
        return;
    }
    auto stored = iter.value();
    if (!stored) {
        iter.value() = groupInfo;
        return;
    }
    stored->_name = groupInfo->_name;
    stored->_announcement = groupInfo->_announcement;
    stored->_owner_uid = groupInfo->_owner_uid;
    stored->_member_limit = groupInfo->_member_limit;
    stored->_member_count = groupInfo->_member_count;
    stored->_role = groupInfo->_role;
    stored->_is_all_muted = groupInfo->_is_all_muted;
}

void UserMgr::AppendGroupChatMsg(qint64 group_id, const std::shared_ptr<TextChatData> &msg)
{
    auto iter = _group_map.find(group_id);
    if (iter == _group_map.end() || !iter.value() || !msg) {
        return;
    }
    auto &chatMsgs = iter.value()->_chat_msgs;
    for (const auto &one : chatMsgs) {
        if (one && one->_msg_id == msg->_msg_id) {
            return;
        }
    }
    chatMsgs.push_back(msg);
    iter.value()->_last_msg = msg->_msg_content;
}

std::vector<std::shared_ptr<GroupInfoData> > UserMgr::GetGroupListSnapshot() const
{
    return _group_list;
}
