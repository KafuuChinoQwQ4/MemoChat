#include "ChatFeatureController.h"

#include <QFileInfo>
#include <QUrl>
#include <QUuid>

namespace
{
QVariantMap normalizePendingAttachment(const QVariantMap& source)
{
    QVariantMap attachment = source;
    const QString type = attachment.value(QStringLiteral("type")).toString().trimmed();
    const QString fileUrl = attachment.value(QStringLiteral("fileUrl")).toString().trimmed();
    if ((type != QStringLiteral("image") && type != QStringLiteral("file")) || fileUrl.isEmpty())
    {
        return {};
    }

    attachment.insert(QStringLiteral("type"), type);
    attachment.insert(QStringLiteral("fileUrl"), fileUrl);
    if (attachment.value(QStringLiteral("attachmentId")).toString().trimmed().isEmpty())
    {
        attachment.insert(QStringLiteral("attachmentId"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
    if (attachment.value(QStringLiteral("fileName")).toString().trimmed().isEmpty())
    {
        const QFileInfo fileInfo(QUrl(fileUrl).toLocalFile());
        attachment.insert(QStringLiteral("fileName"), fileInfo.fileName());
    }
    return attachment;
}
} // namespace

ChatPendingAttachmentCommandResult
ChatFeatureController::addPendingAttachmentsForDialog(int dialogUid, const QVariantList& attachments)
{
    ChatPendingAttachmentCommandResult result;
    result.dialogUid = dialogUid;
    result.valid = dialogUid != 0;
    if (!result.valid)
    {
        return result;
    }

    result.attachments = _dialogRuntimeState.pendingAttachmentMap.value(dialogUid);
    for (const QVariant& entry : attachments)
    {
        const QVariantMap normalized = normalizePendingAttachment(entry.toMap());
        if (!normalized.isEmpty())
        {
            result.attachments.push_back(normalized);
            result.changed = true;
        }
    }
    if (result.changed)
    {
        setPendingAttachmentsForDialog(dialogUid, result.attachments);
    }
    return result;
}

ChatPendingAttachmentCommandResult
ChatFeatureController::removePendingAttachmentByIdForDialog(int dialogUid, const QString& attachmentId)
{
    ChatPendingAttachmentCommandResult result;
    result.dialogUid = dialogUid;
    result.valid = dialogUid != 0 && !attachmentId.trimmed().isEmpty();
    if (!result.valid)
    {
        return result;
    }

    result.attachments = _dialogRuntimeState.pendingAttachmentMap.value(dialogUid);
    for (int index = result.attachments.size() - 1; index >= 0; --index)
    {
        const QVariantMap attachment = result.attachments.at(index).toMap();
        if (attachment.value(QStringLiteral("attachmentId")).toString() == attachmentId)
        {
            result.attachments.removeAt(index);
            result.changed = true;
            break;
        }
    }
    if (!result.changed)
    {
        return result;
    }

    setPendingAttachmentsForDialog(dialogUid, result.attachments);
    return result;
}

bool ChatFeatureController::beginPendingAttachmentSend(int dialogUid,
                                                       int chatUid,
                                                       qint64 groupId,
                                                       const QVariantList& attachments)
{
    _pendingSendQueueState.reset();
    if (dialogUid == 0 || attachments.isEmpty())
    {
        return false;
    }

    _pendingSendQueueState.dialogUid = dialogUid;
    _pendingSendQueueState.chatUid = chatUid;
    _pendingSendQueueState.groupId = groupId;
    _pendingSendQueueState.queue = attachments;
    _pendingSendQueueState.totalCount = attachments.size();
    return true;
}

bool ChatFeatureController::beginSinglePendingAttachmentSend(int dialogUid,
                                                             int chatUid,
                                                             qint64 groupId,
                                                             const QVariantMap& attachment)
{
    if (attachment.isEmpty())
    {
        resetPendingAttachmentSendQueue();
        return false;
    }
    return beginPendingAttachmentSend(dialogUid, chatUid, groupId, QVariantList{attachment});
}

bool ChatFeatureController::pendingAttachmentSendQueueEmpty() const
{
    return _pendingSendQueueState.queue.isEmpty();
}

ChatPendingSendQueueSnapshot ChatFeatureController::pendingAttachmentSendSnapshot() const
{
    return _pendingSendQueueState.snapshot();
}

QVariantMap ChatFeatureController::currentPendingAttachmentToSend() const
{
    return _pendingSendQueueState.snapshot().currentAttachment;
}

int ChatFeatureController::pendingAttachmentSendDialogUid() const
{
    return _pendingSendQueueState.dialogUid;
}

int ChatFeatureController::pendingAttachmentSendChatUid() const
{
    return _pendingSendQueueState.chatUid;
}

qint64 ChatFeatureController::pendingAttachmentSendGroupId() const
{
    return _pendingSendQueueState.groupId;
}

int ChatFeatureController::pendingAttachmentSendTotalCount() const
{
    return _pendingSendQueueState.totalCount;
}

int ChatFeatureController::pendingAttachmentSendRemainingCount() const
{
    return _pendingSendQueueState.queue.size();
}

int ChatFeatureController::pendingAttachmentSendCurrentIndex() const
{
    if (_pendingSendQueueState.queue.isEmpty())
    {
        return 0;
    }
    return qMax(1, _pendingSendQueueState.totalCount - _pendingSendQueueState.queue.size() + 1);
}

bool ChatFeatureController::advancePendingAttachmentSendQueue()
{
    if (!_pendingSendQueueState.queue.isEmpty())
    {
        _pendingSendQueueState.queue.removeFirst();
    }
    if (_pendingSendQueueState.queue.isEmpty())
    {
        _pendingSendQueueState.reset();
        return false;
    }
    return true;
}

void ChatFeatureController::resetPendingAttachmentSendQueue()
{
    _pendingSendQueueState.reset();
}

UploadedAttachmentDispatchResult
ChatFeatureController::dispatchUploadedAttachment(const QVariantMap& attachment,
                                                  const UploadedMediaInfo& uploaded,
                                                  const UploadedAttachmentDestination& destination)
{
    UploadedAttachmentDispatchPort port;
    port.dispatchCurrentPrivateContent = _uploadedAttachmentDispatchPort.dispatchCurrentPrivateContent;
    port.dispatchCurrentGroupContent = _uploadedAttachmentDispatchPort.dispatchCurrentGroupContent;
    port.dispatchPrivatePacket = _uploadedAttachmentDispatchPort.dispatchPrivatePacket;
    port.dispatchGroupPayload = _uploadedAttachmentDispatchPort.dispatchGroupPayload;
    return UploadedAttachmentDispatchService::dispatch(attachment, uploaded, destination, port);
}
