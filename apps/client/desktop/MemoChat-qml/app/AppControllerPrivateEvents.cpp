#include "AppController.h"

#include "ConversationSyncService.h"
#include "DialogListService.h"
#include "MessageContentCodec.h"
#include "usermgr.h"

#include <QDateTime>
#include <QDebug>

namespace
{
std::shared_ptr<AuthInfo> buildDialogPlaceholder(const FriendListModel& dialogListModel, int uid)
{
    const int dialogIndex = dialogListModel.indexOfUid(uid);
    const QVariantMap dialogItem = dialogIndex >= 0 ? dialogListModel.get(dialogIndex) : QVariantMap();
    return DialogListService::buildPlaceholderAuthInfo(uid, dialogItem, QStringLiteral("qrc:/res/head_1.png"));
}
} // namespace

void AppController::onTextChatMsg(std::shared_ptr<TextChatMsg> msg)
{
    if (!msg || msg->_chat_msgs.empty())
    {
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return;
    }

    const int selfUid = selfInfo->_uid;
    const bool fromSelf = (msg->_from_uid == selfUid);
    const int peerUid = (msg->_from_uid == selfUid) ? msg->_to_uid : msg->_from_uid;
    qInfo() << "Private chat message received, peer uid:" << peerUid << "from self:" << fromSelf
            << "current chat uid:" << _chat_state.uid << "current group id:" << _group_state.currentId
            << "batch size:" << static_cast<int>(msg->_chat_msgs.size());
    auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    if (!friendInfo && peerUid > 0)
    {
        auto placeholder = buildDialogPlaceholder(_dialog_list_model, peerUid);
        _gateway.userMgr()->AddFriend(placeholder);
        _chat_list_model.upsertFriend(placeholder);
        _contact_list_model.upsertFriend(placeholder);
        _dialog_list_model.upsertFriend(placeholder);
        friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    }
    qint64 latestPeerTs = 0;
    for (const auto& one : msg->_chat_msgs)
    {
        if (one && (one->_deleted_at_ms > 0 || one->_msg_content == QStringLiteral("[消息已撤回]")))
        {
            one->_msg_state = QStringLiteral("deleted");
        }
        else if (one && one->_edited_at_ms > 0)
        {
            one->_msg_state = QStringLiteral("edited");
        }
        if (one && one->_from_uid == peerUid)
        {
            latestPeerTs = qMax(latestPeerTs, one->_created_at);
        }
    }
    _gateway.userMgr()->AppendFriendChatMsg(peerUid, msg->_chat_msgs);
    _private_cache_store.upsertMessages(selfUid, peerUid, msg->_chat_msgs);
    friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    QString preview;
    if (friendInfo && !friendInfo->_chat_msgs.empty())
    {
        preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
    }
    else
    {
        preview = MessageContentCodec::toPreviewText(msg->_chat_msgs.back()->_msg_content);
    }
    qint64 lastTs = QDateTime::currentMSecsSinceEpoch();
    if (!msg->_chat_msgs.empty() && msg->_chat_msgs.back())
    {
        lastTs = msg->_chat_msgs.back()->_created_at;
    }
    ConversationSyncService::updatePrivatePreview(_chat_list_model, _dialog_list_model, peerUid, preview, lastTs);
    const bool isViewingCurrentPrivate = (_group_state.currentId <= 0 && peerUid == _chat_state.uid);
    if (isViewingCurrentPrivate)
    {
        ConversationSyncService::clearPrivateUnread(_chat_list_model, _dialog_list_model, peerUid);
        if (latestPeerTs > 0)
        {
            sendPrivateReadAck(peerUid, latestPeerTs);
        }
    }
    else if (!fromSelf)
    {
        ConversationSyncService::incrementPrivateUnread(_dialog_list_model, peerUid);
    }

    if (peerUid != _chat_state.uid)
    {
        qInfo() << "Private chat message stored for background dialog, peer uid:" << peerUid;
        return;
    }

    for (const auto& chat : msg->_chat_msgs)
    {
        _message_model.appendMessage(chat, selfUid);
    }
    ConversationSyncService::syncHistoryCursor(_message_model,
                                               _private_history_state.beforeTs,
                                               _private_history_state.beforeMsgId);
    qInfo() << "Private chat view refreshed from live message, peer uid:" << peerUid
            << "friend msg count:" << (friendInfo ? static_cast<qlonglong>(friendInfo->_chat_msgs.size()) : 0LL)
            << "message model count:" << _message_model.rowCount();
}
