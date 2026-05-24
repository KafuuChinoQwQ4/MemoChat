#include "AppController.h"
#include "DialogListService.h"
#include "IconPathUtils.h"
#include "IChatTransport.h"
#include "usermgr.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDebug>

namespace {
constexpr int kHeartbeatIntervalMs = 5000;
constexpr int kDefaultChatConnectTimeoutMs = 1200;
constexpr int kDefaultBackupDialDelayMs = 250;
constexpr int kDefaultChatLoginTimeoutMs = 5000;
constexpr int kPostLoginBootstrapDelayMs = 100;

ChatTransportKind parseTransportKind(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("quic")) {
        return ChatTransportKind::Quic;
    }
    return ChatTransportKind::Tcp;
}
}

void AppController::onLoginHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (id != ReqId::ID_LOGIN_USER) {
        return;
    }

    if (err != ErrorCodes::SUCCESS) {
        _ignore_next_login_disconnect = false;
        setBusy(false);
        setTip("网络请求错误", true);
        return;
    }

    QJsonObject obj;
    if (!parseJson(res, obj)) {
        _ignore_next_login_disconnect = false;
        setBusy(false);
        setTip("json解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        _ignore_next_login_disconnect = false;
        setBusy(false);
        if (error == ErrorCodes::ERR_VERSION_TOO_LOW) {
            const QString minVersion = obj.value("min_version").toString(QStringLiteral("2.0.0"));
            setTip(QString("客户端版本过低，请升级到 %1 或以上").arg(minVersion), true);
            return;
        }

        switch (error) {
        case 1009:
            setTip("邮箱或密码错误", true);
            break;
        case 1001:
            setTip("登录参数格式错误", true);
            break;
        case 1002:
            setTip("登录服务暂不可用，请稍后重试", true);
            break;
        default:
            setTip(QString("登录失败（错误码:%1）").arg(error), true);
            break;
        }
        return;
    }

    ServerInfo server_info;
    server_info.Uid = obj.value("uid").toInt();
    server_info.Host = obj.value("host").toString();
    server_info.Port = obj.value("port").toString();
    server_info.Token = obj.value("token").toString();
    server_info.LoginTicket = obj.value("login_ticket").toString();
    server_info.ProtocolVersion = obj.value("protocol_version").toInt(2);
    server_info.PreferredTransport = parseTransportKind(obj.value("preferred_transport").toString());
    server_info.FallbackTransport = parseTransportKind(obj.value("fallback_transport").toString());
    server_info.ConnectTimeoutMs = kDefaultChatConnectTimeoutMs;
    server_info.BackupDialDelayMs = kDefaultBackupDialDelayMs;
    server_info.TotalLoginTimeoutMs = kDefaultChatLoginTimeoutMs;
    const QJsonArray endpointArray = obj.value("chat_endpoints").toArray();
    for (const auto &endpointValue : endpointArray) {
        const QJsonObject endpointObj = endpointValue.toObject();
        ChatEndpoint endpoint;
        endpoint.transport = parseTransportKind(endpointObj.value("transport").toString());
        endpoint.host = endpointObj.value("host").toString();
        endpoint.port = endpointObj.value("port").toString();
        endpoint.serverName = endpointObj.value("server_name").toString();
        endpoint.priority = endpointObj.value("priority").toInt(server_info.Endpoints.size());
        if (!endpoint.host.trimmed().isEmpty() && !endpoint.port.trimmed().isEmpty()) {
            server_info.Endpoints.push_back(endpoint);
        }
    }
    if (server_info.Endpoints.isEmpty() && !server_info.Host.trimmed().isEmpty() && !server_info.Port.trimmed().isEmpty()) {
        ChatEndpoint fallback;
        fallback.transport = ChatTransportKind::Tcp;
        fallback.host = server_info.Host;
        fallback.port = server_info.Port;
        fallback.serverName = obj.value("server_name").toString();
        fallback.priority = 0;
        server_info.Endpoints.push_back(fallback);
    }
    if (server_info.Uid <= 0 || server_info.Endpoints.isEmpty()) {
        _ignore_next_login_disconnect = false;
        setBusy(false);
        setTip("聊天服务配置无效，请确认服务已启动", true);
        qWarning() << "Invalid chat server response:" << obj;
        return;
    }

    _pending_uid = server_info.Uid;
    _pending_token = server_info.Token;
    _pending_login_ticket = server_info.LoginTicket;
    _pending_trace_id = obj.value("trace_id").toString();
    _gateway.userMgr()->SetToken(_pending_token);
    _chat_protocol_version = server_info.ProtocolVersion;
    _chat_endpoints = server_info.Endpoints;
    _chat_endpoint_index = 0;
    _chat_connect_timeout_ms = server_info.ConnectTimeoutMs;
    _chat_backup_dial_delay_ms = server_info.BackupDialDelayMs;
    _chat_total_login_timeout_ms = server_info.TotalLoginTimeoutMs;
    _chat_login_tcp_fallback_attempted = false;
    _http_login_finished_ms = QDateTime::currentMSecsSinceEpoch();
    _chat_login_timeout_timer.setInterval(_chat_total_login_timeout_ms);
    _message_model.setDownloadAuthContext(_pending_uid, _pending_token);
    setIconDownloadAuthContext(_pending_uid, _pending_token);
    applyCurrentUserProfile(obj.value(QStringLiteral("user_profile")).toObject(), false);
    _chat_server_host = _chat_endpoints.front().host;
    _chat_server_port = _chat_endpoints.front().port;
    _chat_server_name = _chat_endpoints.front().serverName;
    resetReconnectState();
    setPage(ChatPage);
    qInfo() << "HTTP login succeeded, connecting chat server host:" << _chat_server_host
            << "port:" << _chat_server_port
            << "uid:" << _pending_uid
            << "http_login_ms:" << (_http_login_finished_ms > _login_started_ms ? (_http_login_finished_ms - _login_started_ms) : 0)
            << "endpoint_count:" << _chat_endpoints.size()
            << "protocol_version:" << _chat_protocol_version;
    setTip("正在连接聊天服务...", false);
    _chat_connect_started_ms = QDateTime::currentMSecsSinceEpoch();
    _gateway.chatTransport()->connectToServer(server_info);
}

void AppController::onSwitchToChat()
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    qInfo() << "Chat login succeeded, switching to chat page for uid:" << _pending_uid;
    qInfo() << "login.stage.summary"
            << "http_login_ms:" << (_http_login_finished_ms > _login_started_ms ? (_http_login_finished_ms - _login_started_ms) : 0)
            << "chat_connect_ms:" << (_chat_connect_finished_ms > _chat_connect_started_ms ? (_chat_connect_finished_ms - _chat_connect_started_ms) : 0)
            << "chat_auth_ms:" << (_chat_connect_finished_ms > 0 ? (nowMs - _chat_connect_finished_ms) : 0)
            << "login_total_ms:" << (_login_started_ms > 0 ? (nowMs - _login_started_ms) : 0)
            << "server:" << _chat_server_name;
    _ignore_next_login_disconnect = false;
    _chat_login_timeout_timer.stop();
    resetReconnectState();
    resetHeartbeatTracking();
    _last_heartbeat_ack_ms = QDateTime::currentMSecsSinceEpoch();
    setPage(ChatPage);
    setBusy(false);
    setTip("", false);
    setSearchPending(false);
    setChatLoadingMore(false);
    setPrivateHistoryLoading(false);
    setCanLoadMorePrivateHistory(false);
    _private_history_before_ts = 0;
    _private_history_before_msg_id.clear();
    _private_history_pending_before_ts = 0;
    _private_history_pending_before_msg_id.clear();
    _private_history_pending_peer_uid = 0;
    _group_history_before_seq = 0;
    _group_history_has_more = true;
    _group_history_loading = false;
    _dialog_bootstrap_loading = false;
    _chat_list_initialized = false;
    setDialogsReady(false);
    setContactsReady(false);
    setGroupsReady(false);
    setApplyReady(false);
    _pending_group_msg_group_map.clear();
    _dialog_mention_map.clear();
    _dialog_pending_attachment_map.clear();
    _pending_send_queue.clear();
    _pending_send_total_count = 0;
    _pending_send_dialog_uid = 0;
    _pending_send_chat_uid = 0;
    _pending_send_group_id = 0;
    setCurrentPendingAttachments(QVariantList());
    setPendingReplyContext(QString(), QString(), QString());
    setContactLoadingMore(false);
    setAuthStatus("", false);
    setSettingsStatus("", false);
    setContactPane(ApplyRequestPane);
    setCurrentContact(0, "", "", "qrc:/res/head_1.jpg", "", 0);
    _post_login_bootstrap_started = false;

    auto user_info = _gateway.userMgr()->GetUserInfo();
    if (user_info) {
        setIconDownloadAuthContext(user_info->_uid, _pending_token);
        _message_model.setDownloadAuthContext(user_info->_uid, _pending_token);
        applyCurrentUserProfile(user_info->_uid, user_info->_name, user_info->_nick, user_info->_icon,
                                user_info->_desc, user_info->_user_id, user_info->_sex, true);
    } else {
        _dialog_draft_map.clear();
        _dialog_pending_attachment_map.clear();
        _dialog_server_mute_map.clear();
        _dialog_mention_map.clear();
        setCurrentDraftText("");
        setCurrentPendingAttachments(QVariantList());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
    }

    beginPostLoginBootstrap();
}

void AppController::beginPostLoginBootstrap()
{
    if (_page != ChatPage || _post_login_bootstrap_started || !isChatTransportReady()) {
        return;
    }

    _post_login_bootstrap_started = true;
    runPostLoginBootstrap();
}

void AppController::runPostLoginBootstrap()
{
    auto user_info = _gateway.userMgr()->GetUserInfo();
    if (user_info) {
        setIconDownloadAuthContext(user_info->_uid, _pending_token);
        _message_model.setDownloadAuthContext(user_info->_uid, _pending_token);
        _private_cache_store.openForUser(user_info->_uid);
        _group_cache_store.openForUser(user_info->_uid);
        loadDraftStore(user_info->_uid);
    }

    QTimer::singleShot(kPostLoginBootstrapDelayMs, this, [this]() {
        if (_page != ChatPage) {
            return;
        }
        qInfo() << "[PERF] Stage-0: Bootstrap all data in parallel, ts:" << QDateTime::currentMSecsSinceEpoch();
        // Parallel bootstrap: fire all requests simultaneously for minimum latency
        bootstrapDialogs();
        bootstrapApplies();
        requestRelationBootstrap();
        _heartbeat_timer.start(kHeartbeatIntervalMs);
        onHeartbeatTimeout();
    });

    QTimer::singleShot(kPostLoginBootstrapDelayMs, this, [this]() {
        if (_page != ChatPage || _dialogs_ready) {
            return;
        }
        qInfo() << "[PERF] Stage-1: Ensure chat list initialized, ts:" << QDateTime::currentMSecsSinceEpoch();
        ensureChatListInitialized();
    });
}

void AppController::requestRelationBootstrap()
{
    if (_chat_protocol_version < 3 || !_gateway.chatTransport()->isConnected()) {
        return;
    }

    QJsonObject obj;
    obj["protocol_version"] = _chat_protocol_version;
    if (_pending_uid > 0) {
        obj["uid"] = _pending_uid;
    }
    if (!_pending_trace_id.isEmpty()) {
        obj["trace_id"] = _pending_trace_id;
    }
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GET_RELATION_BOOTSTRAP_REQ, payload);
}

void AppController::onRelationBootstrapUpdated()
{
    if (_page != ChatPage) {
        return;
    }

    _chat_list_initialized = false;
    ensureChatListInitialized();

    const auto friendSnapshot = _gateway.userMgr()->GetFriendListSnapshot();
    for (const auto &friendInfo : friendSnapshot) {
        if (friendInfo) {
            _chat_list_model.upsertFriend(friendInfo);
        }
    }

    // 增量更新联系人列表：避免全量 clear + set 操作
    // 只有在未初始化时才全量加载，已初始化时使用增量更新
    const auto contactList = _gateway.userMgr()->GetConListPerPage();
    if (!_contacts_ready) {
        _contact_list_model.clear();
        _contact_list_model.setFriends(contactList);
    } else {
        // 已初始化时使用 upsert 增量更新
        for (const auto &contact : contactList) {
            if (contact) {
                _contact_list_model.upsertFriend(contact);
            }
        }
    }
    _gateway.userMgr()->UpdateContactLoadedCount();
    refreshContactLoadMoreState();
    setContactsReady(true);

    refreshApplyModel();
    setApplyReady(true);

    // 增量更新会话列表
    refreshDialogModelIncremental();
    if (_current_chat_uid > 0 && _current_group_id <= 0) {
        const auto friendInfo = _gateway.userMgr()->GetFriendById(_current_chat_uid);
        if (friendInfo) {
            setCurrentChatPeerName(DialogListService::privateDisplayName(friendInfo));
            setCurrentChatPeerIcon(friendInfo->_icon.trimmed().isEmpty()
                                   ? QStringLiteral("qrc:/res/head_1.jpg")
                                   : friendInfo->_icon);
        }
    }
    emit pendingApplyChanged();
}

void AppController::onRegisterCountdownTimeout()
{
    if (_register_countdown > 0) {
        --_register_countdown;
        emit registerCountdownChanged();
    }

    if (_register_countdown <= 0) {
        _register_countdown_timer.stop();
        _register_success_page = false;
        emit registerSuccessPageChanged();
        switchToLogin();
    }
}
