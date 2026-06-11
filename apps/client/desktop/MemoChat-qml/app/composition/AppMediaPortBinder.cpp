#include "AppCoordinators.h"
#include "AppPortRegistry.h"

void AppPortRegistry::bindMediaPorts()
{
    _media_coordinator = std::make_unique<MediaCoordinator>(MediaSendPort{
        [this]()
        {
            MediaSendSnapshot snapshot;
            snapshot.currentChatUid = _chat_state.uid;
            snapshot.currentGroupId = currentGroupId();
            snapshot.currentDialogUid = currentDialogUid();
            snapshot.uploadInProgress = _media_upload_state.inProgress;
            snapshot.pendingAttachments = currentPendingAttachments();
            snapshot.replyToMsgId = _features.chatFeatureController.replyToMsgId();
            snapshot.replyTargetName = _features.chatFeatureController.replyTargetName();
            snapshot.replyPreviewText = _features.chatFeatureController.replyPreviewText();
            return snapshot;
        },
        [this](const QString& tip, bool isError)
        {
            setTip(tip, isError);
        },
        [this](const QString& content, const QString& preview)
        {
            return _features.chatFeatureController.dispatchCurrentPrivateContent(content, preview);
        },
        [this](const QString& content, const QString& preview)
        {
            return _features.chatFeatureController.dispatchCurrentGroupContent(content, preview);
        },
        [this]()
        {
            setPendingReplyContext(QString(), QString(), QString());
        },
        [this](int dialogUid, int chatUid, qint64 groupId, const QVariantList& attachments)
        {
            _features.chatFeatureController.beginPendingAttachmentSend(dialogUid, chatUid, groupId, attachments);
        },
        [this]()
        {
            startPendingAttachmentRunner();
        },
        [this](const QVariantList& attachments)
        {
            addPendingAttachments(attachments);
        },
        [this](const QString& attachmentId)
        {
            removePendingAttachmentById(attachmentId);
        },
        [this](int dialogUid)
        {
            _features.chatFeatureController.clearPendingAttachmentsForDialog(dialogUid);
        },
        [this](const QVariantList& attachments)
        {
            setCurrentPendingAttachments(attachments);
        }});
}
