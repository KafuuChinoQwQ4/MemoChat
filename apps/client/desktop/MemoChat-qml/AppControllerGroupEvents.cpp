#include "AppController.h"
#include "ConversationSyncService.h"
#include "IChatTransport.h"
#include "MessagePayloadService.h"
#include "MessageContentCodec.h"
#include "usermgr.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>
#include <QtGlobal>

void AppController::onGroupListUpdated()
{
    refreshGroupModel();
    refreshDialogModel();
    requestDialogList();
    if (_current_group_id > 0) {
        auto groupInfo = _gateway.userMgr()->GetGroupById(_current_group_id);
        if (groupInfo) {
            setCurrentGroup(groupInfo->_group_id, groupInfo->_name, groupInfo->_group_code);
            const QString currentGroupIcon = groupInfo->_icon.trimmed().isEmpty()
                ? QStringLiteral("qrc:/res/chat_icon.png")
                : groupInfo->_icon;
            setCurrentChatPeerIcon(currentGroupIcon);
            _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
            syncCurrentDialogDraft();
            return;
        }

        setCurrentGroup(0, "");
        if (_current_chat_uid > 0) {
            loadCurrentChatMessages();
        } else {
            _message_model.clear();
            setCurrentChatPeerName("");
            setCurrentChatPeerIcon("qrc:/res/head_1.jpg");
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
    if (!selfInfo || selfInfo->_uid <= 0) {
        return;
    }
    req["fromuid"] = selfInfo->_uid;
    const QByteArray payload = QJsonDocument(req).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GET_GROUP_LIST_REQ, payload);
    if (_current_group_id <= 0) {
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
    if (!event.isEmpty() && event != "group_read_ack") {
        setGroupStatus(QString("群事件：%1").arg(event), false);
    }

    if (event == "group_read_ack") {
        auto selfInfo = _gateway.userMgr()->GetUserInfo();
        if (!selfInfo) {
            return;
        }
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        const int readerUid = payload.value("fromuid").toInt();
        if (groupId <= 0 || readerUid <= 0 || readerUid == selfInfo->_uid) {
            return;
        }
        qint64 readTs = payload.value("read_ts").toVariant().toLongLong();
        if (readTs <= 0) {
            readTs = QDateTime::currentMSecsSinceEpoch();
        } else if (readTs < 100000000000LL) {
            readTs *= 1000;
        }
        if (_gateway.userMgr()->MarkGroupOutgoingReadUntil(groupId, selfInfo->_uid, readTs) <= 0) {
            return;
        }
        auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
        if (groupInfo && _group_cache_store.isReady()) {
            _group_cache_store.upsertMessages(selfInfo->_uid, groupId, groupInfo->_chat_msgs);
        }
        if (groupId == _current_group_id && groupInfo) {
            _message_model.setMessages(groupInfo->_chat_msgs, selfInfo->_uid);
        }
        return;
    }

    if (event == "group_dissolved") {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        if (groupId > 0) {
            _gateway.userMgr()->RemoveGroup(groupId);
            clearCurrentGroupConversation(groupId);
            refreshGroupModel();
            refreshDialogModel();
        }
        refreshGroupList();
        return;
    }

    if (event == "group_msg_edited" || event == "group_msg_revoked") {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        const MessageUpdateFields updateFields = MessagePayloadService::parseMessageUpdateFields(payload, event);
        if (groupId > 0 && !updateFields.msgId.isEmpty() && !updateFields.content.isEmpty()) {
            _gateway.userMgr()->UpdateGroupChatMsgContent(groupId,
                                                          updateFields.msgId,
                                                          updateFields.content,
                                                          updateFields.state,
                                                          updateFields.editedAtMs,
                                                          updateFields.deletedAtMs);
            auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
            if (groupInfo) {
                auto selfInfo = _gateway.userMgr()->GetUserInfo();
                if (selfInfo && _group_cache_store.isReady()) {
                    _group_cache_store.upsertMessages(selfInfo->_uid, groupId, groupInfo->_chat_msgs);
                }
                if (groupId == _current_group_id) {
                    _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
                }
            }
        }
        return;
    }

    if (event == "group_icon_updated"
        && payload.value("groupid").toVariant().toLongLong() == _current_group_id) {
        const QString newGroupIcon = payload.value("icon").toString().trimmed().isEmpty()
            ? QStringLiteral("qrc:/res/chat_icon.png")
            : payload.value("icon").toString();
        setCurrentChatPeerIcon(newGroupIcon);
    }
    refreshGroupList();
}

void AppController::onGroupChatMsg(std::shared_ptr<GroupChatMsg> msg)
{
    if (!msg || !msg->_msg) {
        return;
    }

    auto textMsg = MessagePayloadService::buildGroupIncomingMessage(*msg);
    if (!textMsg) {
        return;
    }
    _gateway.userMgr()->UpsertGroupChatMsg(msg->_group_id, textMsg);
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (selfInfo && _group_cache_store.isReady()) {
        _group_cache_store.upsertMessages(selfInfo->_uid, msg->_group_id, {textMsg});
    }

    const int pseudoUid = ConversationSyncService::resolveGroupDialogUid(_group_uid_map, msg->_group_id);
    if (pseudoUid != 0) {
        const QString preview = MessageContentCodec::toPreviewText(msg->_msg->_msg_content);
        const int selfUid = _gateway.userMgr()->GetUid();
        const bool fromSelf = (msg->_from_uid == selfUid);
        bool mentionedMe = false;
        if (!fromSelf && msg->_msg->_mention_all) {
            mentionedMe = true;
        } else if (!fromSelf && !msg->_msg->_mentions.isEmpty()) {
            for (const auto &one : msg->_msg->_mentions) {
                if (one.toInt() == selfUid) {
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
        const bool isViewingCurrentGroup = (_current_group_id == msg->_group_id && _current_chat_uid <= 0);
        if (isViewingCurrentGroup) {
            ConversationSyncService::clearGroupUnreadAndMention(_dialog_list_model, _dialog_mention_map, pseudoUid);
        } else if (!fromSelf) {
            ConversationSyncService::incrementGroupUnreadAndMention(
                _dialog_list_model, _dialog_mention_map, pseudoUid, mentionedMe);
        }
    }

    if (msg->_group_id != _current_group_id) {
        return;
    }
    _message_model.appendMessage(textMsg, _gateway.userMgr()->GetUid());
    sendGroupReadAck(msg->_group_id, textMsg->_created_at);
}

void AppController::onGroupRsp(ReqId reqId, int error, QJsonObject payload)
{
    auto actionText = [](ReqId id) -> QString {
        switch (id) {
        case ID_CREATE_GROUP_RSP: return "建群";
        case ID_GET_GROUP_LIST_RSP: return "刷新群列表";
        case ID_INVITE_GROUP_MEMBER_RSP: return "邀请成员";
        case ID_APPLY_JOIN_GROUP_RSP: return "申请入群";
        case ID_REVIEW_GROUP_APPLY_RSP: return "审核申请";
        case ID_GROUP_CHAT_MSG_RSP: return "发送群消息";
        case ID_GROUP_HISTORY_RSP: return "拉取群历史";
        case ID_EDIT_GROUP_MSG_RSP: return "编辑群消息";
        case ID_REVOKE_GROUP_MSG_RSP: return "撤回群消息";
        case ID_FORWARD_GROUP_MSG_RSP: return "转发群消息";
        case ID_UPDATE_GROUP_ANNOUNCEMENT_RSP: return "更新群公告";
        case ID_UPDATE_GROUP_ICON_RSP: return "更新群头像";
        case ID_MUTE_GROUP_MEMBER_RSP: return "禁言成员";
        case ID_SET_GROUP_ADMIN_RSP: return "设置管理员";
        case ID_KICK_GROUP_MEMBER_RSP: return "踢出成员";
        case ID_QUIT_GROUP_RSP: return "退出群聊";
        case ID_DISSOLVE_GROUP_RSP: return "解散群聊";
        case ID_SYNC_DRAFT_RSP: return "同步草稿";
        case ID_PIN_DIALOG_RSP: return "置顶会话";
        case ID_EDIT_PRIVATE_MSG_RSP: return "编辑私聊消息";
        case ID_REVOKE_PRIVATE_MSG_RSP: return "撤回私聊消息";
        case ID_FORWARD_PRIVATE_MSG_RSP: return "转发私聊消息";
        default: return "群操作";
        }
    };

    if (error != ErrorCodes::SUCCESS) {
        if (reqId == ID_GROUP_HISTORY_RSP) {
            const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
            if (groupId <= 0 || groupId == _current_group_id) {
                _group_history_loading = false;
                setPrivateHistoryLoading(false);
            }
        }
        if (reqId == ID_GROUP_CHAT_MSG_RSP) {
            QString clientMsgId = payload.value("client_msg_id").toString();
            if (clientMsgId.isEmpty()) {
                clientMsgId = payload.value("msg").toObject().value("msgid").toString();
            }
            qint64 groupId = payload.value("groupid").toVariant().toLongLong();
            if (groupId <= 0 && !clientMsgId.isEmpty()) {
                groupId = _pending_group_msg_group_map.value(clientMsgId, 0);
            }

            if (groupId > 0 && !clientMsgId.isEmpty()) {
                if (_gateway.userMgr()->UpdateGroupChatMsgState(groupId, clientMsgId, "failed")
                    && groupId == _current_group_id) {
                    _message_model.updateMessageState(clientMsgId, "failed");
                }
                _pending_group_msg_group_map.remove(clientMsgId);
            }
        }
        if (reqId == ID_SYNC_DRAFT_RSP) {
            return;
        }
        if (reqId == ID_EDIT_PRIVATE_MSG_RSP
            || reqId == ID_REVOKE_PRIVATE_MSG_RSP
            || reqId == ID_FORWARD_PRIVATE_MSG_RSP) {
            setTip(QString("%1失败（错误码:%2）").arg(actionText(reqId)).arg(error), true);
        } else {
            const QString serverMessage = payload.value("message").toString().trimmed();
            setGroupStatus(serverMessage.isEmpty()
                               ? QString("%1失败（错误码:%2）").arg(actionText(reqId)).arg(error)
                               : QString("%1失败：%2（错误码:%3）").arg(actionText(reqId), serverMessage).arg(error),
                           true);
        }
        return;
    }

    switch (reqId) {
    case ID_CREATE_GROUP_RSP: {
        const QString groupName = payload.value("name").toString();
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        setGroupStatus(groupName.isEmpty() ? "群聊创建成功" : QString("群聊创建成功：%1").arg(groupName), false);
        if (groupId > 0) {
            refreshGroupModel();
            refreshDialogModel();
            const int dialogUid = ConversationSyncService::resolveGroupDialogUid(_group_uid_map, groupId);
            if (dialogUid != 0) {
                selectDialogByUid(dialogUid);
            }
            emit groupCreated(groupId);
        }
        refreshGroupList();
        break;
    }
    case ID_INVITE_GROUP_MEMBER_RSP:
        setGroupStatus("邀请成员成功", false);
        refreshGroupList();
        break;
    case ID_APPLY_JOIN_GROUP_RSP:
        setGroupStatus("入群申请已提交", false);
        break;
    case ID_REVIEW_GROUP_APPLY_RSP:
        setGroupStatus("审核操作已完成", false);
        refreshGroupList();
        break;
    case ID_GROUP_HISTORY_RSP: {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
        auto selfInfo = _gateway.userMgr()->GetUserInfo();
        if (groupInfo && selfInfo && _group_cache_store.isReady()) {
            _group_cache_store.upsertMessages(selfInfo->_uid, groupId, groupInfo->_chat_msgs);
        }
        const bool isCurrentGroupHistory = groupId == _current_group_id;
        if (isCurrentGroupHistory) {
            _group_history_loading = false;
            setPrivateHistoryLoading(false);
        }
        qInfo() << "Group history response, group id:" << groupId
                << "current group id:" << _current_group_id
                << "message count:" << payload.value("messages").toArray().size()
                << "cached group msg count:" << (groupInfo ? static_cast<qlonglong>(groupInfo->_chat_msgs.size()) : 0LL)
                << "has more:" << payload.value("has_more").toBool(false);
        if (isCurrentGroupHistory && groupInfo) {
            _group_history_has_more = payload.value("has_more").toBool(false);
            setCanLoadMorePrivateHistory(_group_history_has_more);
            _group_history_before_seq = payload.value("next_before_seq").toVariant().toLongLong();
            _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
            if (_group_history_before_seq <= 0) {
                for (const auto &one : groupInfo->_chat_msgs) {
                    if (!one || one->_group_seq <= 0) {
                        continue;
                    }
                    if (_group_history_before_seq <= 0 || one->_group_seq < _group_history_before_seq) {
                        _group_history_before_seq = one->_group_seq;
                    }
                }
            }
            qint64 readTs = 0;
            if (!groupInfo->_chat_msgs.empty() && groupInfo->_chat_msgs.back()) {
                readTs = groupInfo->_chat_msgs.back()->_created_at;
            }
            sendGroupReadAck(groupId, readTs);
            const int dialogUid = ConversationSyncService::resolveGroupDialogUid(_group_uid_map, groupId);
            ConversationSyncService::clearGroupUnreadAndMention(
                _dialog_list_model, _dialog_mention_map, dialogUid);
            setGroupStatus("历史消息已加载", false);
        } else {
            qInfo() << "Group history response cached for non-current group, group id:" << groupId;
        }
        break;
    }
    case ID_EDIT_GROUP_MSG_RSP:
    case ID_REVOKE_GROUP_MSG_RSP: {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        const MessageUpdateFields updateFields = MessagePayloadService::parseMessageUpdateFields(
            payload,
            reqId == ID_REVOKE_GROUP_MSG_RSP ? QStringLiteral("group_msg_revoked")
                                            : QStringLiteral("group_msg_edited"));
        if (groupId > 0 && !updateFields.msgId.isEmpty() && !updateFields.content.isEmpty()) {
            _gateway.userMgr()->UpdateGroupChatMsgContent(groupId,
                                                          updateFields.msgId,
                                                          updateFields.content,
                                                          updateFields.state,
                                                          updateFields.editedAtMs,
                                                          updateFields.deletedAtMs);
            auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
            if (groupInfo) {
                auto selfInfo = _gateway.userMgr()->GetUserInfo();
                if (selfInfo && _group_cache_store.isReady()) {
                    _group_cache_store.upsertMessages(selfInfo->_uid, groupId, groupInfo->_chat_msgs);
                }
                if (groupId == _current_group_id) {
                    _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
                }
            }
        }
        setGroupStatus(reqId == ID_EDIT_GROUP_MSG_RSP ? "群消息已编辑" : "群消息已撤回", false);
        break;
    }
    case ID_EDIT_PRIVATE_MSG_RSP:
    case ID_REVOKE_PRIVATE_MSG_RSP: {
        auto selfInfo = _gateway.userMgr()->GetUserInfo();
        if (!selfInfo) {
            break;
        }
        const int peerUid = payload.value("peer_uid").toInt();
        const MessageUpdateFields updateFields = MessagePayloadService::parseMessageUpdateFields(
            payload,
            reqId == ID_REVOKE_PRIVATE_MSG_RSP ? QStringLiteral("private_msg_revoked")
                                              : QStringLiteral("private_msg_edited"));
        if (peerUid > 0 && !updateFields.msgId.isEmpty() && !updateFields.content.isEmpty()) {
            _gateway.userMgr()->UpdatePrivateChatMsgContent(peerUid,
                                                            updateFields.msgId,
                                                            updateFields.content,
                                                            updateFields.state,
                                                            updateFields.editedAtMs,
                                                            updateFields.deletedAtMs);
            auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
            if (friendInfo && _private_cache_store.isReady()) {
                _private_cache_store.upsertMessages(selfInfo->_uid, peerUid, friendInfo->_chat_msgs);
                if (!friendInfo->_chat_msgs.empty() && friendInfo->_chat_msgs.back()) {
                    const QString preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
                    const qint64 lastTs = friendInfo->_chat_msgs.back()->_created_at;
                    ConversationSyncService::updatePrivatePreview(
                        _chat_list_model, _dialog_list_model, peerUid, preview, lastTs);
                }
            }
            if (peerUid == _current_chat_uid) {
                _message_model.patchMessageContent(updateFields.msgId,
                                                   updateFields.content,
                                                   updateFields.state,
                                                   updateFields.editedAtMs,
                                                   updateFields.deletedAtMs);
            }
        }
        setTip(reqId == ID_EDIT_PRIVATE_MSG_RSP ? "私聊消息已编辑" : "私聊消息已撤回", false);
        break;
    }
    case ID_FORWARD_PRIVATE_MSG_RSP: {
        auto selfInfo = _gateway.userMgr()->GetUserInfo();
        if (!selfInfo) {
            break;
        }
        const int peerUid = payload.value("peer_uid").toInt();
        const QJsonObject msgObj = payload.value("msg").toObject();
        auto forwardedMsg = MessagePayloadService::buildPrivateForwardedMessage(payload, msgObj, selfInfo->_uid);
        if (peerUid > 0 && forwardedMsg && !forwardedMsg->_msg_id.isEmpty()) {
            _gateway.userMgr()->AppendFriendChatMsg(peerUid, {forwardedMsg});
            if (_private_cache_store.isReady()) {
                _private_cache_store.upsertMessages(selfInfo->_uid, peerUid, {forwardedMsg});
            }
            auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
            if (friendInfo && !friendInfo->_chat_msgs.empty() && friendInfo->_chat_msgs.back()) {
                const QString preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
                const qint64 lastTs = friendInfo->_chat_msgs.back()->_created_at;
                ConversationSyncService::updatePrivatePreview(
                    _chat_list_model, _dialog_list_model, peerUid, preview, lastTs);
            }
            if (_current_group_id <= 0 && peerUid == _current_chat_uid) {
                _message_model.upsertMessage(forwardedMsg, selfInfo->_uid);
            }
        }
        setTip("消息已转发", false);
        break;
    }
    case ID_UPDATE_GROUP_ANNOUNCEMENT_RSP:
        setGroupStatus("群公告已更新", false);
        refreshGroupList();
        break;
    case ID_UPDATE_GROUP_ICON_RSP:
        if (_current_group_id == payload.value("groupid").toVariant().toLongLong()) {
            const QString updatedGroupIcon = payload.value("icon").toString().trimmed().isEmpty()
                ? QStringLiteral("qrc:/res/chat_icon.png")
                : payload.value("icon").toString();
            setCurrentChatPeerIcon(updatedGroupIcon);
        }
        setGroupStatus("群头像已更新", false);
        refreshGroupList();
        break;
    case ID_MUTE_GROUP_MEMBER_RSP:
        setGroupStatus("禁言设置已生效", false);
        break;
    case ID_SET_GROUP_ADMIN_RSP:
        setGroupStatus("管理员设置已更新", false);
        refreshGroupList();
        break;
    case ID_KICK_GROUP_MEMBER_RSP:
        setGroupStatus("成员已移出群聊", false);
        refreshGroupList();
        break;
    case ID_QUIT_GROUP_RSP:
        setGroupStatus("已退出当前群聊", false);
        _gateway.userMgr()->RemoveGroup(payload.value("groupid").toVariant().toLongLong());
        refreshGroupList();
        clearCurrentGroupConversation(payload.value("groupid").toVariant().toLongLong());
        break;
    case ID_DISSOLVE_GROUP_RSP: {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        setGroupStatus("群聊已解散", false);
        _gateway.userMgr()->RemoveGroup(groupId);
        refreshGroupList();
        clearCurrentGroupConversation(groupId);
        break;
    }
    case ID_GROUP_CHAT_MSG_RSP:
    case ID_FORWARD_GROUP_MSG_RSP: {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        const QJsonObject msgObj = payload.value("msg").toObject();
        const QString ackMsgId = payload.value("client_msg_id").toString(msgObj.value("msgid").toString());
        auto selfInfo = _gateway.userMgr()->GetUserInfo();
        if (groupId > 0 && !ackMsgId.isEmpty()) {
            const int selfUid = selfInfo ? selfInfo->_uid : _gateway.userMgr()->GetUid();
            const QString senderName = payload.value("from_nick").toString(
                payload.value("from_name").toString(selfInfo ? selfInfo->_nick : QString()));
            const QString senderIcon = normalizeIconPath(payload.value("from_icon").toString(
                selfInfo ? selfInfo->_icon : QString()));
            auto correctedMsg = MessagePayloadService::buildGroupAckMessage(payload,
                                                                            msgObj,
                                                                            selfUid,
                                                                            senderName,
                                                                            senderIcon);
            if (!correctedMsg || correctedMsg->_msg_id.isEmpty() || correctedMsg->_msg_content.isEmpty()) {
                if (payload.value("status").toString() == QStringLiteral("accepted")) {
                    if (_gateway.userMgr()->UpdateGroupChatMsgState(groupId, ackMsgId, QStringLiteral("accepted"))
                        && groupId == _current_group_id) {
                        _message_model.updateMessageState(ackMsgId, QStringLiteral("accepted"));
                    }
                    if (selfInfo && _group_cache_store.isReady()) {
                        auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
                        if (groupInfo) {
                            _group_cache_store.upsertMessages(selfInfo->_uid, groupId, groupInfo->_chat_msgs);
                        }
                    }
                }
                break;
            }
            const QString msgId = correctedMsg->_msg_id;
            _gateway.userMgr()->UpsertGroupChatMsg(groupId, correctedMsg);
            _pending_group_msg_group_map.remove(msgId);
            if (selfInfo && _group_cache_store.isReady()) {
                _group_cache_store.upsertMessages(selfInfo->_uid, groupId, {correctedMsg});
            }

            const int pseudoUid = ConversationSyncService::resolveGroupDialogUid(_group_uid_map, groupId);
            if (pseudoUid != 0) {
                const QString preview = MessageContentCodec::toPreviewText(correctedMsg->_msg_content);
                ConversationSyncService::updateGroupPreview(_group_list_model,
                                                            _dialog_list_model,
                                                            pseudoUid,
                                                            preview,
                                                            preview,
                                                            correctedMsg->_created_at);
            }

            if (groupId == _current_group_id) {
                _message_model.upsertMessage(correctedMsg, _gateway.userMgr()->GetUid());
                sendGroupReadAck(groupId, correctedMsg->_created_at);
            }
        }
        if (reqId == ID_FORWARD_GROUP_MSG_RSP) {
            setGroupStatus("消息已转发", false);
        }
        break;
    }
    case ID_SYNC_DRAFT_RSP: {
        const QString dialogType = payload.value("dialog_type").toString();
        int dialogUid = 0;
        if (dialogType == "group") {
            const qint64 groupId = payload.value("group_id").toVariant().toLongLong();
            if (groupId > 0) {
                dialogUid = ConversationSyncService::resolveGroupDialogUid(_group_uid_map, groupId);
            }
        } else if (dialogType == "private") {
            dialogUid = payload.value("peer_uid").toInt();
        }
        if (dialogUid != 0) {
            const QString draftText = payload.value("draft_text").toString();
            if (draftText.trimmed().isEmpty()) {
                _dialog_draft_map.remove(dialogUid);
            } else {
                _dialog_draft_map.insert(dialogUid, draftText);
            }
            const int muteState = payload.value("mute_state").toInt(
                _dialog_server_mute_map.value(dialogUid, 0));
            _dialog_server_mute_map.insert(dialogUid, muteState > 0 ? 1 : 0);
            const int idx = _dialog_list_model.indexOfUid(dialogUid);
            if (idx >= 0) {
                const QVariantMap item = _dialog_list_model.get(idx);
                _dialog_list_model.setDialogMeta(dialogUid,
                                                 item.value("dialogType").toString(),
                                                 item.value("unreadCount").toInt(),
                                                 item.value("pinnedRank").toInt(),
                                                 draftText,
                                                 item.value("lastMsgTs").toLongLong(),
                                                 muteState);
            } else {
                applyDraftToDialogModel(dialogUid, draftText);
            }
            if (dialogUid == currentDialogUid()) {
                setCurrentDraftText(draftText);
                setCurrentDialogMuted(muteState > 0);
            }
            const int ownerUid = _gateway.userMgr()->GetUid();
            if (ownerUid > 0) {
                saveDraftStore(ownerUid);
            }
        }
        break;
    }
    case ID_PIN_DIALOG_RSP: {
        const QString dialogType = payload.value("dialog_type").toString();
        int dialogUid = 0;
        if (dialogType == "group") {
            const qint64 groupId = payload.value("group_id").toVariant().toLongLong();
            if (groupId > 0) {
                dialogUid = ConversationSyncService::resolveGroupDialogUid(_group_uid_map, groupId);
            }
        } else if (dialogType == "private") {
            dialogUid = payload.value("peer_uid").toInt();
        }
        if (dialogUid != 0) {
            const int pinnedRank = qMax(0, payload.value("pinned_rank").toInt(0));
            _dialog_server_pinned_map.insert(dialogUid, pinnedRank);
            if (pinnedRank > 0) {
                _dialog_local_pinned_set.insert(dialogUid);
            } else {
                _dialog_local_pinned_set.remove(dialogUid);
            }
            if (dialogUid == currentDialogUid()) {
                setCurrentDialogPinned(pinnedRank > 0);
            }
            const int ownerUid = _gateway.userMgr()->GetUid();
            if (ownerUid > 0) {
                saveDraftStore(ownerUid);
            }
            refreshDialogModel();
        }
        break;
    }
    case ID_GET_GROUP_LIST_RSP:
    default:
        break;
    }
}
