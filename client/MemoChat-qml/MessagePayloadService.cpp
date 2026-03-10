#include "MessagePayloadService.h"

#include <QDateTime>
#include <QJsonDocument>

namespace {
qint64 pickLongLong(const QJsonObject &primary,
                    const QJsonObject &secondary,
                    const char *key,
                    bool useNowWhenEmpty = false)
{
    qint64 value = primary.value(key).toVariant().toLongLong();
    if (value <= 0) {
        value = secondary.value(key).toVariant().toLongLong();
    }
    return MessagePayloadService::normalizeEpochMs(value, useNowWhenEmpty);
}
}

qint64 MessagePayloadService::normalizeEpochMs(qint64 value, bool useNowWhenEmpty)
{
    if (value <= 0) {
        return useNowWhenEmpty ? QDateTime::currentMSecsSinceEpoch() : 0;
    }
    if (value < 100000000000LL) {
        return value * 1000;
    }
    return value;
}

QString MessagePayloadService::extractForwardMetaJson(const QJsonValue &value)
{
    if (value.isObject()) {
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    }
    if (value.isArray()) {
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    }
    if (value.isString()) {
        return value.toString();
    }
    return {};
}

QString MessagePayloadService::resolveMessageState(const QString &content,
                                                  qint64 editedAtMs,
                                                  qint64 deletedAtMs,
                                                  const QString &msgType)
{
    if (deletedAtMs > 0
        || msgType == QStringLiteral("revoke")
        || content == QStringLiteral("[消息已撤回]")) {
        return QStringLiteral("deleted");
    }
    if (editedAtMs > 0) {
        return QStringLiteral("edited");
    }
    return QStringLiteral("sent");
}

MessageUpdateFields MessagePayloadService::parseMessageUpdateFields(const QJsonObject &payload,
                                                                   const QString &event,
                                                                   bool allowEmptyContent)
{
    MessageUpdateFields fields;
    fields.msgId = payload.value(QStringLiteral("msgid")).toString();
    fields.content = payload.value(QStringLiteral("content")).toString();
    fields.editedAtMs = normalizeEpochMs(payload.value(QStringLiteral("edited_at_ms")).toVariant().toLongLong());
    fields.deletedAtMs = normalizeEpochMs(payload.value(QStringLiteral("deleted_at_ms")).toVariant().toLongLong());
    if (event == QStringLiteral("group_msg_revoked") || event == QStringLiteral("private_msg_revoked")) {
        fields.state = QStringLiteral("deleted");
    } else if (event == QStringLiteral("group_msg_edited") || event == QStringLiteral("private_msg_edited")) {
        fields.state = QStringLiteral("edited");
    } else {
        fields.state = resolveMessageState(fields.content, fields.editedAtMs, fields.deletedAtMs);
    }
    if (!allowEmptyContent && fields.content.isEmpty()) {
        fields.msgId.clear();
    }
    return fields;
}

std::shared_ptr<TextChatData> MessagePayloadService::buildPrivateHistoryMessage(const QJsonObject &obj)
{
    const qint64 editedAtMs = normalizeEpochMs(obj.value(QStringLiteral("edited_at_ms")).toVariant().toLongLong());
    const qint64 deletedAtMs = normalizeEpochMs(obj.value(QStringLiteral("deleted_at_ms")).toVariant().toLongLong());
    const QString content = obj.value(QStringLiteral("content")).toString();
    return std::make_shared<TextChatData>(
        obj.value(QStringLiteral("msgid")).toString(),
        content,
        obj.value(QStringLiteral("fromuid")).toInt(),
        obj.value(QStringLiteral("touid")).toInt(),
        QString(),
        normalizeEpochMs(obj.value(QStringLiteral("created_at")).toVariant().toLongLong(), true),
        QString(),
        resolveMessageState(content, editedAtMs, deletedAtMs),
        0,
        0,
        obj.value(QStringLiteral("reply_to_server_msg_id")).toVariant().toLongLong(),
        extractForwardMetaJson(obj.value(QStringLiteral("forward_meta"))),
        editedAtMs,
        deletedAtMs);
}

std::shared_ptr<TextChatData> MessagePayloadService::buildPrivateForwardedMessage(const QJsonObject &payload,
                                                                                  const QJsonObject &msgObj,
                                                                                  int fromUid)
{
    QString msgId = payload.value(QStringLiteral("client_msg_id")).toString();
    if (msgId.isEmpty()) {
        msgId = msgObj.value(QStringLiteral("msgid")).toString();
    }
    const QString content = msgObj.value(QStringLiteral("content")).toString();
    const qint64 createdAt = pickLongLong(payload, msgObj, "created_at", true);
    const qint64 replyToServerMsgId = pickLongLong(payload, msgObj, "reply_to_server_msg_id");
    QString forwardMetaJson = extractForwardMetaJson(payload.value(QStringLiteral("forward_meta")));
    if (forwardMetaJson.isEmpty()) {
        forwardMetaJson = extractForwardMetaJson(msgObj.value(QStringLiteral("forward_meta")));
    }
    const qint64 editedAtMs = pickLongLong(payload, msgObj, "edited_at_ms");
    const qint64 deletedAtMs = pickLongLong(payload, msgObj, "deleted_at_ms");
    return std::make_shared<TextChatData>(msgId,
                                          content,
                                          fromUid,
                                          payload.value(QStringLiteral("peer_uid")).toInt(),
                                          QString(),
                                          createdAt,
                                          QString(),
                                          resolveMessageState(content, editedAtMs, deletedAtMs),
                                          0,
                                          0,
                                          replyToServerMsgId,
                                          forwardMetaJson,
                                          editedAtMs,
                                          deletedAtMs);
}

std::shared_ptr<TextChatData> MessagePayloadService::buildGroupAckMessage(const QJsonObject &payload,
                                                                          const QJsonObject &msgObj,
                                                                          int fromUid,
                                                                          const QString &fromName,
                                                                          const QString &fromIcon)
{
    QString msgId = payload.value(QStringLiteral("client_msg_id")).toString();
    if (msgId.isEmpty()) {
        msgId = msgObj.value(QStringLiteral("msgid")).toString();
    }
    const QString content = msgObj.value(QStringLiteral("content")).toString();
    const qint64 createdAt = pickLongLong(payload, msgObj, "created_at", true);
    const qint64 serverMsgId = pickLongLong(payload, msgObj, "server_msg_id");
    const qint64 groupSeq = pickLongLong(payload, msgObj, "group_seq");
    const qint64 replyToServerMsgId = pickLongLong(payload, msgObj, "reply_to_server_msg_id");
    QString forwardMetaJson = extractForwardMetaJson(payload.value(QStringLiteral("forward_meta")));
    if (forwardMetaJson.isEmpty()) {
        forwardMetaJson = extractForwardMetaJson(msgObj.value(QStringLiteral("forward_meta")));
    }
    const qint64 editedAtMs = pickLongLong(payload, msgObj, "edited_at_ms");
    const qint64 deletedAtMs = pickLongLong(payload, msgObj, "deleted_at_ms");
    return std::make_shared<TextChatData>(msgId,
                                          content,
                                          fromUid,
                                          0,
                                          fromName,
                                          createdAt,
                                          fromIcon,
                                          resolveMessageState(content,
                                                              editedAtMs,
                                                              deletedAtMs,
                                                              msgObj.value(QStringLiteral("msgtype")).toString()),
                                          serverMsgId,
                                          groupSeq,
                                          replyToServerMsgId,
                                          forwardMetaJson,
                                          editedAtMs,
                                          deletedAtMs);
}

std::shared_ptr<TextChatData> MessagePayloadService::buildGroupIncomingMessage(const GroupChatMsg &msg)
{
    if (!msg._msg) {
        return nullptr;
    }
    return std::make_shared<TextChatData>(msg._msg->_msg_id,
                                          msg._msg->_msg_content,
                                          msg._msg->_from_uid,
                                          static_cast<int>(msg._group_id),
                                          msg._from_name,
                                          msg._msg->_created_at,
                                          msg._from_icon,
                                          resolveMessageState(msg._msg->_msg_content,
                                                              msg._msg->_edited_at_ms,
                                                              msg._msg->_deleted_at_ms,
                                                              msg._msg->_msg_type),
                                          msg._msg->_server_msg_id,
                                          msg._msg->_group_seq,
                                          msg._msg->_reply_to_server_msg_id,
                                          msg._msg->_forward_meta_json,
                                          msg._msg->_edited_at_ms,
                                          msg._msg->_deleted_at_ms);
}
