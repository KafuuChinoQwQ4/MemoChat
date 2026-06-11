#include "AppController.h"

#include "ChatEventDependenciesFactory.h"
#include "PrivateChatEventService.h"
#include "usermgr.h"

#include <QDebug>
#include <utility>

void AppController::clearPrivateHistoryFeatureState()
{
    _features.chatFeatureController.clearPrivateHistoryState();
}

void AppController::resetGroupConversationFeatureState()
{
    _features.chatFeatureController.resetGroupHistoryState();
}

PrivateChatEventContext AppController::privateChatEventContext(int selfUid) const
{
    PrivateChatEventContext context;
    context.selfUid = selfUid;
    context.currentPeerUid = _chat_state.uid;
    context.currentGroupId = currentGroupId();
    return context;
}

GroupConversationContext AppController::groupConversationContext(int selfUid) const
{
    GroupConversationContext context;
    context.selfUid = selfUid;
    context.currentGroupId = currentGroupId();
    context.currentPrivatePeerUid = _chat_state.uid;
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (selfInfo)
    {
        context.selfName = selfInfo->_nick.isEmpty() ? selfInfo->_name : selfInfo->_nick;
        context.selfIcon = selfInfo->_icon;
    }
    return context;
}

void AppController::applyTextChatMsg(std::shared_ptr<TextChatMsg> msg)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return;
    }

    const bool fromSelf = msg && msg->_from_uid == selfInfo->_uid;
    const int peerUid = msg ? (fromSelf ? msg->_to_uid : msg->_from_uid) : 0;
    qInfo() << "Private chat message received, peer uid:" << peerUid << "from self:" << fromSelf
            << "current chat uid:" << _chat_state.uid << "current group id:" << currentGroupId()
            << "batch size:" << (msg ? static_cast<int>(msg->_chat_msgs.size()) : 0);

    PrivateIncomingMessageRequest request;
    request.context = privateChatEventContext(selfInfo->_uid);
    request.message = std::move(msg);
    const PrivateIncomingMessageResult result = _features.chatFeatureController.handlePrivateIncomingMessage(
        request,
        memochat::app::makePrivateChatEventDependencies(
            _features.chatFeatureController,
            _gateway.userMgr(),
            [this](std::shared_ptr<AuthInfo> authInfo)
            {
                _features.contactController.upsertContact(std::move(authInfo));
            },
            [this](int peerUid, qint64 readTs)
            {
                _features.chatFeatureController.sendPrivateReadAckForPeer(peerUid, readTs);
            }));

    if (!result.success)
    {
        return;
    }
    if (!result.currentDialog)
    {
        qInfo() << "Private chat message stored for background dialog, peer uid:" << result.peerUid;
        return;
    }
    auto friendInfo = _gateway.userMgr()->GetFriendById(result.peerUid);
    qInfo() << "Private chat view refreshed from live message, peer uid:" << result.peerUid
            << "friend msg count:" << (friendInfo ? static_cast<qlonglong>(friendInfo->_chat_msgs.size()) : 0LL)
            << "message model count:" << _features.chatFeatureController.messageCount();
}
