#include "AppController.h"

#include "ConversationSyncService.h"
#include "IChatTransport.h"
#include "MessageContentCodec.h"
#include "usermgr.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUuid>

bool AppController::isChatTransportReady() const
{
    const auto transport = _gateway.chatTransport();
    return transport && transport->isConnected();
}

bool AppController::dispatchChatContent(const QString& content, const QString& previewText)
{
    if (_chat_state.uid <= 0)
    {
        return false;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return false;
    }
    if (!isChatTransportReady())
    {
        setTip("聊天连接未就绪，请重新登录", true);
        return false;
    }

    OutgoingChatPacket packet;
    packet.fromUid = selfInfo->_uid;
    packet.toUid = _chat_state.uid;
    packet.msgId = QUuid::createUuid().toString();
    packet.content = content;
    packet.createdAt = QDateTime::currentMSecsSinceEpoch();

    QString errorText;
    if (!_chat_controller.dispatchChatText(packet, &errorText))
    {
        if (!errorText.isEmpty())
        {
            setTip(errorText, true);
        }
        return false;
    }

    auto msg = std::make_shared<TextChatData>(packet.msgId,
                                              content,
                                              packet.fromUid,
                                              packet.toUid,
                                              QString(),
                                              packet.createdAt,
                                              QString(),
                                              QStringLiteral("sending"));
    _gateway.userMgr()->AppendFriendChatMsg(_chat_state.uid, {msg});
    _private_cache_store.upsertMessages(packet.fromUid, _chat_state.uid, {msg});
    auto friendInfo = _gateway.userMgr()->GetFriendById(_chat_state.uid);
    _message_model.appendMessage(msg, packet.fromUid);
    ConversationSyncService::syncHistoryCursor(_message_model,
                                               _private_history_state.beforeTs,
                                               _private_history_state.beforeMsgId);

    const QString resolvedPreview = previewText.isEmpty() ? MessageContentCodec::toPreviewText(content) : previewText;
    ConversationSyncService::updatePrivatePreview(_chat_list_model,
                                                  _dialog_list_model,
                                                  _chat_state.uid,
                                                  resolvedPreview,
                                                  packet.createdAt);
    ConversationSyncService::clearPrivateUnread(_chat_list_model, _dialog_list_model, _chat_state.uid);
    qInfo() << "Private chat message dispatched, peer uid:" << _chat_state.uid << "msg id:" << packet.msgId
            << "friend msg count:" << (friendInfo ? static_cast<qlonglong>(friendInfo->_chat_msgs.size()) : 0LL)
            << "message model count:" << _message_model.rowCount();
    return true;
}

bool AppController::dispatchGroupChatContent(const QString& content, const QString& previewText)
{
    if (_group_state.currentId <= 0)
    {
        return false;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return false;
    }
    if (!isChatTransportReady())
    {
        setTip("聊天连接未就绪，请重新登录", true);
        return false;
    }

    const QString msgId = QUuid::createUuid().toString();
    const qint64 createdAt = QDateTime::currentMSecsSinceEpoch();
    const DecodedMessageContent decoded = MessageContentCodec::decode(content);
    const QString plainText = decoded.content;
    const bool mentionAll = plainText.contains("@all", Qt::CaseInsensitive) || plainText.contains("@所有人");
    QJsonArray mentions;
    const QRegularExpression mentionIdRegex("@u([1-9][0-9]{8})");
    QRegularExpressionMatchIterator mentionIt = mentionIdRegex.globalMatch(plainText);
    while (mentionIt.hasNext())
    {
        const QRegularExpressionMatch m = mentionIt.next();
        bool ok = false;
        const int uid = m.captured(1).toInt(&ok);
        if (ok && uid > 0)
        {
            mentions.append(uid);
        }
    }
    qint64 replyToServerMsgId = 0;
    if (!_dialog_state.replyToMsgId.isEmpty())
    {
        auto groupInfo = _gateway.userMgr()->GetGroupById(_group_state.currentId);
        if (groupInfo)
        {
            for (const auto& one : groupInfo->_chat_msgs)
            {
                if (!one || one->_msg_id != _dialog_state.replyToMsgId)
                {
                    continue;
                }
                if (one->_server_msg_id > 0)
                {
                    replyToServerMsgId = one->_server_msg_id;
                }
                break;
            }
        }
    }

    QJsonObject msgObj;
    msgObj["msgid"] = msgId;
    msgObj["content"] = content;
    msgObj["msgtype"] = decoded.type.isEmpty() ? "text" : decoded.type;
    msgObj["mentions"] = mentions;
    msgObj["mention_all"] = mentionAll;
    if (replyToServerMsgId > 0)
    {
        msgObj["reply_to_server_msg_id"] = replyToServerMsgId;
    }
    if (!decoded.fileName.isEmpty())
    {
        msgObj["file_name"] = decoded.fileName;
    }
    if (!decoded.mimeType.isEmpty())
    {
        msgObj["mime"] = decoded.mimeType;
    }
    if (decoded.sizeBytes > 0)
    {
        msgObj["size"] = static_cast<qint64>(decoded.sizeBytes);
    }

    QJsonObject payloadObj;
    payloadObj["fromuid"] = selfInfo->_uid;
    payloadObj["groupid"] = static_cast<qint64>(_group_state.currentId);
    payloadObj["msg"] = msgObj;
    const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GROUP_CHAT_MSG_REQ, payload);

    const QString senderName = selfInfo->_nick.isEmpty() ? selfInfo->_name : selfInfo->_nick;
    auto msg = std::make_shared<TextChatData>(msgId,
                                              content,
                                              selfInfo->_uid,
                                              0,
                                              senderName,
                                              createdAt,
                                              _user_state.icon,
                                              "sending",
                                              0,
                                              0,
                                              replyToServerMsgId);
    _gateway.userMgr()->UpsertGroupChatMsg(_group_state.currentId, msg);
    _group_state.pendingMsgGroupMap.insert(msgId, _group_state.currentId);
    if (_group_cache_store.isReady())
    {
        _group_cache_store.upsertMessages(selfInfo->_uid, _group_state.currentId, {msg});
    }
    _message_model.appendMessage(msg, selfInfo->_uid);

    const int pseudoUid =
        ConversationSyncService::resolveGroupDialogUid(_group_state.dialogUidMap, _group_state.currentId);

    if (pseudoUid != 0)
    {
        const QString resolvedPreview =
            previewText.isEmpty() ? MessageContentCodec::toPreviewText(content) : previewText;
        ConversationSyncService::updateGroupPreview(_group_list_model,
                                                    _dialog_list_model,
                                                    pseudoUid,
                                                    resolvedPreview,
                                                    resolvedPreview,
                                                    createdAt);
        ConversationSyncService::clearGroupUnreadAndMention(_dialog_list_model, _dialog_state.mentionMap, pseudoUid);
    }

    return true;
}
