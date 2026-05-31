#include "ChatMessageDispatcherGroupPayload.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>

namespace ChatMessageDispatcherGroupPayload
{
qint64 normalizeCreatedAt(qint64 createdAt)
{
    if (createdAt <= 0)
    {
        return QDateTime::currentMSecsSinceEpoch();
    }
    if (createdAt < 100000000000LL)
    {
        return createdAt * 1000;
    }
    return createdAt;
}

QString compactJsonValue(const QJsonValue& value)
{
    if (value.isObject())
    {
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    }
    if (value.isArray())
    {
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    }
    if (value.isString())
    {
        return value.toString();
    }
    return {};
}

QString messageState(const QJsonObject& message)
{
    const qint64 deletedAtMs = message.value("deleted_at_ms").toVariant().toLongLong();
    if (deletedAtMs > 0 || message.value("msgtype").toString() == QStringLiteral("revoke") ||
                                                                                 message.value("content").toString() ==
                                                                                     QStringLiteral("[消息已撤回]"))
    {
        return QStringLiteral("deleted");
    }

    const qint64 editedAtMs = message.value("edited_at_ms").toVariant().toLongLong();
    if (editedAtMs > 0)
    {
        return QStringLiteral("edited");
    }

    return QStringLiteral("sent");
}

QJsonObject normalizedNotifyMessage(const QJsonObject& payload)
{
    QJsonObject message = payload.value("msg").toObject();

    const qint64 topCreatedAt = normalizeCreatedAt(payload.value("created_at").toVariant().toLongLong());
    const qint64 messageCreatedAt = message.value("created_at").toVariant().toLongLong();
    message["created_at"] = normalizeCreatedAt(messageCreatedAt <= 0 ? topCreatedAt : messageCreatedAt);

    const qint64 topServerMsgId = payload.value("server_msg_id").toVariant().toLongLong();
    if (!message.contains("server_msg_id") || message.value("server_msg_id").toVariant().toLongLong() <= 0)
    {
        message["server_msg_id"] = topServerMsgId;
    }

    const qint64 topGroupSeq = payload.value("group_seq").toVariant().toLongLong();
    if (!message.contains("group_seq") || message.value("group_seq").toVariant().toLongLong() <= 0)
    {
        message["group_seq"] = topGroupSeq;
    }

    if (!message.contains("reply_to_server_msg_id"))
    {
        const qint64 topReplyToServerMsgId = payload.value("reply_to_server_msg_id").toVariant().toLongLong();
        if (topReplyToServerMsgId > 0)
        {
            message["reply_to_server_msg_id"] = topReplyToServerMsgId;
        }
    }

    if (!message.contains("forward_meta") && payload.contains("forward_meta"))
    {
        message["forward_meta"] = payload.value("forward_meta");
    }
    if (!message.contains("edited_at_ms") && payload.contains("edited_at_ms"))
    {
        message["edited_at_ms"] = payload.value("edited_at_ms");
    }
    if (!message.contains("deleted_at_ms") && payload.contains("deleted_at_ms"))
    {
        message["deleted_at_ms"] = payload.value("deleted_at_ms");
    }

    return message;
}
} // namespace ChatMessageDispatcherGroupPayload
