#include "ChatFeatureControllerDialogRuntimeInternal.h"

using namespace memochat::chat::dialog_runtime;

QString ChatFeatureController::dialogMetaActionText(int reqId)
{
    if (reqId == kSyncDraftResponseId)
    {
        return QStringLiteral("同步草稿");
    }
    if (reqId == kPinDialogResponseId)
    {
        return QStringLiteral("置顶会话");
    }
    return {};
}

QString ChatFeatureController::currentDraftText() const
{
    return _dialogRuntimeState.currentDraftText;
}

QVariantList ChatFeatureController::currentPendingAttachments() const
{
    return _dialogRuntimeState.currentPendingAttachments;
}

bool ChatFeatureController::hasPendingAttachments() const
{
    return !_dialogRuntimeState.currentPendingAttachments.isEmpty();
}

bool ChatFeatureController::currentDialogPinned() const
{
    return _dialogRuntimeState.currentPinned;
}

bool ChatFeatureController::currentDialogMuted() const
{
    return _dialogRuntimeState.currentMuted;
}

bool ChatFeatureController::hasPendingReply() const
{
    return !_dialogRuntimeState.replyToMsgId.isEmpty();
}

QString ChatFeatureController::replyTargetName() const
{
    return _dialogRuntimeState.replyTargetName;
}

QString ChatFeatureController::replyPreviewText() const
{
    return _dialogRuntimeState.replyPreviewText;
}

QString ChatFeatureController::replyToMsgId() const
{
    return _dialogRuntimeState.replyToMsgId;
}

bool ChatFeatureController::setCurrentDraftText(const QString& text)
{
    if (_dialogRuntimeState.currentDraftText == text)
    {
        return false;
    }
    _dialogRuntimeState.currentDraftText = text;
    return true;
}

bool ChatFeatureController::setCurrentPendingAttachments(const QVariantList& attachments)
{
    if (_dialogRuntimeState.currentPendingAttachments == attachments)
    {
        return false;
    }
    _dialogRuntimeState.currentPendingAttachments = attachments;
    return true;
}

bool ChatFeatureController::setCurrentDialogPinned(bool pinned)
{
    if (_dialogRuntimeState.currentPinned == pinned)
    {
        return false;
    }
    _dialogRuntimeState.currentPinned = pinned;
    return true;
}

bool ChatFeatureController::setCurrentDialogMuted(bool muted)
{
    if (_dialogRuntimeState.currentMuted == muted)
    {
        return false;
    }
    _dialogRuntimeState.currentMuted = muted;
    return true;
}

bool ChatFeatureController::setPendingReplyContext(const QString& msgId,
                                                   const QString& senderName,
                                                   const QString& previewText)
{
    const QString normalizedMsgId = msgId.trimmed();
    const QString normalizedSender = senderName.trimmed();
    const QString normalizedPreview = normalizeReplyPreview(previewText);
    if (_dialogRuntimeState.replyToMsgId == normalizedMsgId &&
        _dialogRuntimeState.replyTargetName == normalizedSender &&
        _dialogRuntimeState.replyPreviewText == normalizedPreview)
    {
        return false;
    }
    _dialogRuntimeState.replyToMsgId = normalizedMsgId;
    _dialogRuntimeState.replyTargetName = normalizedSender;
    _dialogRuntimeState.replyPreviewText = normalizedPreview;
    return true;
}

bool ChatFeatureController::updateLastEmittedDialogUid(int dialogUid)
{
    if (_dialogRuntimeState.lastEmittedUid == dialogUid)
    {
        return false;
    }
    _dialogRuntimeState.lastEmittedUid = dialogUid;
    return true;
}

ChatDialogRuntimeSnapshot ChatFeatureController::snapshotForDialog(int dialogUid) const
{
    ChatDialogRuntimeSnapshot snapshot;
    snapshot.dialogUid = dialogUid;
    snapshot.hasDialog = dialogUid != 0;
    if (!snapshot.hasDialog)
    {
        return snapshot;
    }

    snapshot.draftText = _dialogRuntimeState.draftMap.value(dialogUid);
    snapshot.pendingAttachments = _dialogRuntimeState.pendingAttachmentMap.value(dialogUid);
    snapshot.pinned = _dialogRuntimeState.localPinnedSet.contains(dialogUid) ||
                      _dialogRuntimeState.serverPinnedMap.value(dialogUid, 0) > 0;
    snapshot.muted = _dialogRuntimeState.serverMuteMap.value(dialogUid, 0) > 0;
    return snapshot;
}
