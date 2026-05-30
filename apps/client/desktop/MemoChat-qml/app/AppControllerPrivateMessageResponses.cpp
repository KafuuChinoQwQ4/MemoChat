#include "AppController.h"
#include "ConversationSyncService.h"
#include "MessageContentCodec.h"
#include "MessagePayloadService.h"
#include "usermgr.h"

#include <QJsonObject>

void AppController::handlePrivateMessageMutationRsp(ReqId reqId, const QJsonObject& payload)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return;
    }
    const int peerUid = payload.value("peer_uid").toInt();
    const MessageUpdateFields updateFields =
        MessagePayloadService::parseMessageUpdateFields(payload,
                                                        reqId == ID_REVOKE_PRIVATE_MSG_RSP
                                                        ? QStringLiteral("private_msg_revoked")
                                                        : QStringLiteral("private_msg_edited"));
    if (peerUid > 0 && !updateFields.msgId.isEmpty() && !updateFields.content.isEmpty())
    {
        _gateway.userMgr()->UpdatePrivateChatMsgContent(peerUid,
                                                        updateFields.msgId,
                                                        updateFields.content,
                                                        updateFields.state,
                                                        updateFields.editedAtMs,
                                                        updateFields.deletedAtMs);
        auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
        if (friendInfo && _private_cache_store.isReady())
        {
            _private_cache_store.upsertMessages(selfInfo->_uid, peerUid, friendInfo->_chat_msgs);
            if (!friendInfo->_chat_msgs.empty() && friendInfo->_chat_msgs.back())
            {
                const QString preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
                const qint64 lastTs = friendInfo->_chat_msgs.back()->_created_at;
                ConversationSyncService::updatePrivatePreview(_chat_list_model,
                                                              _dialog_list_model,
                                                              peerUid,
                                                              preview,
                                                              lastTs);
            }
        }
        if (peerUid == _chat_state.uid)
        {
            _message_model.patchMessageContent(updateFields.msgId,
                                               updateFields.content,
                                               updateFields.state,
                                               updateFields.editedAtMs,
                                               updateFields.deletedAtMs);
        }
    }
    setTip(reqId == ID_EDIT_PRIVATE_MSG_RSP ? "私聊消息已编辑" : "私聊消息已撤回", false);
}

void AppController::handlePrivateForwardRsp(const QJsonObject& payload)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return;
    }
    const int peerUid = payload.value("peer_uid").toInt();
    const QJsonObject msgObj = payload.value("msg").toObject();
    auto forwardedMsg = MessagePayloadService::buildPrivateForwardedMessage(payload, msgObj, selfInfo->_uid);
    if (peerUid > 0 && forwardedMsg && !forwardedMsg->_msg_id.isEmpty())
    {
        _gateway.userMgr()->AppendFriendChatMsg(peerUid, {forwardedMsg});
        if (_private_cache_store.isReady())
        {
            _private_cache_store.upsertMessages(selfInfo->_uid, peerUid, {forwardedMsg});
        }
        auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
        if (friendInfo && !friendInfo->_chat_msgs.empty() && friendInfo->_chat_msgs.back())
        {
            const QString preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
            const qint64 lastTs = friendInfo->_chat_msgs.back()->_created_at;
            ConversationSyncService::updatePrivatePreview(_chat_list_model,
                                                          _dialog_list_model,
                                                          peerUid,
                                                          preview,
                                                          lastTs);
        }
        if (_group_state.currentId <= 0 && peerUid == _chat_state.uid)
        {
            _message_model.upsertMessage(forwardedMsg, selfInfo->_uid);
        }
    }
    setTip("消息已转发", false);
}
