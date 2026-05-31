#include "AppCoordinators.h"

#include "AppChatConnectionCoordinator.h"
#include "AppController.h"
#include "IconPathUtils.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

namespace
{
constexpr int kDefaultChatConnectTimeoutMs = 1200;
constexpr int kDefaultBackupDialDelayMs = 250;
constexpr int kDefaultChatLoginTimeoutMs = 5000;

ChatTransportKind parseTransportKind(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("quic"))
    {
        return ChatTransportKind::Quic;
    }
    return ChatTransportKind::Tcp;
}
} // namespace

void SessionAuthCoordinator::onLoginHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (id != ReqId::ID_LOGIN_USER)
    {
        return;
    }

    if (err != ErrorCodes::SUCCESS)
    {
        _app._chat_recovery_state.ignoreNextLoginDisconnect = false;
        _app.setBusy(false);
        _app.setTip("网络请求错误", true);
        return;
    }

    QJsonObject obj;
    if (!_app._auth_controller.parseJson(res, obj))
    {
        _app._chat_recovery_state.ignoreNextLoginDisconnect = false;
        _app.setBusy(false);
        _app.setTip("json解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS)
    {
        _app._chat_recovery_state.ignoreNextLoginDisconnect = false;
        _app.setBusy(false);
        if (error == ErrorCodes::ERR_VERSION_TOO_LOW)
        {
            const QString minVersion = obj.value("min_version").toString(QStringLiteral("2.0.0"));
            _app.setTip(QString("客户端版本过低，请升级到 %1 或以上").arg(minVersion), true);
            return;
        }

        switch (error)
        {
            case 1009:
                _app.setTip("邮箱或密码错误", true);
                break;
            case 1001:
                _app.setTip("登录参数格式错误", true);
                break;
            case 1002:
                _app.setTip("登录服务暂不可用，请稍后重试", true);
                break;
            default:
                _app.setTip(QString("登录失败（错误码:%1）").arg(error), true);
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
    for (const auto& endpointValue : endpointArray)
    {
        const QJsonObject endpointObj = endpointValue.toObject();
        ChatEndpoint endpoint;
        endpoint.transport = parseTransportKind(endpointObj.value("transport").toString());
        endpoint.host = endpointObj.value("host").toString();
        endpoint.port = endpointObj.value("port").toString();
        endpoint.serverName = endpointObj.value("server_name").toString();
        endpoint.priority = endpointObj.value("priority").toInt(server_info.Endpoints.size());
        if (!endpoint.host.trimmed().isEmpty() && !endpoint.port.trimmed().isEmpty())
        {
            server_info.Endpoints.push_back(endpoint);
        }
    }
    if (server_info.Endpoints.isEmpty() && !server_info.Host.trimmed().isEmpty() &&
        !server_info.Port.trimmed().isEmpty())
    {
        ChatEndpoint fallback;
        fallback.transport = ChatTransportKind::Tcp;
        fallback.host = server_info.Host;
        fallback.port = server_info.Port;
        fallback.serverName = obj.value("server_name").toString();
        fallback.priority = 0;
        server_info.Endpoints.push_back(fallback);
    }
    if (server_info.Uid <= 0 || server_info.Endpoints.isEmpty())
    {
        _app._chat_recovery_state.ignoreNextLoginDisconnect = false;
        _app.setBusy(false);
        _app.setTip("聊天服务配置无效，请确认服务已启动", true);
        qWarning() << "Invalid chat server response:" << obj;
        return;
    }

    _app._pending_login_state.uid = server_info.Uid;
    _app._pending_login_state.token = server_info.Token;
    _app._pending_login_state.loginTicket = server_info.LoginTicket;
    _app._pending_login_state.traceId = obj.value("trace_id").toString();
    _app._gateway.userMgr()->SetToken(_app._pending_login_state.token);
    _app._chat_endpoint_state.protocolVersion = server_info.ProtocolVersion;
    _app._chat_endpoint_state.endpoints = server_info.Endpoints;
    _app._chat_endpoint_state.endpointIndex = 0;
    _app._chat_endpoint_state.connectTimeoutMs = server_info.ConnectTimeoutMs;
    _app._chat_endpoint_state.backupDialDelayMs = server_info.BackupDialDelayMs;
    _app._chat_endpoint_state.totalLoginTimeoutMs = server_info.TotalLoginTimeoutMs;
    _app._chat_recovery_state.loginTcpFallbackAttempted = false;
    _app._chat_endpoint_state.httpLoginFinishedMs = QDateTime::currentMSecsSinceEpoch();
    _app._chat_login_timeout_timer.setInterval(_app._chat_endpoint_state.totalLoginTimeoutMs);
    _app._message_model.setDownloadAuthContext(_app._pending_login_state.uid, _app._pending_login_state.token);
    setIconDownloadAuthContext(_app._pending_login_state.uid, _app._pending_login_state.token);
    _app.applyCurrentUserProfile(obj.value(QStringLiteral("user_profile")).toObject(), false);
    _app._chat_endpoint_state.host = _app._chat_endpoint_state.endpoints.front().host;
    _app._chat_endpoint_state.port = _app._chat_endpoint_state.endpoints.front().port;
    _app._chat_endpoint_state.serverName = _app._chat_endpoint_state.endpoints.front().serverName;
    _app._chat_connection_coordinator->resetReconnectState();
    _app.setPage(AppController::ChatPage);
    qInfo() << "HTTP login succeeded, connecting chat server host:" << _app._chat_endpoint_state.host
            << "port:" << _app._chat_endpoint_state.port << "uid:" << _app._pending_login_state.uid << "http_login_ms:"
            << (_app._chat_endpoint_state.httpLoginFinishedMs > _app._chat_endpoint_state.loginStartedMs
                    ? (_app._chat_endpoint_state.httpLoginFinishedMs - _app._chat_endpoint_state.loginStartedMs)
                    : 0)
            << "endpoint_count:" << _app._chat_endpoint_state.endpoints.size()
            << "protocol_version:" << _app._chat_endpoint_state.protocolVersion;
    _app.setTip("正在连接聊天服务...", false);
    _app._chat_endpoint_state.connectStartedMs = QDateTime::currentMSecsSinceEpoch();
    _app._gateway.chatTransport()->connectToServer(server_info);
}
