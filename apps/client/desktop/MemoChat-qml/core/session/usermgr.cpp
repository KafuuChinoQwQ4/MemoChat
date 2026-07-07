#include "usermgr.h"

UserMgr::~UserMgr()
{
}

UserMgr::UserMgr()
    : _user_info(nullptr)
    , _chat_loaded(0)
    , _contact_loaded(0)
    , _group_loaded(0)
{
}

void UserMgr::SetUserInfo(std::shared_ptr<UserInfo> user_info)
{
    _user_info = user_info;
}

void UserMgr::SetToken(QString token)
{
    QMutexLocker locker(&_token_mutex);
    _token = token;
}

int UserMgr::GetUid()
{
    if (!_user_info)
    {
        return 0;
    }
    return _user_info->_uid;
}

QString UserMgr::GetName()
{
    if (!_user_info)
    {
        return {};
    }
    return _user_info->_name;
}

QString UserMgr::GetNick()
{
    if (!_user_info)
    {
        return {};
    }
    return _user_info->_nick;
}

QString UserMgr::GetIcon()
{
    if (!_user_info)
    {
        return {};
    }
    return _user_info->_icon;
}

QString UserMgr::GetDesc()
{
    if (!_user_info)
    {
        return {};
    }
    return _user_info->_desc;
}

QString UserMgr::GetToken()
{
    QMutexLocker locker(&_token_mutex);
    return _token;
}

void UserMgr::UpdateNickAndDesc(const QString& nick, const QString& desc)
{
    if (!_user_info)
    {
        return;
    }
    _user_info->_nick = nick;
    _user_info->_desc = desc;
}

void UserMgr::UpdateIcon(const QString& icon)
{
    if (!_user_info)
    {
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
    {
        QMutexLocker locker(&_token_mutex);
        _token.clear();
    }
    _chat_loaded = 0;
    _contact_loaded = 0;
    _group_loaded = 0;
}

void UserMgr::SlotAddFriendRsp(std::shared_ptr<AuthRsp> rsp)
{
    AddFriend(rsp);
}

void UserMgr::SlotAddFriendAuth(std::shared_ptr<AuthInfo> auth)
{
    AddFriend(auth);
}
