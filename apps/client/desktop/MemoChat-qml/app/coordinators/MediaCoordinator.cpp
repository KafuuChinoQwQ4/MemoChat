#include "AppCoordinators.h"

#include "LocalFilePickerService.h"
#include "MessageContentCodec.h"

MediaCoordinator::MediaCoordinator(MediaSendPort port)
    : _port(std::move(port))
{
}

void MediaCoordinator::sendTextMessage(const QString& text)
{
    const MediaSendSnapshot snapshot = _port.snapshot();
    if (snapshot.currentChatUid <= 0 && snapshot.currentGroupId <= 0)
    {
        return;
    }

    QString content = text;
    if (content.trimmed().isEmpty())
    {
        return;
    }
    if (content.size() > 1024)
    {
        _port.setTip("单条消息不能超过1024字符", true);
        return;
    }

    if (snapshot.currentGroupId > 0)
    {
        if (!snapshot.replyToMsgId.isEmpty())
        {
            content = MessageContentCodec::encodeReplyText(content,
                                                           snapshot.replyToMsgId,
                                                           snapshot.replyTargetName,
                                                           snapshot.replyPreviewText);
        }
        if (!_port.dispatchGroupContent(content, QString()))
        {
            _port.setTip("群消息发送失败", true);
        }
        else
        {
            _port.cancelReply();
        }
        return;
    }

    if (!_port.dispatchPrivateContent(content, content))
    {
        _port.setTip("消息发送失败", true);
    }
}

void MediaCoordinator::sendCurrentComposerPayload(const QString& text)
{
    const MediaSendSnapshot snapshot = _port.snapshot();
    if (snapshot.currentChatUid <= 0 && snapshot.currentGroupId <= 0)
    {
        return;
    }
    if (snapshot.uploadInProgress)
    {
        _port.setTip("已有文件上传中，请稍后", true);
        return;
    }
    if (!snapshot.pendingAttachments.isEmpty())
    {
        _port.beginPendingAttachmentSend(snapshot.currentDialogUid,
                                         snapshot.currentChatUid,
                                         snapshot.currentGroupId,
                                         snapshot.pendingAttachments);
        _port.startPendingAttachmentRunner();
        return;
    }
    sendTextMessage(text);
}

void MediaCoordinator::sendImageMessage()
{
    const MediaSendSnapshot snapshot = _port.snapshot();
    if (snapshot.currentChatUid <= 0 && snapshot.currentGroupId <= 0)
    {
        return;
    }
    if (snapshot.uploadInProgress)
    {
        _port.setTip("已有文件上传中，请稍后", true);
        return;
    }
    QVariantList attachments;
    QString errorText;
    if (!LocalFilePickerService::pickImageUrls(&attachments, &errorText))
    {
        if (!errorText.isEmpty())
        {
            _port.setTip(errorText, true);
        }
        return;
    }
    _port.addPendingAttachments(attachments);
}

void MediaCoordinator::sendFileMessage()
{
    const MediaSendSnapshot snapshot = _port.snapshot();
    if (snapshot.currentChatUid <= 0 && snapshot.currentGroupId <= 0)
    {
        return;
    }
    if (snapshot.uploadInProgress)
    {
        _port.setTip("已有文件上传中，请稍后", true);
        return;
    }
    QVariantList attachments;
    QString errorText;
    if (!LocalFilePickerService::pickFileUrls(&attachments, &errorText))
    {
        if (!errorText.isEmpty())
        {
            _port.setTip(errorText, true);
        }
        return;
    }
    _port.addPendingAttachments(attachments);
}

void MediaCoordinator::removePendingAttachment(const QString& attachmentId)
{
    if (_port.snapshot().uploadInProgress)
    {
        return;
    }
    _port.removePendingAttachmentById(attachmentId);
}

void MediaCoordinator::clearPendingAttachments()
{
    const MediaSendSnapshot snapshot = _port.snapshot();
    if (snapshot.uploadInProgress)
    {
        return;
    }
    const int dialogUid = snapshot.currentDialogUid;
    if (dialogUid == 0)
    {
        return;
    }
    _port.clearPendingAttachmentsForDialog(dialogUid);
    _port.setCurrentPendingAttachments(QVariantList());
}
