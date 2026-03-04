#ifndef USERDATA_H
#define USERDATA_H
#include <QString>
#include <memory>
#include <QJsonArray>
#include <vector>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

class SearchInfo {
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

class AddFriendApply {
public:
    AddFriendApply(int from_uid, QString name, QString desc,
                   QString icon, QString nick, int sex, QString userId = "");
	int _from_uid;
    QString _user_id;
	QString _name;
	QString _desc;
    QString _icon;
    QString _nick;
    int     _sex;
};

struct ApplyInfo {
    ApplyInfo(int uid, QString name, QString desc,
        QString icon, QString nick, int sex, int status, QString userId = "")
        :_uid(uid),_name(name),_desc(desc),
        _icon(icon),_nick(nick),_sex(sex),_status(status),_user_id(userId){}

    ApplyInfo(std::shared_ptr<AddFriendApply> addinfo)
        :_uid(addinfo->_from_uid),_name(addinfo->_name),
          _desc(addinfo->_desc),_icon(addinfo->_icon),
          _nick(addinfo->_nick),_sex(addinfo->_sex),
          _status(0),_user_id(addinfo->_user_id)
    {}
    void SetIcon(QString head){
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

struct AuthInfo {
    AuthInfo(int uid, QString name,
             QString nick, QString icon, int sex, QString userId = ""):
        _uid(uid), _name(name), _nick(nick), _icon(icon),
        _sex(sex), _user_id(userId){}
    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    QString _user_id;
};

struct AuthRsp {
    AuthRsp(int peer_uid, QString peer_name,
            QString peer_nick, QString peer_icon, int peer_sex, QString userId = "")
        :_uid(peer_uid),_name(peer_name),_nick(peer_nick),
          _icon(peer_icon),_sex(peer_sex),_user_id(userId)
    {}

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    QString _user_id;
};

struct TextChatData;
struct FriendInfo {
    FriendInfo(int uid, QString name, QString nick, QString icon,
        int sex, QString desc, QString back, QString last_msg="", QString userId=""):_uid(uid),
        _name(name),_nick(nick),_icon(icon),_sex(sex),_desc(desc),
        _back(back),_last_msg(last_msg),_user_id(userId){}

    FriendInfo(std::shared_ptr<AuthInfo> auth_info):_uid(auth_info->_uid),
    _nick(auth_info->_nick),_icon(auth_info->_icon),_name(auth_info->_name),
      _sex(auth_info->_sex),_user_id(auth_info->_user_id){}

    FriendInfo(std::shared_ptr<AuthRsp> auth_rsp):_uid(auth_rsp->_uid),
    _nick(auth_rsp->_nick),_icon(auth_rsp->_icon),_name(auth_rsp->_name),
      _sex(auth_rsp->_sex),_user_id(auth_rsp->_user_id){}

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

struct GroupMemberData {
    int _uid = 0;
    QString _user_id;
    QString _name;
    QString _nick;
    QString _icon;
    int _role = 1;
    qint64 _mute_until = 0;
};

struct GroupInfoData {
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

struct UserInfo {
    UserInfo(int uid, QString name, QString nick, QString icon, int sex, QString last_msg = "", QString desc="", QString userId=""):
        _uid(uid),_name(name),_nick(nick),_icon(icon),_sex(sex),_last_msg(last_msg),_desc(desc),_user_id(userId){}

    UserInfo(std::shared_ptr<AuthInfo> auth):
        _uid(auth->_uid),_name(auth->_name),_nick(auth->_nick),
        _icon(auth->_icon),_sex(auth->_sex),_last_msg(""),_desc(""),_user_id(auth->_user_id){}

    UserInfo(int uid, QString name, QString icon):
    _uid(uid), _name(name), _icon(icon),_nick(_name),
    _sex(0),_last_msg(""),_desc(""),_user_id(""){

    }

    UserInfo(std::shared_ptr<AuthRsp> auth):
        _uid(auth->_uid),_name(auth->_name),_nick(auth->_nick),
        _icon(auth->_icon),_sex(auth->_sex),_last_msg(""),_user_id(auth->_user_id){}

    UserInfo(std::shared_ptr<SearchInfo> search_info):
        _uid(search_info->_uid),_name(search_info->_name),_nick(search_info->_nick),
    _icon(search_info->_icon),_sex(search_info->_sex),_last_msg(""),_user_id(search_info->_user_id){

    }

    UserInfo(std::shared_ptr<FriendInfo> friend_info):
        _uid(friend_info->_uid),_name(friend_info->_name),_nick(friend_info->_nick),
        _icon(friend_info->_icon),_sex(friend_info->_sex),_last_msg(""),_user_id(friend_info->_user_id){
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

struct TextChatData{
    TextChatData(QString msg_id, QString msg_content, int fromuid, int touid,
                 const QString &from_name = QString(), qint64 created_at = 0,
                 const QString &from_icon = QString(), const QString &msg_state = QStringLiteral("sent"),
                 qint64 server_msg_id = 0, qint64 group_seq = 0,
                 qint64 reply_to_server_msg_id = 0, const QString &forward_meta_json = QString(),
                 qint64 edited_at_ms = 0, qint64 deleted_at_ms = 0)
        :_msg_id(msg_id),_msg_content(msg_content),_from_uid(fromuid),_to_uid(touid),
         _from_name(from_name),_created_at(created_at),_from_icon(from_icon),_msg_state(msg_state),
         _server_msg_id(server_msg_id),_group_seq(group_seq),
         _reply_to_server_msg_id(reply_to_server_msg_id),_forward_meta_json(forward_meta_json),
         _edited_at_ms(edited_at_ms),_deleted_at_ms(deleted_at_ms){
        if (_created_at <= 0) {
            _created_at = QDateTime::currentMSecsSinceEpoch();
        } else if (_created_at < 100000000000LL) {
            _created_at *= 1000;
        }
        if (_server_msg_id < 0) {
            _server_msg_id = 0;
        }
        if (_group_seq < 0) {
            _group_seq = 0;
        }
        if (_reply_to_server_msg_id < 0) {
            _reply_to_server_msg_id = 0;
        }
        if (_edited_at_ms > 0 && _edited_at_ms < 100000000000LL) {
            _edited_at_ms *= 1000;
        }
        if (_deleted_at_ms > 0 && _deleted_at_ms < 100000000000LL) {
            _deleted_at_ms *= 1000;
        }
    }
    QString _msg_id;
    QString _msg_content;
    int _from_uid;
    int _to_uid;
    QString _from_name;
    qint64 _created_at;
    QString _from_icon;
    QString _msg_state;
    qint64 _server_msg_id;
    qint64 _group_seq;
    qint64 _reply_to_server_msg_id;
    QString _forward_meta_json;
    qint64 _edited_at_ms;
    qint64 _deleted_at_ms;
};

struct GroupChatData {
    GroupChatData(QString msg_id, QString msg_content, int fromuid, qint64 groupid,
                  QString msg_type = "text", qint64 created_at = 0,
                  QString file_name = "", QString mime = "", int size = 0,
                  qint64 server_msg_id = 0, qint64 group_seq = 0,
                  qint64 reply_to_server_msg_id = 0, const QString &forward_meta_json = QString(),
                  qint64 edited_at_ms = 0, qint64 deleted_at_ms = 0,
                  const QJsonArray &mentions = QJsonArray(), bool mention_all = false)
        : _msg_id(msg_id), _msg_content(msg_content), _from_uid(fromuid), _group_id(groupid),
          _msg_type(msg_type), _created_at(created_at), _file_name(file_name), _mime(mime), _size(size),
          _server_msg_id(server_msg_id), _group_seq(group_seq),
          _reply_to_server_msg_id(reply_to_server_msg_id), _forward_meta_json(forward_meta_json),
          _edited_at_ms(edited_at_ms), _deleted_at_ms(deleted_at_ms),
          _mentions(mentions), _mention_all(mention_all) {}

    QString _msg_id;
    QString _msg_content;
    int _from_uid;
    qint64 _group_id;
    QString _msg_type;
    qint64 _created_at;
    QString _file_name;
    QString _mime;
    int _size;
    qint64 _server_msg_id;
    qint64 _group_seq;
    qint64 _reply_to_server_msg_id;
    QString _forward_meta_json;
    qint64 _edited_at_ms;
    qint64 _deleted_at_ms;
    QJsonArray _mentions;
    bool _mention_all;
};

struct GroupChatMsg {
    GroupChatMsg(qint64 groupid, int fromuid, QJsonObject msg_obj, const QString &from_name = QString(), const QString &from_icon = QString())
        : _group_id(groupid), _from_uid(fromuid), _from_name(from_name), _from_icon(from_icon) {
        auto content = msg_obj.value("content").toString();
        auto msgid = msg_obj.value("msgid").toString();
        auto msgtype = msg_obj.value("msgtype").toString("text");
        auto created_at = msg_obj.value("created_at").toVariant().toLongLong();
        if (created_at <= 0) {
            created_at = QDateTime::currentMSecsSinceEpoch();
        } else if (created_at < 100000000000LL) {
            created_at *= 1000;
        }
        auto file_name = msg_obj.value("file_name").toString();
        auto mime = msg_obj.value("mime").toString();
        auto size = msg_obj.value("size").toInt();
        auto server_msg_id = msg_obj.value("server_msg_id").toVariant().toLongLong();
        auto group_seq = msg_obj.value("group_seq").toVariant().toLongLong();
        auto reply_to_server_msg_id = msg_obj.value("reply_to_server_msg_id").toVariant().toLongLong();
        QString forward_meta_json;
        const QJsonValue forwardMetaValue = msg_obj.value("forward_meta");
        if (forwardMetaValue.isObject()) {
            forward_meta_json = QString::fromUtf8(QJsonDocument(forwardMetaValue.toObject()).toJson(QJsonDocument::Compact));
        } else if (forwardMetaValue.isArray()) {
            forward_meta_json = QString::fromUtf8(QJsonDocument(forwardMetaValue.toArray()).toJson(QJsonDocument::Compact));
        } else if (forwardMetaValue.isString()) {
            forward_meta_json = forwardMetaValue.toString();
        }
        auto edited_at_ms = msg_obj.value("edited_at_ms").toVariant().toLongLong();
        auto deleted_at_ms = msg_obj.value("deleted_at_ms").toVariant().toLongLong();
        auto mentions = msg_obj.value("mentions").toArray();
        const bool mention_all = msg_obj.value("mention_all").toBool(false);
        _msg = std::make_shared<GroupChatData>(msgid,
                                               content,
                                               fromuid,
                                               groupid,
                                               msgtype,
                                               created_at,
                                               file_name,
                                               mime,
                                               size,
                                               server_msg_id,
                                               group_seq,
                                               reply_to_server_msg_id,
                                               forward_meta_json,
                                               edited_at_ms,
                                               deleted_at_ms,
                                               mentions,
                                               mention_all);
    }

    qint64 _group_id;
    int _from_uid;
    QString _from_name;
    QString _from_icon;
    std::shared_ptr<GroupChatData> _msg;
};

struct TextChatMsg{
    TextChatMsg(int fromuid, int touid, QJsonArray arrays):
        _from_uid(fromuid),_to_uid(touid){
        for(auto  msg_data : arrays){
            auto msg_obj = msg_data.toObject();
            auto content = msg_obj["content"].toString();
            auto msgid = msg_obj["msgid"].toString();
            auto createdAt = msg_obj["created_at"].toVariant().toLongLong();
            if (createdAt <= 0) {
                createdAt = QDateTime::currentMSecsSinceEpoch();
            }
            auto reply_to_server_msg_id = msg_obj["reply_to_server_msg_id"].toVariant().toLongLong();
            QString forward_meta_json;
            const QJsonValue forwardMetaValue = msg_obj["forward_meta"];
            if (forwardMetaValue.isObject()) {
                forward_meta_json = QString::fromUtf8(QJsonDocument(forwardMetaValue.toObject()).toJson(QJsonDocument::Compact));
            } else if (forwardMetaValue.isArray()) {
                forward_meta_json = QString::fromUtf8(QJsonDocument(forwardMetaValue.toArray()).toJson(QJsonDocument::Compact));
            } else if (forwardMetaValue.isString()) {
                forward_meta_json = forwardMetaValue.toString();
            }
            auto edited_at_ms = msg_obj["edited_at_ms"].toVariant().toLongLong();
            auto deleted_at_ms = msg_obj["deleted_at_ms"].toVariant().toLongLong();
            auto msg_ptr = std::make_shared<TextChatData>(msgid,
                                                          content,
                                                          fromuid,
                                                          touid,
                                                          QString(),
                                                          createdAt,
                                                          QString(),
                                                          QStringLiteral("sent"),
                                                          0,
                                                          0,
                                                          reply_to_server_msg_id,
                                                          forward_meta_json,
                                                          edited_at_ms,
                                                          deleted_at_ms);
            _chat_msgs.push_back(msg_ptr);
        }
    }
    int _to_uid;
    int _from_uid;
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;
};

#endif
