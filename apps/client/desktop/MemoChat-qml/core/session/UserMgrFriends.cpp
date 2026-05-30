#include "usermgr.h"

#include <QJsonArray>
#include <algorithm>

void UserMgr::AppendFriendList(QJsonArray array)
{
    _friend_list.clear();
    _friend_map.clear();
    _chat_loaded = 0;
    _contact_loaded = 0;

    for (const QJsonValue& value : array)
    {
        auto name = value["name"].toString();
        auto desc = value["desc"].toString();
        auto icon = value["icon"].toString();
        auto nick = value["nick"].toString();
        auto sex = value["sex"].toInt();
        auto uid = value["uid"].toInt();
        auto userId = value["user_id"].toString();
        auto back = value["back"].toString();

        auto info = std::make_shared<FriendInfo>(uid, name, nick, icon, sex, desc, back, "", userId);
        _friend_list.push_back(info);
        _friend_map.insert(uid, info);
    }
}

std::vector<std::shared_ptr<FriendInfo>> UserMgr::GetChatListPerPage()
{
    std::vector<std::shared_ptr<FriendInfo>> friend_list;
    int begin = _chat_loaded;
    int end = begin + CHAT_COUNT_PER_PAGE;

    if (begin >= _friend_list.size())
    {
        return friend_list;
    }

    if (end > _friend_list.size())
    {
        friend_list = std::vector<std::shared_ptr<FriendInfo>>(_friend_list.begin() + begin, _friend_list.end());
        return friend_list;
    }

    friend_list = std::vector<std::shared_ptr<FriendInfo>>(_friend_list.begin() + begin, _friend_list.begin() + end);
    return friend_list;
}

std::vector<std::shared_ptr<FriendInfo>> UserMgr::GetConListPerPage()
{
    std::vector<std::shared_ptr<FriendInfo>> friend_list;
    int begin = _contact_loaded;
    int end = begin + CHAT_COUNT_PER_PAGE;

    if (begin >= _friend_list.size())
    {
        return friend_list;
    }

    if (end > _friend_list.size())
    {
        friend_list = std::vector<std::shared_ptr<FriendInfo>>(_friend_list.begin() + begin, _friend_list.end());
        return friend_list;
    }

    friend_list = std::vector<std::shared_ptr<FriendInfo>>(_friend_list.begin() + begin, _friend_list.begin() + end);
    return friend_list;
}

bool UserMgr::IsLoadChatFin()
{
    if (_chat_loaded >= _friend_list.size())
    {
        return true;
    }

    return false;
}

void UserMgr::UpdateChatLoadedCount()
{
    int begin = _chat_loaded;
    int end = begin + CHAT_COUNT_PER_PAGE;

    if (begin >= _friend_list.size())
    {
        return;
    }

    if (end > _friend_list.size())
    {
        _chat_loaded = _friend_list.size();
        return;
    }

    _chat_loaded = end;
}

void UserMgr::UpdateContactLoadedCount()
{
    int begin = _contact_loaded;
    int end = begin + CHAT_COUNT_PER_PAGE;

    if (begin >= _friend_list.size())
    {
        return;
    }

    if (end > _friend_list.size())
    {
        _contact_loaded = _friend_list.size();
        return;
    }

    _contact_loaded = end;
}

bool UserMgr::IsLoadConFin()
{
    if (_contact_loaded >= _friend_list.size())
    {
        return true;
    }

    return false;
}

bool UserMgr::CheckFriendById(int uid)
{
    auto iter = _friend_map.find(uid);
    if (iter == _friend_map.end())
    {
        return false;
    }

    return true;
}

void UserMgr::AddFriend(std::shared_ptr<AuthRsp> auth_rsp)
{
    auto friend_info = std::make_shared<FriendInfo>(auth_rsp);
    auto iter = _friend_map.find(friend_info->_uid);
    if (iter == _friend_map.end())
    {
        _friend_list.push_back(friend_info);
    }
    _friend_map[friend_info->_uid] = friend_info;
}

void UserMgr::AddFriend(std::shared_ptr<AuthInfo> auth_info)
{
    auto friend_info = std::make_shared<FriendInfo>(auth_info);
    auto iter = _friend_map.find(friend_info->_uid);
    if (iter == _friend_map.end())
    {
        _friend_list.push_back(friend_info);
    }
    _friend_map[friend_info->_uid] = friend_info;
}

std::shared_ptr<FriendInfo> UserMgr::GetFriendById(int uid)
{
    auto find_it = _friend_map.find(uid);
    if (find_it == _friend_map.end())
    {
        return nullptr;
    }

    return *find_it;
}

void UserMgr::RemoveFriend(int uid)
{
    _friend_map.remove(uid);
    _friend_list.erase(std::remove_if(_friend_list.begin(),
                                      _friend_list.end(),
                                      [uid](const std::shared_ptr<FriendInfo>& info)
                                      {
                                          return info && info->_uid == uid;
                                      }),
                       _friend_list.end());
    if (_chat_loaded > static_cast<int>(_friend_list.size()))
    {
        _chat_loaded = static_cast<int>(_friend_list.size());
    }
    if (_contact_loaded > static_cast<int>(_friend_list.size()))
    {
        _contact_loaded = static_cast<int>(_friend_list.size());
    }
}

std::vector<std::shared_ptr<FriendInfo>> UserMgr::GetFriendListSnapshot() const
{
    return _friend_list;
}
