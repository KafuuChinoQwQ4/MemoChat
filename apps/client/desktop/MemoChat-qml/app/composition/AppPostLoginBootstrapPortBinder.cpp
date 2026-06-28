#include "AppChatConnectionCoordinator.h"
#include "AppCoordinators.h"
#include "AppPortRegistry.h"
#include "IconPathUtils.h"
#include "usermgr.h"

#include <QTimer>
#include <utility>

PostLoginBootstrapPort AppPortRegistry::makePostLoginBootstrapPort()
{
    PostLoginBootstrapPort port;

    port.snapshot = [this]()
    {
        PostLoginBootstrapSnapshot snapshot;
        const AppPendingLoginState& pending = _session_coordinator->pendingLoginState();
        const AppChatEndpointState& endpointState = _session_coordinator->chatEndpointState();
        snapshot.isChatPage = page() == _constants.chatPage;
        snapshot.postLoginBootstrapStarted = _shell_state.bootstrapState().postLoginBootstrapStarted;
        snapshot.chatTransportReady = isChatTransportReady();
        snapshot.chatLoginCompleted = _shell_state.bootstrapState().chatLoginCompleted;
        snapshot.dialogsReady = _shell_state.bootstrapState().dialogsReady;
        snapshot.pendingUid = pending.uid;
        snapshot.pendingToken = pending.token;
        snapshot.endpointServerName = endpointState.serverName;
        snapshot.loginStartedMs = endpointState.loginStartedMs;
        snapshot.httpLoginFinishedMs = endpointState.httpLoginFinishedMs;
        snapshot.connectStartedMs = endpointState.connectStartedMs;
        snapshot.connectFinishedMs = endpointState.connectFinishedMs;
        return snapshot;
    };
    port.currentUserInfo = [this]()
    {
        return _gateway.userMgr()->GetUserInfo();
    };
    port.setIgnoreNextLoginDisconnect = [this](bool ignore)
    {
        _session_coordinator->setIgnoreNextLoginDisconnect(ignore);
    };
    port.stopLoginTimeoutTimer = [this]()
    {
        _chat_login_timeout_timer.stop();
    };
    port.resetReconnectState = [this]()
    {
        _chat_connection_coordinator->resetReconnectState();
    };
    port.resetHeartbeatTracking = [this]()
    {
        _chat_connection_coordinator->resetHeartbeatTracking();
    };
    port.setLastHeartbeatAckMs = [this](qint64 ackMs)
    {
        _session_coordinator->setLastHeartbeatAckMs(ackMs);
    };
    port.switchToChatPage = [this]()
    {
        setPage(_constants.chatPage);
    };
    port.resetChatEntryRuntime = [this]()
    {
        setBusy(false);
        setTip(QString(), false);
        setSearchPending(false);
        setChatLoadingMore(false);
        setPrivateHistoryLoading(false);
        setCanLoadMorePrivateHistory(false);
        _shell_state.loadingState().groupHistoryLoading = false;
        _shell_state.bootstrapState().dialogBootstrapLoading = false;
        _shell_state.bootstrapState().chatListInitialized = false;
        _shell_state.bootstrapState().chatLoginCompleted = false;
        setDialogsReady(false);
        setContactsReady(false);
        setGroupsReady(false);
        setApplyReady(false);
        _features.chatFeatureController.resetForPostLoginEntry();
        setCurrentPendingAttachments(QVariantList());
        setPendingReplyContext(QString(), QString(), QString());
        setContactLoadingMore(false);
        setAuthStatus(QString(), false);
        setSettingsStatus(QString(), false);
        setContactPane(_constants.applyRequestPane);
        setCurrentContact(0, QString(), QString(), QStringLiteral("qrc:/res/head_1.png"), QString(), 0);
    };
    port.applyLoggedInUserSession = [this](const std::shared_ptr<UserInfo>& userInfo, const QString& token)
    {
        if (!userInfo)
        {
            return;
        }
        setIconDownloadAuthContext(userInfo->_uid, token);
        _features.chatFeatureController.setMessageDownloadAuthContext(userInfo->_uid, token);
        applyCurrentUserProfile(userInfo->_uid,
                                userInfo->_name,
                                userInfo->_nick,
                                userInfo->_icon,
                                userInfo->_desc,
                                userInfo->_user_id,
                                userInfo->_sex,
                                true);
    };
    port.clearMissingUserDialogState = [this]()
    {
        _features.chatFeatureController.clearDialogDecorationStore();
        setCurrentDraftText(QString());
        setCurrentPendingAttachments(QVariantList());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
    };
    port.setPostLoginBootstrapStarted = [this](bool started)
    {
        _shell_state.bootstrapState().postLoginBootstrapStarted = started;
    };
    port.setChatLoginCompleted = [this](bool completed)
    {
        _shell_state.bootstrapState().chatLoginCompleted = completed;
    };
    port.runDelayed = [this](int delayMs, std::function<void()> callback)
    {
        QTimer::singleShot(delayMs, &_async_receiver, std::move(callback));
    };
    port.openCachesAndDraftsForUser = [this](int uid)
    {
        _features.chatFeatureController.openCacheStoresForUser(uid);
        loadDraftStore(uid);
    };
    port.bootstrapDialogs = [this]()
    {
        bootstrapDialogs();
    };
    port.ensureContactsInitialized = [this]()
    {
        _features.contactController.ensureContactsInitialized();
    };
    port.ensureApplyInitialized = [this]()
    {
        ensureApplyInitialized();
    };
    port.requestRelationBootstrap = [this]()
    {
        requestRelationBootstrap();
    };
    port.startHeartbeatTimer = [this](int intervalMs)
    {
        _heartbeat_timer.start(intervalMs);
    };
    port.sendHeartbeatNow = [this]()
    {
        _chat_connection_coordinator->onHeartbeatTimeout();
    };
    port.ensureChatListInitialized = [this]()
    {
        ensureChatListInitialized();
    };

    return port;
}
