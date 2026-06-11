#include "ChatFeatureController.h"

#include "usermgr.h"

#include <QDebug>

namespace
{
constexpr int kGroupReadAckRequestId = 1071;

template <typename Handler, typename... Args> void callIfPresent(const Handler& handler, Args&&... args)
{
    if (handler)
    {
        handler(std::forward<Args>(args)...);
    }
}

void fillFeatureOwnedGroupConversationDependencies(ChatFeatureController& controller,
                                                   GroupConversationDependencies& dependencies)
{
    dependencies.state = &controller.groupConversationState();
    if (!dependencies.dialogListModel)
    {
        dependencies.dialogListModel = &controller.dialogListModel();
    }
    if (!dependencies.clearGroupUnreadAndMention)
    {
        dependencies.clearGroupUnreadAndMention = [&controller](FriendListModel& dialogListModel, int dialogUid)
        {
            controller.clearGroupUnreadAndMention(dialogListModel, dialogUid);
        };
    }
    if (!dependencies.incrementGroupUnreadAndMention)
    {
        dependencies.incrementGroupUnreadAndMention =
            [&controller](FriendListModel& dialogListModel, int dialogUid, bool mentioned)
        {
            controller.incrementGroupUnreadAndMention(dialogListModel, dialogUid, mentioned);
        };
    }
    if (!dependencies.dialogDecorationState)
    {
        dependencies.dialogDecorationState = [&controller]()
        {
            return controller.dialogDecorationState();
        };
    }
}
} // namespace

GroupConversationState& ChatFeatureController::groupConversationState()
{
    return _groupConversationState;
}

const GroupConversationState& ChatFeatureController::groupConversationState() const
{
    return _groupConversationState;
}

void ChatFeatureController::clearGroupConversationState()
{
    _groupConversationState.reset();
}

ChatGroupConversationClearResult
ChatFeatureController::clearGroupConversation(const ChatGroupConversationClearRequest& request,
                                              const ChatGroupConversationClearPort& port)
{
    ChatGroupConversationClearResult result;
    const qint64 targetGroupId = request.groupId > 0 ? request.groupId : request.currentGroupId;
    result.targetGroupId = targetGroupId;

    if (targetGroupId > 0)
    {
        const int dialogUid = port.dialogUidForGroup ? port.dialogUidForGroup(targetGroupId) : 0;
        result.dialogUid = dialogUid;
        if (dialogUid != 0)
        {
            removeMentionForDialog(dialogUid);
            _dialogListModel.clearMention(dialogUid);
            _dialogListModel.removeByUid(dialogUid);
            result.dialogRemoved = true;
            callIfPresent(port.removeGroupByDialogUid, dialogUid);
            result.groupRemoved = true;
            callIfPresent(port.removeGroupDialogMapping, dialogUid);
            result.mappingRemoved = true;
        }
    }

    if (targetGroupId <= 0 || request.currentGroupId == targetGroupId)
    {
        callIfPresent(port.clearCurrentGroup);
        resetGroupHistoryState();
        callIfPresent(port.setGroupHistoryLoading, false);
        callIfPresent(port.setPendingReplyContext, QString(), QString(), QString());
        callIfPresent(port.clearMessageModel);
        callIfPresent(port.resetCurrentPeer);
        callIfPresent(port.setCurrentDraftText, QString());
        callIfPresent(port.setCurrentDialogPinned, false);
        callIfPresent(port.setCurrentDialogMuted, false);
        callIfPresent(port.setCanLoadMorePrivateHistory, false);
        callIfPresent(port.emitCurrentDialogUidChangedIfNeeded);
        result.currentConversationCleared = true;
        result.currentRuntimeReset = true;
    }

    result.success = targetGroupId > 0 || result.currentConversationCleared;
    return result;
}

void ChatFeatureController::resetGroupHistoryState()
{
    _groupConversationState.historyBeforeSeq = 0;
    _groupConversationState.historyHasMore = true;
}

GroupConversationDependencies
ChatFeatureController::groupConversationDependencies(std::shared_ptr<UserMgr> userMgr,
                                                     FriendListModel* groupListModel,
                                                     std::function<void(int, const QByteArray&)> dispatchPayload)
{
    GroupConversationDependencies dependencies;
    dependencies.state = &_groupConversationState;
    dependencies.dialogUidMap = &_groupDialogUidMap;
    dependencies.cacheStore = &_groupCacheStore;
    dependencies.messageModel = &_messageModel;
    dependencies.groupListModel = groupListModel;
    dependencies.groupById = [userMgr](qint64 groupId)
    {
        return userMgr ? userMgr->GetGroupById(groupId) : nullptr;
    };
    dependencies.upsertGroup = [userMgr](std::shared_ptr<GroupInfoData> groupInfo)
    {
        if (userMgr)
        {
            userMgr->UpsertGroup(std::move(groupInfo));
        }
    };
    dependencies.upsertGroupMessage = [userMgr](qint64 groupId, const std::shared_ptr<TextChatData>& message)
    {
        if (userMgr)
        {
            userMgr->UpsertGroupChatMsg(groupId, message);
        }
    };
    dependencies.updateGroupMessageState = [userMgr](qint64 groupId, const QString& msgId, const QString& state)
    {
        return userMgr ? userMgr->UpdateGroupChatMsgState(groupId, msgId, state) : false;
    };
    dependencies.updateGroupMessageContent = [userMgr](qint64 groupId,
                                                       const QString& msgId,
                                                       const QString& content,
                                                       const QString& state,
                                                       qint64 editedAtMs,
                                                       qint64 deletedAtMs)
    {
        return userMgr ? userMgr->UpdateGroupChatMsgContent(groupId, msgId, content, state, editedAtMs, deletedAtMs)
                       : false;
    };
    dependencies.markGroupOutgoingReadUntil = [userMgr](qint64 groupId, int selfUid, qint64 readTs)
    {
        return userMgr ? userMgr->MarkGroupOutgoingReadUntil(groupId, selfUid, readTs) : 0;
    };
    dependencies.dispatchPayload = std::move(dispatchPayload);
    fillFeatureOwnedGroupConversationDependencies(*this, dependencies);
    return dependencies;
}

GroupSendResult ChatFeatureController::sendGroupText(const GroupSendRequest& request,
                                                     GroupConversationDependencies dependencies)
{
    fillFeatureOwnedGroupConversationDependencies(*this, dependencies);
    return GroupConversationService::sendText(request, dependencies);
}

GroupSendResult ChatFeatureController::sendCurrentGroupText(const QString& content, const QString& previewText)
{
    const ChatCurrentSendSnapshot snapshot =
        _currentSendPort.snapshot ? _currentSendPort.snapshot() : ChatCurrentSendSnapshot{};

    GroupSendRequest request;
    request.context = snapshot.context;
    request.groupId = snapshot.currentGroupId;
    request.content = content;
    request.previewText = previewText;
    request.replyToMsgId = replyToMsgId();

    GroupConversationDependencies dependencies =
        _currentSendPort.groupDependencies ? _currentSendPort.groupDependencies() : GroupConversationDependencies{};
    return sendGroupText(request, dependencies);
}

bool ChatFeatureController::dispatchCurrentGroupContent(const QString& content, const QString& previewText)
{
    const qint64 groupId = _contentDispatchPort.currentGroupId ? _contentDispatchPort.currentGroupId() : 0;
    if (groupId <= 0)
    {
        return false;
    }
    if (_contentDispatchPort.isTransportReady && !_contentDispatchPort.isTransportReady())
    {
        if (_contentDispatchPort.setTip)
        {
            _contentDispatchPort.setTip(QStringLiteral("聊天连接未就绪，请重新登录"), true);
        }
        return false;
    }

    const GroupSendResult result = sendCurrentGroupText(content, previewText);
    if (!result.success)
    {
        if (_contentDispatchPort.setTip && !result.errorText.isEmpty())
        {
            _contentDispatchPort.setTip(result.errorText, true);
        }
        return false;
    }

    const auto groupInfo = _contentDispatchPort.groupById ? _contentDispatchPort.groupById(groupId) : nullptr;
    qInfo() << "Group chat message dispatched, group id:" << groupId << "msg id:" << result.msgId
            << "group msg count:" << (groupInfo ? static_cast<qlonglong>(groupInfo->_chat_msgs.size()) : 0LL)
            << "message model count:" << messageCount();
    return result.dispatched;
}

GroupHistoryBuildResult ChatFeatureController::buildGroupHistoryRequest(const GroupHistoryBuildRequest& request,
                                                                        GroupConversationDependencies dependencies)
{
    fillFeatureOwnedGroupConversationDependencies(*this, dependencies);
    return GroupConversationService::buildHistoryRequest(request, dependencies);
}

GroupHistoryRequestResult ChatFeatureController::requestGroupHistory(const GroupHistoryRequestCommand& request,
                                                                     GroupConversationDependencies dependencies,
                                                                     const GroupHistoryRequestPort& port)
{
    GroupHistoryRequestCommand command = request;
    command.bootstrap = false;
    if (command.targetGroupId <= 0)
    {
        command.targetGroupId = command.currentGroupId;
    }
    fillFeatureOwnedGroupConversationDependencies(*this, dependencies);
    return GroupConversationService::requestHistory(command, dependencies, port);
}

GroupHistoryRequestResult ChatFeatureController::requestCurrentGroupHistory()
{
    const ChatCurrentGroupHistorySnapshot snapshot =
        _currentGroupHistoryPort.snapshot ? _currentGroupHistoryPort.snapshot() : ChatCurrentGroupHistorySnapshot{};

    GroupHistoryRequestCommand request;
    request.selfUid = snapshot.selfUid;
    request.currentGroupId = snapshot.currentGroupId;
    request.targetGroupId = snapshot.currentGroupId;
    request.currentGroupLoading = snapshot.groupHistoryLoading;
    request.limit = 50;

    GroupConversationDependencies dependencies = _currentGroupHistoryPort.dependencies
                                                     ? _currentGroupHistoryPort.dependencies()
                                                     : GroupConversationDependencies{};
    return requestGroupHistory(request, std::move(dependencies), _currentGroupHistoryPort.requestPort);
}

GroupHistoryRequestResult
ChatFeatureController::requestGroupHistoryForBootstrap(const GroupHistoryRequestCommand& request,
                                                       GroupConversationDependencies dependencies,
                                                       const GroupHistoryRequestPort& port)
{
    GroupHistoryRequestCommand command = request;
    command.bootstrap = true;
    fillFeatureOwnedGroupConversationDependencies(*this, dependencies);
    return GroupConversationService::requestHistory(command, dependencies, port);
}

GroupOpenResult ChatFeatureController::openGroupConversation(const GroupOpenRequest& request,
                                                             GroupConversationDependencies dependencies)
{
    fillFeatureOwnedGroupConversationDependencies(*this, dependencies);
    return GroupConversationService::openGroupConversation(request, dependencies);
}

GroupHistoryResponseResult ChatFeatureController::handleGroupHistoryResponse(const GroupHistoryResponseRequest& request,
                                                                             GroupConversationDependencies dependencies,
                                                                             const GroupHistoryResponsePort& port)
{
    fillFeatureOwnedGroupConversationDependencies(*this, dependencies);
    return GroupConversationService::handleHistoryResponse(request, dependencies, port);
}

GroupIncomingResult ChatFeatureController::handleGroupIncomingMessage(const GroupIncomingRequest& request,
                                                                      GroupConversationDependencies dependencies)
{
    fillFeatureOwnedGroupConversationDependencies(*this, dependencies);
    return GroupConversationService::handleIncomingMessage(request, dependencies);
}

GroupAckResult ChatFeatureController::handleGroupMessageAck(const GroupAckRequest& request,
                                                            GroupConversationDependencies dependencies)
{
    fillFeatureOwnedGroupConversationDependencies(*this, dependencies);
    return GroupConversationService::handleMessageAck(request, dependencies);
}

GroupAckResult ChatFeatureController::handleGroupMessageStatus(const GroupAckRequest& request,
                                                               GroupConversationDependencies dependencies)
{
    fillFeatureOwnedGroupConversationDependencies(*this, dependencies);
    return GroupConversationService::handleMessageStatus(request, dependencies);
}

GroupAckResult ChatFeatureController::handleGroupMessageError(const GroupAckRequest& request,
                                                              GroupConversationDependencies dependencies)
{
    fillFeatureOwnedGroupConversationDependencies(*this, dependencies);
    return GroupConversationService::handleMessageError(request, dependencies);
}

GroupMutationResult ChatFeatureController::handleGroupMessageMutation(const GroupMutationRequest& request,
                                                                      GroupConversationDependencies dependencies)
{
    fillFeatureOwnedGroupConversationDependencies(*this, dependencies);
    return GroupConversationService::handleMessageMutation(request, dependencies);
}

GroupReadAckResult ChatFeatureController::handleGroupReadAckEvent(const GroupReadAckRequest& request,
                                                                  GroupConversationDependencies dependencies)
{
    fillFeatureOwnedGroupConversationDependencies(*this, dependencies);
    return GroupConversationService::handleReadAckEvent(request, dependencies);
}

ChatReadAckCommandResult ChatFeatureController::sendGroupReadAck(const ChatReadAckCommand& request,
                                                                 const ChatReadAckDispatchPort& port) const
{
    ChatReadAckCommandResult result;
    result.groupPath = true;
    result.selfUid = request.selfUid;
    result.groupId = request.groupId;
    result.readTs = request.readTs;
    result.requestId = kGroupReadAckRequestId;

    result.compactPayload =
        GroupConversationService::buildReadAckPayload(request.selfUid, request.groupId, request.readTs);
    if (result.compactPayload.isEmpty())
    {
        result.skipped = true;
        result.errorText = QStringLiteral("invalid group read ack command");
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

ChatReadAckCommandResult ChatFeatureController::sendGroupReadAckForGroup(qint64 groupId, qint64 readTs) const
{
    ChatReadAckCommand request;
    request.selfUid = _readAckPort.selfUid ? _readAckPort.selfUid() : 0;
    request.groupId = groupId;
    request.readTs = readTs;
    return sendGroupReadAck(request, _readAckPort.dispatch);
}
