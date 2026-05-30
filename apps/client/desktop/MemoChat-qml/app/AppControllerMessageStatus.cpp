#include "AppController.h"

#include "MessagePayloadService.h"
#include "usermgr.h"

#include <QJsonObject>

void AppController::onMessageStatus(QJsonObject payload)
{
    const int error = payload.value("error").toInt(ErrorCodes::ERR_JSON);
    const QString status = payload.value("status").toString();
    const QString scope = payload.value("scope").toString();
    const QString clientMsgId = payload.value("client_msg_id").toString();

    if (clientMsgId.isEmpty())
    {
        return;
    }

    const QString nextState = [error, status]() -> QString
    {
        if (error != ErrorCodes::SUCCESS)
        {
            return QStringLiteral("failed");
        }
        if (status == QStringLiteral("accepted"))
        {
            return QStringLiteral("accepted");
        }
        if (status == QStringLiteral("queued_retry"))
        {
            return QStringLiteral("queued_retry");
        }
        if (status == QStringLiteral("offline_pending"))
        {
            return QStringLiteral("offline_pending");
        }
        if (status == QStringLiteral("persisted") || status == QStringLiteral("delivered"))
        {
            return QStringLiteral("sent");
        }
        return status.isEmpty() ? QStringLiteral("sent") : status;
    }();

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const int selfUid = selfInfo ? selfInfo->_uid : _gateway.userMgr()->GetUid();

    if (scope == QStringLiteral("group") || payload.value("groupid").toVariant().toLongLong() > 0)
    {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        if (groupId <= 0)
        {
            return;
        }

        const QJsonObject msgObj = payload.value("msg").toObject();
        if (error == ErrorCodes::SUCCESS && status == QStringLiteral("persisted") && !msgObj.isEmpty())
        {
            const QString senderName =
                payload.value("from_nick")
                    .toString(payload.value("from_name").toString(selfInfo ? selfInfo->_nick : QString()));
            const QString senderIcon =
                normalizeIconPath(payload.value("from_icon").toString(selfInfo ? selfInfo->_icon : QString()));
            auto correctedMsg =
                MessagePayloadService::buildGroupAckMessage(payload, msgObj, selfUid, senderName, senderIcon);
            if (correctedMsg)
            {
                _gateway.userMgr()->UpsertGroupChatMsg(groupId, correctedMsg);
                if (selfInfo && _group_cache_store.isReady())
                {
                    _group_cache_store.upsertMessages(selfInfo->_uid, groupId, {correctedMsg});
                }
                if (groupId == _group_state.currentId)
                {
                    _message_model.upsertMessage(correctedMsg, selfUid);
                }
                _group_state.pendingMsgGroupMap.remove(clientMsgId);
                return;
            }
        }

        if (_gateway.userMgr()->UpdateGroupChatMsgState(groupId, clientMsgId, nextState) &&
            groupId == _group_state.currentId)
        {
            _message_model.updateMessageState(clientMsgId, nextState);
        }
        if (selfInfo)
        {
            auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
            if (groupInfo && _group_cache_store.isReady())
            {
                _group_cache_store.upsertMessages(selfInfo->_uid, groupId, groupInfo->_chat_msgs);
            }
        }
        if (nextState == QStringLiteral("failed"))
        {
            _group_state.pendingMsgGroupMap.remove(clientMsgId);
        }
        return;
    }

    const int peerUid = payload.value("peer_uid").toInt(payload.value("touid").toInt());
    if (peerUid <= 0)
    {
        return;
    }
    if (_gateway.userMgr()->UpdatePrivateChatMsgState(peerUid, clientMsgId, nextState) && peerUid == _chat_state.uid)
    {
        _message_model.updateMessageState(clientMsgId, nextState);
    }
    if (selfInfo)
    {
        auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
        if (friendInfo)
        {
            _private_cache_store.upsertMessages(selfInfo->_uid, peerUid, friendInfo->_chat_msgs);
        }
    }
}
