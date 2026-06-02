#include "AppController.h"
#include "AppChatConnectionCoordinator.h"
#include "AppCoordinators.h"
#include "ChatMessageDispatcher.h"
#include "LocalFilePickerService.h"
#include "IChatTransport.h"
#include "httpmgr.h"
#include <QDebug>
#include <utility>

AppController::AppController(QObject* parent)
    : QObject(parent)
    , _page(LoginPage)
    , _chat_tab(ChatTabPage)
    , _contact_pane(ApplyRequestPane)
    , _chat_list_model(this)
    , _group_list_model(this)
    , _dialog_list_model(this)
    , _contact_list_model(this)
    , _message_model(this)
    , _search_result_model(this)
    , _apply_request_model(this)
    , _auth_controller(&_gateway)
    , _call_controller(&_gateway)
    , _call_session_model(this)
    , _livekit_bridge(this)
    , _chat_controller(&_gateway)
    , _contact_controller(&_gateway)
    , _profile_controller(&_gateway)
    , _agent_controller(&_gateway)
    , _pet_controller(&_gateway)
    , _r18_controller(&_gateway)
    , _session_coordinator(std::make_unique<AppSessionCoordinator>(*this))
    , _contact_coordinator_shell(std::make_unique<ContactCoordinatorShell>(*this))
    , _group_coordinator(std::make_unique<GroupCoordinator>(*this))
    , _media_coordinator(std::make_unique<MediaCoordinator>(*this))
    , _call_coordinator(std::make_unique<CallCoordinator>(*this))
    , _profile_coordinator(std::make_unique<ProfileCoordinator>(*this))
    , _chat_connection_coordinator(std::make_unique<AppChatConnectionCoordinator>(*this))
{
    const auto chatTransport = _gateway.chatTransport();
    const auto chatDispatcher = _gateway.chatMessageDispatcher();

    connect(_gateway.httpMgr().get(), &HttpMgr::sig_login_mod_finish, this, &AppController::onLoginHttpFinished);
    connect(_gateway.httpMgr().get(), &HttpMgr::sig_reg_mod_finish, this, &AppController::onRegisterHttpFinished);
    connect(_gateway.httpMgr().get(), &HttpMgr::sig_reset_mod_finish, this, &AppController::onResetHttpFinished);
    connect(_gateway.httpMgr().get(), &HttpMgr::sig_settings_mod_finish, this, &AppController::onSettingsHttpFinished);
    connect(_gateway.httpMgr().get(),
            &HttpMgr::sig_call_mod_finish,
            this,
            [this](ReqId id, QString res, ErrorCodes err)
            {
                _call_coordinator->onCallHttpFinished(id, std::move(res), err);
            });

    connect(chatTransport.get(),
            &IChatTransport::sig_con_success,
            this,
            [this](bool success)
            {
                _chat_connection_coordinator->onTcpConnectFinished(success);
            });
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_login_failed,
            this,
            [this](int err)
            {
                _chat_connection_coordinator->onChatLoginFailed(err);
            });
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_swich_chatdlg, this, &AppController::onSwitchToChat);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_add_auth_friend, this, &AppController::onAddAuthFriend);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_auth_rsp, this, &AppController::onAuthRsp);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_delete_friend_rsp,
            this,
            &AppController::onDeleteFriendRsp);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_text_chat_msg, this, &AppController::onTextChatMsg);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_user_search, this, &AppController::onUserSearch);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_friend_apply, this, &AppController::onFriendApply);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_notify_offline,
            this,
            [this]()
            {
                _chat_connection_coordinator->onNotifyOffline();
            });
    connect(chatTransport.get(),
            &IChatTransport::sig_connection_closed,
            this,
            [this]()
            {
                _chat_connection_coordinator->onConnectionClosed();
            });
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_group_list_updated,
            this,
            &AppController::onGroupListUpdated);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_group_invite, this, &AppController::onGroupInvite);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_group_apply, this, &AppController::onGroupApply);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_group_member_changed,
            this,
            &AppController::onGroupMemberChanged);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_group_chat_msg, this, &AppController::onGroupChatMsg);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_group_rsp, this, &AppController::onGroupRsp);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_relation_bootstrap_updated,
            this,
            &AppController::onRelationBootstrapUpdated);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_dialog_list_rsp, this, &AppController::onDialogListRsp);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_private_history_rsp,
            this,
            &AppController::onPrivateHistoryRsp);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_private_msg_changed,
            this,
            &AppController::onPrivateMsgChanged);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_private_read_ack, this, &AppController::onPrivateReadAck);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_message_status, this, &AppController::onMessageStatus);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_call_event,
            this,
            [this](QJsonObject payload)
            {
                _call_coordinator->onCallEvent(std::move(payload));
            });
    connect(&_livekit_bridge,
            &LivekitBridge::roomJoined,
            this,
            [this]()
            {
                _call_coordinator->onLivekitRoomJoined();
            });
    connect(&_livekit_bridge,
            &LivekitBridge::remoteTrackReady,
            this,
            [this]()
            {
                _call_coordinator->onLivekitRemoteTrackReady();
            });
    connect(&_livekit_bridge,
            &LivekitBridge::roomDisconnected,
            this,
            [this](const QString& reason, bool recoverable)
            {
                _call_coordinator->onLivekitRoomDisconnected(reason, recoverable);
            });
    connect(&_livekit_bridge,
            &LivekitBridge::permissionError,
            this,
            [this](const QString& deviceType, const QString& message)
            {
                _call_coordinator->onLivekitPermissionError(deviceType, message);
            });
    connect(&_livekit_bridge,
            &LivekitBridge::mediaError,
            this,
            [this](const QString& message)
            {
                _call_coordinator->onLivekitMediaError(message);
            });
    connect(&_livekit_bridge,
            &LivekitBridge::reconnecting,
            this,
            [this](const QString& message)
            {
                _call_coordinator->onLivekitReconnecting(message);
            });
    connect(&_livekit_bridge,
            &LivekitBridge::bridgeLog,
            this,
            [](const QString& message)
            {
                qInfo().noquote() << "[livekit]" << message;
            });
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_heartbeat_ack,
            this,
            [this](qint64 ackAtMs)
            {
                _chat_connection_coordinator->onHeartbeatAck(ackAtMs);
            });
    connect(&_apply_request_model,
            &ApplyRequestModel::unapprovedCountChanged,
            this,
            &AppController::pendingApplyChanged);

    connect(&_register_countdown_timer, &QTimer::timeout, this, &AppController::onRegisterCountdownTimeout);
    connect(&_register_code_cooldown_timer, &QTimer::timeout, this, &AppController::onRegisterCodeCooldownTimeout);
    connect(&_heartbeat_timer,
            &QTimer::timeout,
            this,
            [this]()
            {
                _chat_connection_coordinator->onHeartbeatTimeout();
            });
    _chat_login_timeout_timer.setSingleShot(true);
    _chat_login_timeout_timer.setInterval(15000);
    connect(&_chat_login_timeout_timer,
            &QTimer::timeout,
            this,
            [this]()
            {
                if (_chat_recovery_state.reconnecting && _page == ChatPage)
                {
                    _gateway.chatTransport()->CloseConnection();
                    return;
                }
                if (!_shell_state.busy || _page != LoginPage)
                {
                    return;
                }
                const bool fallbackAlreadyAttempted = _chat_recovery_state.loginTcpFallbackAttempted;
                _gateway.chatTransport()->CloseConnection();
                if (!fallbackAlreadyAttempted && _chat_recovery_state.loginTcpFallbackAttempted)
                {
                    return;
                }
                if (_chat_connection_coordinator->tryLoginFallbackToTcp(QStringLiteral("login_timeout")))
                {
                    return;
                }
                setBusy(false);
                setTip("聊天服务登录超时，请重试", true);
            });
}

AppController::Page AppController::page() const
{
    return _page;
}

AppController::~AppController() = default;

CallSessionModel* AppController::callSession()
{
    return &_call_session_model;
}

LivekitBridge* AppController::livekitBridge()
{
    return &_livekit_bridge;
}

void AppController::clearTip()
{
    setTip("", false);
}

void AppController::openExternalResource(const QString& url)
{
    QString errorText;
    if (!LocalFilePickerService::openUrl(url, &errorText))
    {
        if (errorText.isEmpty())
        {
            errorText = "打开资源失败";
        }
        setTip(errorText, true);
    }
}

void AppController::searchUser(const QString& uidText)
{
    _contact_coordinator_shell->searchUser(uidText);
}

void AppController::clearSearchState()
{
    clearSearchResultOnly();
    setSearchPending(false);
    setSearchStatus("", false);
}

void AppController::requestAddFriend(int uid, const QString& bakName, const QVariantList& labels)
{
    _contact_coordinator_shell->requestAddFriend(uid, bakName, labels);
}

void AppController::approveFriend(int uid, const QString& backName, const QVariantList& labels)
{
    _contact_coordinator_shell->approveFriend(uid, backName, labels);
}

void AppController::clearAuthStatus()
{
    setAuthStatus("", false);
}

void AppController::startVoiceChat()
{
    _call_coordinator->startVoiceChat();
}

void AppController::startVideoChat()
{
    _call_coordinator->startVideoChat();
}

void AppController::acceptIncomingCall()
{
    _call_coordinator->acceptIncomingCall();
}

void AppController::rejectIncomingCall()
{
    _call_coordinator->rejectIncomingCall();
}

void AppController::endCurrentCall()
{
    _call_coordinator->endCurrentCall();
}

void AppController::toggleCallMuted()
{
    _call_coordinator->toggleCallMuted();
}

void AppController::toggleCallCamera()
{
    _call_coordinator->toggleCallCamera();
}
