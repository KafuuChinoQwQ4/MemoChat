#pragma once
#include <QString>
#include <memory>
#include <vector>
#include "global.h" // 确保包含 ReqId 定义
#include <QJsonArray>

class AuthInfo {
public:
    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    AuthInfo(int uid, QString name, QString nick, QString icon, int sex):
    _uid(uid), _name(name), _nick(nick), _icon(icon), _sex(sex){}
};

class AuthRsp {
public:
    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    AuthRsp(int uid, QString name, QString nick, QString icon, int sex):
    _uid(uid), _name(name), _nick(nick), _icon(icon), _sex(sex){}
};

class UserInfo {
public:
    UserInfo(int uid, QString name, QString icon):_uid(uid),_name(name),_icon(icon){}
    UserInfo(int uid, QString name, QString nick, QString icon, int sex)
        : _uid(uid), _name(name), _nick(nick), _icon(icon), _sex(sex) {}
    UserInfo(std::shared_ptr<AuthInfo> auth):_uid(auth->_uid),_name(auth->_name),_icon(auth->_icon){}
    UserInfo(std::shared_ptr<AuthRsp> auth):_uid(auth->_uid),_name(auth->_name),_icon(auth->_icon){}
    int _uid;
    QString _name;
    QString _icon;
    QString _nick;
    int _sex = 0;
};

struct AddFriendApply {
    AddFriendApply(int from_uid, QString name, QString desc,
                   QString icon, QString nick, int sex, int status)
        : _from_uid(from_uid), _name(name), _desc(desc),
          _icon(icon), _nick(nick), _sex(sex), _status(status) {}
    int _from_uid;
    QString _name;
    QString _desc;
    QString _icon;
    QString _nick;
    int _sex;
    int _status;
};

class ApplyInfo {
public:
    ApplyInfo(int uid, QString name, QString desc, QString icon, QString nick, int sex, int status)
    :_uid(uid),_name(name),_desc(desc),_icon(icon),_nick(nick),_sex(sex),_status(status){}
    
    ApplyInfo(std::shared_ptr<AddFriendApply> addinfo)
    :_uid(addinfo->_from_uid),_name(addinfo->_name),_desc(addinfo->_desc),
    _icon(addinfo->_icon),_nick(addinfo->_nick),_sex(addinfo->_sex),_status(addinfo->_status){}
    
    int _uid;
    QString _name;
    QString _desc;
    QString _icon;
    QString _nick;
    int _sex;
    int _status;
};

struct TextChatData {
    TextChatData(QString msgid, QString msg_content, int from_uid, int to_uid)
        : _msg_id(msgid), _msg_content(msg_content), _from_uid(from_uid), _to_uid(to_uid) {}
    QString _msg_id;
    QString _msg_content;
    int _from_uid;
    int _to_uid;
};

struct TextChatMsg {
    TextChatMsg(int fromuid, int touid, QJsonArray arrays) :
        _from_uid(fromuid), _to_uid(touid), _chat_msgs(arrays) {}
    int _from_uid;
    int _to_uid;
    QJsonArray _chat_msgs;
};