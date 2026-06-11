#include "GroupConversationServiceInternal.h"

using namespace memochat::chat::group_conversation;

GroupAckResult GroupConversationService::handleMessageAck(const GroupAckRequest& request,
                                                          const GroupConversationDependencies& dependencies)
{
    GroupAckResult result;
    result.statusText = groupAckStatusText(request.reqId);
    if (!hasUsableState(dependencies))
    {
        return result;
    }

    result.groupId = request.payload.value(QStringLiteral("groupid")).toVariant().toLongLong();
    result.clientMsgId = clientMsgIdFor(request.payload);
    const QJsonObject msgObj = request.payload.value(QStringLiteral("msg")).toObject();
    if (result.groupId <= 0 || result.clientMsgId.isEmpty())
    {
        return result;
    }

    const QString senderName = request.payload.value(
        QStringLiteral("from_nick"))
            .toString(request.payload.value(QStringLiteral("from_name")).toString(request.context.selfName));
    const QString senderIcon =
        normalizedSenderIcon(request.payload.value(QStringLiteral("from_icon")).toString(request.context.selfIcon));
    auto correctedMsg = MessagePayloadService::buildGroupAckMessage(request.payload,
                                                                    msgObj,
                                                                    request.context.selfUid,
                                                                    senderName,
                                                                    senderIcon);
    if (!correctedMsg || correctedMsg->_msg_id.isEmpty() || correctedMsg->_msg_content.isEmpty())
    {
        if (request.payload.value(QStringLiteral("status")).toString() ==
                                  QStringLiteral("accepted") && dependencies.updateGroupMessageState)
        {
            result.nextState = QStringLiteral("accepted");
            if (dependencies.updateGroupMessageState(result.groupId, result.clientMsgId, result.nextState) &&
                result.groupId == request.context.currentGroupId && dependencies.messageModel)
            {
                dependencies.messageModel->updateMessageState(result.clientMsgId, result.nextState);
                result.modelPatched = true;
            }
            auto groupInfo = groupById(dependencies, result.groupId);
            result.cacheUpdated =
                cacheWholeGroupIfReady(dependencies, request.context.selfUid, result.groupId, groupInfo);
            result.success = true;
        }
        return result;
    }

    if (dependencies.upsertGroupMessage)
    {
        dependencies.upsertGroupMessage(result.groupId, correctedMsg);
    }
    dependencies.state->pendingMsgGroupMap.remove(correctedMsg->_msg_id);
    dependencies.state->pendingMsgGroupMap.remove(result.clientMsgId);
    result.pendingRemoved = true;
    result.correctedMessage = true;
    result.cacheUpdated = cacheGroupMessages(dependencies, request.context.selfUid, result.groupId, {correctedMsg});

    const int dialogUid = resolveGroupDialogUid(dependencies, result.groupId);
    updateGroupPreview(dependencies,
                       dialogUid,
                       MessageContentCodec::toPreviewText(correctedMsg->_msg_content),
                       MessageContentCodec::toPreviewText(correctedMsg->_msg_content),
                       correctedMsg->_created_at,
                       &result.previewUpdated);

    if (result.groupId == request.context.currentGroupId && dependencies.messageModel)
    {
        dependencies.messageModel->upsertMessage(correctedMsg, request.context.selfUid);
        result.modelPatched = true;
        result.requestedReadAckTs = correctedMsg->_created_at;
    }

    result.success = true;
    return result;
}

GroupAckResult GroupConversationService::handleMessageStatus(const GroupAckRequest& request,
                                                             const GroupConversationDependencies& dependencies)
{
    GroupAckResult result;
    if (!hasUsableState(dependencies))
    {
        return result;
    }
    result.clientMsgId = clientMsgIdFor(request.payload);
    if (result.clientMsgId.isEmpty())
    {
        return result;
    }
    result.groupId = request.payload.value(QStringLiteral("groupid")).toVariant().toLongLong();
    if (result.groupId <= 0)
    {
        result.groupId = dependencies.state->pendingMsgGroupMap.value(result.clientMsgId, 0);
    }
    if (result.groupId <= 0)
    {
        return result;
    }

    const int error = request.payload.value(QStringLiteral("error")).toInt(kProtocolJsonError);
    const QString status = request.payload.value(QStringLiteral("status")).toString();
    const QJsonObject msgObj = request.payload.value(QStringLiteral("msg")).toObject();
    if (error == kProtocolSuccess && status == QStringLiteral("persisted") && !msgObj.isEmpty())
    {
        GroupAckRequest ackRequest = request;
        if (ackRequest.payload.value(QStringLiteral("groupid")).toVariant().toLongLong() <= 0)
        {
            ackRequest.payload.insert(QStringLiteral("groupid"), static_cast<qint64>(result.groupId));
        }
        return handleMessageAck(ackRequest, dependencies);
    }

    result.nextState = MessagePayloadService::resolveMessageStatusState(request.payload);
    if (dependencies.updateGroupMessageState &&
        dependencies.updateGroupMessageState(result.groupId, result.clientMsgId, result.nextState) &&
        result.groupId == request.context.currentGroupId && dependencies.messageModel)
    {
        dependencies.messageModel->updateMessageState(result.clientMsgId, result.nextState);
        result.modelPatched = true;
    }
    const auto groupInfo = groupById(dependencies, result.groupId);
    result.cacheUpdated = cacheWholeGroupIfReady(dependencies, request.context.selfUid, result.groupId, groupInfo);
    if (result.nextState == QStringLiteral("failed"))
    {
        dependencies.state->pendingMsgGroupMap.remove(result.clientMsgId);
        result.pendingRemoved = true;
    }
    result.success = true;
    return result;
}

GroupAckResult GroupConversationService::handleMessageError(const GroupAckRequest& request,
                                                            const GroupConversationDependencies& dependencies)
{
    GroupAckResult result;
    result.statusText = groupMessageErrorStatusText(request.payload, request.errorCode);
    result.statusIsError = true;
    if (!hasUsableState(dependencies))
    {
        return result;
    }
    result.clientMsgId = clientMsgIdFor(request.payload);
    if (result.clientMsgId.isEmpty())
    {
        return result;
    }
    result.groupId = request.payload.value(QStringLiteral("groupid")).toVariant().toLongLong();
    if (result.groupId <= 0)
    {
        result.groupId = dependencies.state->pendingMsgGroupMap.value(result.clientMsgId, 0);
    }
    if (result.groupId <= 0)
    {
        return result;
    }
    result.nextState = QStringLiteral("failed");
    if (dependencies.updateGroupMessageState &&
        dependencies.updateGroupMessageState(result.groupId, result.clientMsgId, result.nextState) &&
        result.groupId == request.context.currentGroupId && dependencies.messageModel)
    {
        dependencies.messageModel->updateMessageState(result.clientMsgId, result.nextState);
        result.modelPatched = true;
    }
    dependencies.state->pendingMsgGroupMap.remove(result.clientMsgId);
    result.pendingRemoved = true;
    result.success = true;
    return result;
}
