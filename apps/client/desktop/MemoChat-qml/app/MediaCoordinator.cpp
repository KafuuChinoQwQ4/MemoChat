#include "AppCoordinators.h"

#include "AppController.h"
#include "LocalFilePickerService.h"
#include "MessageContentCodec.h"

MediaCoordinator::MediaCoordinator(AppController& controller)
    : _app(controller)
{
}

void MediaCoordinator::sendTextMessage(const QString& text)
{
    if (_app._chat_state.uid <= 0 && _app._group_state.currentId <= 0)
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
        _app.setTip("单条消息不能超过1024字符", true);
        return;
    }

    if (_app._group_state.currentId > 0)
    {
        if (!_app._dialog_state.replyToMsgId.isEmpty())
        {
            content = MessageContentCodec::encodeReplyText(content,
                                                           _app._dialog_state.replyToMsgId,
                                                           _app._dialog_state.replyTargetName,
                                                           _app._dialog_state.replyPreviewText);
        }
        if (!_app.dispatchGroupChatContent(content, QString()))
        {
            _app.setTip("群消息发送失败", true);
        }
        else
        {
            _app.cancelReplyMessage();
        }
        return;
    }

    if (!_app.dispatchChatContent(content, content))
    {
        _app.setTip("消息发送失败", true);
    }
}

void MediaCoordinator::sendCurrentComposerPayload(const QString& text)
{
    if (_app._chat_state.uid <= 0 && _app._group_state.currentId <= 0)
    {
        return;
    }
    if (_app._media_upload_state.inProgress)
    {
        _app.setTip("已有文件上传中，请稍后", true);
        return;
    }
    if (!_app._dialog_state.currentPendingAttachments.isEmpty())
    {
        _app._pending_send_state.dialogUid = _app.currentDialogUid();
        _app._pending_send_state.chatUid = _app._chat_state.uid;
        _app._pending_send_state.groupId = _app._group_state.currentId;
        _app._pending_send_state.queue = _app._dialog_state.currentPendingAttachments;
        _app._pending_send_state.totalCount = _app._pending_send_state.queue.size();
        _app.processPendingAttachmentQueue();
        return;
    }
    sendTextMessage(text);
}

void MediaCoordinator::sendImageMessage()
{
    if (_app._chat_state.uid <= 0 && _app._group_state.currentId <= 0)
    {
        return;
    }
    if (_app._media_upload_state.inProgress)
    {
        _app.setTip("已有文件上传中，请稍后", true);
        return;
    }
    QVariantList attachments;
    QString errorText;
    if (!LocalFilePickerService::pickImageUrls(&attachments, &errorText))
    {
        if (!errorText.isEmpty())
        {
            _app.setTip(errorText, true);
        }
        return;
    }
    _app.addPendingAttachments(attachments);
}

void MediaCoordinator::sendFileMessage()
{
    if (_app._chat_state.uid <= 0 && _app._group_state.currentId <= 0)
    {
        return;
    }
    if (_app._media_upload_state.inProgress)
    {
        _app.setTip("已有文件上传中，请稍后", true);
        return;
    }
    QVariantList attachments;
    QString errorText;
    if (!LocalFilePickerService::pickFileUrls(&attachments, &errorText))
    {
        if (!errorText.isEmpty())
        {
            _app.setTip(errorText, true);
        }
        return;
    }
    _app.addPendingAttachments(attachments);
}

void MediaCoordinator::removePendingAttachment(const QString& attachmentId)
{
    if (_app._media_upload_state.inProgress)
    {
        return;
    }
    _app.removePendingAttachmentById(attachmentId);
}

void MediaCoordinator::clearPendingAttachments()
{
    if (_app._media_upload_state.inProgress)
    {
        return;
    }
    const int dialogUid = _app.currentDialogUid();
    if (dialogUid == 0)
    {
        return;
    }
    _app._dialog_state.pendingAttachmentMap.remove(dialogUid);
    _app.setCurrentPendingAttachments(QVariantList());
}
