#include "PrivateChatEventServiceInternal.h"

using namespace memochat::chat::private_event;

PrivateIncomingMessageResult
PrivateChatEventService::handleIncomingMessage(const PrivateIncomingMessageRequest& request,
                                               const PrivateChatEventDependencies& dependencies)
{
    PrivateIncomingMessageResult result;
    if (!request.message || request.message->_chat_msgs.empty() || request.context.selfUid <= 0)
    {
        return result;
    }

    result.messageCount = request.message->_chat_msgs.size();
    result.fromSelf = request.message->_from_uid == request.context.selfUid;
    result.peerUid = result.fromSelf ? request.message->_to_uid : request.message->_from_uid;
    if (result.peerUid <= 0)
    {
        return result;
    }

    bool ensureCalled = false;
    ensureFriend(dependencies, result.peerUid, &ensureCalled);
    result.ensuredFriend = ensureCalled;

    normalizeIncomingMessages(request.message->_chat_msgs);
    const qint64 latestPeerTs = latestPeerMessageTs(request.message->_chat_msgs, result.peerUid);

    if (dependencies.appendFriendMessages)
    {
        dependencies.appendFriendMessages(result.peerUid, request.message->_chat_msgs);
        result.friendMessagesAppended = true;
    }

    result.cacheUpdated =
        upsertCacheIfReady(dependencies, request.context.selfUid, result.peerUid, request.message->_chat_msgs);

    const std::shared_ptr<FriendInfo> friendInfo = lookupFriend(dependencies, result.peerUid);
    const std::vector<std::shared_ptr<TextChatData>> friendMessages =
        friendInfo ? friendInfo->_chat_msgs : std::vector<std::shared_ptr<TextChatData>>{};
    result.previewUpdated =
        updatePreviewFromMessages(dependencies, result.peerUid, friendMessages, request.message->_chat_msgs);

    result.currentDialog = isViewingCurrentPrivate(request.context, result.peerUid);
    if (result.currentDialog)
    {
        if (dependencies.chatListModel && dependencies.dialogListModel)
        {
            ConversationSyncService::clearPrivateUnread(*dependencies.chatListModel,
                                                        *dependencies.dialogListModel,
                                                        result.peerUid);
            result.unreadCleared = true;
        }
        if (latestPeerTs > 0 && dependencies.requestReadAck)
        {
            dependencies.requestReadAck(result.peerUid, latestPeerTs);
            result.requestedReadAckTs = latestPeerTs;
        }
        result.modelAppendCount =
            appendToCurrentModel(dependencies, request.message->_chat_msgs, request.context.selfUid);
        syncHistoryCursorIfPossible(dependencies);
    }
    else if (!result.fromSelf && dependencies.dialogListModel)
    {
        ConversationSyncService::incrementPrivateUnread(*dependencies.dialogListModel, result.peerUid);
        result.unreadIncremented = true;
    }

    result.success = true;
    return result;
}

PrivateMessageChangedResult
PrivateChatEventService::handleMessageChanged(const PrivateMessageChangedRequest& request,
                                              const PrivateChatEventDependencies& dependencies)
{
    PrivateMessageChangedResult result;
    result.event = request.payload.value(QStringLiteral("event")).toString();
    if (request.context.selfUid <= 0 || !protocolSucceeded(request.payload))
    {
        return result;
    }

    const int fromUid = request.payload.value(QStringLiteral("fromuid")).toInt();
    const int peerUidField = request.payload.value(QStringLiteral("peer_uid")).toInt();
    result.peerUid = resolvePrivatePeerUid(request.context.selfUid, fromUid, peerUidField);
    if (result.peerUid <= 0)
    {
        return result;
    }

    const MessageUpdateFields updateFields =
        MessagePayloadService::parseMessageUpdateFields(request.payload, result.event);
    result.msgId = updateFields.msgId;
    result.content = updateFields.content;
    result.state = updateFields.state;
    result.editedAtMs = updateFields.editedAtMs;
    result.deletedAtMs = updateFields.deletedAtMs;
    if (result.msgId.isEmpty() || result.content.isEmpty() || !dependencies.updatePrivateMessageContent)
    {
        return result;
    }

    result.userManagerUpdated = dependencies.updatePrivateMessageContent(result.peerUid,
                                                                         result.msgId,
                                                                         result.content,
                                                                         result.state,
                                                                         result.editedAtMs,
                                                                         result.deletedAtMs);
    if (!result.userManagerUpdated)
    {
        return result;
    }

    const std::shared_ptr<FriendInfo> friendInfo = lookupFriend(dependencies, result.peerUid);
    if (friendInfo)
    {
        result.cacheUpdated =
            upsertCacheIfReady(dependencies, request.context.selfUid, result.peerUid, friendInfo->_chat_msgs);
        result.previewUpdated = updatePreviewFromMessages(dependencies,
                                                          result.peerUid,
                                                          friendInfo->_chat_msgs,
                                                          std::vector<std::shared_ptr<TextChatData>>{});
    }

    if (isViewingCurrentPrivate(request.context, result.peerUid) && dependencies.messageModel)
    {
        result.modelPatched = dependencies.messageModel->patchMessageContent(result.msgId,
                                                                             result.content,
                                                                             result.state,
                                                                             result.editedAtMs,
                                                                             result.deletedAtMs);
    }

    result.success = true;
    return result;
}
