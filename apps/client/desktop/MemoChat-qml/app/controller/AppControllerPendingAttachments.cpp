#include "AppController.h"

void AppController::addPendingAttachments(const QVariantList& attachments)
{
    const int dialogUid = currentDialogUid();
    if (dialogUid == 0 || attachments.isEmpty())
    {
        return;
    }

    const ChatPendingAttachmentCommandResult result =
        _features.chatFeatureController.addPendingAttachmentsForDialog(dialogUid, attachments);
    if (!result.valid || !result.changed)
    {
        return;
    }
    if (dialogUid == currentDialogUid())
    {
        setCurrentPendingAttachments(result.attachments);
    }
}

void AppController::removePendingAttachmentById(const QString& attachmentId, int dialogUid)
{
    const int targetDialogUid = dialogUid == 0 ? currentDialogUid() : dialogUid;
    if (targetDialogUid == 0 || attachmentId.trimmed().isEmpty())
    {
        return;
    }

    const ChatPendingAttachmentCommandResult result =
        _features.chatFeatureController.removePendingAttachmentByIdForDialog(targetDialogUid, attachmentId);
    if (!result.valid || !result.changed)
    {
        return;
    }
    if (targetDialogUid == currentDialogUid())
    {
        setCurrentPendingAttachments(result.attachments);
    }
}

void AppController::setCurrentPendingAttachments(const QVariantList& attachments)
{
    if (!_features.chatFeatureController.setCurrentPendingAttachments(attachments))
    {
        return;
    }
    emit currentPendingAttachmentsChanged();
}
