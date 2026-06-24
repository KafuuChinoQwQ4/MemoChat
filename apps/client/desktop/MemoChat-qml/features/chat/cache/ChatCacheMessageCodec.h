#ifndef CHATCACHEMESSAGECODEC_H
#define CHATCACHEMESSAGECODEC_H

#include <QString>
#include <memory>

#include "userdata.h"

struct PrivateChatCacheRow
{
    QString msgId;
    QString content;
    int fromUid = 0;
    QString fromUserId;
    int toUid = 0;
    qint64 createdAt = 0;
    qint64 replyToServerMsgId = 0;
    QString forwardMetaJson;
    qint64 editedAtMs = 0;
    qint64 deletedAtMs = 0;
};

struct GroupChatCacheRow
{
    QString msgId;
    QString content;
    int fromUid = 0;
    QString fromUserId;
    QString fromName;
    QString fromIcon;
    qint64 createdAt = 0;
    qint64 serverMsgId = 0;
    qint64 groupSeq = 0;
    qint64 replyToServerMsgId = 0;
    QString forwardMetaJson;
    qint64 editedAtMs = 0;
    qint64 deletedAtMs = 0;
};

qint64 normalizeChatCacheTimestamp(qint64 timestamp);
qint64 normalizeChatCacheOptionalTimestamp(qint64 timestamp);
QString cacheMessageState(qint64 editedAtMs, qint64 deletedAtMs);
std::shared_ptr<TextChatData> privateMessageFromCacheRow(const PrivateChatCacheRow& row);
std::shared_ptr<TextChatData> groupMessageFromCacheRow(const GroupChatCacheRow& row);

#endif // CHATCACHEMESSAGECODEC_H
