#include "ChatFeatureController.h"

#include "FriendListModel.h"

#include <QDateTime>
#include <utility>

namespace
{
constexpr qint64 kMessageRevokeWindowMs = 5 * 60 * 1000;
}

namespace
{
template <typename Handler, typename... Args> void invokeIfSet(const Handler& handler, Args&&... args)
{
    if (handler)
    {
        handler(std::forward<Args>(args)...);
    }
}
} // namespace

ChatFeatureController::ChatFeatureController(ChatViewModel& viewModel, QObject* parent)
    : QObject(parent)
    , _viewModel(viewModel)
    , _chatListModel(this)
    , _dialogListModel(this)
    , _messageModel(this)
{
    _viewModel.syncDialogListModel(&_dialogListModel);
    _viewModel.syncMessageModel(&_messageModel);
}

FriendListModel& ChatFeatureController::chatListModel()
{
    return _chatListModel;
}

const FriendListModel& ChatFeatureController::chatListModel() const
{
    return _chatListModel;
}

FriendListModel& ChatFeatureController::dialogListModel()
{
    return _dialogListModel;
}

const FriendListModel& ChatFeatureController::dialogListModel() const
{
    return _dialogListModel;
}

ChatMessageModel& ChatFeatureController::messageModel()
{
    return _messageModel;
}

const ChatMessageModel& ChatFeatureController::messageModel() const
{
    return _messageModel;
}

void ChatFeatureController::setViewProjectionPort(ChatViewProjectionPort port)
{
    _viewProjectionPort = std::move(port);
}

void ChatFeatureController::syncViewModelState()
{
    const ChatViewProjectionState state =
        _viewProjectionPort.snapshot ? _viewProjectionPort.snapshot() : ChatViewProjectionState{};
    syncViewModelState(state);
}

void ChatFeatureController::syncViewModelState(const ChatViewProjectionState& state)
{
    _viewModel.syncChatTab(state.chatTab);
    _viewModel.syncCurrentDialogUid(state.currentDialogUid);
    _viewModel.syncCurrentPeer(state.currentChatPeerName, state.currentChatPeerIcon, state.hasCurrentChat);
    _viewModel.syncCurrentGroup(state.hasCurrentGroup, state.currentGroupRole);
    _viewModel.syncCurrentDraftText(state.currentDraftText);
    _viewModel.syncCurrentPendingAttachments(state.currentPendingAttachments);
    _viewModel.syncCurrentDialogPinned(state.currentDialogPinned);
    _viewModel.syncCurrentDialogMuted(state.currentDialogMuted);
    _viewModel.syncPendingReply(state.hasPendingReply, state.replyTargetName, state.replyPreviewText);
    _viewModel.syncDialogsReady(state.dialogsReady);
    _viewModel.syncChatLoadingMore(state.chatLoadingMore);
    _viewModel.syncCanLoadMoreChats(state.canLoadMoreChats);
    _viewModel.syncPrivateHistoryLoading(state.privateHistoryLoading);
    _viewModel.syncCanLoadMorePrivateHistory(state.canLoadMorePrivateHistory);
    _viewModel.syncMediaUpload(state.mediaUploadInProgress, state.mediaUploadProgressText);
}

PrivateChatCacheStore& ChatFeatureController::privateCacheStore()
{
    return _privateCacheStore;
}

GroupChatCacheStore& ChatFeatureController::groupCacheStore()
{
    return _groupCacheStore;
}

void ChatFeatureController::clearMessageModel()
{
    _messageModel.clear();
}

int ChatFeatureController::messageCount() const
{
    return _messageModel.rowCount();
}

bool ChatFeatureController::containsMessage(const QString& msgId) const
{
    return _messageModel.containsMessage(msgId);
}

bool ChatFeatureController::canRevokeMessage(const QString& msgId) const
{
    return _messageModel.canRevokeMessage(msgId, QDateTime::currentMSecsSinceEpoch(), kMessageRevokeWindowMs);
}

void ChatFeatureController::setMessageDownloadAuthContext(int uid, const QString& token)
{
    _messageModel.setDownloadAuthContext(uid, token);
}

void ChatFeatureController::openCacheStoresForUser(int uid)
{
    _privateCacheStore.openForUser(uid);
    _groupCacheStore.openForUser(uid);
}

void ChatFeatureController::closeCacheStores()
{
    _privateCacheStore.close();
    _groupCacheStore.close();
}

void ChatFeatureController::clearConversationLists()
{
    _chatListModel.clear();
    _dialogListModel.clear();
}

void ChatFeatureController::resetForPostLoginEntry()
{
    clearPrivateHistoryState();
    resetGroupHistoryState();
    clearGroupConversationState();
    clearMentionState();
    clearPendingAttachmentStore();
    resetPendingAttachmentSendQueue();
    clearIncomingMessages();
}

void ChatFeatureController::resetModelsForLogout()
{
    clearConversationLists();
    clearMessageModel();
}

void ChatFeatureController::resetRuntimeForLogout()
{
    clearPrivateHistoryState();
    resetGroupHistoryState();
    resetGroupDialogIdentityMap();
    clearGroupConversationState();
    clearDialogDecorationStore();
    resetPendingAttachmentSendQueue();
    clearIncomingMessages();
}

void ChatFeatureController::setReadAckPort(ChatReadAckPort port)
{
    _readAckPort = std::move(port);
}

void ChatFeatureController::setChatListPagingPort(ChatListPagingPort port)
{
    _chatListPagingPort = std::move(port);
}

ChatListPagingResult ChatFeatureController::ensureChatListInitialized()
{
    ChatListPagingResult result;
    const ChatListPagingSnapshot snapshot =
        _chatListPagingPort.snapshot ? _chatListPagingPort.snapshot() : ChatListPagingSnapshot{};
    if (snapshot.initialized)
    {
        result.skipped = true;
        return result;
    }

    _chatListModel.clear();
    const std::vector<std::shared_ptr<FriendInfo>> page =
        _chatListPagingPort.nextPage ? _chatListPagingPort.nextPage() : std::vector<std::shared_ptr<FriendInfo>>{};
    _chatListModel.setFriends(page);
    result.itemCount = static_cast<int>(page.size());
    if (_chatListPagingPort.markPageLoaded)
    {
        _chatListPagingPort.markPageLoaded();
    }
    if (_chatListPagingPort.setInitialized)
    {
        _chatListPagingPort.setInitialized(true);
        result.initialized = true;
    }
    if (_chatListPagingPort.setCanLoadMore)
    {
        const bool canLoadMore = _chatListPagingPort.loadFinished ? !_chatListPagingPort.loadFinished() : false;
        _chatListPagingPort.setCanLoadMore(canLoadMore);
        result.canLoadMoreProjected = true;
    }
    if (_chatListPagingPort.flushIncomingMessages)
    {
        _chatListPagingPort.flushIncomingMessages();
        result.incomingFlushed = true;
    }
    result.success = true;
    return result;
}

ChatListPagingResult ChatFeatureController::loadMoreChats()
{
    ChatListPagingResult result = ensureChatListInitialized();
    result.skipped = false;
    const ChatListPagingSnapshot snapshot =
        _chatListPagingPort.snapshot ? _chatListPagingPort.snapshot() : ChatListPagingSnapshot{};
    if (snapshot.loading)
    {
        result.skipped = true;
        return result;
    }

    if (_chatListPagingPort.loadFinished && _chatListPagingPort.loadFinished())
    {
        if (_chatListPagingPort.setCanLoadMore)
        {
            _chatListPagingPort.setCanLoadMore(false);
            result.canLoadMoreProjected = true;
        }
        result.skipped = true;
        return result;
    }

    if (_chatListPagingPort.setLoading)
    {
        _chatListPagingPort.setLoading(true);
        result.loadingProjected = true;
    }
    const std::vector<std::shared_ptr<FriendInfo>> page =
        _chatListPagingPort.nextPage ? _chatListPagingPort.nextPage() : std::vector<std::shared_ptr<FriendInfo>>{};
    _chatListModel.appendFriends(page);
    result.itemCount = static_cast<int>(page.size());
    result.pageAppended = true;
    if (_chatListPagingPort.markPageLoaded)
    {
        _chatListPagingPort.markPageLoaded();
    }
    if (_chatListPagingPort.setLoading)
    {
        _chatListPagingPort.setLoading(false);
    }
    if (_chatListPagingPort.setCanLoadMore)
    {
        const bool canLoadMore = _chatListPagingPort.loadFinished ? !_chatListPagingPort.loadFinished() : false;
        _chatListPagingPort.setCanLoadMore(canLoadMore);
        result.canLoadMoreProjected = true;
    }
    result.success = true;
    return result;
}

PrivateHistoryRequestState& ChatFeatureController::privateHistoryState()
{
    return _privateHistoryState;
}

const PrivateHistoryRequestState& ChatFeatureController::privateHistoryState() const
{
    return _privateHistoryState;
}

void ChatFeatureController::clearDialogRuntimeState()
{
    _dialogRuntimeState.clear();
}

void ChatFeatureController::bindRequests()
{
    if (_requestsBound)
    {
        return;
    }
    _requestsBound = true;

    connect(&_viewModel,
            &ChatViewModel::ensureChatListInitializedRequested,
            this,
            [this]()
            {
                ensureChatListInitialized();
            });
    connect(&_viewModel,
            &ChatViewModel::ensureGroupsInitializedRequested,
            this,
            [this]()
            {
                invokeIfSet(_groupConversationPort.ensureGroupsInitialized);
            });
    connect(&_viewModel,
            &ChatViewModel::selectDialogByUidRequested,
            this,
            [this](int uid)
            {
                selectDialogByUid(uid);
            });
    connect(&_viewModel,
            &ChatViewModel::selectChatIndexRequested,
            this,
            [this](int index)
            {
                selectPrivateByIndex(index);
            });
    connect(&_viewModel,
            &ChatViewModel::selectGroupIndexRequested,
            this,
            [this](int index)
            {
                selectGroupByIndex(index);
            });
    connect(&_viewModel,
            &ChatViewModel::loadMoreChatsRequested,
            this,
            [this]()
            {
                loadMoreChats();
            });
    connect(&_viewModel,
            &ChatViewModel::loadMorePrivateHistoryRequested,
            this,
            [this]()
            {
                loadMoreCurrentHistory();
            });
    connect(&_viewModel,
            &ChatViewModel::sendCurrentComposerPayloadRequested,
            this,
            [this](const QString& text)
            {
                invokeIfSet(_privateCommandPort.sendCurrentComposerPayload, text);
            });
    connect(&_viewModel,
            &ChatViewModel::sendImageMessageRequested,
            this,
            [this]()
            {
                invokeIfSet(_privateCommandPort.sendImageMessage);
            });
    connect(&_viewModel,
            &ChatViewModel::sendFileMessageRequested,
            this,
            [this]()
            {
                invokeIfSet(_privateCommandPort.sendFileMessage);
            });
    connect(&_viewModel,
            &ChatViewModel::removePendingAttachmentRequested,
            this,
            [this](const QString& attachmentId)
            {
                invokeIfSet(_pendingAttachmentPort.removePendingAttachment, attachmentId);
            });
    connect(&_viewModel,
            &ChatViewModel::clearPendingAttachmentsRequested,
            this,
            [this]()
            {
                invokeIfSet(_pendingAttachmentPort.clearPendingAttachments);
            });
    connect(&_viewModel,
            &ChatViewModel::updateCurrentDraftRequested,
            this,
            [this](const QString& text)
            {
                updateCurrentDraft(text);
            });
    connect(&_viewModel,
            &ChatViewModel::toggleCurrentDialogPinnedRequested,
            this,
            [this]()
            {
                toggleCurrentDialogPinned();
            });
    connect(&_viewModel,
            &ChatViewModel::toggleCurrentDialogMutedRequested,
            this,
            [this]()
            {
                toggleCurrentDialogMuted();
            });
    connect(&_viewModel,
            &ChatViewModel::toggleDialogPinnedByUidRequested,
            this,
            [this](int dialogUid)
            {
                toggleDialogPinnedByUid(dialogUid);
            });
    connect(&_viewModel,
            &ChatViewModel::toggleDialogMutedByUidRequested,
            this,
            [this](int dialogUid)
            {
                toggleDialogMutedByUid(dialogUid);
            });
    connect(&_viewModel,
            &ChatViewModel::markDialogReadByUidRequested,
            this,
            [this](int dialogUid)
            {
                markDialogReadByUid(dialogUid);
            });
    connect(&_viewModel,
            &ChatViewModel::clearDialogDraftByUidRequested,
            this,
            [this](int dialogUid)
            {
                clearDialogDraftByUid(dialogUid);
            });
    connect(&_viewModel,
            &ChatViewModel::beginReplyMessageRequested,
            this,
            [this](const QString& msgId, const QString& senderName, const QString& previewText)
            {
                if (setPendingReplyContext(msgId, senderName, previewText))
                {
                    _viewModel.syncPendingReply(hasPendingReply(), replyTargetName(), replyPreviewText());
                }
            });
    connect(&_viewModel,
            &ChatViewModel::cancelReplyMessageRequested,
            this,
            [this]()
            {
                if (setPendingReplyContext(QString(), QString(), QString()))
                {
                    _viewModel.syncPendingReply(hasPendingReply(), replyTargetName(), replyPreviewText());
                }
            });
    connect(&_viewModel,
            &ChatViewModel::startVoiceChatRequested,
            this,
            [this]()
            {
                invokeIfSet(_shellIntentPort.startVoiceChat);
            });
    connect(&_viewModel,
            &ChatViewModel::startVideoChatRequested,
            this,
            [this]()
            {
                invokeIfSet(_shellIntentPort.startVideoChat);
            });
}

void ChatFeatureController::setDialogNavigationPort(ChatDialogNavigationPort port)
{
    _dialogNavigationPort = std::move(port);
}

void ChatFeatureController::setGroupConversationPort(ChatGroupConversationPort port)
{
    _groupConversationPort = std::move(port);
}

void ChatFeatureController::setDialogSelectionPort(ChatDialogSelectionPort port)
{
    _dialogSelectionPort = std::move(port);
    _dialogSelectionPort.chatListModel = &_chatListModel;
    _dialogSelectionPort.dialogListModel = &_dialogListModel;
}

void ChatFeatureController::setPrivateCommandPort(ChatPrivateCommandPort port)
{
    _privateCommandPort = std::move(port);
}

void ChatFeatureController::setCurrentSendPort(ChatCurrentSendPort port)
{
    _currentSendPort = std::move(port);
}

void ChatFeatureController::setContentDispatchPort(ChatContentDispatchPort port)
{
    _contentDispatchPort = std::move(port);
}

void ChatFeatureController::setCurrentHistoryPort(ChatCurrentHistoryPort port)
{
    _currentHistoryPort = std::move(port);
}

void ChatFeatureController::setCurrentGroupHistoryPort(ChatCurrentGroupHistoryPort port)
{
    _currentGroupHistoryPort = std::move(port);
}

void ChatFeatureController::setUploadedAttachmentDispatchPort(ChatUploadedAttachmentDispatchPort port)
{
    _uploadedAttachmentDispatchPort = std::move(port);
}

void ChatFeatureController::setPendingAttachmentPort(ChatPendingAttachmentPort port)
{
    _pendingAttachmentPort = std::move(port);
}

void ChatFeatureController::setDialogRuntimePort(ChatDialogRuntimePort port)
{
    _dialogRuntimePort = std::move(port);
}

void ChatFeatureController::setDialogCommandPort(ChatDialogCommandPort port)
{
    _dialogCommandPort = std::move(port);
}

void ChatFeatureController::setShellIntentPort(ChatShellIntentPort port)
{
    _shellIntentPort = std::move(port);
}

void ChatFeatureController::setMessageMutationPort(ChatMessageMutationPort port)
{
    _messageMutationPort = std::move(port);
}

void ChatFeatureController::selectPrivateByIndex(int index)
{
    ChatDialogSelectionService::selectPrivateByIndex(index, _dialogSelectionPort);
}

void ChatFeatureController::selectPrivateByUid(int uid)
{
    ChatDialogSelectionService::selectPrivateByUid(uid, _dialogSelectionPort);
}

void ChatFeatureController::selectGroupByIndex(int index)
{
    ChatDialogSelectionService::selectGroupByIndex(index, _dialogSelectionPort);
}

void ChatFeatureController::selectGroupByDialogUid(int dialogUid, qint64 groupId)
{
    ChatDialogSelectionService::selectGroupByDialogUid(dialogUid, groupId, _dialogSelectionPort);
}

void ChatFeatureController::selectDialogByUid(int dialogUid)
{
    ChatDialogSelectionService::selectDialogByUid(dialogUid, _dialogSelectionPort);
}
