#include "AppController.h"
#include "IChatTransport.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

void AppController::onTcpConnectFinished(bool success)
{
    _chat_recovery_state.ignoreNextLoginDisconnect = false;
    if (!success)
    {
        _chat_login_timeout_timer.stop();
        qWarning() << "Chat transport connect failed. reconnecting:" << _chat_recovery_state.reconnecting
                   << "page:" << _page << "host:" << _chat_endpoint_state.host << "port:" << _chat_endpoint_state.port;
        if (_chat_recovery_state.reconnecting && _page == ChatPage)
        {
            handleChatConnectFailureDuringRecovery();
            return;
        }
        setBusy(false);
        setTip("聊天服务不可用或连接失败，请确认服务已启动", true);
        return;
    }

    _chat_endpoint_state.connectFinishedMs = QDateTime::currentMSecsSinceEpoch();
    qInfo() << "Chat transport connected, sending chat login for uid:" << _pending_login_state.uid;
    setTip("聊天服务连接成功，正在登录...", false);
    QJsonObject obj;
    obj["protocol_version"] = _chat_endpoint_state.protocolVersion;
    if (_chat_endpoint_state.protocolVersion >= 3 && !_pending_login_state.loginTicket.isEmpty())
    {
        obj["login_ticket"] = _pending_login_state.loginTicket;
    }
    else
    {
        obj["uid"] = _pending_login_state.uid;
        obj["token"] = _pending_login_state.token;
    }
    if (!_pending_login_state.traceId.isEmpty())
    {
        obj["trace_id"] = _pending_login_state.traceId;
    }
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_CHAT_LOGIN, payload);
    _chat_login_timeout_timer.start();
}

void AppController::onChatLoginFailed(int err)
{
    _chat_recovery_state.ignoreNextLoginDisconnect = false;
    _chat_login_timeout_timer.stop();
    qWarning() << "Chat login failed, err:" << err << "reconnecting:" << _chat_recovery_state.reconnecting
               << "page:" << _page;
    if (_chat_recovery_state.reconnecting && _page == ChatPage)
    {
        _heartbeat_timer.stop();
        switchToLogin();
        setTip("重连失败，会话已失效，请重新登录", true);
        return;
    }
    setBusy(false);
    if (err == ErrorCodes::ERR_PROTOCOL_VERSION)
    {
        setTip("客户端版本过旧，请升级到 v3 协议客户端", true);
        return;
    }
    if (err == ErrorCodes::ERR_CHAT_TICKET_INVALID || err == ErrorCodes::ERR_CHAT_TICKET_EXPIRED ||
        err == ErrorCodes::ERR_CHAT_SERVER_MISMATCH)
    {
        setTip("聊天登录票据已失效，请重新登录", true);
        return;
    }
    setTip(QString("聊天服务登录失败（错误码:%1）").arg(err), true);
}
