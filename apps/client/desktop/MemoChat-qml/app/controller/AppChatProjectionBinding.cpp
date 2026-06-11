#include "AppController.h"

#include <utility>

void AppController::bindChatFeatureProjectionPort()
{
    ChatViewProjectionPort projectionPort;
    projectionPort.snapshot = [this]()
    {
        ChatViewProjectionState state;
        state.chatTab = _shell_state.chatTab();
        state.currentDialogUid = currentDialogUid();
        state.currentChatPeerName = currentChatPeerName();
        state.currentChatPeerIcon = currentChatPeerIcon();
        state.hasCurrentChat = hasCurrentChat();
        state.hasCurrentGroup = hasCurrentGroup();
        state.currentGroupRole = currentGroupRole();
        state.currentDraftText = currentDraftText();
        state.currentPendingAttachments = currentPendingAttachments();
        state.currentDialogPinned = currentDialogPinned();
        state.currentDialogMuted = currentDialogMuted();
        state.hasPendingReply = hasPendingReply();
        state.replyTargetName = replyTargetName();
        state.replyPreviewText = replyPreviewText();
        state.dialogsReady = dialogsReady();
        state.chatLoadingMore = chatLoadingMore();
        state.canLoadMoreChats = canLoadMoreChats();
        state.privateHistoryLoading = privateHistoryLoading();
        state.canLoadMorePrivateHistory = canLoadMorePrivateHistory();
        state.mediaUploadInProgress = mediaUploadInProgress();
        state.mediaUploadProgressText = mediaUploadProgressText();
        return state;
    };
    _features.chatFeatureController.setViewProjectionPort(std::move(projectionPort));
}
