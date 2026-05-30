#include "AppController.h"

QString AppController::tipText() const
{
    return _shell_state.tipText;
}

bool AppController::tipError() const
{
    return _shell_state.tipError;
}

bool AppController::busy() const
{
    return _shell_state.busy;
}

bool AppController::registerSuccessPage() const
{
    return _shell_state.registerSuccessPage;
}

int AppController::registerCountdown() const
{
    return _shell_state.registerCountdown;
}

AppController::ChatTab AppController::chatTab() const
{
    return _chat_tab;
}

AppController::ContactPane AppController::contactPane() const
{
    return _contact_pane;
}

bool AppController::searchPending() const
{
    return _search_state.pending;
}

QString AppController::searchStatusText() const
{
    return _search_state.statusText;
}

bool AppController::searchStatusError() const
{
    return _search_state.statusError;
}

bool AppController::hasPendingApply() const
{
    return _apply_request_model.hasUnapproved();
}

bool AppController::chatLoadingMore() const
{
    return _loading_state.chatLoadingMore;
}

bool AppController::canLoadMoreChats() const
{
    return _loading_state.canLoadMoreChats;
}

bool AppController::privateHistoryLoading() const
{
    return _loading_state.privateHistoryLoading;
}

bool AppController::canLoadMorePrivateHistory() const
{
    return _loading_state.canLoadMorePrivateHistory;
}

bool AppController::contactLoadingMore() const
{
    return _loading_state.contactLoadingMore;
}

bool AppController::canLoadMoreContacts() const
{
    return _loading_state.canLoadMoreContacts;
}

QString AppController::authStatusText() const
{
    return _feature_status_state.authText;
}

bool AppController::authStatusError() const
{
    return _feature_status_state.authError;
}

QString AppController::settingsStatusText() const
{
    return _feature_status_state.settingsText;
}

bool AppController::settingsStatusError() const
{
    return _feature_status_state.settingsError;
}

QString AppController::groupStatusText() const
{
    return _feature_status_state.groupText;
}

bool AppController::groupStatusError() const
{
    return _feature_status_state.groupError;
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
    return _dialog_state.currentDraftText;
}

QVariantList AppController::currentPendingAttachments() const
{
    return _dialog_state.currentPendingAttachments;
}

bool AppController::hasPendingAttachments() const
{
    return !_dialog_state.currentPendingAttachments.isEmpty();
}

bool AppController::currentDialogPinned() const
{
    return _dialog_state.currentPinned;
}

bool AppController::currentDialogMuted() const
{
    return _dialog_state.currentMuted;
}

bool AppController::hasPendingReply() const
{
    return !_dialog_state.replyToMsgId.isEmpty();
}

QString AppController::replyTargetName() const
{
    return _dialog_state.replyTargetName;
}

QString AppController::replyPreviewText() const
{
    return _dialog_state.replyPreviewText;
}

bool AppController::dialogsReady() const
{
    return _bootstrap_state.dialogsReady;
}

bool AppController::contactsReady() const
{
    return _bootstrap_state.contactsReady;
}

bool AppController::groupsReady() const
{
    return _bootstrap_state.groupsReady;
}

bool AppController::applyReady() const
{
    return _bootstrap_state.applyReady;
}

bool AppController::chatShellBusy() const
{
    return _page == ChatPage && !_bootstrap_state.dialogsReady;
}
