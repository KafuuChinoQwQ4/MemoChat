#pragma once

#include <QString>

struct AppShellRuntimeState
{
    QString tipText;
    bool tipError = false;
    bool busy = false;
    bool registerSuccessPage = false;
    int registerCountdown = 5;
};

struct AppSearchRuntimeState
{
    bool pending = false;
    QString statusText;
    bool statusError = false;
};

struct AppFeatureStatusState
{
    QString authText;
    bool authError = false;
    QString settingsText;
    bool settingsError = false;
    QString groupText;
    bool groupError = false;
};

struct AppLoadingRuntimeState
{
    bool chatLoadingMore = false;
    bool canLoadMoreChats = false;
    bool privateHistoryLoading = false;
    bool groupHistoryLoading = false;
    bool canLoadMorePrivateHistory = false;
    bool contactLoadingMore = false;
    bool canLoadMoreContacts = false;
};

struct AppLazyBootstrapState
{
    bool dialogBootstrapLoading = false;
    bool dialogsReady = false;
    bool contactsReady = false;
    bool groupsReady = false;
    bool applyReady = false;
    bool chatListInitialized = false;
    bool postLoginBootstrapStarted = false;
};

struct AppMediaUploadRuntimeState
{
    bool inProgress = false;
    bool settingsAvatarUploadInProgress = false;
    bool groupIconUploadInProgress = false;
    QString progressText;
};
