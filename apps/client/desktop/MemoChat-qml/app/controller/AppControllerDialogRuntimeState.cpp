#include "AppController.h"

void AppController::setCurrentGroup(qint64 groupId, const QString& name, const QString& groupCode)
{
    const QString normalizedName = (groupId > 0) ? name : QString();
    const QString normalizedCode = (groupId > 0) ? groupCode : QString();
    if (_group_state.currentId == groupId && _group_state.currentName == normalizedName &&
        _group_state.currentCode == normalizedCode)
    {
        return;
    }

    _group_state.currentId = groupId;
    _group_state.currentName = normalizedName;
    _group_state.currentCode = normalizedCode;
    syncGroupControllerState();
    emit currentGroupChanged();
}

void AppController::setMediaUploadInProgress(bool inProgress)
{
    if (_media_upload_state.inProgress == inProgress)
    {
        return;
    }
    _media_upload_state.inProgress = inProgress;
    emit mediaUploadStateChanged();
}

void AppController::setMediaUploadProgressText(const QString& text)
{
    if (_media_upload_state.progressText == text)
    {
        return;
    }
    _media_upload_state.progressText = text;
    emit mediaUploadStateChanged();
}

void AppController::setCurrentDraftText(const QString& text)
{
    if (_dialog_state.currentDraftText == text)
    {
        return;
    }
    _dialog_state.currentDraftText = text;
    emit currentDraftTextChanged();
}

void AppController::setCurrentDialogPinned(bool pinned)
{
    if (_dialog_state.currentPinned == pinned)
    {
        return;
    }
    _dialog_state.currentPinned = pinned;
    emit currentDialogPinnedChanged();
}

void AppController::setCurrentDialogMuted(bool muted)
{
    if (_dialog_state.currentMuted == muted)
    {
        return;
    }
    _dialog_state.currentMuted = muted;
    emit currentDialogMutedChanged();
}

void AppController::setPendingReplyContext(const QString& msgId, const QString& senderName, const QString& previewText)
{
    const QString normalizedMsgId = msgId.trimmed();
    const QString normalizedSender = senderName.trimmed();
    QString normalizedPreview = previewText.trimmed();
    if (normalizedPreview.length() > 120)
    {
        normalizedPreview = normalizedPreview.left(120);
    }
    if (_dialog_state.replyToMsgId == normalizedMsgId && _dialog_state.replyTargetName == normalizedSender &&
        _dialog_state.replyPreviewText == normalizedPreview)
    {
        return;
    }
    _dialog_state.replyToMsgId = normalizedMsgId;
    _dialog_state.replyTargetName = normalizedSender;
    _dialog_state.replyPreviewText = normalizedPreview;
    emit pendingReplyChanged();
}
