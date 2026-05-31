#include "AppController.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

void AppController::sendGroupReadAck(qint64 groupId, qint64 readTs)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || groupId <= 0)
    {
        return;
    }
    if (readTs <= 0)
    {
        readTs = QDateTime::currentMSecsSinceEpoch();
    }
    QJsonObject payloadObj;
    payloadObj["fromuid"] = selfInfo->_uid;
    payloadObj["groupid"] = static_cast<qint64>(groupId);
    payloadObj["read_ts"] = static_cast<qint64>(readTs);
    const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GROUP_READ_ACK_REQ, payload);
}

void AppController::sendPrivateReadAck(int peerUid, qint64 readTs)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || peerUid <= 0)
    {
        return;
    }
    if (readTs <= 0)
    {
        readTs = QDateTime::currentMSecsSinceEpoch();
    }
    QJsonObject payloadObj;
    payloadObj["fromuid"] = selfInfo->_uid;
    payloadObj["peer_uid"] = peerUid;
    payloadObj["read_ts"] = static_cast<qint64>(readTs);
    const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_PRIVATE_READ_ACK_REQ, payload);
}
