#include "AppController.h"

#include "AppCoordinators.h"
#include "MediaPendingAttachmentRunner.h"
#include "MediaUploadService.h"
#include "usermgr.h"

void AppController::startPendingAttachmentRunner()
{
    auto* runner = new MediaPendingAttachmentRunner(
        MediaPendingAttachmentRunnerPort{
            [this]()
            {
                return _features.chatFeatureController.pendingAttachmentSendSnapshot();
            },
            [this]()
            {
                MediaPendingAttachmentSessionSnapshot snapshot;
                auto selfInfo = _gateway.userMgr()->GetUserInfo();
                snapshot.selfUid = selfInfo ? selfInfo->_uid : 0;
                snapshot.authToken = _session_coordinator->authToken();
                return snapshot;
            },
            [this]()
            {
                return currentDialogUid();
            },
            [](const MediaUploadRequest& request,
               const MediaPendingAttachmentRunnerPort::UploadProgressCallback& progress)
            {
                return MediaUploadService::uploadLocalFile(request, progress);
            },
            [this](const QVariantMap& attachment,
                   const UploadedMediaInfo& uploaded,
                   const UploadedAttachmentDestination& destination)
            {
                return _features.chatFeatureController.dispatchUploadedAttachment(attachment, uploaded, destination);
            },
            [this](bool inProgress)
            {
                setMediaUploadInProgress(inProgress);
            },
            [this](const QString& text)
            {
                setMediaUploadProgressText(text);
            },
            [this](const QString& text, bool isError)
            {
                setTip(text, isError);
            },
            [this]()
            {
                _features.chatFeatureController.resetPendingAttachmentSendQueue();
            },
            [this](const QString& attachmentId, int dialogUid)
            {
                removePendingAttachmentById(attachmentId, dialogUid);
            },
            [this]()
            {
                return _features.chatFeatureController.advancePendingAttachmentSendQueue();
            }},
        this);
    connect(runner, &MediaPendingAttachmentRunner::finished, runner, &QObject::deleteLater);
    runner->start();
}
