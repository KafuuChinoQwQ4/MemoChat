#include "AppController.h"

#include "AppCoordinators.h"
#include "ChatEventDependenciesFactory.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <utility>

void AppController::bindChatFeatureSendPorts()
{
    ChatCurrentSendPort currentSendPort;
    currentSendPort.snapshot = [this]()
    {
        ChatCurrentSendSnapshot snapshot;
        auto selfInfo = _gateway.userMgr()->GetUserInfo();
        snapshot.selfUid = selfInfo ? selfInfo->_uid : _gateway.userMgr()->GetUid();
        snapshot.currentPrivatePeerUid = _chat_state.uid;
        snapshot.currentGroupId = currentGroupId();
        snapshot.context = groupConversationContext(snapshot.selfUid);
        return snapshot;
    };
    currentSendPort.privateDependencies = [this]()
    {
        return _features.chatFeatureController.privateSendDependencies();
    };
    currentSendPort.groupDependencies = [this]()
    {
        return memochat::app::makeGroupConversationDependencies(
            _features.chatFeatureController,
            _gateway.userMgr(),
            _features.groupController.groupListModel(),
            [this](int reqId, const QByteArray& payload)
            {
                if (const auto transport = _gateway.chatTransport())
                {
                    transport->slot_send_data(static_cast<ReqId>(reqId), payload);
                }
            });
    };

    ChatUploadedAttachmentDispatchPort attachmentDispatchPort;
    attachmentDispatchPort.dispatchCurrentPrivateContent = [this](const QString& content, const QString& preview)
    {
        return _features.chatFeatureController.dispatchCurrentPrivateContent(content, preview);
    };
    attachmentDispatchPort.dispatchCurrentGroupContent = [this](const QString& content, const QString& preview)
    {
        return _features.chatFeatureController.dispatchCurrentGroupContent(content, preview);
    };
    attachmentDispatchPort.dispatchPrivatePacket = [this](const OutgoingChatPacket& packet, QString* errorText)
    {
        return _features.chatController.dispatchChatText(packet, errorText);
    };
    attachmentDispatchPort.dispatchGroupPayload = [this](int reqId, const QByteArray& payload)
    {
        if (const auto transport = _gateway.chatTransport())
        {
            transport->slot_send_data(static_cast<ReqId>(reqId), payload);
        }
    };

    ChatContentDispatchPort contentDispatchPort;
    contentDispatchPort.isTransportReady = [this]()
    {
        return isChatTransportReady();
    };
    contentDispatchPort.setTip = [this](const QString& tip, bool isError)
    {
        setTip(tip, isError);
    };
    contentDispatchPort.currentPrivatePeerUid = [this]()
    {
        return _chat_state.uid;
    };
    contentDispatchPort.currentGroupId = [this]()
    {
        return currentGroupId();
    };
    contentDispatchPort.privateFriendById = [this](int peerUid)
    {
        return _gateway.userMgr()->GetFriendById(peerUid);
    };
    contentDispatchPort.groupById = [this](qint64 groupId)
    {
        return _gateway.userMgr()->GetGroupById(groupId);
    };

    _features.chatFeatureController.setPrivatePacketDispatcher(
        [this](const OutgoingChatPacket& packet, QString* errorText)
        {
            return _features.chatController.dispatchChatText(packet, errorText);
        });
    _features.chatFeatureController.bindPrivateMessageStore(_gateway.userMgr());
    _features.chatFeatureController.setCurrentSendPort(std::move(currentSendPort));
    _features.chatFeatureController.setContentDispatchPort(std::move(contentDispatchPort));
    _features.chatFeatureController.setUploadedAttachmentDispatchPort(std::move(attachmentDispatchPort));
}
