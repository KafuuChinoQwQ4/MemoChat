#include "ChatController.h"
#include "ClientGateway.h"
#include "tcpmgr.h"
#include "global.h"
#include <QUuid>

ChatController::ChatController(ClientGateway *gateway)
    : _gateway(gateway)
{
}

bool ChatController::dispatchChatText(const OutgoingChatPacket &packet, QString *errorText) const
{
    const auto sendPayload = [this](const QByteArray &payload) {
        _gateway->tcpMgr()->slot_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, payload);
    };
    return ChatDispatchService::dispatchTextPayload(packet, sendPayload, errorText);
}

QString ChatController::buildCallJoinUrl(int selfUid, int peerUid, const QString &callType) const
{
    const int firstUid = qMin(selfUid, peerUid);
    const int secondUid = qMax(selfUid, peerUid);
    const QString roomSuffix = QUuid::createUuid().toString(QUuid::WithoutBraces).left(12);
    const QString roomName = QString("memochat-%1-%2-%3-%4")
                                 .arg(callType)
                                 .arg(firstUid)
                                 .arg(secondUid)
                                 .arg(roomSuffix);
    return QString("https://meet.jit.si/%1").arg(roomName);
}
