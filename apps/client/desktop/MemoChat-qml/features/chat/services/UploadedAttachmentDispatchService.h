#ifndef UPLOADEDATTACHMENTDISPATCHSERVICE_H
#define UPLOADEDATTACHMENTDISPATCHSERVICE_H

#include "ChatDispatchService.h"
#include "MediaUploadResult.h"

#include <QByteArray>
#include <QVariantMap>
#include <QtGlobal>
#include <functional>

struct UploadedAttachmentDestination
{
    int selfUid = 0;
    int dialogUid = 0;
    int currentDialogUid = 0;
    int chatUid = 0;
    qint64 groupId = 0;
};

struct UploadedAttachmentDispatchPort
{
    std::function<bool(const QString&, const QString&)> dispatchCurrentPrivateContent;
    std::function<bool(const QString&, const QString&)> dispatchCurrentGroupContent;
    std::function<bool(const OutgoingChatPacket&, QString*)> dispatchPrivatePacket;
    std::function<void(int, const QByteArray&)> dispatchGroupPayload;
};

struct UploadedAttachmentDispatchResult
{
    bool success = false;
    QString errorText;
    bool dispatchedCurrentDialog = false;
    bool dispatchedGroupPayload = false;
    bool dispatchedPrivatePacket = false;
    int requestId = 0;
    QString encodedContent;
    QString previewText;
};

class UploadedAttachmentDispatchService
{
public:
    static UploadedAttachmentDispatchResult dispatch(const QVariantMap& attachment,
                                                     const UploadedMediaInfo& uploaded,
                                                     const UploadedAttachmentDestination& destination,
                                                     const UploadedAttachmentDispatchPort& port);
};

#endif // UPLOADEDATTACHMENTDISPATCHSERVICE_H
