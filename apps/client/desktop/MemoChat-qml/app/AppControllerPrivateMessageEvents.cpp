#include "AppController.h"

#include "ConversationSyncService.h"
#include "MessageContentCodec.h"
#include "MessagePayloadService.h"
#include "usermgr.h"

#include <QJsonObject>

void AppController::onPrivateMsgChanged(QJsonObject payload)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return;
    }
    if (payload.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
    {
        return;
    }

    const int selfUid = selfInfo->_uid;
    const int fromUid = payload.value("fromuid").toInt();
    const int peerUidField = payload.value("peer_uid").toInt();
    const QString event = payload.value("event").toString();
    const MessageUpdateFields updateFields = MessagePayloadService::parseMessageUpdateFields(payload, event);
    const QString& msgId = updateFields.msgId;
    const QString& content = updateFields.content;
    if (msgId.isEmpty() || content.isEmpty())
    {
        return;
    }

    int peerUid = 0;
    if (fromUid == selfUid)
    {
        peerUid = peerUidField;
    }
    else if (fromUid > 0)
    {
        peerUid = fromUid;
    }
    else
    {
        peerUid = peerUidField;
    }
    if (peerUid <= 0)
    {
        return;
    }

    if (!_gateway.userMgr()->UpdatePrivateChatMsgContent(peerUid,
                                                         msgId,
                                                         content,
                                                         updateFields.state,
                                                         updateFields.editedAtMs,
                                                         updateFields.deletedAtMs))
    {
        return;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    if (!friendInfo)
    {
        return;
    }

    if (_private_cache_store.isReady())
    {
        _private_cache_store.upsertMessages(selfUid, peerUid, friendInfo->_chat_msgs);
    }
    if (!friendInfo->_chat_msgs.empty() && friendInfo->_chat_msgs.back())
    {
        const QString preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
        const qint64 lastTs = friendInfo->_chat_msgs.back()->_created_at;
        ConversationSyncService::updatePrivatePreview(_chat_list_model, _dialog_list_model, peerUid, preview, lastTs);
    }
    if (peerUid == _chat_state.uid)
    {
        _message_model.patchMessageContent(msgId,
                                           content,
                                           updateFields.state,
                                           updateFields.editedAtMs,
                                           updateFields.deletedAtMs);
    }
}
