#include "AppChatConnectionCoordinator.h"
#include "AppCoordinators.h"
#include "AppPortRegistry.h"
#include "IChatTransport.h"
#include "IconPathUtils.h"
#include "httpmgr.h"
#include "usermgr.h"

SessionLogoutPort AppPortRegistry::makeSessionLogoutPort()
{
    SessionLogoutPort port;

    port.currentUserUid = [this]()
    {
        return currentUserUid();
    };
    port.stopSessionTimers = [this]()
    {
        _heartbeat_timer.stop();
        _chat_login_timeout_timer.stop();
    };
    port.resetPresenceSurfaces = [this]()
    {
        _features.callController.resetCallSurface();
        _features.petController.stopStream();
    };
    port.resetConnectionRuntime = [this]()
    {
        _shell_state.bootstrapState().postLoginBootstrapStarted = false;
        _chat_connection_coordinator->resetReconnectState();
        _chat_connection_coordinator->resetHeartbeatTracking();
    };
    port.closeNetworkResources = [this]()
    {
        if (const auto transport = _gateway.chatTransport())
        {
            transport->CloseConnection();
        }
        if (auto http = _gateway.httpMgr())
        {
            http->clearConnectionCache();
        }
        _features.chatFeatureController.closeCacheStores();
        _gateway.userMgr()->ResetSession();
    };
    port.resetAuthShellState = [this]()
    {
        _features.authViewModel.syncRegisterSuccessPage(false);
        setBusy(false);
        setTip(QString(), false);
        setSearchPending(false);
        setSearchStatus(QString(), false);
        setAuthStatus(QString(), false);
        setSettingsStatus(QString(), false);
        setGroupStatus(QString(), false);
        setContactPane(_constants.applyRequestPane);
    };
    port.resetFeatureModelsForLogout = [this]()
    {
        _features.chatFeatureController.resetModelsForLogout();
        _features.contactController.resetContactFeature();
        _features.groupController.resetGroupFeature();
    };
    port.resetFeatureRuntimeForLogout = [this]()
    {
        setChatLoadingMore(false);
        setPrivateHistoryLoading(false);
        _shell_state.loadingState().groupHistoryLoading = false;
        _shell_state.bootstrapState().dialogBootstrapLoading = false;
        _shell_state.bootstrapState().chatListInitialized = false;
        setDialogsReady(false);
        setGroupsReady(false);
        setCanLoadMorePrivateHistory(false);
        setContactLoadingMore(false);
        if (_shell_state.loadingState().canLoadMoreChats)
        {
            _shell_state.loadingState().canLoadMoreChats = false;
        }
        _features.contactController.setCanLoadMoreContacts(false);
        _features.agentController.resetForLogout();
        setCurrentContact(0, QString(), QString(), QStringLiteral("qrc:/res/head_1.png"), QString(), 0);
        _chat_state.uid = 0;
        emitCurrentDialogUidChangedIfNeeded();
        _features.chatFeatureController.resetRuntimeForLogout();
        setCurrentDraftText(QString());
        setCurrentPendingAttachments(QVariantList());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        setPendingReplyContext(QString(), QString(), QString());
        setCurrentChatPeerName(QString());
        setCurrentChatPeerIcon(QStringLiteral("qrc:/res/head_1.png"));
        _features.chatFeatureController.syncViewModelState();
    };
    port.clearCurrentUserState = [this](int previousUserUid)
    {
        if (_shell_state.resetCurrentUser(previousUserUid))
        {
            syncShellViewModelState();
        }
    };
    port.resetMediaRuntimeForLogout = [this]()
    {
        setMediaUploadInProgress(false);
        setMediaUploadProgressText(QString());
        _media_upload_state.settingsAvatarUploadInProgress = false;
    };
    port.clearDownloadAuthContext = [this]()
    {
        _features.chatFeatureController.setMessageDownloadAuthContext(0, QString());
        setIconDownloadAuthContext(0, QString());
    };

    return port;
}
