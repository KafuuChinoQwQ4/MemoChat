#include "ChatMessageDispatcherGroupHistory.h"

#include "ChatMessageDispatcherGroupPayload.h"

namespace ChatMessageDispatcherGroupHistory
{
std::shared_ptr<TextChatData> buildHistoryTextMessage(const QJsonObject& message)
{
    const QString fromName = message.value("from_nick").toString(message.value("from_name").toString());
    const QString fromIcon = message.value("from_icon").toString();
    const qint64 createdAt =
        ChatMessageDispatcherGroupPayload::normalizeCreatedAt(message.value("created_at").toVariant().toLongLong());
    const qint64 serverMsgId = message.value("server_msg_id").toVariant().toLongLong();
    const qint64 groupSeq = message.value("group_seq").toVariant().toLongLong();
    const qint64 replyToServerMsgId = message.value("reply_to_server_msg_id").toVariant().toLongLong();
    const QString forwardMetaJson = ChatMessageDispatcherGroupPayload::compactJsonValue(message.value("forward_meta"));
    const qint64 editedAtMs = message.value("edited_at_ms").toVariant().toLongLong();
    const qint64 deletedAtMs = message.value("deleted_at_ms").toVariant().toLongLong();
    const QString state = ChatMessageDispatcherGroupPayload::messageState(message);

    return std::make_shared<TextChatData>(message.value("msgid").toString(),
                                          message.value("content").toString(),
                                          message.value("fromuid").toInt(),
                                          0,
                                          fromName,
                                          createdAt,
                                          fromIcon,
                                          state,
                                          serverMsgId,
                                          groupSeq,
                                          replyToServerMsgId,
                                          forwardMetaJson,
                                          editedAtMs,
                                          deletedAtMs);
}
} // namespace ChatMessageDispatcherGroupHistory
