#include "AppController.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace
{
constexpr int kHeartbeatAckMissThreshold = 2;
constexpr qint64 kHeartbeatAckGraceMs = 15000;
} // namespace

void AppController::onHeartbeatTimeout()
{
    if (_page != ChatPage)
    {
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (_chat_recovery_state.lastHeartbeatSentMs > 0 &&
        _chat_recovery_state.lastHeartbeatAckMs < _chat_recovery_state.lastHeartbeatSentMs)
    {
        ++_chat_recovery_state.heartbeatAckMissCount;
    }
    else
    {
        _chat_recovery_state.heartbeatAckMissCount = 0;
    }

    if (_chat_recovery_state.heartbeatAckMissCount >= kHeartbeatAckMissThreshold &&
        (nowMs - _chat_recovery_state.lastHeartbeatSentMs) >= kHeartbeatAckGraceMs)
    {
        _chat_recovery_state.closedByHeartbeatWatchdog = true;
        _heartbeat_timer.stop();
        _gateway.chatTransport()->CloseConnection();
        return;
    }

    QJsonObject hb;
    hb["fromuid"] = selfInfo->_uid;
    const QByteArray payload = QJsonDocument(hb).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_HEART_BEAT_REQ, payload);
    _chat_recovery_state.lastHeartbeatSentMs = nowMs;
}

void AppController::onHeartbeatAck(qint64 ackAtMs)
{
    if (ackAtMs <= 0)
    {
        ackAtMs = QDateTime::currentMSecsSinceEpoch();
    }
    _chat_recovery_state.lastHeartbeatAckMs = ackAtMs;
    _chat_recovery_state.heartbeatAckMissCount = 0;
}

void AppController::onNotifyOffline()
{
    _chat_login_timeout_timer.stop();
    if (_page != ChatPage)
    {
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
