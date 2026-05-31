#include "AppController.h"

#include "IChatTransport.h"
#include "MessageContentCodec.h"
#include "usermgr.h"

#include <QDateTime>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QtConcurrent>

void AppController::processPendingAttachmentQueue()
{
    if (_pending_send_state.queue.isEmpty())
    {
        _pending_send_state.reset();
        setMediaUploadInProgress(false);
        setMediaUploadProgressText(QString());
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        setTip("用户状态异常，请重新登录", true);
        _pending_send_state.reset();
        setMediaUploadInProgress(false);
        setMediaUploadProgressText(QString());
        return;
    }
    if (_pending_login_state.token.trimmed().isEmpty())
    {
        setTip("登录态失效，请重新登录", true);
        _pending_send_state.reset();
        setMediaUploadInProgress(false);
        setMediaUploadProgressText(QString());
        return;
    }

    setMediaUploadInProgress(true);
    const QVariantMap currentAttachment = _pending_send_state.queue.first().toMap();
    const int attachmentIndex = qMax(1, _pending_send_state.totalCount - _pending_send_state.queue.size() + 1);
    const QString uploadType = currentAttachment.value(QStringLiteral("type")).toString();
    const QString uploadFileUrl = currentAttachment.value(QStringLiteral("fileUrl")).toString();
    const QString uploadFallbackName = currentAttachment.value(QStringLiteral("fileName")).toString();
    setMediaUploadProgressText(
        QString("正在发送附件 %1/%2").arg(attachmentIndex).arg(qMax(1, _pending_send_state.totalCount)));

    auto* watcher = new QFutureWatcher<MediaUploadResult>(this);
    const int uploadUid = selfInfo->_uid;
    const QString token = _pending_login_state.token;
    const int targetDialogUid = _pending_send_state.dialogUid;
    const qint64 targetGroupId = _pending_send_state.groupId;
    const int targetChatUid = _pending_send_state.chatUid;

    const auto future = QtConcurrent::run(
        [this, uploadFileUrl, uploadType, uploadFallbackName, uploadUid, token, attachmentIndex]()
        {
            MediaUploadRequest request;
            request.localFileUrl = uploadFileUrl;
            request.mediaType = uploadType;
            request.uid = uploadUid;
            request.token = token;
            request.fallbackName = uploadFallbackName;

            MediaUploadResult result = MediaUploadService::uploadLocalFile(
                request,
                [this, attachmentIndex](int percent, const QString& stage)
                {
                    QMetaObject::invokeMethod(
                        this,
                        [this, percent, stage, attachmentIndex]()
                        {
                            const int bounded = qBound(0, percent, 100);
                            setMediaUploadProgressText(QString("正在发送附件 %1/%2 %3 %4%")
                                                           .arg(attachmentIndex)
                                                           .arg(qMax(1, _pending_send_state.totalCount))
                                                           .arg(stage)
                                                           .arg(bounded));
                        },
                        Qt::QueuedConnection);
                });
            return result;
        });

    connect(watcher,
            &QFutureWatcher<MediaUploadResult>::finished,
            this,
            [this, watcher, currentAttachment, uploadType, targetDialogUid, targetGroupId, targetChatUid]()
            {
                const MediaUploadResult result = watcher->result();
                watcher->deleteLater();

                if (!result.ok)
                {
                    const QString fallbackErr =
                        (uploadType == QStringLiteral("image")) ? "图片上传失败" : "文件上传失败";
                    _pending_send_state.reset();
                    setMediaUploadInProgress(false);
                    setMediaUploadProgressText(QString());
                    setTip(result.errorText.isEmpty() ? fallbackErr : result.errorText, true);
                    return;
                }

                if (!sendUploadedAttachmentToDialog(currentAttachment,
                                                    result.info,
                                                    targetDialogUid,
                                                    targetChatUid,
                                                    targetGroupId))
                {
                    _pending_send_state.reset();
                    setMediaUploadInProgress(false);
                    setMediaUploadProgressText(QString());
                    return;
                }

                removePendingAttachmentById(currentAttachment.value(QStringLiteral("attachmentId")).toString(),
                                                                    targetDialogUid);
                if (!_pending_send_state.queue.isEmpty())
                {
                    _pending_send_state.queue.removeFirst();
                }
                if (_pending_send_state.queue.isEmpty())
                {
                    _pending_send_state.reset();
                    setMediaUploadInProgress(false);
                    setMediaUploadProgressText(QString());
                    return;
                }
                processPendingAttachmentQueue();
            });
    watcher->setFuture(future);
}

bool AppController::sendUploadedAttachmentToDialog(const QVariantMap& attachment,
                                                   const UploadedMediaInfo& uploaded,
                                                   int dialogUid,
                                                   int chatUid,
                                                   qint64 groupId)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        setTip("用户状态异常，请重新登录", true);
        return false;
    }

    const QString attachmentType = attachment.value(QStringLiteral("type")).toString();
    const bool sameDialog = (currentDialogUid() == dialogUid);
    if (attachmentType == QStringLiteral("image"))
    {
        const QString encoded = MessageContentCodec::encodeImage(uploaded.remoteUrl);
        if (groupId > 0)
        {
            if (sameDialog)
            {
                if (!dispatchGroupChatContent(encoded, "[图片]"))
                {
                    setTip("群图片发送失败", true);
                    return false;
                }
            }
            else
            {
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
                _gateway.chatTransport()->slot_send_data(ReqId::ID_GROUP_CHAT_MSG_REQ,
                                                         QJsonDocument(payloadObj).toJson(QJsonDocument::Compact));
            }
            return true;
        }

        if (sameDialog)
        {
            if (!dispatchChatContent(encoded, "[图片]"))
            {
                setTip("图片发送失败", true);
                return false;
            }
        }
        else
        {
            OutgoingChatPacket packet;
            packet.fromUid = selfInfo->_uid;
            packet.toUid = chatUid;
            packet.msgId = QUuid::createUuid().toString();
            packet.content = encoded;
            packet.createdAt = QDateTime::currentMSecsSinceEpoch();
            QString errText;
            if (!_chat_controller.dispatchChatText(packet, &errText))
            {
                setTip(errText.isEmpty() ? "图片发送失败" : errText, true);
                return false;
            }
        }
        return true;
    }

    const QString fallbackName = attachment.value(QStringLiteral("fileName")).toString();
    const QString remoteName = uploaded.fileName.isEmpty() ? fallbackName : uploaded.fileName;
    const QString encoded =
        MessageContentCodec::encodeFile(uploaded.remoteUrl, remoteName, uploaded.mimeType, uploaded.sizeBytes);
    const QString preview = remoteName.isEmpty() ? "[文件]" : QString("[文件] %1").arg(remoteName);
    if (groupId > 0)
    {
        if (sameDialog)
        {
            if (!dispatchGroupChatContent(encoded, preview))
            {
                setTip("群文件发送失败", true);
                return false;
            }
        }
        else
        {
            const QString msgId = QUuid::createUuid().toString();
            const DecodedMessageContent decoded = MessageContentCodec::decode(encoded);
            QJsonObject msgObj;
            msgObj["msgid"] = msgId;
            msgObj["content"] = encoded;
            msgObj["msgtype"] = decoded.type.isEmpty() ? "text" : decoded.type;
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
            payloadObj["groupid"] = static_cast<qint64>(groupId);
            payloadObj["msg"] = msgObj;
            _gateway.chatTransport()->slot_send_data(ReqId::ID_GROUP_CHAT_MSG_REQ,
                                                     QJsonDocument(payloadObj).toJson(QJsonDocument::Compact));
        }
        return true;
    }

    if (sameDialog)
    {
        if (!dispatchChatContent(encoded, preview))
        {
            setTip("文件发送失败", true);
            return false;
        }
    }
    else
    {
        OutgoingChatPacket packet;
        packet.fromUid = selfInfo->_uid;
        packet.toUid = chatUid;
        packet.msgId = QUuid::createUuid().toString();
        packet.content = encoded;
        packet.createdAt = QDateTime::currentMSecsSinceEpoch();
        QString errText;
        if (!_chat_controller.dispatchChatText(packet, &errText))
        {
            setTip(errText.isEmpty() ? "文件发送失败" : errText, true);
            return false;
        }
    }
    return true;
}
