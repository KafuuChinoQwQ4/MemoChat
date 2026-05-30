#include "ChatCacheMessageCodec.h"

#include <QDateTime>

qint64 normalizeChatCacheTimestamp(qint64 timestamp)
{
    if (timestamp <= 0)
    {
        return QDateTime::currentMSecsSinceEpoch();
    }
    if (timestamp < 100000000000LL)
    {
        return timestamp * 1000;
    }
    return timestamp;
}

qint64 normalizeChatCacheOptionalTimestamp(qint64 timestamp)
{
    if (timestamp <= 0)
    {
        return 0;
    }
    return normalizeChatCacheTimestamp(timestamp);
}

QString cacheMessageState(qint64 editedAtMs, qint64 deletedAtMs)
{
    if (normalizeChatCacheOptionalTimestamp(deletedAtMs) > 0)
    {
        return QStringLiteral("deleted");
    }
    if (normalizeChatCacheOptionalTimestamp(editedAtMs) > 0)
    {
        return QStringLiteral("edited");
    }
    return QStringLiteral("sent");
}

std::shared_ptr<TextChatData> privateMessageFromCacheRow(const PrivateChatCacheRow& row)
{
    const qint64 editedAtMs = normalizeChatCacheOptionalTimestamp(row.editedAtMs);
    const qint64 deletedAtMs = normalizeChatCacheOptionalTimestamp(row.deletedAtMs);
    return std::make_shared<TextChatData>(row.msgId,
                                          row.content,
                                          row.fromUid,
                                          row.toUid,
                                          QString(),
                                          row.createdAt,
                                          QString(),
                                          cacheMessageState(editedAtMs, deletedAtMs),
                                          0,
                                          0,
                                          row.replyToServerMsgId,
                                          row.forwardMetaJson,
                                          editedAtMs,
                                          deletedAtMs);
}

std::shared_ptr<TextChatData> groupMessageFromCacheRow(const GroupChatCacheRow& row)
{
    const qint64 editedAtMs = normalizeChatCacheOptionalTimestamp(row.editedAtMs);
    const qint64 deletedAtMs = normalizeChatCacheOptionalTimestamp(row.deletedAtMs);
    return std::make_shared<TextChatData>(row.msgId,
                                          row.content,
                                          row.fromUid,
                                          0,
                                          row.fromName,
                                          normalizeChatCacheTimestamp(row.createdAt),
                                          row.fromIcon,
                                          cacheMessageState(editedAtMs, deletedAtMs),
                                          row.serverMsgId,
                                          row.groupSeq,
                                          row.replyToServerMsgId,
                                          row.forwardMetaJson,
                                          editedAtMs,
                                          deletedAtMs);
}
