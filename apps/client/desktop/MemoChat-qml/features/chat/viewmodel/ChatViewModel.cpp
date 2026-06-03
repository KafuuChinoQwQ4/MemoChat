#include "ChatViewModel.h"

#include "ChatMessageModel.h"
#include "FriendListModel.h"

ChatViewModel::ChatViewModel(QObject* parent)
    : QObject(parent)
{
}

int ChatViewModel::chatTab() const
{
    return _state.chatTab;
}

int ChatViewModel::currentDialogUid() const
{
    return _state.currentDialogUid;
}

QString ChatViewModel::currentChatPeerName() const
{
    return _state.currentChatPeerName;
}

QString ChatViewModel::currentChatPeerIcon() const
{
    return _state.currentChatPeerIcon;
}

bool ChatViewModel::hasCurrentChat() const
{
    return _state.hasCurrentChat;
}

bool ChatViewModel::hasCurrentGroup() const
{
    return _state.hasCurrentGroup;
}

int ChatViewModel::currentGroupRole() const
{
    return _state.currentGroupRole;
}

FriendListModel* ChatViewModel::dialogListModel() const
{
    return _dialogListModel;
}

ChatMessageModel* ChatViewModel::messageModel() const
{
    return _messageModel;
}

QString ChatViewModel::currentDraftText() const
{
    return _state.currentDraftText;
}

QVariantList ChatViewModel::currentPendingAttachments() const
{
    return _state.currentPendingAttachments;
}

bool ChatViewModel::hasPendingAttachments() const
{
    return !_state.currentPendingAttachments.isEmpty();
}

bool ChatViewModel::currentDialogPinned() const
{
    return _state.currentDialogPinned;
}

bool ChatViewModel::currentDialogMuted() const
{
    return _state.currentDialogMuted;
}

bool ChatViewModel::hasPendingReply() const
{
    return _state.hasPendingReply;
}

QString ChatViewModel::replyTargetName() const
{
    return _state.replyTargetName;
}

QString ChatViewModel::replyPreviewText() const
{
    return _state.replyPreviewText;
}

bool ChatViewModel::dialogsReady() const
{
    return _state.dialogsReady;
}

bool ChatViewModel::chatLoadingMore() const
{
    return _state.chatLoadingMore;
}

bool ChatViewModel::canLoadMoreChats() const
{
    return _state.canLoadMoreChats;
}

bool ChatViewModel::privateHistoryLoading() const
{
    return _state.privateHistoryLoading;
}

bool ChatViewModel::canLoadMorePrivateHistory() const
{
    return _state.canLoadMorePrivateHistory;
}

bool ChatViewModel::mediaUploadInProgress() const
{
    return _state.mediaUploadInProgress;
}

QString ChatViewModel::mediaUploadProgressText() const
{
    return _state.mediaUploadProgressText;
}

void ChatViewModel::syncDialogListModel(FriendListModel* model)
{
    if (_dialogListModel == model)
    {
        return;
    }
    _dialogListModel = model;
    emit modelChanged();
}

void ChatViewModel::syncMessageModel(ChatMessageModel* model)
{
    if (_messageModel == model)
    {
        return;
    }
    _messageModel = model;
    emit modelChanged();
}

void ChatViewModel::syncChatTab(int tab)
{
    if (_state.chatTab == tab)
    {
        return;
    }
    _state.chatTab = tab;
    emit stateChanged();
}

void ChatViewModel::syncCurrentDialogUid(int uid)
{
    if (_state.currentDialogUid == uid)
    {
        return;
    }
    _state.currentDialogUid = uid;
    emit stateChanged();
}

void ChatViewModel::syncCurrentPeer(const QString& name, const QString& icon, bool hasCurrentChat)
{
    if (_state.currentChatPeerName == name && _state.currentChatPeerIcon == icon &&
        _state.hasCurrentChat == hasCurrentChat)
    {
        return;
    }
    _state.currentChatPeerName = name;
    _state.currentChatPeerIcon = icon;
    _state.hasCurrentChat = hasCurrentChat;
    emit stateChanged();
}

void ChatViewModel::syncCurrentGroup(bool hasCurrentGroup, int role)
{
    if (_state.hasCurrentGroup == hasCurrentGroup && _state.currentGroupRole == role)
    {
        return;
    }
    _state.hasCurrentGroup = hasCurrentGroup;
    _state.currentGroupRole = role;
    emit stateChanged();
}

void ChatViewModel::syncCurrentDraftText(const QString& text)
{
    if (_state.currentDraftText == text)
    {
        return;
    }
    _state.currentDraftText = text;
    emit stateChanged();
}

void ChatViewModel::syncCurrentPendingAttachments(const QVariantList& attachments)
{
    if (_state.currentPendingAttachments == attachments)
    {
        return;
    }
    _state.currentPendingAttachments = attachments;
    emit stateChanged();
}

void ChatViewModel::syncCurrentDialogPinned(bool pinned)
{
    if (_state.currentDialogPinned == pinned)
    {
        return;
    }
    _state.currentDialogPinned = pinned;
    emit stateChanged();
}

void ChatViewModel::syncCurrentDialogMuted(bool muted)
{
    if (_state.currentDialogMuted == muted)
    {
        return;
    }
    _state.currentDialogMuted = muted;
    emit stateChanged();
}

void ChatViewModel::syncPendingReply(bool hasReply, const QString& targetName, const QString& previewText)
{
    if (_state.hasPendingReply == hasReply && _state.replyTargetName == targetName &&
        _state.replyPreviewText == previewText)
    {
        return;
    }
    _state.hasPendingReply = hasReply;
    _state.replyTargetName = targetName;
    _state.replyPreviewText = previewText;
    emit stateChanged();
}

void ChatViewModel::syncDialogsReady(bool ready)
{
    if (_state.dialogsReady == ready)
    {
        return;
    }
    _state.dialogsReady = ready;
    emit stateChanged();
}

void ChatViewModel::syncChatLoadingMore(bool loading)
{
    if (_state.chatLoadingMore == loading)
    {
        return;
    }
    _state.chatLoadingMore = loading;
    emit stateChanged();
}

void ChatViewModel::syncCanLoadMoreChats(bool canLoad)
{
    if (_state.canLoadMoreChats == canLoad)
    {
        return;
    }
    _state.canLoadMoreChats = canLoad;
    emit stateChanged();
}

void ChatViewModel::syncPrivateHistoryLoading(bool loading)
{
    if (_state.privateHistoryLoading == loading)
    {
        return;
    }
    _state.privateHistoryLoading = loading;
    emit stateChanged();
}

void ChatViewModel::syncCanLoadMorePrivateHistory(bool canLoad)
{
    if (_state.canLoadMorePrivateHistory == canLoad)
    {
        return;
    }
    _state.canLoadMorePrivateHistory = canLoad;
    emit stateChanged();
}

void ChatViewModel::syncMediaUpload(bool inProgress, const QString& progressText)
{
    if (_state.mediaUploadInProgress == inProgress && _state.mediaUploadProgressText == progressText)
    {
        return;
    }
    _state.mediaUploadInProgress = inProgress;
    _state.mediaUploadProgressText = progressText;
    emit stateChanged();
}

void ChatViewModel::ensureChatListInitialized()
{
    emit ensureChatListInitializedRequested();
}

void ChatViewModel::ensureGroupsInitialized()
{
    emit ensureGroupsInitializedRequested();
}

void ChatViewModel::selectDialogByUid(int uid)
{
    emit selectDialogByUidRequested(uid);
}

void ChatViewModel::selectChatIndex(int index)
{
    emit selectChatIndexRequested(index);
}

void ChatViewModel::selectGroupIndex(int index)
{
    emit selectGroupIndexRequested(index);
}

void ChatViewModel::loadMoreChats()
{
    emit loadMoreChatsRequested();
}

void ChatViewModel::loadMorePrivateHistory()
{
    emit loadMorePrivateHistoryRequested();
}

void ChatViewModel::sendCurrentComposerPayload(const QString& text)
{
    emit sendCurrentComposerPayloadRequested(text);
}

void ChatViewModel::sendImageMessage()
{
    emit sendImageMessageRequested();
}

void ChatViewModel::sendFileMessage()
{
    emit sendFileMessageRequested();
}

void ChatViewModel::removePendingAttachment(const QString& attachmentId)
{
    emit removePendingAttachmentRequested(attachmentId);
}

void ChatViewModel::clearPendingAttachments()
{
    emit clearPendingAttachmentsRequested();
}

void ChatViewModel::updateCurrentDraft(const QString& text)
{
    emit updateCurrentDraftRequested(text);
}

void ChatViewModel::toggleCurrentDialogPinned()
{
    emit toggleCurrentDialogPinnedRequested();
}

void ChatViewModel::toggleCurrentDialogMuted()
{
    emit toggleCurrentDialogMutedRequested();
}

void ChatViewModel::toggleDialogPinnedByUid(int dialogUid)
{
    emit toggleDialogPinnedByUidRequested(dialogUid);
}

void ChatViewModel::toggleDialogMutedByUid(int dialogUid)
{
    emit toggleDialogMutedByUidRequested(dialogUid);
}

void ChatViewModel::markDialogReadByUid(int dialogUid)
{
    emit markDialogReadByUidRequested(dialogUid);
}

void ChatViewModel::clearDialogDraftByUid(int dialogUid)
{
    emit clearDialogDraftByUidRequested(dialogUid);
}

void ChatViewModel::beginReplyMessage(const QString& msgId, const QString& senderName, const QString& previewText)
{
    emit beginReplyMessageRequested(msgId, senderName, previewText);
}

void ChatViewModel::cancelReplyMessage()
{
    emit cancelReplyMessageRequested();
}

void ChatViewModel::startVoiceChat()
{
    emit startVoiceChatRequested();
}

void ChatViewModel::startVideoChat()
{
    emit startVideoChatRequested();
}
