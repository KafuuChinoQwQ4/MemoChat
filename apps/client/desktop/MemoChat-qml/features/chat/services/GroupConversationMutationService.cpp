#include "GroupConversationServiceInternal.h"

using namespace memochat::chat::group_conversation;

GroupMutationResult GroupConversationService::handleMessageMutation(const GroupMutationRequest& request,
                                                                    const GroupConversationDependencies& dependencies)
{
    GroupMutationResult result;
    result.groupId = request.payload.value(QStringLiteral("groupid")).toVariant().toLongLong();
    QString event = request.event;
    if (event.isEmpty())
    {
        event = request.payload.value(QStringLiteral("event")).toString();
    }
    if (event.isEmpty())
    {
        event = request.reqId == kRevokeGroupMsgRsp ? QStringLiteral("group_msg_revoked")
                                                    : QStringLiteral("group_msg_edited");
    }
    result.statusText = groupMutationStatusText(request.reqId, event);
    const bool revoked = event == QStringLiteral("group_msg_revoked");
    MessageUpdateFields updateFields = MessagePayloadService::parseMessageUpdateFields(request.payload, event, revoked);
    if (revoked && updateFields.content.isEmpty())
    {
        updateFields.content = QStringLiteral("[消息已撤回]");
    }
    result.msgId = updateFields.msgId;
    if (result.groupId <= 0 || updateFields.msgId.isEmpty() || updateFields.content.isEmpty() ||
        !dependencies.updateGroupMessageContent)
    {
        return result;
    }

    result.userManagerUpdated = dependencies.updateGroupMessageContent(result.groupId,
                                                                       updateFields.msgId,
                                                                       updateFields.content,
                                                                       updateFields.state,
                                                                       updateFields.editedAtMs,
                                                                       updateFields.deletedAtMs);
    const auto groupInfo = groupById(dependencies, result.groupId);
    result.cacheUpdated = cacheWholeGroupIfReady(dependencies, request.context.selfUid, result.groupId, groupInfo);
    if (groupInfo && result.groupId == request.context.currentGroupId && dependencies.messageModel)
    {
        dependencies.messageModel->setMessages(groupInfo->_chat_msgs, request.context.selfUid);
        result.modelSet = true;
    }
    result.success = result.userManagerUpdated;
    return result;
}

GroupReadAckResult GroupConversationService::handleReadAckEvent(const GroupReadAckRequest& request,
                                                                const GroupConversationDependencies& dependencies)
{
    GroupReadAckResult result;
    result.groupId = request.payload.value(QStringLiteral("groupid")).toVariant().toLongLong();
    result.readerUid = request.payload.value(QStringLiteral("fromuid")).toInt();
    if (result.groupId <= 0 || result.readerUid <= 0 || result.readerUid == request.context.selfUid ||
        !dependencies.markGroupOutgoingReadUntil)
    {
        return result;
    }

    result.readTs = MessagePayloadService::normalizeEpochMs(
        request.payload.value(QStringLiteral("read_ts")).toVariant().toLongLong(), true);
    result.updatedCount =
        dependencies.markGroupOutgoingReadUntil(result.groupId, request.context.selfUid, result.readTs);
    if (result.updatedCount <= 0)
    {
        return result;
    }

    const auto groupInfo = groupById(dependencies, result.groupId);
    result.cacheUpdated = cacheWholeGroupIfReady(dependencies, request.context.selfUid, result.groupId, groupInfo);
    if (groupInfo && result.groupId == request.context.currentGroupId && dependencies.messageModel)
    {
        dependencies.messageModel->setMessages(groupInfo->_chat_msgs, request.context.selfUid);
        result.modelSet = true;
    }
    result.success = true;
    return result;
}

QByteArray GroupConversationService::buildReadAckPayload(int selfUid, qint64 groupId, qint64 readTs)
{
    if (selfUid <= 0 || groupId <= 0)
    {
        return {};
    }
    const qint64 normalizedReadTs = readTs <= 0 ? QDateTime::currentMSecsSinceEpoch() : readTs;
    QJsonObject payloadObj;
    payloadObj[QStringLiteral("fromuid")] = selfUid;
    payloadObj[QStringLiteral("groupid")] = static_cast<qint64>(groupId);
    payloadObj[QStringLiteral("read_ts")] = static_cast<qint64>(normalizedReadTs);
    return QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
}
