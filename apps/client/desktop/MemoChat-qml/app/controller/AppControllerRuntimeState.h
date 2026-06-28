#pragma once

#include <QString>

struct AppLoadingRuntimeState
{
    bool chatLoadingMore = false;
    bool canLoadMoreChats = false;
    bool privateHistoryLoading = false;
    bool groupHistoryLoading = false;
    bool canLoadMorePrivateHistory = false;
};

struct AppLazyBootstrapState
{
    bool dialogBootstrapLoading = false;
    bool dialogsReady = false;
    bool groupsReady = false;
    bool chatListInitialized = false;
    bool postLoginBootstrapStarted = false;
    bool chatLoginCompleted = false;
};

struct AppMediaUploadRuntimeState
{
    bool inProgress = false;
    bool settingsAvatarUploadInProgress = false;
    QString progressText;
};
