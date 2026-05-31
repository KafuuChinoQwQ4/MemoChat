#include "AppController.h"

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

void AppController::startMediaUploadAndSend(const QString& fileUrl,
                                            const QString& mediaType,
                                            const QString& fallbackName)
{
    Q_UNUSED(fallbackName);
    QVariantMap attachment;
    attachment.insert(QStringLiteral("attachmentId"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    attachment.insert(QStringLiteral("fileUrl"), fileUrl);
    attachment.insert(QStringLiteral("type"), mediaType);
    attachment.insert(QStringLiteral("fileName"), QFileInfo(QUrl(fileUrl).toLocalFile()).fileName());
    _pending_send_state.dialogUid = currentDialogUid();
    _pending_send_state.chatUid = _chat_state.uid;
    _pending_send_state.groupId = _group_state.currentId;
    _pending_send_state.queue = {attachment};
    _pending_send_state.totalCount = 1;
    processPendingAttachmentQueue();
}

void AppController::addPendingAttachments(const QVariantList& attachments)
{
    const int dialogUid = currentDialogUid();
    if (dialogUid == 0 || attachments.isEmpty())
    {
        return;
    }

    QVariantList nextList = _dialog_state.pendingAttachmentMap.value(dialogUid);
    for (const QVariant& entry : attachments)
    {
        const QVariantMap normalized = normalizePendingAttachment(entry.toMap());
        if (!normalized.isEmpty())
        {
            nextList.push_back(normalized);
        }
    }
    _dialog_state.pendingAttachmentMap.insert(dialogUid, nextList);
    if (dialogUid == currentDialogUid())
    {
        setCurrentPendingAttachments(nextList);
    }
}

void AppController::removePendingAttachmentById(const QString& attachmentId, int dialogUid)
{
    const int targetDialogUid = dialogUid == 0 ? currentDialogUid() : dialogUid;
    if (targetDialogUid == 0 || attachmentId.trimmed().isEmpty())
    {
        return;
    }

    QVariantList attachments = _dialog_state.pendingAttachmentMap.value(targetDialogUid);
    bool removed = false;
    for (int index = attachments.size() - 1; index >= 0; --index)
    {
        const QVariantMap attachment = attachments.at(index).toMap();
        if (attachment.value(QStringLiteral("attachmentId")).toString() == attachmentId)
        {
            attachments.removeAt(index);
            removed = true;
            break;
        }
    }
    if (!removed)
    {
        return;
    }
    if (attachments.isEmpty())
    {
        _dialog_state.pendingAttachmentMap.remove(targetDialogUid);
    }
    else
    {
        _dialog_state.pendingAttachmentMap.insert(targetDialogUid, attachments);
    }
    if (targetDialogUid == currentDialogUid())
    {
        setCurrentPendingAttachments(attachments);
    }
}

void AppController::setCurrentPendingAttachments(const QVariantList& attachments)
{
    if (_dialog_state.currentPendingAttachments == attachments)
    {
        return;
    }
    _dialog_state.currentPendingAttachments = attachments;
    emit currentPendingAttachmentsChanged();
}
