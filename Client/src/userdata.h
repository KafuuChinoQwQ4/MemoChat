#pragma once
#include <QString>
#include <memory>
#include <vector>

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
    UserInfo(std::shared_ptr<AuthInfo> auth):_uid(auth->_uid),_name(auth->_name),_icon(auth->_icon){}
    UserInfo(std::shared_ptr<AuthRsp> auth):_uid(auth->_uid),_name(auth->_name),_icon(auth->_icon){}
    int _uid;
    QString _name;
    QString _icon;
};

class AddFriendApply {
public:
    int _from_uid;
    QString _name;
    QString _desc;
    QString _icon;
    QString _nick;
    int _sex;
    int _status;
    AddFriendApply(int from_uid, QString name, QString desc, QString icon, QString nick, int sex, int status)
    :_from_uid(from_uid),_name(name),_desc(desc),_icon(icon),_nick(nick),_sex(sex),_status(status){}
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