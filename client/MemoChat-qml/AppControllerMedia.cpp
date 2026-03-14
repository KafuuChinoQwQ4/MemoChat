#include "AppController.h"
#include "AppCoordinators.h"
#include "ConversationSyncService.h"
#include "LocalFilePickerService.h"
#include "MessageContentCodec.h"
#include "tcpmgr.h"
#include "usermgr.h"
#include <QDateTime>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>
#include <QUuid>
#include <QtConcurrent>

namespace {
struct MediaUploadTaskResult {
    bool ok = false;
    UploadedMediaInfo uploaded;
    QString errorText;
};

QVariantMap normalizePendingAttachment(const QVariantMap &source)
{
    QVariantMap attachment = source;
    const QString type = attachment.value(QStringLiteral("type")).toString().trimmed();
    const QString fileUrl = attachment.value(QStringLiteral("fileUrl")).toString().trimmed();
    if ((type != QStringLiteral("image") && type != QStringLiteral("file")) || fileUrl.isEmpty()) {
        return {};
    }

    if (attachment.value(QStringLiteral("attachmentId")).toString().trimmed().isEmpty()) {
        attachment.insert(QStringLiteral("attachmentId"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
    if (attachment.value(QStringLiteral("fileName")).toString().trimmed().isEmpty()) {
        const QFileInfo fileInfo(QUrl(fileUrl).toLocalFile());
        attachment.insert(QStringLiteral("fileName"), fileInfo.fileName());
    }
    return attachment;
}
}

void AppController::sendTextMessage(const QString &text)
{
    _media_coordinator->sendTextMessage(text);
}

void AppController::sendCurrentComposerPayload(const QString &text)
{
    _media_coordinator->sendCurrentComposerPayload(text);
}

void AppController::sendImageMessage()
{
    _media_coordinator->sendImageMessage();
}

void AppController::sendFileMessage()
{
    _media_coordinator->sendFileMessage();
}

void AppController::startMediaUploadAndSend(const QString &fileUrl, const QString &mediaType, const QString &fallbackName)
{
    Q_UNUSED(fallbackName);
    QVariantMap attachment;
    attachment.insert(QStringLiteral("attachmentId"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    attachment.insert(QStringLiteral("fileUrl"), fileUrl);
    attachment.insert(QStringLiteral("type"), mediaType);
    attachment.insert(QStringLiteral("fileName"), QFileInfo(QUrl(fileUrl).toLocalFile()).fileName());
    _pending_send_dialog_uid = currentDialogUid();
    _pending_send_chat_uid = _current_chat_uid;
    _pending_send_group_id = _current_group_id;
    _pending_send_queue = { attachment };
    _pending_send_total_count = 1;
    processPendingAttachmentQueue();
}

void AppController::removePendingAttachment(const QString &attachmentId)
{
    _media_coordinator->removePendingAttachment(attachmentId);
}

void AppController::clearPendingAttachments()
{
    _media_coordinator->clearPendingAttachments();
}

void AppController::addPendingAttachments(const QVariantList &attachments)
{
    const int dialogUid = currentDialogUid();
    if (dialogUid == 0 || attachments.isEmpty()) {
        return;
    }

    QVariantList nextList = _dialog_pending_attachment_map.value(dialogUid);
    for (const QVariant &entry : attachments) {
        const QVariantMap normalized = normalizePendingAttachment(entry.toMap());
        if (!normalized.isEmpty()) {
            nextList.push_back(normalized);
        }
    }
    _dialog_pending_attachment_map.insert(dialogUid, nextList);
    if (dialogUid == currentDialogUid()) {
        setCurrentPendingAttachments(nextList);
    }
}

void AppController::removePendingAttachmentById(const QString &attachmentId, int dialogUid)
{
    const int targetDialogUid = dialogUid == 0 ? currentDialogUid() : dialogUid;
    if (targetDialogUid == 0 || attachmentId.trimmed().isEmpty()) {
        return;
    }

    QVariantList attachments = _dialog_pending_attachment_map.value(targetDialogUid);
    bool removed = false;
    for (int index = attachments.size() - 1; index >= 0; --index) {
        const QVariantMap attachment = attachments.at(index).toMap();
        if (attachment.value(QStringLiteral("attachmentId")).toString() == attachmentId) {
            attachments.removeAt(index);
            removed = true;
            break;
        }
    }
    if (!removed) {
        return;
    }
    if (attachments.isEmpty()) {
        _dialog_pending_attachment_map.remove(targetDialogUid);
    } else {
        _dialog_pending_attachment_map.insert(targetDialogUid, attachments);
    }
    if (targetDialogUid == currentDialogUid()) {
        setCurrentPendingAttachments(attachments);
    }
}

void AppController::setCurrentPendingAttachments(const QVariantList &attachments)
{
    if (_current_pending_attachments == attachments) {
        return;
    }
    _current_pending_attachments = attachments;
    emit currentPendingAttachmentsChanged();
}

void AppController::processPendingAttachmentQueue()
{
    if (_pending_send_queue.isEmpty()) {
        _pending_send_total_count = 0;
        _pending_send_dialog_uid = 0;
        _pending_send_chat_uid = 0;
        _pending_send_group_id = 0;
        setMediaUploadInProgress(false);
        setMediaUploadProgressText(QString());
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        setTip("用户状态异常，请重新登录", true);
        _pending_send_queue.clear();
        setMediaUploadInProgress(false);
        setMediaUploadProgressText(QString());
        return;
    }
    if (_pending_token.trimmed().isEmpty()) {
        setTip("登录态失效，请重新登录", true);
        _pending_send_queue.clear();
        setMediaUploadInProgress(false);
        setMediaUploadProgressText(QString());
        return;
    }

    setMediaUploadInProgress(true);
    const QVariantMap currentAttachment = _pending_send_queue.first().toMap();
    const int attachmentIndex = qMax(1, _pending_send_total_count - _pending_send_queue.size() + 1);
    const QString uploadType = currentAttachment.value(QStringLiteral("type")).toString();
    const QString uploadFileUrl = currentAttachment.value(QStringLiteral("fileUrl")).toString();
    setMediaUploadProgressText(QString("正在发送附件 %1/%2").arg(attachmentIndex).arg(qMax(1, _pending_send_total_count)));

    auto *watcher = new QFutureWatcher<MediaUploadTaskResult>(this);
    const int uploadUid = selfInfo->_uid;
    const QString token = _pending_token;
    const int targetDialogUid = _pending_send_dialog_uid;
    const qint64 targetGroupId = _pending_send_group_id;
    const int targetChatUid = _pending_send_chat_uid;

    const auto future = QtConcurrent::run([this, uploadFileUrl, uploadType, uploadUid, token, attachmentIndex]() {
        MediaUploadTaskResult result;
        result.ok = MediaUploadService::uploadLocalFile(
            uploadFileUrl,
            uploadType,
            uploadUid,
            token,
            &result.uploaded,
            &result.errorText,
            [this, attachmentIndex](int percent, const QString &stage) {
                QMetaObject::invokeMethod(this, [this, percent, stage, attachmentIndex]() {
                    const int bounded = qBound(0, percent, 100);
                    setMediaUploadProgressText(QString("正在发送附件 %1/%2 %3 %4%")
                                                   .arg(attachmentIndex)
                                                   .arg(qMax(1, _pending_send_total_count))
                                                   .arg(stage)
                                                   .arg(bounded));
                }, Qt::QueuedConnection);
            });
        return result;
    });

    connect(watcher, &QFutureWatcher<MediaUploadTaskResult>::finished, this, [this, watcher, currentAttachment, uploadType, targetDialogUid, targetGroupId, targetChatUid]() {
        const MediaUploadTaskResult result = watcher->result();
        watcher->deleteLater();

        if (!result.ok) {
            const QString fallbackErr = (uploadType == QStringLiteral("image")) ? "图片上传失败" : "文件上传失败";
            _pending_send_queue.clear();
            _pending_send_total_count = 0;
            _pending_send_dialog_uid = 0;
            _pending_send_chat_uid = 0;
            _pending_send_group_id = 0;
            setMediaUploadInProgress(false);
            setMediaUploadProgressText(QString());
            setTip(result.errorText.isEmpty() ? fallbackErr : result.errorText, true);
            return;
        }

        if (!sendUploadedAttachmentToDialog(currentAttachment, result.uploaded, targetDialogUid, targetChatUid, targetGroupId)) {
            _pending_send_queue.clear();
            _pending_send_total_count = 0;
            _pending_send_dialog_uid = 0;
            _pending_send_chat_uid = 0;
            _pending_send_group_id = 0;
            setMediaUploadInProgress(false);
            setMediaUploadProgressText(QString());
            return;
        }

        removePendingAttachmentById(currentAttachment.value(QStringLiteral("attachmentId")).toString(), targetDialogUid);
        if (!_pending_send_queue.isEmpty()) {
            _pending_send_queue.removeFirst();
        }
        if (_pending_send_queue.isEmpty()) {
            _pending_send_total_count = 0;
            _pending_send_dialog_uid = 0;
            _pending_send_chat_uid = 0;
            _pending_send_group_id = 0;
            setMediaUploadInProgress(false);
            setMediaUploadProgressText(QString());
            return;
        }
        processPendingAttachmentQueue();
    });
    watcher->setFuture(future);
}

bool AppController::sendUploadedAttachmentToDialog(const QVariantMap &attachment, const UploadedMediaInfo &uploaded,
                                                   int dialogUid, int chatUid, qint64 groupId)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        setTip("用户状态异常，请重新登录", true);
        return false;
    }

    const QString attachmentType = attachment.value(QStringLiteral("type")).toString();
    const bool sameDialog = (currentDialogUid() == dialogUid);
    if (attachmentType == QStringLiteral("image")) {
        const QString encoded = MessageContentCodec::encodeImage(uploaded.remoteUrl);
        if (groupId > 0) {
            if (sameDialog) {
                if (!dispatchGroupChatContent(encoded, "[图片]")) {
                    setTip("群图片发送失败", true);
                    return false;
                }
            } else {
                const QString msgId = QUuid::createUuid().toString();
                const DecodedMessageContent decoded = MessageContentCodec::decode(encoded);
                QJsonObject msgObj;
                msgObj["msgid"] = msgId;
                msgObj["content"] = encoded;
                msgObj["msgtype"] = decoded.type.isEmpty() ? "text" : decoded.type;
                QJsonObject payloadObj;
                payloadObj["fromuid"] = selfInfo->_uid;
                payloadObj["groupid"] = static_cast<qint64>(groupId);
                payloadObj["msg"] = msgObj;
                _gateway.tcpMgr()->slot_send_data(ReqId::ID_GROUP_CHAT_MSG_REQ,
                                                  QJsonDocument(payloadObj).toJson(QJsonDocument::Compact));
            }
            return true;
        }

        if (sameDialog) {
            if (!dispatchChatContent(encoded, "[图片]")) {
                setTip("图片发送失败", true);
                return false;
            }
        } else {
            OutgoingChatPacket packet;
            packet.fromUid = selfInfo->_uid;
            packet.toUid = chatUid;
            packet.msgId = QUuid::createUuid().toString();
            packet.content = encoded;
            packet.createdAt = QDateTime::currentMSecsSinceEpoch();
            QString errText;
            if (!_chat_controller.dispatchChatText(packet, &errText)) {
                setTip(errText.isEmpty() ? "图片发送失败" : errText, true);
                return false;
            }
        }
        return true;
    }

    const QString fallbackName = attachment.value(QStringLiteral("fileName")).toString();
    const QString remoteName = uploaded.fileName.isEmpty() ? fallbackName : uploaded.fileName;
    const QString encoded = MessageContentCodec::encodeFile(uploaded.remoteUrl,
                                                            remoteName,
                                                            uploaded.mimeType,
                                                            uploaded.sizeBytes);
    const QString preview = remoteName.isEmpty() ? "[文件]" : QString("[文件] %1").arg(remoteName);
    if (groupId > 0) {
        if (sameDialog) {
            if (!dispatchGroupChatContent(encoded, preview)) {
                setTip("群文件发送失败", true);
                return false;
            }
        } else {
            const QString msgId = QUuid::createUuid().toString();
            const DecodedMessageContent decoded = MessageContentCodec::decode(encoded);
            QJsonObject msgObj;
            msgObj["msgid"] = msgId;
            msgObj["content"] = encoded;
            msgObj["msgtype"] = decoded.type.isEmpty() ? "text" : decoded.type;
            if (!decoded.fileName.isEmpty()) {
                msgObj["file_name"] = decoded.fileName;
            }
            if (!decoded.mimeType.isEmpty()) {
                msgObj["mime"] = decoded.mimeType;
            }
            if (decoded.sizeBytes > 0) {
                msgObj["size"] = static_cast<qint64>(decoded.sizeBytes);
            }
            QJsonObject payloadObj;
            payloadObj["fromuid"] = selfInfo->_uid;
            payloadObj["groupid"] = static_cast<qint64>(groupId);
            payloadObj["msg"] = msgObj;
            _gateway.tcpMgr()->slot_send_data(ReqId::ID_GROUP_CHAT_MSG_REQ,
                                              QJsonDocument(payloadObj).toJson(QJsonDocument::Compact));
        }
        return true;
    }

    if (sameDialog) {
        if (!dispatchChatContent(encoded, preview)) {
            setTip("文件发送失败", true);
            return false;
        }
    } else {
        OutgoingChatPacket packet;
        packet.fromUid = selfInfo->_uid;
        packet.toUid = chatUid;
        packet.msgId = QUuid::createUuid().toString();
        packet.content = encoded;
        packet.createdAt = QDateTime::currentMSecsSinceEpoch();
        QString errText;
        if (!_chat_controller.dispatchChatText(packet, &errText)) {
            setTip(errText.isEmpty() ? "文件发送失败" : errText, true);
            return false;
        }
    }
    return true;
}

bool AppController::isChatTransportReady() const
{
    const auto tcp = _gateway.tcpMgr();
    return tcp && tcp->isConnected();
}

bool AppController::dispatchChatContent(const QString &content, const QString &previewText)
{
    if (_current_chat_uid <= 0) {
        return false;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return false;
    }
    if (!isChatTransportReady()) {
        setTip("聊天连接未就绪，请重新登录", true);
        return false;
    }

    OutgoingChatPacket packet;
    packet.fromUid = selfInfo->_uid;
    packet.toUid = _current_chat_uid;
    packet.msgId = QUuid::createUuid().toString();
    packet.content = content;
    packet.createdAt = QDateTime::currentMSecsSinceEpoch();

    QString errorText;
    if (!_chat_controller.dispatchChatText(packet, &errorText)) {
        if (!errorText.isEmpty()) {
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
    _gateway.userMgr()->AppendFriendChatMsg(_current_chat_uid, {msg});
    _private_cache_store.upsertMessages(packet.fromUid, _current_chat_uid, {msg});
    auto friendInfo = _gateway.userMgr()->GetFriendById(_current_chat_uid);
    _message_model.appendMessage(msg, packet.fromUid);
    ConversationSyncService::syncHistoryCursor(_message_model, _private_history_before_ts, _private_history_before_msg_id);

    const QString resolvedPreview = previewText.isEmpty() ? MessageContentCodec::toPreviewText(content) : previewText;
    ConversationSyncService::updatePrivatePreview(
        _chat_list_model, _dialog_list_model, _current_chat_uid, resolvedPreview, packet.createdAt);
    ConversationSyncService::clearPrivateUnread(_chat_list_model, _dialog_list_model, _current_chat_uid);
    qInfo() << "Private chat message dispatched, peer uid:" << _current_chat_uid
            << "msg id:" << packet.msgId
            << "friend msg count:" << (friendInfo ? static_cast<qlonglong>(friendInfo->_chat_msgs.size()) : 0LL)
            << "message model count:" << _message_model.rowCount();
    return true;
}

bool AppController::dispatchGroupChatContent(const QString &content, const QString &previewText)
{
    if (_current_group_id <= 0) {
        return false;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return false;
    }
    if (!isChatTransportReady()) {
        setTip("聊天连接未就绪，请重新登录", true);
        return false;
    }

    const QString msgId = QUuid::createUuid().toString();
    const qint64 createdAt = QDateTime::currentMSecsSinceEpoch();
    const DecodedMessageContent decoded = MessageContentCodec::decode(content);
    const QString plainText = decoded.content;
    const bool mentionAll = plainText.contains("@all", Qt::CaseInsensitive)
        || plainText.contains("@所有人");
    QJsonArray mentions;
    const QRegularExpression mentionIdRegex("@u([1-9][0-9]{8})");
    QRegularExpressionMatchIterator mentionIt = mentionIdRegex.globalMatch(plainText);
    while (mentionIt.hasNext()) {
        const QRegularExpressionMatch m = mentionIt.next();
        bool ok = false;
        const int uid = m.captured(1).toInt(&ok);
        if (ok && uid > 0) {
            mentions.append(uid);
        }
    }
    qint64 replyToServerMsgId = 0;
    if (!_reply_to_msg_id.isEmpty()) {
        auto groupInfo = _gateway.userMgr()->GetGroupById(_current_group_id);
        if (groupInfo) {
            for (const auto &one : groupInfo->_chat_msgs) {
                if (!one || one->_msg_id != _reply_to_msg_id) {
                    continue;
                }
                if (one->_server_msg_id > 0) {
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
    if (replyToServerMsgId > 0) {
        msgObj["reply_to_server_msg_id"] = replyToServerMsgId;
    }
    if (!decoded.fileName.isEmpty()) {
        msgObj["file_name"] = decoded.fileName;
    }
    if (!decoded.mimeType.isEmpty()) {
        msgObj["mime"] = decoded.mimeType;
    }
    if (decoded.sizeBytes > 0) {
        msgObj["size"] = static_cast<qint64>(decoded.sizeBytes);
    }

    QJsonObject payloadObj;
    payloadObj["fromuid"] = selfInfo->_uid;
    payloadObj["groupid"] = static_cast<qint64>(_current_group_id);
    payloadObj["msg"] = msgObj;
    const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_GROUP_CHAT_MSG_REQ, payload);

    const QString senderName = selfInfo->_nick.isEmpty() ? selfInfo->_name : selfInfo->_nick;
    auto msg = std::make_shared<TextChatData>(msgId,
                                              content,
                                              selfInfo->_uid,
                                              0,
                                              senderName,
                                              createdAt,
                                              _current_user_icon,
                                              "sending",
                                              0,
                                              0,
                                              replyToServerMsgId);
    _gateway.userMgr()->UpsertGroupChatMsg(_current_group_id, msg);
    _pending_group_msg_group_map.insert(msgId, _current_group_id);
    if (_group_cache_store.isReady()) {
        _group_cache_store.upsertMessages(selfInfo->_uid, _current_group_id, {msg});
    }
    _message_model.appendMessage(msg, selfInfo->_uid);

    const int pseudoUid = ConversationSyncService::resolveGroupDialogUid(_group_uid_map, _current_group_id);

    if (pseudoUid != 0) {
        const QString resolvedPreview = previewText.isEmpty() ? MessageContentCodec::toPreviewText(content) : previewText;
        ConversationSyncService::updateGroupPreview(_group_list_model,
                                                    _dialog_list_model,
                                                    pseudoUid,
                                                    resolvedPreview,
                                                    resolvedPreview,
                                                    createdAt);
        ConversationSyncService::clearGroupUnreadAndMention(_dialog_list_model, _dialog_mention_map, pseudoUid);
    }

    return true;
}

bool AppController::ensureCallTargetFromCurrentChat()
{
    if (hasCurrentContact()) {
        return true;
    }
    if (_current_group_id > 0) {
        return false;
    }
    if (_current_chat_uid <= 0) {
        return false;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(_current_chat_uid);
    if (!friendInfo) {
        return false;
    }

    setCurrentContact(friendInfo->_uid,
                      friendInfo->_name,
                      friendInfo->_nick,
                      friendInfo->_icon,
                      friendInfo->_back,
                      friendInfo->_sex,
                      friendInfo->_user_id);
    return hasCurrentContact();
}

void AppController::sendCallInvite(const QString &callType)
{
    startCallFlow(callType);
}
