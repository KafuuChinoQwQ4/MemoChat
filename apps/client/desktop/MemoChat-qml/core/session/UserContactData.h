#ifndef USERCONTACTDATA_H
#define USERCONTACTDATA_H

#include <QString>
#include <memory>
#include <vector>

struct TextChatData;

class SearchInfo
{
public:
    SearchInfo(int uid, QString name, QString nick, QString desc, int sex, QString icon, QString userId = "");
    int _uid;
    QString _user_id;
    QString _name;
    QString _nick;
    QString _desc;
    int _sex;
    QString _icon;
};

class AddFriendApply
{
public:
    AddFriendApply(int from_uid, QString name, QString desc, QString icon, QString nick, int sex, QString userId = "");
    int _from_uid;
    QString _user_id;
    QString _name;
    QString _desc;
    QString _icon;
    QString _nick;
    int _sex;
};

struct ApplyInfo
{
    ApplyInfo(int uid, QString name, QString desc, QString icon, QString nick, int sex, int status, QString userId = "")
        : _uid(uid)
        , _name(name)
        , _desc(desc)
        , _icon(icon)
        , _nick(nick)
        , _sex(sex)
        , _status(status)
        , _user_id(userId)
    {
    }

    ApplyInfo(std::shared_ptr<AddFriendApply> addinfo)
        : _uid(addinfo->_from_uid)
        , _name(addinfo->_name)
        , _desc(addinfo->_desc)
        , _icon(addinfo->_icon)
        , _nick(addinfo->_nick)
        , _sex(addinfo->_sex)
        , _status(0)
        , _user_id(addinfo->_user_id)
    {
    }

    void SetIcon(QString head)
    {
        _icon = head;
    }

    int _uid;
    QString _name;
    QString _desc;
    QString _icon;
    QString _nick;
    int _sex;
    int _status;
    QString _user_id;
};

struct AuthInfo
{
    AuthInfo(int uid, QString name, QString nick, QString icon, int sex, QString userId = "")
        : _uid(uid)
        , _name(name)
        , _nick(nick)
        , _icon(icon)
        , _sex(sex)
        , _user_id(userId)
    {
    }

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    QString _user_id;
};

struct AuthRsp
{
    AuthRsp(int peer_uid, QString peer_name, QString peer_nick, QString peer_icon, int peer_sex, QString userId = "")
        : _uid(peer_uid)
        , _name(peer_name)
        , _nick(peer_nick)
        , _icon(peer_icon)
        , _sex(peer_sex)
        , _user_id(userId)
    {
    }

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    QString _user_id;
};

struct FriendInfo
{
    FriendInfo(int uid,
               QString name,
               QString nick,
               QString icon,
               int sex,
               QString desc,
               QString back,
               QString last_msg = "",
               QString userId = "")
        : _uid(uid)
        , _name(name)
        , _nick(nick)
        , _icon(icon)
        , _sex(sex)
        , _desc(desc)
        , _back(back)
        , _last_msg(last_msg)
        , _user_id(userId)
    {
    }

    FriendInfo(std::shared_ptr<AuthInfo> auth_info)
        : _uid(auth_info->_uid)
        , _nick(auth_info->_nick)
        , _icon(auth_info->_icon)
        , _name(auth_info->_name)
        , _sex(auth_info->_sex)
        , _user_id(auth_info->_user_id)
    {
    }

    FriendInfo(std::shared_ptr<AuthRsp> auth_rsp)
        : _uid(auth_rsp->_uid)
        , _nick(auth_rsp->_nick)
        , _icon(auth_rsp->_icon)
        , _name(auth_rsp->_name)
        , _sex(auth_rsp->_sex)
        , _user_id(auth_rsp->_user_id)
    {
    }

    void AppendChatMsgs(const std::vector<std::shared_ptr<TextChatData>> text_vec);

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    QString _desc;
    QString _back;
    QString _last_msg;
    QString _user_id;
    QString _dialog_type;
    int _unread_count = 0;
    int _pinned_rank = 0;
    QString _draft_text;
    qint64 _last_msg_ts = 0;
    int _mute_state = 0;
    int _mention_count = 0;
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;
};

struct UserInfo
{
    UserInfo(int uid,
             QString name,
             QString nick,
             QString icon,
             int sex,
             QString last_msg = "",
             QString desc = "",
             QString userId = "")
        : _uid(uid)
        , _name(name)
        , _nick(nick)
        , _icon(icon)
        , _sex(sex)
        , _desc(desc)
        , _last_msg(last_msg)
        , _user_id(userId)
    {
    }

    UserInfo(std::shared_ptr<AuthInfo> auth)
        : _uid(auth->_uid)
        , _name(auth->_name)
        , _nick(auth->_nick)
        , _icon(auth->_icon)
        , _sex(auth->_sex)
        , _desc("")
        , _last_msg("")
        , _user_id(auth->_user_id)
    {
    }

    UserInfo(int uid, QString name, QString icon)
        : _uid(uid)
        , _name(name)
        , _nick(_name)
        , _icon(icon)
        , _sex(0)
        , _desc("")
        , _last_msg("")
        , _user_id("")
    {
    }

    UserInfo(std::shared_ptr<AuthRsp> auth)
        : _uid(auth->_uid)
        , _name(auth->_name)
        , _nick(auth->_nick)
        , _icon(auth->_icon)
        , _sex(auth->_sex)
        , _last_msg("")
        , _user_id(auth->_user_id)
    {
    }

    UserInfo(std::shared_ptr<SearchInfo> search_info)
        : _uid(search_info->_uid)
        , _name(search_info->_name)
        , _nick(search_info->_nick)
        , _icon(search_info->_icon)
        , _sex(search_info->_sex)
        , _last_msg("")
        , _user_id(search_info->_user_id)
    {
    }

    UserInfo(std::shared_ptr<FriendInfo> friend_info)
        : _uid(friend_info->_uid)
        , _name(friend_info->_name)
        , _nick(friend_info->_nick)
        , _icon(friend_info->_icon)
        , _sex(friend_info->_sex)
        , _last_msg("")
        , _user_id(friend_info->_user_id)
    {
        _chat_msgs = friend_info->_chat_msgs;
    }

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    QString _desc;
    QString _last_msg;
    QString _user_id;
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;
};

#endif // USERCONTACTDATA_H
