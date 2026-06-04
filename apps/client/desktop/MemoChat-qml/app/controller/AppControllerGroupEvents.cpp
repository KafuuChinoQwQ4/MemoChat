#include "AppController.h"
#include "ConversationSyncService.h"
#include "DialogListService.h"
#include "IChatTransport.h"
#include "MessagePayloadService.h"
#include "MessageContentCodec.h"
#include "usermgr.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

void AppController::onGroupListUpdated()
{
    refreshGroupModel();
    refreshDialogModel();
    requestDialogList();
    if (_group_state.currentId > 0)
    {
        auto groupInfo = _gateway.userMgr()->GetGroupById(_group_state.currentId);
        if (groupInfo)
        {
            setCurrentGroup(groupInfo->_group_id, groupInfo->_name, groupInfo->_group_code);
            const QString currentGroupIcon =
                groupInfo->_icon.trimmed().isEmpty() ? QStringLiteral("qrc:/res/chat_icon.png") : groupInfo->_icon;
            setCurrentChatPeerIcon(currentGroupIcon);
            _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
            syncCurrentDialogDraft();
            return;
        }

        setCurrentGroup(0, "");
        if (_chat_state.uid > 0)
        {
            loadCurrentChatMessages();
        }
        else
        {
            _message_model.clear();
            setCurrentChatPeerName("");
            setCurrentChatPeerIcon("qrc:/res/head_1.png");
            setCurrentDraftText("");
            setCurrentDialogPinned(false);
            setCurrentDialogMuted(false);
        }
    }
}

void AppController::onGroupInvite(qint64 groupId, QString groupCode, QString groupName, int operatorUid)
{
    Q_UNUSED(operatorUid);
    setGroupStatus(QString("收到群邀请：%1").arg(groupName), false);
    QJsonObject req;
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || selfInfo->_uid <= 0)
    {
        return;
    }
    req["fromuid"] = selfInfo->_uid;
    const QByteArray payload = QJsonDocument(req).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GET_GROUP_LIST_REQ, payload);
    if (_group_state.currentId <= 0)
    {
        setCurrentGroup(groupId, groupName, groupCode);
    }
}

void AppController::onGroupApply(qint64 groupId, int applicantUid, QString applicantUserId, QString reason)
{
    Q_UNUSED(groupId);
    const QString displayId = applicantUserId.isEmpty() ? QString::number(applicantUid) : applicantUserId;
    setGroupStatus(QString("收到入群申请：%1 %2").arg(displayId).arg(reason), false);
}

void AppController::onGroupMemberChanged(QJsonObject payload)
{
    const QString event = payload.value("event").toString();
    if (!event.isEmpty() && event != "group_read_ack")
    {
        setGroupStatus(QString("群事件：%1").arg(event), false);
    }

    if (event == "group_read_ack")
    {
        auto selfInfo = _gateway.userMgr()->GetUserInfo();
        if (!selfInfo)
        {
            return;
        }
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        const int readerUid = payload.value("fromuid").toInt();
        if (groupId <= 0 || readerUid <= 0 || readerUid == selfInfo->_uid)
        {
            return;
        }
        qint64 readTs = payload.value("read_ts").toVariant().toLongLong();
        if (readTs <= 0)
        {
            readTs = QDateTime::currentMSecsSinceEpoch();
        }
        else if (readTs < 100000000000LL)
        {
            readTs *= 1000;
        }
        if (_gateway.userMgr()->MarkGroupOutgoingReadUntil(groupId, selfInfo->_uid, readTs) <= 0)
        {
            return;
        }
        auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
        if (groupInfo && _group_cache_store.isReady())
        {
            _group_cache_store.upsertMessages(selfInfo->_uid, groupId, groupInfo->_chat_msgs);
        }
        if (groupId == _group_state.currentId && groupInfo)
        {
            _message_model.setMessages(groupInfo->_chat_msgs, selfInfo->_uid);
        }
        return;
    }

    if (event == "group_dissolved")
    {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        if (groupId > 0)
        {
            _gateway.userMgr()->RemoveGroup(groupId);
            clearCurrentGroupConversation(groupId);
            refreshGroupModel();
            refreshDialogModel();
        }
        refreshGroupList();
        return;
    }

    if (event == "group_msg_edited" || event == "group_msg_revoked")
    {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        const MessageUpdateFields updateFields = MessagePayloadService::parseMessageUpdateFields(payload, event);
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
        return;
    }

    if (event == "group_icon_updated" && payload.value("groupid").toVariant().toLongLong() == _group_state.currentId)
    {
        const QString newGroupIcon =
            payload.value("icon").toString().trimmed().isEmpty() ? QStringLiteral("qrc:/res/chat_icon.png")
                                                                 : payload.value("icon").toString();
        setCurrentChatPeerIcon(newGroupIcon);
    }
    refreshGroupList();
}

void AppController::onGroupChatMsg(std::shared_ptr<GroupChatMsg> msg)
{
    if (bufferIncomingGroupMessage(msg))
    {
        return;
    }
    applyGroupChatMsg(msg);
}

void AppController::applyGroupChatMsg(std::shared_ptr<GroupChatMsg> msg)
{
    if (!msg || !msg->_msg)
    {
        return;
    }

    auto textMsg = MessagePayloadService::buildGroupIncomingMessage(*msg);
    if (!textMsg)
    {
        return;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(msg->_group_id);
    if (!groupInfo)
    {
        const int pseudoUid = ConversationSyncService::resolveGroupDialogUid(_group_state.dialogUidMap, msg->_group_id);
        const QString groupName = QString("群聊%1").arg(msg->_group_id);
        const QString groupIcon =
            msg->_from_icon.trimmed().isEmpty() ? QStringLiteral("qrc:/res/chat_icon.png") : msg->_from_icon;
        groupInfo = std::make_shared<GroupInfoData>();
        groupInfo->_group_id = msg->_group_id;
        groupInfo->_name = groupName;
        groupInfo->_icon = groupIcon;
        _gateway.userMgr()->UpsertGroup(groupInfo);
        if (pseudoUid != 0)
        {
            DialogEntrySeed seed;
            seed.dialogUid = pseudoUid;
            seed.dialogType = QStringLiteral("group");
            seed.name = groupName;
            seed.nick = groupName;
            seed.icon = groupIcon;
            seed.previewText = MessageContentCodec::toPreviewText(msg->_msg->_msg_content);
            seed.lastMsgTs = msg->_msg->_created_at;
            const DialogDecorationState decorationState{&_dialog_state.draftMap,
                                                        &_dialog_state.mentionMap,
                                                        &_dialog_state.serverPinnedMap,
                                                        &_dialog_state.serverMuteMap,
                                                        &_dialog_state.localPinnedSet};
            auto item = DialogListService::buildDialogEntry(seed, decorationState);
            _group_list_model.upsertFriend(item);
            _dialog_list_model.upsertFriend(item);
        }
    }
    _gateway.userMgr()->UpsertGroupChatMsg(msg->_group_id, textMsg);
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (selfInfo && _group_cache_store.isReady())
    {
        _group_cache_store.upsertMessages(selfInfo->_uid, msg->_group_id, {textMsg});
    }

    const int pseudoUid = ConversationSyncService::resolveGroupDialogUid(_group_state.dialogUidMap, msg->_group_id);
    if (pseudoUid != 0)
    {
        const QString preview = MessageContentCodec::toPreviewText(msg->_msg->_msg_content);
        const int selfUid = _gateway.userMgr()->GetUid();
        const bool fromSelf = (msg->_from_uid == selfUid);
        bool mentionedMe = false;
        if (!fromSelf && msg->_msg->_mention_all)
        {
            mentionedMe = true;
        }
        else if (!fromSelf && !msg->_msg->_mentions.isEmpty())
        {
            for (const auto& one : msg->_msg->_mentions)
            {
                if (one.toInt() == selfUid)
                {
                    mentionedMe = true;
                    break;
                }
            }
        }
        ConversationSyncService::updateGroupPreview(_group_list_model,
                                                    _dialog_list_model,
                                                    pseudoUid,
                                                    preview,
                                                    mentionedMe ? QString("[有人@你] %1").arg(preview) : preview,
                                                    msg->_msg->_created_at);
        const bool isViewingCurrentGroup = (_group_state.currentId == msg->_group_id && _chat_state.uid <= 0);
        if (isViewingCurrentGroup)
        {
            ConversationSyncService::clearGroupUnreadAndMention(_dialog_list_model,
                                                                _dialog_state.mentionMap,
                                                                pseudoUid);
        }
        else if (!fromSelf)
        {
            ConversationSyncService::incrementGroupUnreadAndMention(_dialog_list_model,
                                                                    _dialog_state.mentionMap,
                                                                    pseudoUid,
                                                                    mentionedMe);
        }
    }

    if (msg->_group_id != _group_state.currentId)
    {
        return;
    }
    _message_model.appendMessage(textMsg, _gateway.userMgr()->GetUid());
    sendGroupReadAck(msg->_group_id, textMsg->_created_at);
}
