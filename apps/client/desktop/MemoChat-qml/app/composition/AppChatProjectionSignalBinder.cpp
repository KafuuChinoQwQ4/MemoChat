#include "AppController.h"

void AppController::bindAppChatProjectionSignals()
{
    connect(this, &AppController::chatTabChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::currentChatPeerChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::currentDialogUidChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::currentGroupChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::chatLoadingMoreChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::canLoadMoreChatsChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::privateHistoryLoadingChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::canLoadMorePrivateHistoryChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::mediaUploadStateChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::currentDraftTextChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::currentPendingAttachmentsChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::currentDialogPinnedChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::currentDialogMutedChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::pendingReplyChanged, this, &AppController::syncChatViewModelState);
    connect(this, &AppController::lazyBootstrapStateChanged, this, &AppController::syncChatViewModelState);
}
