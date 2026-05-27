#include "AppController.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDebug>

namespace {
constexpr int kHeartbeatAckMissThreshold = 2;
constexpr qint64 kHeartbeatAckGraceMs = 15000;
constexpr int kChatReconnectMaxAttempts = 2;
constexpr int kChatReconnectDelayMs = 300;

bool hasEndpointKind(const QVector<ChatEndpoint> &endpoints, ChatTransportKind kind)
{
    for (const auto &endpoint : endpoints) {
        if (endpoint.transport == kind) {
            return true;
        }
    }
    return false;
}
}

void AppController::onTcpConnectFinished(bool success)
{
    _ignore_next_login_disconnect = false;
    if (!success) {
        _chat_login_timeout_timer.stop();
        qWarning() << "Chat transport connect failed. reconnecting:" << _reconnecting_chat
                   << "page:" << _page
                   << "host:" << _chat_server_host
                   << "port:" << _chat_server_port;
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
    qInfo() << "Chat transport connected, sending chat login for uid:" << _pending_uid;
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
    if (_ignore_next_login_disconnect) {
        _chat_login_timeout_timer.stop();
        _ignore_next_login_disconnect = false;
        resetReconnectState();
        resetHeartbeatTracking();
        return;
    }

    if (_page != ChatPage) {
        if (_busy) {
            if (_chat_login_timeout_timer.isActive()) {
                _chat_login_timeout_timer.stop();
                _ignore_next_login_disconnect = false;
                if (tryLoginFallbackToTcp(QStringLiteral("connection_closed"))) {
                    return;
                }
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

bool AppController::tryLoginFallbackToTcp(const QString &reason)
{
    if (_chat_login_tcp_fallback_attempted) {
        return false;
    }
    if (_page != LoginPage || !_busy) {
        return false;
    }
    if (!hasEndpointKind(_chat_endpoints, ChatTransportKind::Tcp)) {
        return false;
    }
    if (_pending_uid <= 0 || (_chat_protocol_version >= 3 ? _pending_login_ticket.isEmpty() : _pending_token.isEmpty())) {
        return false;
    }

    ServerInfo serverInfo;
    serverInfo.Uid = _pending_uid;
    serverInfo.Token = _pending_token;
    serverInfo.LoginTicket = _pending_login_ticket;
    serverInfo.ProtocolVersion = _chat_protocol_version;
    serverInfo.Endpoints = _chat_endpoints;
    serverInfo.PreferredTransport = ChatTransportKind::Tcp;
    serverInfo.FallbackTransport = ChatTransportKind::Tcp;
    serverInfo.ConnectTimeoutMs = _chat_connect_timeout_ms;
    serverInfo.BackupDialDelayMs = _chat_backup_dial_delay_ms;
    serverInfo.TotalLoginTimeoutMs = _chat_total_login_timeout_ms;
    for (const auto &endpoint : _chat_endpoints) {
        if (endpoint.transport == ChatTransportKind::Tcp) {
            _chat_server_host = endpoint.host;
            _chat_server_port = endpoint.port;
            _chat_server_name = endpoint.serverName;
            serverInfo.Host = endpoint.host;
            serverInfo.Port = endpoint.port;
            serverInfo.ServerName = endpoint.serverName;
            break;
        }
    }
    if (serverInfo.Host.trimmed().isEmpty() || serverInfo.Port.trimmed().isEmpty()) {
        return false;
    }

    qWarning() << "Chat login over current transport failed; falling back to tcp."
               << "reason:" << reason
               << "host:" << serverInfo.Host
               << "port:" << serverInfo.Port;
    _chat_login_tcp_fallback_attempted = true;
    setTip("QUIC 登录未完成，正在回退到 TCP 长连接...", false);
    _chat_connect_started_ms = QDateTime::currentMSecsSinceEpoch();
    _gateway.chatTransport()->connectToServer(serverInfo);
    return true;
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
    serverInfo.PreferredTransport = hasEndpointKind(_chat_endpoints, ChatTransportKind::Quic)
        ? ChatTransportKind::Quic
        : ChatTransportKind::Tcp;
    serverInfo.FallbackTransport = ChatTransportKind::Tcp;
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
