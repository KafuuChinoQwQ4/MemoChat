#include "PrivateChatEventServiceInternal.h"

using namespace memochat::chat::private_event;

PrivateMessageChangedResult
PrivateChatEventService::handleMessageMutationResponse(const PrivateMessageMutationResponseRequest& request,
                                                       const PrivateChatEventDependencies& dependencies)
{
    PrivateMessageChangedResult result;
    result.reqId = request.reqId;
    result.event = privateMutationEventForResponse(request.reqId);
    result.statusText = privateMutationStatusText(request.reqId);
    result.statusIsError = result.event.isEmpty();
    if (result.event.isEmpty())
    {
        return result;
    }

    QJsonObject servicePayload = request.payload;
    servicePayload.insert(QStringLiteral("error"), kProtocolSuccess);
    servicePayload.insert(QStringLiteral("event"), result.event);

    PrivateMessageChangedRequest changedRequest;
    changedRequest.context = request.context;
    changedRequest.payload = std::move(servicePayload);
    PrivateMessageChangedResult changedResult = handleMessageChanged(changedRequest, dependencies);
    changedResult.reqId = result.reqId;
    changedResult.event = result.event;
    changedResult.statusText = result.statusText;
    changedResult.statusIsError = result.statusIsError;
    return changedResult;
}

bool PrivateChatEventService::isPrivateMessageResponse(int reqId)
{
    return reqId == kEditPrivateMsgRsp || reqId == kRevokePrivateMsgRsp || reqId == kForwardPrivateMsgRsp;
}

QString PrivateChatEventService::privateMessageResponseActionText(int reqId)
{
    return privateResponseActionText(reqId);
}

PrivateForwardResponseResult
PrivateChatEventService::handleForwardResponse(const PrivateForwardResponseRequest& request,
                                               const PrivateChatEventDependencies& dependencies)
{
    PrivateForwardResponseResult result;
    result.statusText = QStringLiteral("消息转发失败");
    if (request.context.selfUid <= 0)
    {
        result.isError = true;
        return result;
    }

    result.peerUid = request.payload.value(QStringLiteral("peer_uid")).toInt();
    const QJsonObject msgObj = request.payload.value(QStringLiteral("msg")).toObject();
    const std::shared_ptr<TextChatData> forwardedMessage =
        MessagePayloadService::buildPrivateForwardedMessage(request.payload, msgObj, request.context.selfUid);
    if (result.peerUid <= 0 || !forwardedMessage || forwardedMessage->_msg_id.isEmpty() ||
        forwardedMessage->_msg_content.isEmpty())
    {
        result.isError = true;
        return result;
    }

    result.msgId = forwardedMessage->_msg_id;
    result.content = forwardedMessage->_msg_content;
    result.createdAt = forwardedMessage->_created_at;
    const std::vector<std::shared_ptr<TextChatData>> forwardedMessages{forwardedMessage};
    if (dependencies.appendFriendMessages)
    {
        dependencies.appendFriendMessages(result.peerUid, forwardedMessages);
        result.userManagerUpdated = true;
    }

    result.cacheUpdated = upsertCacheIfReady(dependencies, request.context.selfUid, result.peerUid, forwardedMessages);

    const std::shared_ptr<FriendInfo> friendInfo = lookupFriend(dependencies, result.peerUid);
    const std::vector<std::shared_ptr<TextChatData>> friendMessages =
        friendInfo ? friendInfo->_chat_msgs : std::vector<std::shared_ptr<TextChatData>>{};
    result.previewUpdated = updatePreviewFromMessages(dependencies, result.peerUid, friendMessages, forwardedMessages);

    if (isViewingCurrentPrivate(request.context, result.peerUid) && dependencies.messageModel)
    {
        const int beforeCount = dependencies.messageModel->rowCount();
        dependencies.messageModel->upsertMessage(forwardedMessage, request.context.selfUid);
        result.modelUpserted = dependencies.messageModel->rowCount() != beforeCount ||
                               dependencies.messageModel->containsMessage(result.msgId);
    }

    result.success = true;
    result.statusText = QStringLiteral("消息已转发");
    result.isError = false;
    return result;
}
