#include "AppController.h"

QString AppController::tipText() const
{
    return _features.authViewModel.tipText();
}

bool AppController::tipError() const
{
    return _features.authViewModel.tipError();
}

bool AppController::busy() const
{
    return _features.authViewModel.busy();
}

AppController::ChatTab AppController::chatTab() const
{
    return static_cast<ChatTab>(_shell_state.chatTab());
}

AppController::ContactPane AppController::contactPane() const
{
    return static_cast<ContactPane>(_features.contactController.contactPane());
}

bool AppController::searchPending() const
{
    return _features.contactController.searchPending();
}

QString AppController::searchStatusText() const
{
    return _features.contactController.searchStatusText();
}

bool AppController::searchStatusError() const
{
    return _features.contactController.searchStatusError();
}

bool AppController::hasPendingApply() const
{
    return _features.contactController.hasPendingApply();
}

bool AppController::chatLoadingMore() const
{
    return _shell_state.loadingState().chatLoadingMore;
}

bool AppController::canLoadMoreChats() const
{
    return _shell_state.loadingState().canLoadMoreChats;
}

bool AppController::privateHistoryLoading() const
{
    return _shell_state.loadingState().privateHistoryLoading;
}

bool AppController::canLoadMorePrivateHistory() const
{
    return _shell_state.loadingState().canLoadMorePrivateHistory;
}

bool AppController::contactLoadingMore() const
{
    return _features.contactController.contactLoadingMore();
}

bool AppController::canLoadMoreContacts() const
{
    return _features.contactController.canLoadMoreContacts();
}

bool AppController::mediaUploadInProgress() const
{
    return _media_upload_state.inProgress;
}

QString AppController::mediaUploadProgressText() const
{
    return _media_upload_state.progressText;
}

QString AppController::currentDraftText() const
{
    return _features.chatFeatureController.currentDraftText();
}

QVariantList AppController::currentPendingAttachments() const
{
    return _features.chatFeatureController.currentPendingAttachments();
}

bool AppController::hasPendingAttachments() const
{
    return _features.chatFeatureController.hasPendingAttachments();
}

bool AppController::currentDialogPinned() const
{
    return _features.chatFeatureController.currentDialogPinned();
}

bool AppController::currentDialogMuted() const
{
    return _features.chatFeatureController.currentDialogMuted();
}

bool AppController::hasPendingReply() const
{
    return _features.chatFeatureController.hasPendingReply();
}

QString AppController::replyTargetName() const
{
    return _features.chatFeatureController.replyTargetName();
}

QString AppController::replyPreviewText() const
{
    return _features.chatFeatureController.replyPreviewText();
}

bool AppController::dialogsReady() const
{
    return _shell_state.bootstrapState().dialogsReady;
}

bool AppController::contactsReady() const
{
    return _features.contactController.contactsReady();
}

bool AppController::groupsReady() const
{
    return _shell_state.bootstrapState().groupsReady;
}

bool AppController::applyReady() const
{
    return _features.contactController.applyReady();
}

bool AppController::chatShellBusy() const
{
    return page() == ChatPage && !_shell_state.bootstrapState().dialogsReady;
}
