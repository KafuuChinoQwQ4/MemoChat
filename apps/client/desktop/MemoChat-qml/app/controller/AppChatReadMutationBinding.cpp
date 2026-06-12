#include "AppController.h"

#include "IChatTransport.h"
#include "usermgr.h"

#include <utility>

void AppController::bindChatFeatureReadMutationPorts()
{
    ChatReadAckPort readAckPort;
    readAckPort.selfUid = [this]()
    {
        auto selfInfo = _gateway.userMgr()->GetUserInfo();
        return selfInfo ? selfInfo->_uid : 0;
    };
    readAckPort.dispatch.dispatchPayload = [this](int reqId, const QByteArray& payload)
    {
        if (const auto transport = _gateway.chatTransport())
        {
            transport->slot_send_data(static_cast<ReqId>(reqId), payload);
        }
    };

    ChatMessageMutationPort mutationPort;
    mutationPort.snapshot = [this]()
    {
        ChatMessageMutationSnapshot snapshot;
        if (const auto selfInfo = _gateway.userMgr() ? _gateway.userMgr()->GetUserInfo() : nullptr)
        {
            snapshot.selfUid = selfInfo->_uid;
        }
        const qint64 groupId = currentGroupId();
        if (groupId > 0)
        {
            snapshot.target.groupId = groupId;
        }
        else
        {
            snapshot.target.peerUid = _chat_state.uid;
        }
        return snapshot;
    };
    mutationPort.privateMessageExists = [this](const QString& messageId)
    {
        return _features.chatFeatureController.containsMessage(messageId);
    };
    mutationPort.canRevokeMessage = [this](const QString& messageId)
    {
        return _features.chatFeatureController.canRevokeMessage(messageId);
    };
    mutationPort.setPrivateStatus = [this](const QString& status, bool isError)
    {
        setTip(status, isError);
    };
    mutationPort.setGroupStatus = [this](const QString& status, bool isError)
    {
        setGroupStatus(status, isError);
    };
    mutationPort.dispatchPayload = [this](int reqId, const QByteArray& payload)
    {
        if (const auto transport = _gateway.chatTransport())
        {
            transport->slot_send_data(static_cast<ReqId>(reqId), payload);
        }
    };

    _features.chatFeatureController.setReadAckPort(std::move(readAckPort));
    _features.chatFeatureController.setMessageMutationPort(std::move(mutationPort));
}
