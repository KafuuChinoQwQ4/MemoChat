#ifndef USERGROUPDATA_H
#define USERGROUPDATA_H

#include <QString>
#include <memory>
#include <vector>
#include <QtGlobal>

struct TextChatData;

struct GroupMemberData
{
    int _uid = 0;
    QString _user_id;
    QString _name;
    QString _nick;
    QString _icon;
    int _role = 1;
    qint64 _mute_until = 0;
};

struct GroupInfoData
{
    qint64 _group_id = 0;
    QString _group_code;
    QString _name;
    QString _icon;
    QString _announcement;
    int _owner_uid = 0;
    int _member_limit = 200;
    int _member_count = 0;
    int _role = 1;
    int _is_all_muted = 0;
    qint64 _permission_bits = 0;
    QString _last_msg;
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;
    std::vector<std::shared_ptr<GroupMemberData>> _members;
};

#endif // USERGROUPDATA_H
