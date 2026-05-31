#include "AppCoordinators.h"

#include "AppChatConnectionCoordinator.h"
#include "AppController.h"
#include "IconPathUtils.h"
#include "usermgr.h"

#include <QDateTime>
#include <QTimer>
#include <QVariantList>
#include <QDebug>

namespace
{
constexpr int kHeartbeatIntervalMs = 5000;
constexpr int kPostLoginBootstrapDelayMs = 100;
} // namespace

SessionChatEntryCoordinator::SessionChatEntryCoordinator(AppController& controller)
    : _app(controller)
{
}

void SessionChatEntryCoordinator::onSwitchToChat()
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    qInfo() << "Chat login succeeded, switching to chat page for uid:" << _app._pending_login_state.uid;
    qInfo() << "login.stage.summary"
            << "http_login_ms:"
            << (_app._chat_endpoint_state.httpLoginFinishedMs > _app._chat_endpoint_state.loginStartedMs
                    ? (_app._chat_endpoint_state.httpLoginFinishedMs - _app._chat_endpoint_state.loginStartedMs)
                    : 0)
            << "chat_connect_ms:"
            << (_app._chat_endpoint_state.connectFinishedMs > _app._chat_endpoint_state.connectStartedMs
                    ? (_app._chat_endpoint_state.connectFinishedMs - _app._chat_endpoint_state.connectStartedMs)
                    : 0)
            << "chat_auth_ms:"
            << (_app._chat_endpoint_state.connectFinishedMs > 0 ? (nowMs - _app._chat_endpoint_state.connectFinishedMs)
                                                                : 0)
            << "login_total_ms:"
            << (_app._chat_endpoint_state.loginStartedMs > 0 ? (nowMs - _app._chat_endpoint_state.loginStartedMs) : 0)
            << "server:" << _app._chat_endpoint_state.serverName;
    _app._chat_recovery_state.ignoreNextLoginDisconnect = false;
    _app._chat_login_timeout_timer.stop();
    _app._chat_connection_coordinator->resetReconnectState();
    _app._chat_connection_coordinator->resetHeartbeatTracking();
    _app._chat_recovery_state.lastHeartbeatAckMs = QDateTime::currentMSecsSinceEpoch();
    _app.setPage(AppController::ChatPage);
    _app.setBusy(false);
    _app.setTip("", false);
    _app.setSearchPending(false);
    _app.setChatLoadingMore(false);
    _app.setPrivateHistoryLoading(false);
    _app.setCanLoadMorePrivateHistory(false);
    _app._private_history_state.beforeTs = 0;
    _app._private_history_state.beforeMsgId.clear();
    _app._private_history_state.pendingBeforeTs = 0;
    _app._private_history_state.pendingBeforeMsgId.clear();
    _app._private_history_state.pendingPeerUid = 0;
    _app._group_state.historyBeforeSeq = 0;
    _app._group_state.historyHasMore = true;
    _app._loading_state.groupHistoryLoading = false;
    _app._bootstrap_state.dialogBootstrapLoading = false;
    _app._bootstrap_state.chatListInitialized = false;
    _app.setDialogsReady(false);
    _app.setContactsReady(false);
    _app.setGroupsReady(false);
    _app.setApplyReady(false);
    _app._group_state.pendingMsgGroupMap.clear();
    _app._dialog_state.mentionMap.clear();
    _app._dialog_state.pendingAttachmentMap.clear();
    _app._pending_send_state.reset();
    _app.setCurrentPendingAttachments(QVariantList());
    _app.setPendingReplyContext(QString(), QString(), QString());
    _app.setContactLoadingMore(false);
    _app.setAuthStatus("", false);
    _app.setSettingsStatus("", false);
    _app.setContactPane(AppController::ApplyRequestPane);
    _app.setCurrentContact(0, "", "", "qrc:/res/head_1.png", "", 0);
    _app._bootstrap_state.postLoginBootstrapStarted = false;

    auto user_info = _app._gateway.userMgr()->GetUserInfo();
    if (user_info)
    {
        setIconDownloadAuthContext(user_info->_uid, _app._pending_login_state.token);
        _app._message_model.setDownloadAuthContext(user_info->_uid, _app._pending_login_state.token);
        _app.applyCurrentUserProfile(user_info->_uid,
                                     user_info->_name,
                                     user_info->_nick,
                                     user_info->_icon,
                                     user_info->_desc,
                                     user_info->_user_id,
                                     user_info->_sex,
                                     true);
    }
    else
    {
        _app._dialog_state.draftMap.clear();
        _app._dialog_state.pendingAttachmentMap.clear();
        _app._dialog_state.serverMuteMap.clear();
        _app._dialog_state.mentionMap.clear();
        _app.setCurrentDraftText("");
        _app.setCurrentPendingAttachments(QVariantList());
        _app.setCurrentDialogPinned(false);
        _app.setCurrentDialogMuted(false);
    }

    beginPostLoginBootstrap();
}

void SessionChatEntryCoordinator::beginPostLoginBootstrap()
{
    if (_app._page != AppController::ChatPage || _app._bootstrap_state.postLoginBootstrapStarted ||
        !_app.isChatTransportReady())
    {
        return;
    }

    _app._bootstrap_state.postLoginBootstrapStarted = true;
    runPostLoginBootstrap();
}

void SessionChatEntryCoordinator::runPostLoginBootstrap()
{
    auto user_info = _app._gateway.userMgr()->GetUserInfo();
    if (user_info)
    {
        setIconDownloadAuthContext(user_info->_uid, _app._pending_login_state.token);
        _app._message_model.setDownloadAuthContext(user_info->_uid, _app._pending_login_state.token);
        _app._private_cache_store.openForUser(user_info->_uid);
        _app._group_cache_store.openForUser(user_info->_uid);
        _app.loadDraftStore(user_info->_uid);
    }

    QTimer::singleShot(kPostLoginBootstrapDelayMs,
                       &_app,
                       [this]()
                       {
                           if (_app._page != AppController::ChatPage)
                           {
                               return;
                           }
                           qInfo() << "[PERF] Stage-0: Bootstrap all data in parallel, ts:"
                                   << QDateTime::currentMSecsSinceEpoch();
                           // Parallel bootstrap: fire all requests simultaneously for minimum latency
                           _app.bootstrapDialogs();
                           _app.bootstrapApplies();
                           _app.requestRelationBootstrap();
                           _app._heartbeat_timer.start(kHeartbeatIntervalMs);
                           _app._chat_connection_coordinator->onHeartbeatTimeout();
                       });

    QTimer::singleShot(kPostLoginBootstrapDelayMs,
                       &_app,
                       [this]()
                       {
                           if (_app._page != AppController::ChatPage || _app._bootstrap_state.dialogsReady)
                           {
                               return;
                           }
                           qInfo() << "[PERF] Stage-1: Ensure chat list initialized, ts:"
                                   << QDateTime::currentMSecsSinceEpoch();
                           _app.ensureChatListInitialized();
                       });
}
