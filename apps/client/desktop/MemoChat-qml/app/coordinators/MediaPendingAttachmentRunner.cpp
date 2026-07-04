#include "MediaPendingAttachmentRunner.h"

#include <QFutureWatcher>
#include <QMetaObject>
#include <QtConcurrent>
#include <QtGlobal>
#include <utility>

namespace
{
QString uploadFailureFallback(const QString& attachmentType)
{
    return attachmentType == QStringLiteral("image") ? QStringLiteral("图片上传失败") : QStringLiteral("文件上传失败");
}

MediaUploadRequest uploadRequestFrom(const QVariantMap& attachment,
                                     const ChatPendingSendQueueSnapshot& queueSnapshot,
                                     const MediaPendingAttachmentSessionSnapshot& session)
{
    MediaUploadRequest request;
    request.localFileUrl = attachment.value(QStringLiteral("fileUrl")).toString();
    request.mediaType = attachment.value(QStringLiteral("type")).toString();
    request.uid = session.selfUid;
    request.token = session.authToken;
    request.fallbackName = attachment.value(QStringLiteral("fileName")).toString();
    if (queueSnapshot.groupId > 0)
    {
        request.grantGroupId = queueSnapshot.groupId;
    }
    else if (queueSnapshot.chatUid > 0 && queueSnapshot.chatUid != session.selfUid)
    {
        request.grantUids.append(queueSnapshot.chatUid);
    }
    return request;
}
} // namespace

MediaPendingAttachmentRunner::MediaPendingAttachmentRunner(MediaPendingAttachmentRunnerPort port, QObject* parent)
    : QObject(parent)
    , _port(std::move(port))
{
}

void MediaPendingAttachmentRunner::setProgressText(const MediaPendingAttachmentRunnerPort& port, const QString& text)
{
    if (port.setProgressText)
    {
        port.setProgressText(text);
    }
}

void MediaPendingAttachmentRunner::resetUploadState(const MediaPendingAttachmentRunnerPort& port)
{
    if (port.setUploadInProgress)
    {
        port.setUploadInProgress(false);
    }
    setProgressText(port, QString());
}

void MediaPendingAttachmentRunner::start()
{
    if (_running)
    {
        return;
    }
    _running = true;
    processNextAsync();
}

MediaPendingAttachmentProcessResult
MediaPendingAttachmentRunner::processNext(const MediaPendingAttachmentRunnerPort& port)
{
    const ChatPendingSendQueueSnapshot queueSnapshot =
        port.queueSnapshot ? port.queueSnapshot() : ChatPendingSendQueueSnapshot();
    if (!queueSnapshot.hasCurrent)
    {
        if (port.resetQueue)
        {
            port.resetQueue();
        }
        resetUploadState(port);
        return {};
    }

    const MediaPendingAttachmentSessionSnapshot session =
        port.sessionSnapshot ? port.sessionSnapshot() : MediaPendingAttachmentSessionSnapshot();
    if (session.selfUid <= 0 || session.authToken.trimmed().isEmpty())
    {
        if (port.setTip)
        {
            port.setTip(QStringLiteral("用户状态异常，请重新登录"), true);
        }
        if (port.resetQueue)
        {
            port.resetQueue();
        }
        resetUploadState(port);
        return {};
    }

    if (port.setUploadInProgress)
    {
        port.setUploadInProgress(true);
    }

    const QVariantMap currentAttachment = queueSnapshot.currentAttachment;
    const int attachmentIndex = qMax(1, queueSnapshot.currentIndex);
    const int totalCount = qMax(1, queueSnapshot.totalCount);
    const QString attachmentType = currentAttachment.value(QStringLiteral("type")).toString();
    setProgressText(port, QStringLiteral("正在发送附件 %1/%2").arg(attachmentIndex).arg(totalCount));

    const MediaUploadRequest request = uploadRequestFrom(currentAttachment, queueSnapshot, session);
    MediaUploadResult uploadResult;
    if (port.uploadLocalFile)
    {
        uploadResult = port.uploadLocalFile(request,
                                            [port, attachmentIndex, totalCount](int percent, const QString& stage)
                                            {
                                                const int bounded = qBound(0, percent, 100);
                                                MediaPendingAttachmentRunner::setProgressText(
                                                    port,
                                                    QStringLiteral("正在发送附件 %1/%2 %3 %4%")
                                                                       .arg(attachmentIndex)
                                                                       .arg(totalCount)
                                                                       .arg(stage)
                                                                       .arg(bounded));
                                            });
    }
    else
    {
        uploadResult.errorText = uploadFailureFallback(attachmentType);
    }

    if (!uploadResult.ok)
    {
        if (port.resetQueue)
        {
            port.resetQueue();
        }
        resetUploadState(port);
        if (port.setTip)
        {
            port.setTip(uploadResult.errorText.isEmpty() ? uploadFailureFallback(attachmentType)
                                                         : uploadResult.errorText,
                        true);
        }
        return {true, false};
    }

    UploadedAttachmentDestination destination;
    destination.selfUid = session.selfUid;
    destination.dialogUid = queueSnapshot.dialogUid;
    destination.currentDialogUid = port.currentDialogUid ? port.currentDialogUid() : 0;
    destination.chatUid = queueSnapshot.chatUid;
    destination.groupId = queueSnapshot.groupId;

    UploadedAttachmentDispatchResult dispatchResult;
    if (port.dispatchUploadedAttachment)
    {
        dispatchResult = port.dispatchUploadedAttachment(currentAttachment, uploadResult.info, destination);
    }
    else
    {
        dispatchResult.errorText = QStringLiteral("附件发送失败");
    }

    if (!dispatchResult.success)
    {
        if (port.resetQueue)
        {
            port.resetQueue();
        }
        resetUploadState(port);
        if (port.setTip && !dispatchResult.errorText.isEmpty())
        {
            port.setTip(dispatchResult.errorText, true);
        }
        return {true, false};
    }

    if (port.removePendingAttachmentById)
    {
        port.removePendingAttachmentById(currentAttachment.value(QStringLiteral("attachmentId")).toString(),
                                                                 queueSnapshot.dialogUid);
    }

    const bool hasNext = port.advanceQueue ? port.advanceQueue() : false;
    if (!hasNext)
    {
        resetUploadState(port);
        return {true, false};
    }

    return {true, true};
}

void MediaPendingAttachmentRunner::processAll(const MediaPendingAttachmentRunnerPort& port)
{
    while (processNext(port).shouldContinue)
    {
    }
}

void MediaPendingAttachmentRunner::processNextAsync()
{
    const ChatPendingSendQueueSnapshot queueSnapshot =
        _port.queueSnapshot ? _port.queueSnapshot() : ChatPendingSendQueueSnapshot();
    if (!queueSnapshot.hasCurrent)
    {
        if (_port.resetQueue)
        {
            _port.resetQueue();
        }
        resetUploadState(_port);
        _running = false;
        emit finished();
        return;
    }

    const MediaPendingAttachmentSessionSnapshot session =
        _port.sessionSnapshot ? _port.sessionSnapshot() : MediaPendingAttachmentSessionSnapshot();
    if (session.selfUid <= 0 || session.authToken.trimmed().isEmpty())
    {
        if (_port.setTip)
        {
            _port.setTip(QStringLiteral("用户状态异常，请重新登录"), true);
        }
        if (_port.resetQueue)
        {
            _port.resetQueue();
        }
        resetUploadState(_port);
        _running = false;
        emit finished();
        return;
    }

    if (_port.setUploadInProgress)
    {
        _port.setUploadInProgress(true);
    }

    const QVariantMap currentAttachment = queueSnapshot.currentAttachment;
    const int attachmentIndex = qMax(1, queueSnapshot.currentIndex);
    const int totalCount = qMax(1, queueSnapshot.totalCount);
    setProgressText(_port, QStringLiteral("正在发送附件 %1/%2").arg(attachmentIndex).arg(totalCount));

    const MediaUploadRequest request = uploadRequestFrom(currentAttachment, queueSnapshot, session);
    auto* watcher = new QFutureWatcher<MediaUploadResult>(this);
    connect(watcher,
            &QFutureWatcher<MediaUploadResult>::finished,
            this,
            [this, watcher, queueSnapshot, session, currentAttachment]()
            {
                const MediaUploadResult uploadResult = watcher->result();
                watcher->deleteLater();
                handleUploadFinished(queueSnapshot, session, currentAttachment, uploadResult);
            });

    const MediaPendingAttachmentRunnerPort port = _port;
    const auto future = QtConcurrent::run(
        [this, port, request, attachmentIndex, totalCount]()
        {
            if (!port.uploadLocalFile)
            {
                MediaUploadResult result;
                result.errorText = uploadFailureFallback(request.mediaType);
                return result;
            }

            return port.uploadLocalFile(request,
                                        [this, attachmentIndex, totalCount](int percent, const QString& stage)
                                        {
                                            QMetaObject::invokeMethod(
                                                this,
                                                [this, percent, stage, attachmentIndex, totalCount]()
                                                {
                                                    const int bounded = qBound(0, percent, 100);
                                                    setProgressText(_port,
                                                                    QStringLiteral("正在发送附件 %1/%2 %3 %4%")
                                                                                       .arg(attachmentIndex)
                                                                                       .arg(totalCount)
                                                                                       .arg(stage)
                                                                                       .arg(bounded));
                                                },
                                                Qt::QueuedConnection);
                                        });
        });
    watcher->setFuture(future);
}

void MediaPendingAttachmentRunner::handleUploadFinished(const ChatPendingSendQueueSnapshot& queueSnapshot,
                                                        const MediaPendingAttachmentSessionSnapshot& session,
                                                        const QVariantMap& currentAttachment,
                                                        const MediaUploadResult& uploadResult)
{
    const QString attachmentType = currentAttachment.value(QStringLiteral("type")).toString();
    if (!uploadResult.ok)
    {
        if (_port.resetQueue)
        {
            _port.resetQueue();
        }
        resetUploadState(_port);
        if (_port.setTip)
        {
            _port.setTip(uploadResult.errorText.isEmpty() ? uploadFailureFallback(attachmentType)
                                                          : uploadResult.errorText,
                         true);
        }
        _running = false;
        emit finished();
        return;
    }

    UploadedAttachmentDestination destination;
    destination.selfUid = session.selfUid;
    destination.dialogUid = queueSnapshot.dialogUid;
    destination.currentDialogUid = _port.currentDialogUid ? _port.currentDialogUid() : 0;
    destination.chatUid = queueSnapshot.chatUid;
    destination.groupId = queueSnapshot.groupId;

    UploadedAttachmentDispatchResult dispatchResult;
    if (_port.dispatchUploadedAttachment)
    {
        dispatchResult = _port.dispatchUploadedAttachment(currentAttachment, uploadResult.info, destination);
    }
    else
    {
        dispatchResult.errorText = QStringLiteral("附件发送失败");
    }

    if (!dispatchResult.success)
    {
        if (_port.resetQueue)
        {
            _port.resetQueue();
        }
        resetUploadState(_port);
        if (_port.setTip && !dispatchResult.errorText.isEmpty())
        {
            _port.setTip(dispatchResult.errorText, true);
        }
        _running = false;
        emit finished();
        return;
    }

    if (_port.removePendingAttachmentById)
    {
        _port.removePendingAttachmentById(currentAttachment.value(QStringLiteral("attachmentId")).toString(),
                                                                  queueSnapshot.dialogUid);
    }

    const bool hasNext = _port.advanceQueue ? _port.advanceQueue() : false;
    if (!hasNext)
    {
        resetUploadState(_port);
        _running = false;
        emit finished();
        return;
    }
    processNextAsync();
}
