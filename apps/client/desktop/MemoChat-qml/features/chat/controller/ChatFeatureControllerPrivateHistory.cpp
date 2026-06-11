#include "ChatFeatureController.h"

#include "PrivateChatEventService.h"
#include "PrivateChatHistoryRequestService.h"
#include "PrivateChatHistoryService.h"

#include "DialogListService.h"
#include "usermgr.h"

#include <utility>

namespace
{
constexpr int kPrivateReadAckRequestId = 1076;

void fillFeatureOwnedPrivateEventDependencies(ChatFeatureController& controller,
                                              PrivateChatEventDependencies& dependencies)
{
    if (!dependencies.chatListModel)
    {
        dependencies.chatListModel = &controller.chatListModel();
    }
    if (!dependencies.dialogListModel)
    {
        dependencies.dialogListModel = &controller.dialogListModel();
    }
    if (!dependencies.historyBeforeTs)
    {
        dependencies.historyBeforeTs = &controller.privateHistoryState().beforeTs;
    }
    if (!dependencies.historyBeforeMsgId)
    {
        dependencies.historyBeforeMsgId = &controller.privateHistoryState().beforeMsgId;
    }
    if (!dependencies.ensureFriend)
    {
        dependencies.ensureFriend = [&controller, &dependencies](int peerUid)
        {
            auto friendInfo = dependencies.friendById ? dependencies.friendById(peerUid) : nullptr;
            if (friendInfo || peerUid <= 0)
            {
                return friendInfo;
            }

            const int dialogIndex = controller.dialogListModel().indexOfUid(peerUid);
            const QVariantMap dialogItem =
                dialogIndex >= 0 ? controller.dialogListModel().get(dialogIndex) : QVariantMap();
            auto placeholder =
                DialogListService::buildPlaceholderAuthInfo(peerUid, dialogItem, QStringLiteral("qrc:/res/head_1.png"));
            if (dependencies.addFriend)
            {
                dependencies.addFriend(placeholder);
            }
            controller.chatListModel().upsertFriend(placeholder);
            if (dependencies.upsertContact)
            {
                dependencies.upsertContact(placeholder);
            }
            controller.dialogListModel().upsertFriend(placeholder);
            friendInfo = dependencies.friendById ? dependencies.friendById(peerUid) : nullptr;
            return friendInfo ? friendInfo : std::make_shared<FriendInfo>(placeholder);
        };
    }
}
} // namespace

void ChatFeatureController::clearPrivateHistoryState()
{
    PrivateChatHistoryRequestService::clearState(_privateHistoryState);
}

PrivateHistoryLoadCurrentResult
ChatFeatureController::loadCurrentPrivateHistory(const PrivateHistoryLoadCurrentRequest& request,
                                                 PrivateHistoryLoadCurrentDependencies dependencies)
{
    if (!dependencies.historyState)
    {
        dependencies.historyState = &_privateHistoryState;
    }
    return PrivateChatHistoryRequestService::loadCurrent(request, dependencies);
}

PrivateHistoryLoadCurrentDependencies
ChatFeatureController::privateHistoryLoadCurrentDependencies(std::shared_ptr<UserMgr> userMgr,
                                                             std::function<void(const QString&)> setCurrentPeerName,
                                                             std::function<void(const QString&)> setCurrentPeerIcon,
                                                             std::function<void(bool)> setLoading,
                                                             std::function<void(bool)> setCanLoadMore,
                                                             std::function<void(int, qint64)> requestReadAck)
{
    PrivateHistoryLoadCurrentDependencies dependencies;
    dependencies.messageModel = &_messageModel;
    dependencies.historyState = &_privateHistoryState;
    dependencies.peerProfile = [userMgr](int peerUid)
    {
        PrivateHistoryPeerProfile profile;
        auto friendInfo = userMgr ? userMgr->GetFriendById(peerUid) : nullptr;
        if (!friendInfo)
        {
            return profile;
        }
        profile.available = true;
        profile.displayName = DialogListService::privateDisplayName(friendInfo);
        profile.icon = friendInfo->_icon;
        return profile;
    };
    dependencies.friendMessages = [userMgr](int peerUid)
    {
        auto friendInfo = userMgr ? userMgr->GetFriendById(peerUid) : nullptr;
        return friendInfo ? friendInfo->_chat_msgs : std::vector<std::shared_ptr<TextChatData>>{};
    };
    dependencies.loadRecentMessages = [this](int selfUid, int peerUid, int limit)
    {
        return _privateCacheStore.loadRecentMessages(selfUid, peerUid, limit);
    };
    dependencies.appendFriendMessages =
        [userMgr](int peerUid, const std::vector<std::shared_ptr<TextChatData>>& messages)
    {
        if (userMgr)
        {
            userMgr->AppendFriendChatMsg(peerUid, messages);
        }
    };
    dependencies.setCurrentPeerName = std::move(setCurrentPeerName);
    dependencies.setCurrentPeerIcon = std::move(setCurrentPeerIcon);
    dependencies.setLoading = std::move(setLoading);
    dependencies.setCanLoadMore = std::move(setCanLoadMore);
    dependencies.requestReadAck = std::move(requestReadAck);
    return dependencies;
}

PrivateHistoryRequestBuildResult
ChatFeatureController::buildPrivateHistoryRequest(const PrivateHistoryRequestBuildRequest& request,
                                                  PrivateHistoryRequestBuildDependencies dependencies)
{
    if (!dependencies.historyState)
    {
        dependencies.historyState = &_privateHistoryState;
    }
    return PrivateChatHistoryRequestService::buildRequest(request, dependencies);
}

PrivateHistoryRequestBuildResult ChatFeatureController::buildPrivateHistoryBootstrapRequest(
    const PrivateHistoryBootstrapRequest& request,
    const PrivateHistoryBootstrapDependencies& dependencies) const
{
    return PrivateChatHistoryRequestService::buildBootstrapRequest(request, dependencies);
}

PrivateHistoryRequestBuildResult
ChatFeatureController::buildPrivateHistoryLoadMoreRequest(const PrivateHistoryLoadMoreRequest& request,
                                                          PrivateHistoryLoadMoreDependencies dependencies)
{
    if (!dependencies.historyState)
    {
        dependencies.historyState = &_privateHistoryState;
    }
    return PrivateChatHistoryRequestService::buildLoadMoreRequest(request, dependencies);
}

ChatHistoryLoadMoreResult ChatFeatureController::loadMoreCurrentHistory()
{
    const ChatHistoryLoadMoreRequest request =
        _currentHistoryPort.snapshot ? _currentHistoryPort.snapshot() : ChatHistoryLoadMoreRequest{};
    PrivateHistoryLoadMoreDependencies privateDependencies = _currentHistoryPort.privateDependencies
                                                                 ? _currentHistoryPort.privateDependencies()
                                                                 : PrivateHistoryLoadMoreDependencies{};
    GroupConversationDependencies groupDependencies = _currentHistoryPort.groupDependencies
                                                          ? _currentHistoryPort.groupDependencies()
                                                          : GroupConversationDependencies{};
    return loadMoreCurrentHistory(request,
                                  std::move(privateDependencies),
                                  std::move(groupDependencies),
                                  _currentHistoryPort.groupRequestPort);
}

ChatHistoryLoadMoreResult
ChatFeatureController::loadMoreCurrentHistory(const ChatHistoryLoadMoreRequest& request,
                                              PrivateHistoryLoadMoreDependencies privateDependencies,
                                              GroupConversationDependencies groupDependencies,
                                              const GroupHistoryRequestPort& groupPort)
{
    ChatHistoryLoadMoreResult result;
    result.peerUid = request.currentPrivatePeerUid;
    result.groupId = request.currentGroupId;

    if (request.privateHistoryLoading)
    {
        result.skipped = true;
        result.errorText = QStringLiteral("current history is already loading");
        return result;
    }
    if (!request.canLoadMorePrivateHistory)
    {
        result.skipped = true;
        result.errorText = QStringLiteral("current history has no more messages");
        return result;
    }

    if (request.currentGroupId > 0)
    {
        result.groupPath = true;
        GroupHistoryRequestCommand groupRequest;
        groupRequest.selfUid = request.selfUid;
        groupRequest.currentGroupId = request.currentGroupId;
        groupRequest.targetGroupId = request.currentGroupId;
        groupRequest.currentGroupLoading = request.groupHistoryLoading;
        groupRequest.limit = request.groupLimit;
        const GroupHistoryRequestResult groupResult =
            requestGroupHistory(groupRequest, std::move(groupDependencies), groupPort);
        result.success = groupResult.success;
        result.skipped = groupResult.skipped;
        result.dispatched = groupResult.dispatched;
        result.groupLoadingProjected = groupResult.loadingProjected;
        result.groupId = groupResult.groupId;
        if (result.skipped && groupResult.skippedByLoading)
        {
            result.errorText = QStringLiteral("group history is already loading");
        }
        else if (result.skipped && groupResult.skippedByNoMore)
        {
            result.errorText = QStringLiteral("group history has no more messages");
        }
        return result;
    }

    if (request.currentPrivatePeerUid <= 0)
    {
        result.skipped = true;
        result.errorText = QStringLiteral("no current conversation");
        return result;
    }

    result.privatePath = true;
    PrivateHistoryLoadMoreRequest privateRequest;
    privateRequest.currentPeerUid = request.currentPrivatePeerUid;
    privateRequest.selfUid = request.selfUid;
    privateRequest.privateHistoryLoading = request.privateHistoryLoading;
    privateRequest.canLoadMorePrivateHistory = request.canLoadMorePrivateHistory;
    const PrivateHistoryRequestBuildResult privateResult =
        buildPrivateHistoryLoadMoreRequest(privateRequest, privateDependencies);
    result.success = privateResult.success;
    result.skipped = !privateResult.success;
    result.dispatched = privateResult.dispatched;
    result.loadingSet = privateResult.loadingSet;
    result.peerUid = privateResult.peerUid;
    result.beforeTs = privateResult.beforeTs;
    result.beforeMsgId = privateResult.beforeMsgId;
    result.errorText = privateResult.errorText;
    return result;
}

PrivateHistoryResponseResult
ChatFeatureController::handlePrivateHistoryResponse(const PrivateHistoryResponseRequest& request,
                                                    PrivateHistoryResponseDependencies dependencies)
{
    if (!dependencies.historyState)
    {
        dependencies.historyState = &_privateHistoryState;
    }
    if (!dependencies.chatListModel)
    {
        dependencies.chatListModel = &_chatListModel;
    }
    if (!dependencies.dialogListModel)
    {
        dependencies.dialogListModel = &_dialogListModel;
    }
    return PrivateChatHistoryService::handleResponse(request, dependencies);
}

PrivateHistoryResponseDependencies
ChatFeatureController::privateHistoryResponseDependencies(std::shared_ptr<UserMgr> userMgr,
                                                          std::function<void(bool)> setLoading,
                                                          std::function<void(bool)> setCanLoadMore,
                                                          std::function<void(int, qint64)> requestReadAck)
{
    PrivateHistoryResponseDependencies dependencies;
    dependencies.cacheStore = &_privateCacheStore;
    dependencies.messageModel = &_messageModel;
    dependencies.historyState = &_privateHistoryState;
    dependencies.chatListModel = &_chatListModel;
    dependencies.dialogListModel = &_dialogListModel;
    dependencies.setLoading = std::move(setLoading);
    dependencies.setCanLoadMore = std::move(setCanLoadMore);
    dependencies.appendFriendMessages =
        [userMgr](int peerUid, const std::vector<std::shared_ptr<TextChatData>>& messages)
    {
        if (userMgr)
        {
            userMgr->AppendFriendChatMsg(peerUid, messages);
        }
    };
    dependencies.friendMessages = [userMgr](int peerUid)
    {
        auto friendInfo = userMgr ? userMgr->GetFriendById(peerUid) : nullptr;
        return friendInfo ? friendInfo->_chat_msgs : std::vector<std::shared_ptr<TextChatData>>{};
    };
    dependencies.requestReadAck = std::move(requestReadAck);
    return dependencies;
}

PrivateChatEventDependencies
ChatFeatureController::privateEventDependencies(std::shared_ptr<UserMgr> userMgr,
                                                std::function<void(std::shared_ptr<AuthInfo>)> upsertContact,
                                                std::function<void(int, qint64)> requestReadAck)
{
    PrivateChatEventDependencies dependencies;
    dependencies.cacheStore = &_privateCacheStore;
    dependencies.messageModel = &_messageModel;
    dependencies.chatListModel = &_chatListModel;
    dependencies.dialogListModel = &_dialogListModel;
    dependencies.historyBeforeTs = &_privateHistoryState.beforeTs;
    dependencies.historyBeforeMsgId = &_privateHistoryState.beforeMsgId;
    dependencies.friendById = [userMgr](int peerUid)
    {
        return userMgr ? userMgr->GetFriendById(peerUid) : nullptr;
    };
    dependencies.addFriend = [userMgr](std::shared_ptr<AuthInfo> authInfo)
    {
        if (userMgr)
        {
            userMgr->AddFriend(std::move(authInfo));
        }
    };
    dependencies.upsertContact = std::move(upsertContact);
    dependencies.appendFriendMessages =
        [userMgr](int peerUid, const std::vector<std::shared_ptr<TextChatData>>& messages)
    {
        if (userMgr)
        {
            userMgr->AppendFriendChatMsg(peerUid, messages);
        }
    };
    dependencies.updatePrivateMessageContent = [userMgr](int peerUid,
                                                         const QString& msgId,
                                                         const QString& content,
                                                         const QString& state,
                                                         qint64 editedAtMs,
                                                         qint64 deletedAtMs)
    {
        return userMgr ? userMgr->UpdatePrivateChatMsgContent(peerUid, msgId, content, state, editedAtMs, deletedAtMs)
                       : false;
    };
    dependencies.markOutgoingReadUntil = [userMgr](int peerUid, int selfUid, qint64 readTs)
    {
        return userMgr ? userMgr->MarkPrivateOutgoingReadUntil(peerUid, selfUid, readTs) : 0;
    };
    dependencies.updatePrivateMessageState = [userMgr](int peerUid, const QString& clientMsgId, const QString& state)
    {
        return userMgr ? userMgr->UpdatePrivateChatMsgState(peerUid, clientMsgId, state) : false;
    };
    dependencies.requestReadAck = std::move(requestReadAck);
    fillFeatureOwnedPrivateEventDependencies(*this, dependencies);
    return dependencies;
}

PrivateIncomingMessageResult
ChatFeatureController::handlePrivateIncomingMessage(const PrivateIncomingMessageRequest& request,
                                                    PrivateChatEventDependencies dependencies)
{
    fillFeatureOwnedPrivateEventDependencies(*this, dependencies);
    return PrivateChatEventService::handleIncomingMessage(request, dependencies);
}

PrivateMessageChangedResult
ChatFeatureController::handlePrivateMessageChanged(const PrivateMessageChangedRequest& request,
                                                   PrivateChatEventDependencies dependencies)
{
    fillFeatureOwnedPrivateEventDependencies(*this, dependencies);
    return PrivateChatEventService::handleMessageChanged(request, dependencies);
}

PrivateMessageChangedResult
ChatFeatureController::handlePrivateMessageMutationResponse(const PrivateMessageMutationResponseRequest& request,
                                                            PrivateChatEventDependencies dependencies)
{
    fillFeatureOwnedPrivateEventDependencies(*this, dependencies);
    return PrivateChatEventService::handleMessageMutationResponse(request, dependencies);
}

PrivateForwardResponseResult
ChatFeatureController::handlePrivateForwardResponse(const PrivateForwardResponseRequest& request,
                                                    PrivateChatEventDependencies dependencies)
{
    fillFeatureOwnedPrivateEventDependencies(*this, dependencies);
    return PrivateChatEventService::handleForwardResponse(request, dependencies);
}

PrivateReadAckResult ChatFeatureController::handlePrivateReadAck(const PrivateReadAckRequest& request,
                                                                 PrivateChatEventDependencies dependencies)
{
    fillFeatureOwnedPrivateEventDependencies(*this, dependencies);
    return PrivateChatEventService::handleReadAck(request, dependencies);
}

ChatReadAckCommandResult ChatFeatureController::sendPrivateReadAck(const ChatReadAckCommand& request,
                                                                   const ChatReadAckDispatchPort& port) const
{
    ChatReadAckCommandResult result;
    result.privatePath = true;
    result.selfUid = request.selfUid;
    result.peerUid = request.peerUid;
    result.readTs = request.readTs;
    result.requestId = kPrivateReadAckRequestId;

    result.compactPayload =
        PrivateChatEventService::buildReadAckPayload(request.selfUid, request.peerUid, request.readTs);
    if (result.compactPayload.isEmpty())
    {
        result.skipped = true;
        result.errorText = QStringLiteral("invalid private read ack command");
        return result;
    }

    result.success = true;
    if (port.dispatchPayload)
    {
        port.dispatchPayload(result.requestId, result.compactPayload);
        result.dispatched = true;
    }
    return result;
}

ChatReadAckCommandResult ChatFeatureController::sendPrivateReadAckForPeer(int peerUid, qint64 readTs) const
{
    ChatReadAckCommand request;
    request.selfUid = _readAckPort.selfUid ? _readAckPort.selfUid() : 0;
    request.peerUid = peerUid;
    request.readTs = readTs;
    return sendPrivateReadAck(request, _readAckPort.dispatch);
}

PrivateMessageStatusResult ChatFeatureController::handlePrivateMessageStatus(const PrivateMessageStatusRequest& request,
                                                                             PrivateChatEventDependencies dependencies)
{
    fillFeatureOwnedPrivateEventDependencies(*this, dependencies);
    return PrivateChatEventService::handleMessageStatus(request, dependencies);
}
