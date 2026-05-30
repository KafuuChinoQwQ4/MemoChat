#include "AppController.h"
#include "ConversationSyncService.h"
#include "MessageContentCodec.h"
#include "MessagePayloadService.h"
#include "usermgr.h"

#include <QJsonObject>

void AppController::handleGroupMessageMutationRsp(ReqId reqId, const QJsonObject& payload)
{
    const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
    const MessageUpdateFields updateFields =
        MessagePayloadService::parseMessageUpdateFields(payload,
                                                        reqId == ID_REVOKE_GROUP_MSG_RSP
                                                        ? QStringLiteral("group_msg_revoked")
                                                        : QStringLiteral("group_msg_edited"));
    if (groupId > 0 && !updateFields.msgId.isEmpty() && !updateFields.content.isEmpty())
    {
        _gateway.userMgr()->UpdateGroupChatMsgContent(groupId,
                                                      updateFields.msgId,
                                                      updateFields.content,
                                                      updateFields.state,
                                                      updateFields.editedAtMs,
                                                      updateFields.deletedAtMs);
        auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
        if (groupInfo)
        {
            auto selfInfo = _gateway.userMgr()->GetUserInfo();
            if (selfInfo && _group_cache_store.isReady())
            {
                _group_cache_store.upsertMessages(selfInfo->_uid, groupId, groupInfo->_chat_msgs);
            }
            if (groupId == _group_state.currentId)
            {
                _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
            }
        }
    }
    setGroupStatus(reqId == ID_EDIT_GROUP_MSG_RSP ? "群消息已编辑" : "群消息已撤回", false);
}

void AppController::handleGroupMessageAckRsp(ReqId reqId, const QJsonObject& payload)
{
    const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
    const QJsonObject msgObj = payload.value("msg").toObject();
    const QString ackMsgId = payload.value("client_msg_id").toString(msgObj.value("msgid").toString());
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (groupId > 0 && !ackMsgId.isEmpty())
    {
        const int selfUid = selfInfo ? selfInfo->_uid : _gateway.userMgr()->GetUid();
        const QString senderName =
            payload.value("from_nick")
                .toString(payload.value("from_name").toString(selfInfo ? selfInfo->_nick : QString()));
        const QString senderIcon =
            normalizeIconPath(payload.value("from_icon").toString(selfInfo ? selfInfo->_icon : QString()));
        auto correctedMsg =
            MessagePayloadService::buildGroupAckMessage(payload, msgObj, selfUid, senderName, senderIcon);
        if (!correctedMsg || correctedMsg->_msg_id.isEmpty() || correctedMsg->_msg_content.isEmpty())
        {
            if (payload.value("status").toString() == QStringLiteral("accepted"))
            {
                if (_gateway.userMgr()->UpdateGroupChatMsgState(groupId,
                                                                ackMsgId,
                                                                QStringLiteral("accepted")) &&
                                                                groupId == _group_state.currentId)
                {
                    _message_model.updateMessageState(ackMsgId, QStringLiteral("accepted"));
                }
                if (selfInfo && _group_cache_store.isReady())
                {
                    auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
                    if (groupInfo)
                    {
                        _group_cache_store.upsertMessages(selfInfo->_uid, groupId, groupInfo->_chat_msgs);
                    }
                }
            }
            return;
        }
        const QString msgId = correctedMsg->_msg_id;
        _gateway.userMgr()->UpsertGroupChatMsg(groupId, correctedMsg);
        _group_state.pendingMsgGroupMap.remove(msgId);
        if (selfInfo && _group_cache_store.isReady())
        {
            _group_cache_store.upsertMessages(selfInfo->_uid, groupId, {correctedMsg});
        }

        const int pseudoUid = ConversationSyncService::resolveGroupDialogUid(_group_state.dialogUidMap, groupId);
        if (pseudoUid != 0)
        {
            const QString preview = MessageContentCodec::toPreviewText(correctedMsg->_msg_content);
            ConversationSyncService::updateGroupPreview(_group_list_model,
                                                        _dialog_list_model,
                                                        pseudoUid,
                                                        preview,
                                                        preview,
                                                        correctedMsg->_created_at);
        }

        if (groupId == _group_state.currentId)
        {
            _message_model.upsertMessage(correctedMsg, _gateway.userMgr()->GetUid());
            sendGroupReadAck(groupId, correctedMsg->_created_at);
        }
    }
    if (reqId == ID_FORWARD_GROUP_MSG_RSP)
    {
        setGroupStatus("消息已转发", false);
    }
}
