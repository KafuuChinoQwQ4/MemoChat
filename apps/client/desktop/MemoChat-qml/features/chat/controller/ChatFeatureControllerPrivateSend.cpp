#include "ChatFeatureController.h"

#include "PrivateChatSendService.h"
#include "usermgr.h"

#include <QDebug>
#include <utility>

void ChatFeatureController::setPrivatePacketDispatcher(
    std::function<bool(const OutgoingChatPacket&, QString*)> dispatcher)
{
    _privatePacketDispatcher = std::move(dispatcher);
}

void ChatFeatureController::setPrivateAppendFriendMessages(
    std::function<void(int, const std::vector<std::shared_ptr<TextChatData>>&)> appendFriendMessages)
{
    _privateAppendFriendMessages = std::move(appendFriendMessages);
}

void ChatFeatureController::bindPrivateMessageStore(std::shared_ptr<UserMgr> userMgr)
{
    setPrivateAppendFriendMessages(
        [userMgr](int peerUid, const std::vector<std::shared_ptr<TextChatData>>& messages)
        {
            if (userMgr)
            {
                userMgr->AppendFriendChatMsg(peerUid, messages);
            }
        });
}

PrivateChatSendDependencies ChatFeatureController::privateSendDependencies()
{
    PrivateChatSendDependencies dependencies;
    dependencies.cacheStore = &_privateCacheStore;
    dependencies.messageModel = &_messageModel;
    return dependencies;
}

PrivateChatSendResult ChatFeatureController::sendPrivateText(const PrivateChatSendRequest& request,
                                                             PrivateChatSendDependencies dependencies)
{
    if (!dependencies.dispatchPacket)
    {
        dependencies.dispatchPacket = _privatePacketDispatcher;
    }
    if (!dependencies.appendFriendMessages)
    {
        dependencies.appendFriendMessages = _privateAppendFriendMessages;
    }
    if (!dependencies.historyBeforeTs)
    {
        dependencies.historyBeforeTs = &_privateHistoryState.beforeTs;
    }
    if (!dependencies.historyBeforeMsgId)
    {
        dependencies.historyBeforeMsgId = &_privateHistoryState.beforeMsgId;
    }
    if (!dependencies.chatListModel)
    {
        dependencies.chatListModel = &_chatListModel;
    }
    if (!dependencies.dialogListModel)
    {
        dependencies.dialogListModel = &_dialogListModel;
    }
    return PrivateChatSendService::send(request, dependencies);
}

PrivateChatSendResult ChatFeatureController::sendCurrentPrivateText(const QString& content, const QString& previewText)
{
    const ChatCurrentSendSnapshot snapshot =
        _currentSendPort.snapshot ? _currentSendPort.snapshot() : ChatCurrentSendSnapshot{};

    PrivateChatSendRequest request;
    request.peerUid = snapshot.currentPrivatePeerUid;
    request.content = content;
    request.previewText = previewText;

    PrivateChatSendDependencies dependencies =
        _currentSendPort.privateDependencies ? _currentSendPort.privateDependencies() : PrivateChatSendDependencies{};
    dependencies.selfUid = snapshot.selfUid;
    return sendPrivateText(request, dependencies);
}

bool ChatFeatureController::dispatchCurrentPrivateContent(const QString& content, const QString& previewText)
{
    if (_contentDispatchPort.isTransportReady && !_contentDispatchPort.isTransportReady())
    {
        if (_contentDispatchPort.setTip)
        {
            _contentDispatchPort.setTip(QStringLiteral("聊天连接未就绪，请重新登录"), true);
        }
        return false;
    }

    const PrivateChatSendResult result = sendCurrentPrivateText(content, previewText);
    if (!result.success)
    {
        if (_contentDispatchPort.setTip && !result.errorText.isEmpty())
        {
            _contentDispatchPort.setTip(result.errorText, true);
        }
        return false;
    }

    const int peerUid = _contentDispatchPort.currentPrivatePeerUid ? _contentDispatchPort.currentPrivatePeerUid() : 0;
    const auto friendInfo =
        _contentDispatchPort.privateFriendById ? _contentDispatchPort.privateFriendById(peerUid) : nullptr;
    qInfo() << "Private chat message dispatched, peer uid:" << peerUid << "msg id:" << result.packet.msgId
            << "friend msg count:" << (friendInfo ? static_cast<qlonglong>(friendInfo->_chat_msgs.size()) : 0LL)
            << "message model count:" << messageCount();
    return true;
}

MessageMutationCommandResult ChatFeatureController::runCurrentMessageMutation(MessageMutationCommand command,
                                                                              const QString& msgId,
                                                                              const QString& text) const
{
    const ChatMessageMutationSnapshot snapshot =
        _messageMutationPort.snapshot ? _messageMutationPort.snapshot() : ChatMessageMutationSnapshot{};

    MessageMutationCommandRequest request;
    request.selfUid = snapshot.selfUid;
    request.command = command;
    request.target = snapshot.target;
    request.msgId = msgId;
    request.text = text;

    MessageMutationCommandDependencies dependencies;
    dependencies.privateMessageExists = _messageMutationPort.privateMessageExists;
    dependencies.canRevokeMessage = _messageMutationPort.canRevokeMessage;
    dependencies.setPrivateStatus = _messageMutationPort.setPrivateStatus;
    dependencies.setGroupStatus = _messageMutationPort.setGroupStatus;
    dependencies.dispatchPayload = _messageMutationPort.dispatchPayload;
    return MessageMutationCommandService::run(request, dependencies);
}

MessageMutationCommandResult ChatFeatureController::editCurrentMessage(const QString& msgId, const QString& text) const
{
    return runCurrentMessageMutation(MessageMutationCommand::Edit, msgId, text);
}

MessageMutationCommandResult ChatFeatureController::revokeCurrentMessage(const QString& msgId) const
{
    return runCurrentMessageMutation(MessageMutationCommand::Revoke, msgId);
}

MessageMutationCommandResult ChatFeatureController::forwardCurrentMessage(const QString& msgId) const
{
    return runCurrentMessageMutation(MessageMutationCommand::Forward, msgId);
}
