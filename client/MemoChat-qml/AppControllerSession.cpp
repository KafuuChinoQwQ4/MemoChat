#include "AppController.h"
#include "IconPathUtils.h"
#include "IChatTransport.h"
#include "usermgr.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDebug>

namespace {
constexpr int kHeartbeatIntervalMs = 10000;
constexpr int kHeartbeatAckMissThreshold = 2;
constexpr qint64 kHeartbeatAckGraceMs = 22000;
constexpr int kChatReconnectMaxAttempts = 2;
constexpr int kChatReconnectDelayMs = 300;
constexpr int kDefaultChatConnectTimeoutMs = 1200;
constexpr int kDefaultBackupDialDelayMs = 250;
constexpr int kDefaultChatLoginTimeoutMs = 5000;

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
    _http_login_finished_ms = QDateTime::currentMSecsSinceEpoch();
    _chat_login_timeout_timer.setInterval(_chat_total_login_timeout_ms);
    _message_model.setDownloadAuthContext(_pending_uid, _pending_token);
    setIconDownloadAuthContext(_pending_uid, _pending_token);
    _chat_server_host = _chat_endpoints.front().host;
    _chat_server_port = _chat_endpoints.front().port;
    _chat_server_name = _chat_endpoints.front().serverName;
    resetReconnectState();
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

void AppController::onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS) {
        setBusy(false);
        setTip("网络请求错误", true);
        return;
    }

    QJsonObject obj;
    if (!parseJson(res, obj)) {
        setBusy(false);
        setTip("json解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        setBusy(false);
        setTip("参数错误", true);
        return;
    }

    if (id == ReqId::ID_GET_VARIFY_CODE) {
        setBusy(false);
        setTip("验证码已发送到邮箱，注意查收", false);
        return;
    }

    if (id == ReqId::ID_REG_USER) {
        setBusy(false);
        setTip("用户注册成功", false);
        if (!_register_success_page) {
            _register_success_page = true;
            emit registerSuccessPageChanged();
        }

        _register_countdown = 5;
        emit registerCountdownChanged();
        _register_countdown_timer.start(1000);
    }
}

void AppController::onResetHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS) {
        setBusy(false);
        setTip("网络请求错误", true);
        return;
    }

    QJsonObject obj;
    if (!parseJson(res, obj)) {
        setBusy(false);
        setTip("json解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        setBusy(false);
        setTip("参数错误", true);
        return;
    }

    setBusy(false);
    if (id == ReqId::ID_GET_VARIFY_CODE) {
        setTip("验证码已发送到邮箱，注意查收", false);
        return;
    }

    if (id == ReqId::ID_RESET_PWD) {
        setTip("重置成功, 点击返回登录", false);
    }
}

void AppController::onSettingsHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (id != ReqId::ID_UPDATE_PROFILE) {
        return;
    }

    if (err != ErrorCodes::SUCCESS) {
        setSettingsStatus("资料同步失败：网络错误", true);
        return;
    }

    QJsonObject obj;
    if (!parseJson(res, obj)) {
        setSettingsStatus("资料同步失败：响应解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        setSettingsStatus("资料同步失败", true);
        return;
    }

    const QString nick = obj.value("nick").toString(_current_user_nick);
    const QString desc = obj.value("desc").toString(_current_user_desc);
    const QString icon = normalizeIconPath(obj.value("icon").toString(_current_user_icon));

    _gateway.userMgr()->UpdateNickAndDesc(nick, desc);
    _gateway.userMgr()->UpdateIcon(icon);

    bool changed = false;
    if (_current_user_nick != nick) {
        _current_user_nick = nick;
        changed = true;
    }
    if (_current_user_desc != desc) {
        _current_user_desc = desc;
        changed = true;
    }
    if (_current_user_icon != icon) {
        _current_user_icon = icon;
        changed = true;
    }

    if (changed) {
        emit currentUserChanged();
    }
    setSettingsStatus("资料已同步", false);
}

void AppController::onTcpConnectFinished(bool success)
{
    _ignore_next_login_disconnect = false;
    if (!success) {
        _chat_login_timeout_timer.stop();
        qWarning() << "Chat TCP connect failed. reconnecting:" << _reconnecting_chat
                   << "page:" << _page
                   << "host:" << _chat_server_host
                   << "port:" << _chat_server_port;
        if (_chat_endpoint_index + 1 < _chat_endpoints.size()) {
            ++_chat_endpoint_index;
            const auto nextEndpoint = _chat_endpoints.at(_chat_endpoint_index);
            _chat_server_host = nextEndpoint.host;
            _chat_server_port = nextEndpoint.port;
            _chat_server_name = nextEndpoint.serverName;
            ServerInfo retryInfo;
            retryInfo.Uid = _pending_uid;
            retryInfo.Host = _chat_server_host;
            retryInfo.Port = _chat_server_port;
            retryInfo.Token = _pending_token;
            retryInfo.LoginTicket = _pending_login_ticket;
            retryInfo.ServerName = _chat_server_name;
            retryInfo.ProtocolVersion = _chat_protocol_version;
            retryInfo.Endpoints = _chat_endpoints;
            retryInfo.ConnectTimeoutMs = _chat_connect_timeout_ms;
            retryInfo.BackupDialDelayMs = _chat_backup_dial_delay_ms;
            retryInfo.TotalLoginTimeoutMs = _chat_total_login_timeout_ms;
            qWarning() << "Trying backup chat endpoint" << (_chat_endpoint_index + 1)
                       << "/" << _chat_endpoints.size()
                       << "host:" << _chat_server_host
                       << "port:" << _chat_server_port
                       << "server:" << _chat_server_name;
            _chat_connect_started_ms = QDateTime::currentMSecsSinceEpoch();
            _gateway.chatTransport()->connectToServer(retryInfo);
            return;
        }
        if (_reconnecting_chat && _page == ChatPage) {
            if (tryReconnectChat()) {
                return;
            }
            _heartbeat_timer.stop();
            switchToLogin();
            setTip("聊天重连失败，请重新登录", true);
            return;
        }
        setBusy(false);
        setTip("聊天服务不可用或连接失败，请确认服务已启动", true);
        return;
    }

    _chat_connect_finished_ms = QDateTime::currentMSecsSinceEpoch();
    qInfo() << "Chat TCP connected, sending chat login for uid:" << _pending_uid;
    setTip("聊天服务连接成功，正在登录...", false);
    QJsonObject obj;
    obj["protocol_version"] = _chat_protocol_version;
    if (_chat_protocol_version >= 3 && !_pending_login_ticket.isEmpty()) {
        obj["login_ticket"] = _pending_login_ticket;
    } else {
        obj["uid"] = _pending_uid;
        obj["token"] = _pending_token;
    }
    if (!_pending_trace_id.isEmpty()) {
        obj["trace_id"] = _pending_trace_id;
    }
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_CHAT_LOGIN, payload);
    _chat_login_timeout_timer.start();
}

void AppController::onChatLoginFailed(int err)
{
    _ignore_next_login_disconnect = false;
    _chat_login_timeout_timer.stop();
    qWarning() << "Chat login failed, err:" << err
               << "reconnecting:" << _reconnecting_chat
               << "page:" << _page;
    if (_reconnecting_chat && _page == ChatPage) {
        _heartbeat_timer.stop();
        switchToLogin();
        setTip("重连失败，会话已失效，请重新登录", true);
        return;
    }
    setBusy(false);
    if (err == ErrorCodes::ERR_PROTOCOL_VERSION) {
        setTip("客户端版本过旧，请升级到 v3 协议客户端", true);
        return;
    }
    if (err == ErrorCodes::ERR_CHAT_TICKET_INVALID
        || err == ErrorCodes::ERR_CHAT_TICKET_EXPIRED
        || err == ErrorCodes::ERR_CHAT_SERVER_MISMATCH) {
        setTip("聊天登录票据已失效，请重新登录", true);
        return;
    }
    setTip(QString("聊天服务登录失败（错误码:%1）").arg(err), true);
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
    setPage(ChatPage);

    auto user_info = _gateway.userMgr()->GetUserInfo();
    if (user_info) {
        setIconDownloadAuthContext(user_info->_uid, _pending_token);
        _message_model.setDownloadAuthContext(user_info->_uid, _pending_token);
        _private_cache_store.openForUser(user_info->_uid);
        _group_cache_store.openForUser(user_info->_uid);
        loadDraftStore(user_info->_uid);
        const QString name = user_info->_name;
        const QString nick = user_info->_nick;
        const QString icon = normalizeIconPath(user_info->_icon);
        const QString desc = user_info->_desc;
        const QString userId = user_info->_user_id;

        if (_current_user_name != name) {
            _current_user_name = name;
        }
        if (_current_user_nick != nick) {
            _current_user_nick = nick;
        }
        if (_current_user_icon != icon) {
            _current_user_icon = icon;
        }
        if (_current_user_desc != desc) {
            _current_user_desc = desc;
        }
        if (_current_user_id != userId) {
            _current_user_id = userId;
        }
        emit currentUserChanged();
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

    QTimer::singleShot(0, this, [this]() {
        if (_page != ChatPage) {
            return;
        }
        bootstrapDialogs();

        _heartbeat_timer.start(kHeartbeatIntervalMs);
        onHeartbeatTimeout();
    });

    QTimer::singleShot(120, this, [this]() {
        if (_page != ChatPage || _dialogs_ready) {
            return;
        }
        ensureChatListInitialized();
    });

    QTimer::singleShot(220, this, [this]() {
        if (_page != ChatPage) {
            return;
        }
        bootstrapApplies();
    });

    QTimer::singleShot(40, this, [this]() {
        if (_page != ChatPage) {
            return;
        }
        requestRelationBootstrap();
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

    _contact_list_model.clear();
    const auto contactList = _gateway.userMgr()->GetConListPerPage();
    _contact_list_model.setFriends(contactList);
    _gateway.userMgr()->UpdateContactLoadedCount();
    refreshContactLoadMoreState();
    setContactsReady(true);

    refreshApplyModel();
    setApplyReady(true);
    refreshDialogModel();
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

void AppController::onHeartbeatTimeout()
{
    if (_page != ChatPage) {
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (_last_heartbeat_sent_ms > 0 && _last_heartbeat_ack_ms < _last_heartbeat_sent_ms) {
        ++_heartbeat_ack_miss_count;
    } else {
        _heartbeat_ack_miss_count = 0;
    }

    if (_heartbeat_ack_miss_count >= kHeartbeatAckMissThreshold
        && (nowMs - _last_heartbeat_sent_ms) >= kHeartbeatAckGraceMs) {
        _closed_by_heartbeat_watchdog = true;
        _heartbeat_timer.stop();
        _gateway.chatTransport()->CloseConnection();
        return;
    }

    QJsonObject hb;
    hb["fromuid"] = selfInfo->_uid;
    const QByteArray payload = QJsonDocument(hb).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_HEART_BEAT_REQ, payload);
    _last_heartbeat_sent_ms = nowMs;
}

void AppController::onHeartbeatAck(qint64 ackAtMs)
{
    if (ackAtMs <= 0) {
        ackAtMs = QDateTime::currentMSecsSinceEpoch();
    }
    _last_heartbeat_ack_ms = ackAtMs;
    _heartbeat_ack_miss_count = 0;
}

void AppController::onNotifyOffline()
{
    _chat_login_timeout_timer.stop();
    if (_page != ChatPage) {
        return;
    }

    qWarning() << "Received offline notification for current session.";
    _heartbeat_timer.stop();
    resetReconnectState();
    resetHeartbeatTracking();
    _gateway.chatTransport()->CloseConnection();
    switchToLogin();
    setTip("同账号异地登录，该终端下线", true);
}

void AppController::onConnectionClosed()
{
    qWarning() << "Chat connection closed. page:" << _page
               << "busy:" << _busy
               << "reconnecting:" << _reconnecting_chat
               << "ignore_disconnect:" << _ignore_next_login_disconnect;
    if (_page != ChatPage) {
        if (_ignore_next_login_disconnect) {
            _chat_login_timeout_timer.stop();
            _ignore_next_login_disconnect = false;
            resetReconnectState();
            resetHeartbeatTracking();
            return;
        }

        if (_busy) {
            if (_chat_login_timeout_timer.isActive()) {
                _chat_login_timeout_timer.stop();
                _ignore_next_login_disconnect = false;
                setBusy(false);
                setTip("聊天连接已断开，登录已取消，请重试", true);
            }
            resetReconnectState();
            resetHeartbeatTracking();
            return;
        }

        _chat_login_timeout_timer.stop();
        resetReconnectState();
        resetHeartbeatTracking();
        return;
    }

    if (_call_session_model.visible()) {
        finalizeEndedCall(QStringLiteral("通话链路已断开"));
    }

    _chat_login_timeout_timer.stop();
    _heartbeat_timer.stop();
    if (tryReconnectChat()) {
        return;
    }
    const bool heartbeatTimeout = isHeartbeatLikelyTimeout();
    qWarning() << "Chat reconnect exhausted. heartbeat timeout:" << heartbeatTimeout;
    switchToLogin();
    setTip(heartbeatTimeout ? "心跳超时，请重新登录" : "聊天连接已断开，请重新登录", true);
}

bool AppController::tryReconnectChat()
{
    if (_page != ChatPage) {
        return false;
    }
    if (_chat_endpoints.isEmpty()) {
        return false;
    }
    if (_pending_uid <= 0 || (_chat_protocol_version >= 3 ? _pending_login_ticket.isEmpty() : _pending_token.isEmpty())) {
        return false;
    }
    if (_chat_reconnect_attempts >= kChatReconnectMaxAttempts) {
        qWarning() << "Reconnect attempts exhausted for uid:" << _pending_uid
                   << "host:" << _chat_server_host
                   << "port:" << _chat_server_port;
        return false;
    }

    _reconnecting_chat = true;
    ++_chat_reconnect_attempts;
    qInfo() << "Attempting chat reconnect" << _chat_reconnect_attempts
            << "/" << kChatReconnectMaxAttempts
            << "uid:" << _pending_uid
            << "host:" << _chat_server_host
            << "port:" << _chat_server_port;
    setTip(QString("聊天连接断开，正在重连（%1/%2）...")
               .arg(_chat_reconnect_attempts)
               .arg(kChatReconnectMaxAttempts), true);

    ServerInfo serverInfo;
    serverInfo.Uid = _pending_uid;
    serverInfo.Token = _pending_token;
    serverInfo.LoginTicket = _pending_login_ticket;
    serverInfo.ProtocolVersion = _chat_protocol_version;
    serverInfo.Endpoints = _chat_endpoints;
    serverInfo.ConnectTimeoutMs = _chat_connect_timeout_ms;
    serverInfo.BackupDialDelayMs = _chat_backup_dial_delay_ms;
    serverInfo.TotalLoginTimeoutMs = _chat_total_login_timeout_ms;
    _chat_endpoint_index = 0;
    _chat_server_host = _chat_endpoints.front().host;
    _chat_server_port = _chat_endpoints.front().port;
    _chat_server_name = _chat_endpoints.front().serverName;
    serverInfo.Host = _chat_server_host;
    serverInfo.Port = _chat_server_port;
    serverInfo.ServerName = _chat_server_name;
    QTimer::singleShot(kChatReconnectDelayMs, this, [this, serverInfo]() {
        if (!_reconnecting_chat || _page != ChatPage) {
            return;
        }
        _chat_connect_started_ms = QDateTime::currentMSecsSinceEpoch();
        _gateway.chatTransport()->connectToServer(serverInfo);
    });
    return true;
}

void AppController::resetReconnectState()
{
    _reconnecting_chat = false;
    _chat_reconnect_attempts = 0;
}

void AppController::resetHeartbeatTracking()
{
    _last_heartbeat_sent_ms = 0;
    _last_heartbeat_ack_ms = 0;
    _heartbeat_ack_miss_count = 0;
    _closed_by_heartbeat_watchdog = false;
}

bool AppController::isHeartbeatLikelyTimeout() const
{
    if (_closed_by_heartbeat_watchdog) {
        return true;
    }
    if (_last_heartbeat_sent_ms <= 0) {
        return false;
    }
    if (_last_heartbeat_ack_ms >= _last_heartbeat_sent_ms) {
        return false;
    }
    return (QDateTime::currentMSecsSinceEpoch() - _last_heartbeat_sent_ms) >= kHeartbeatAckGraceMs;
}
