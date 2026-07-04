#include "AppCoordinators.h"

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
        _port.setIgnoreNextLoginDisconnect(false);
        _port.setBusy(false);
        _port.setTip("网络请求错误", true);
        return;
    }

    QJsonObject obj;
    if (!_port.parseJson(res, obj))
    {
        _port.setIgnoreNextLoginDisconnect(false);
        _port.setBusy(false);
        _port.setTip("json解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS)
    {
        _port.setIgnoreNextLoginDisconnect(false);
        _port.setBusy(false);
        if (error == ErrorCodes::ERR_VERSION_TOO_LOW)
        {
            const QString minVersion = obj.value("min_version").toString(QStringLiteral("2.0.0"));
            _port.setTip(QString("客户端版本过低，请升级到 %1 或以上").arg(minVersion), true);
            return;
        }

        switch (error)
        {
            case 1009:
                _port.setTip("邮箱或密码错误", true);
                break;
            case 1001:
                _port.setTip("登录参数格式错误", true);
                break;
            case 1002:
                _port.setTip("登录服务暂不可用，请稍后重试", true);
                break;
            default:
                _port.setTip(QString("登录失败（错误码:%1）").arg(error), true);
                break;
        }
        return;
    }

    ServerInfo server_info;
    server_info.Uid = obj.value("uid").toInt();
    server_info.Token = obj.value("token").toString();
    server_info.LoginTicket = obj.value("login_ticket").toString();
    server_info.RefreshToken = obj.value("refresh_token").toString();
    server_info.ProtocolVersion = obj.value("protocol_version").toInt(3);
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
    if (server_info.Uid <= 0 || server_info.Token.trimmed().isEmpty() || server_info.LoginTicket.trimmed().isEmpty() ||
        server_info.RefreshToken.trimmed().isEmpty() || server_info.Endpoints.isEmpty())
    {
        _port.setIgnoreNextLoginDisconnect(false);
        _port.setBusy(false);
        _port.setTip("聊天服务配置无效，请确认服务已启动", true);
        qWarning() << "Invalid chat server response:"
                   << "uid" << server_info.Uid << "hasToken" << !server_info.Token.trimmed().isEmpty()
                   << "hasLoginTicket" << !server_info.LoginTicket.trimmed().isEmpty() << "hasRefreshToken"
                   << !server_info.RefreshToken.trimmed().isEmpty() << "endpointCount" << server_info.Endpoints.size();
        return;
    }

    _port.applyLoginSuccess(server_info, obj);
}
