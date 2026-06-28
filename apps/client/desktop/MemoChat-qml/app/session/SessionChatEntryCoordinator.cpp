#include "AppCoordinators.h"

#include "userdata.h"

#include <QDateTime>
#include <QDebug>
#include <QVariantList>

namespace
{
constexpr int kHeartbeatIntervalMs = 5000;
constexpr int kPostLoginBootstrapDelayMs = 100;
} // namespace

SessionChatEntryCoordinator::SessionChatEntryCoordinator(PostLoginBootstrapPort port)
    : _port(std::move(port))
{
}

void SessionChatEntryCoordinator::onSwitchToChat()
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const PostLoginBootstrapSnapshot snapshot = _port.snapshot();
    qInfo() << "Chat login succeeded, switching to chat page for uid:" << snapshot.pendingUid;
    qInfo() << "login.stage.summary"
            << "http_login_ms:"
            << (snapshot.httpLoginFinishedMs > snapshot.loginStartedMs
                    ? (snapshot.httpLoginFinishedMs - snapshot.loginStartedMs)
                    : 0)
            << "chat_connect_ms:"
            << (snapshot.connectFinishedMs > snapshot.connectStartedMs
                    ? (snapshot.connectFinishedMs - snapshot.connectStartedMs)
                    : 0)
            << "chat_auth_ms:" << (snapshot.connectFinishedMs > 0 ? (nowMs - snapshot.connectFinishedMs) : 0)
            << "login_total_ms:" << (snapshot.loginStartedMs > 0 ? (nowMs - snapshot.loginStartedMs) : 0)
            << "server:" << snapshot.endpointServerName;
    _port.setIgnoreNextLoginDisconnect(false);
    _port.stopLoginTimeoutTimer();
    _port.resetReconnectState();
    _port.resetHeartbeatTracking();
    _port.setLastHeartbeatAckMs(QDateTime::currentMSecsSinceEpoch());
    _port.switchToChatPage();
    _port.resetChatEntryRuntime();
    if (_port.setChatLoginCompleted)
    {
        _port.setChatLoginCompleted(true);
    }
    _port.setPostLoginBootstrapStarted(false);

    auto userInfo = _port.currentUserInfo();
    if (userInfo)
    {
        _port.applyLoggedInUserSession(userInfo, snapshot.pendingToken);
    }
    else
    {
        _port.clearMissingUserDialogState();
    }

    beginPostLoginBootstrap();
}

void SessionChatEntryCoordinator::beginPostLoginBootstrap()
{
    const PostLoginBootstrapSnapshot snapshot = _port.snapshot();
    if (!snapshot.isChatPage || snapshot.postLoginBootstrapStarted)
    {
        return;
    }

    if (!snapshot.chatTransportReady || !snapshot.chatLoginCompleted)
    {
        return;
    }

    _port.setPostLoginBootstrapStarted(true);
    runPostLoginBootstrap();
}

void SessionChatEntryCoordinator::runPostLoginBootstrap()
{
    const PostLoginBootstrapSnapshot snapshot = _port.snapshot();
    auto userInfo = _port.currentUserInfo();
    if (userInfo)
    {
        _port.applyLoggedInUserSession(userInfo, snapshot.pendingToken);
        _port.openCachesAndDraftsForUser(userInfo->_uid);
    }

    _port.runDelayed(kPostLoginBootstrapDelayMs,
                     [this]()
                     {
                         if (!_port.snapshot().isChatPage)
                         {
                             return;
                         }
                         qInfo() << "[PERF] Stage-0: Bootstrap all data in parallel, ts:"
                                 << QDateTime::currentMSecsSinceEpoch();
                         _port.bootstrapDialogs();
                         _port.ensureContactsInitialized();
                         _port.ensureApplyInitialized();
                         _port.requestRelationBootstrap();
                         _port.startHeartbeatTimer(kHeartbeatIntervalMs);
                         _port.sendHeartbeatNow();
                     });

    _port.runDelayed(kPostLoginBootstrapDelayMs,
                     [this]()
                     {
                         const PostLoginBootstrapSnapshot current = _port.snapshot();
                         if (!current.isChatPage || current.dialogsReady)
                         {
                             return;
                         }
                         qInfo() << "[PERF] Stage-1: Ensure chat list initialized, ts:"
                                 << QDateTime::currentMSecsSinceEpoch();
                         _port.ensureChatListInitialized();
                     });
}
