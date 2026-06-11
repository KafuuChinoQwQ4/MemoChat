#include "GroupConversationServiceInternal.h"

using namespace memochat::chat::group_conversation;

GroupSendResult GroupConversationService::sendText(const GroupSendRequest& request,
                                                   const GroupConversationDependencies& dependencies)
{
    GroupSendResult result;
    if (!hasUsableState(dependencies) || request.context.selfUid <= 0 || request.groupId <= 0 ||
        !dependencies.dispatchPayload)
    {
        result.errorText = QStringLiteral("群消息发送器不可用");
        return result;
    }

    const QString msgId = QUuid::createUuid().toString();
    const qint64 createdAt = QDateTime::currentMSecsSinceEpoch();
    const DecodedMessageContent decoded = MessageContentCodec::decode(request.content);
    const QString plainText = decoded.content;
    const bool mentionAll = plainText.contains(QStringLiteral("@all"), Qt::CaseInsensitive) ||
                                               plainText.contains(QStringLiteral("@所有人"));
    const qint64 replyToServerMsgId = replyServerMsgIdFor(dependencies, request.groupId, request.replyToMsgId);

    QJsonObject msgObj;
    msgObj[QStringLiteral("msgid")] = msgId;
    msgObj[QStringLiteral("content")] = request.content;
    msgObj[QStringLiteral("msgtype")] = decoded.type.isEmpty() ? QStringLiteral("text") : decoded.type;
    msgObj[QStringLiteral("mentions")] = mentionsForPlainText(plainText);
    msgObj[QStringLiteral("mention_all")] = mentionAll;
    if (replyToServerMsgId > 0)
    {
        msgObj[QStringLiteral("reply_to_server_msg_id")] = replyToServerMsgId;
    }
    if (!decoded.fileName.isEmpty())
    {
        msgObj[QStringLiteral("file_name")] = decoded.fileName;
    }
    if (!decoded.mimeType.isEmpty())
    {
        msgObj[QStringLiteral("mime")] = decoded.mimeType;
    }
    if (decoded.sizeBytes > 0)
    {
        msgObj[QStringLiteral("size")] = static_cast<qint64>(decoded.sizeBytes);
    }

    QJsonObject payloadObj;
    payloadObj[QStringLiteral("fromuid")] = request.context.selfUid;
    payloadObj[QStringLiteral("groupid")] = static_cast<qint64>(request.groupId);
    payloadObj[QStringLiteral("msg")] = msgObj;
    result.compactPayload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
    dependencies.state->pendingMsgGroupMap.insert(msgId, request.groupId);
    result.dispatched = dispatchIfAvailable(kGroupChatRequestId, result.compactPayload, dependencies);

    result.msgId = msgId;
    result.createdAt = createdAt;
    result.replyToServerMsgId = replyToServerMsgId;
    result.optimisticMessage = std::make_shared<TextChatData>(msgId,
                                                              request.content,
                                                              request.context.selfUid,
                                                              0,
                                                              senderDisplayName(request.context),
                                                              createdAt,
                                                              request.context.selfIcon,
                                                              QStringLiteral("sending"), 0, 0, replyToServerMsgId);
    if (dependencies.upsertGroupMessage)
    {
        dependencies.upsertGroupMessage(request.groupId, result.optimisticMessage);
    }
    result.cacheUpdated =
        cacheGroupMessages(dependencies, request.context.selfUid, request.groupId, {result.optimisticMessage});
    if (dependencies.messageModel)
    {
        dependencies.messageModel->appendMessage(result.optimisticMessage, request.context.selfUid);
        result.modelAppended = true;
    }

    const int dialogUid = resolveGroupDialogUid(dependencies, request.groupId);
    updateGroupPreview(dependencies,
                       dialogUid,
                       previewFor(request.content, request.previewText),
                       previewFor(request.content, request.previewText),
                       createdAt,
                       &result.previewUpdated);
    clearUnread(dependencies, dialogUid, &result.unreadCleared);

    result.success = true;
    return result;
}
