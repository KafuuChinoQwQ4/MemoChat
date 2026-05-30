#ifndef USERMESSAGEDATA_H
#define USERMESSAGEDATA_H

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <memory>
#include <vector>
#include <QtGlobal>

struct TextChatData
{
    TextChatData(QString msg_id,
                 QString msg_content,
                 int fromuid,
                 int touid,
                 const QString& from_name = QString(),
                 qint64 created_at = 0,
                 const QString& from_icon = QString(),
                 const QString& msg_state = QStringLiteral("sent"),
                                                           qint64 server_msg_id = 0,
                                                           qint64 group_seq = 0,
                                                           qint64 reply_to_server_msg_id = 0,
                                                           const QString& forward_meta_json = QString(),
                                                           qint64 edited_at_ms = 0,
                                                           qint64 deleted_at_ms = 0)
        : _msg_id(msg_id)
                 , _msg_content(msg_content)
                 , _from_uid(fromuid)
                 , _to_uid(touid)
                 , _from_name(from_name)
                 , _created_at(created_at)
                 , _from_icon(from_icon)
                 , _msg_state(msg_state)
                 , _server_msg_id(server_msg_id)
                 , _group_seq(group_seq)
                 , _reply_to_server_msg_id(reply_to_server_msg_id)
                 , _forward_meta_json(forward_meta_json)
                 , _edited_at_ms(edited_at_ms)
                 , _deleted_at_ms(deleted_at_ms)
    {
        if (_created_at <= 0)
        {
            _created_at = QDateTime::currentMSecsSinceEpoch();
        }
        else if (_created_at < 100000000000LL)
        {
            _created_at *= 1000;
        }
        if (_server_msg_id < 0)
        {
            _server_msg_id = 0;
        }
        if (_group_seq < 0)
        {
            _group_seq = 0;
        }
        if (_reply_to_server_msg_id < 0)
        {
            _reply_to_server_msg_id = 0;
        }
        if (_edited_at_ms > 0 && _edited_at_ms < 100000000000LL)
        {
            _edited_at_ms *= 1000;
        }
        if (_deleted_at_ms > 0 && _deleted_at_ms < 100000000000LL)
        {
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

struct GroupChatData
{
    GroupChatData(QString msg_id,
                  QString msg_content,
                  int fromuid,
                  qint64 groupid,
                  QString msg_type = "text",
                  qint64 created_at = 0,
                  QString file_name = "",
                  QString mime = "",
                  int size = 0,
                  qint64 server_msg_id = 0,
                  qint64 group_seq = 0,
                  qint64 reply_to_server_msg_id = 0,
                  const QString& forward_meta_json = QString(),
                  qint64 edited_at_ms = 0,
                  qint64 deleted_at_ms = 0,
                  const QJsonArray& mentions = QJsonArray(),
                  bool mention_all = false)
        : _msg_id(msg_id)
        , _msg_content(msg_content)
        , _from_uid(fromuid)
        , _group_id(groupid)
        , _msg_type(msg_type)
        , _created_at(created_at)
        , _file_name(file_name)
        , _mime(mime)
        , _size(size)
        , _server_msg_id(server_msg_id)
        , _group_seq(group_seq)
        , _reply_to_server_msg_id(reply_to_server_msg_id)
        , _forward_meta_json(forward_meta_json)
        , _edited_at_ms(edited_at_ms)
        , _deleted_at_ms(deleted_at_ms)
        , _mentions(mentions)
        , _mention_all(mention_all)
    {
    }

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

struct GroupChatMsg
{
    GroupChatMsg(qint64 groupid,
                 int fromuid,
                 QJsonObject msg_obj,
                 const QString& from_name = QString(),
                 const QString& from_icon = QString())
        : _group_id(groupid)
        , _from_uid(fromuid)
        , _from_name(from_name)
        , _from_icon(from_icon)
    {
        auto content = msg_obj.value("content").toString();
        auto msgid = msg_obj.value("msgid").toString();
        auto msgtype = msg_obj.value("msgtype").toString("text");
        auto created_at = msg_obj.value("created_at").toVariant().toLongLong();
        if (created_at <= 0)
        {
            created_at = QDateTime::currentMSecsSinceEpoch();
        }
        else if (created_at < 100000000000LL)
        {
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
        if (forwardMetaValue.isObject())
        {
            forward_meta_json =
                QString::fromUtf8(QJsonDocument(forwardMetaValue.toObject()).toJson(QJsonDocument::Compact));
        }
        else if (forwardMetaValue.isArray())
        {
            forward_meta_json =
                QString::fromUtf8(QJsonDocument(forwardMetaValue.toArray()).toJson(QJsonDocument::Compact));
        }
        else if (forwardMetaValue.isString())
        {
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

struct TextChatMsg
{
    TextChatMsg(int fromuid, int touid, QJsonArray arrays)
        : _to_uid(touid)
        , _from_uid(fromuid)
    {
        for (auto msg_data : arrays)
        {
            auto msg_obj = msg_data.toObject();
            auto content = msg_obj["content"].toString();
            auto msgid = msg_obj["msgid"].toString();
            auto createdAt = msg_obj["created_at"].toVariant().toLongLong();
            if (createdAt <= 0)
            {
                createdAt = QDateTime::currentMSecsSinceEpoch();
            }
            auto reply_to_server_msg_id = msg_obj["reply_to_server_msg_id"].toVariant().toLongLong();
            QString forward_meta_json;
            const QJsonValue forwardMetaValue = msg_obj["forward_meta"];
            if (forwardMetaValue.isObject())
            {
                forward_meta_json =
                    QString::fromUtf8(QJsonDocument(forwardMetaValue.toObject()).toJson(QJsonDocument::Compact));
            }
            else if (forwardMetaValue.isArray())
            {
                forward_meta_json =
                    QString::fromUtf8(QJsonDocument(forwardMetaValue.toArray()).toJson(QJsonDocument::Compact));
            }
            else if (forwardMetaValue.isString())
            {
                forward_meta_json = forwardMetaValue.toString();
            }
            auto edited_at_ms = msg_obj["edited_at_ms"].toVariant().toLongLong();
            auto deleted_at_ms = msg_obj["deleted_at_ms"].toVariant().toLongLong();
            auto msg_ptr = std::make_shared<TextChatData>(
                msgid,
                content,
                fromuid,
                touid,
                QString(),
                createdAt,
                QString(),
                QStringLiteral("sent"), 0, 0, reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms);
            _chat_msgs.push_back(msg_ptr);
        }
    }

    int _to_uid;
    int _from_uid;
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;
};

#endif // USERMESSAGEDATA_H
