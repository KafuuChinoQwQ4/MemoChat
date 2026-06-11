#ifndef MEDIAPENDINGATTACHMENTRUNNER_H
#define MEDIAPENDINGATTACHMENTRUNNER_H

#include "ChatPendingSendQueueState.h"
#include "MediaUploadRequest.h"
#include "MediaUploadResult.h"
#include "UploadedAttachmentDispatchService.h"

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <functional>

struct MediaPendingAttachmentSessionSnapshot
{
    int selfUid = 0;
    QString authToken;
};

struct MediaPendingAttachmentRunnerPort
{
    using UploadProgressCallback = std::function<void(int, const QString&)>;

    std::function<ChatPendingSendQueueSnapshot()> queueSnapshot;
    std::function<MediaPendingAttachmentSessionSnapshot()> sessionSnapshot;
    std::function<int()> currentDialogUid;

    std::function<MediaUploadResult(const MediaUploadRequest&, const UploadProgressCallback&)> uploadLocalFile;
    std::function<UploadedAttachmentDispatchResult(const QVariantMap&,
                                                   const UploadedMediaInfo&,
                                                   const UploadedAttachmentDestination&)>
        dispatchUploadedAttachment;

    std::function<void(bool)> setUploadInProgress;
    std::function<void(const QString&)> setProgressText;
    std::function<void(const QString&, bool)> setTip;

    std::function<void()> resetQueue;
    std::function<void(const QString&, int)> removePendingAttachmentById;
    std::function<bool()> advanceQueue;
};

struct MediaPendingAttachmentProcessResult
{
    bool processedAttachment = false;
    bool shouldContinue = false;
};

class MediaPendingAttachmentRunner : public QObject
{
    Q_OBJECT

public:
    explicit MediaPendingAttachmentRunner(MediaPendingAttachmentRunnerPort port, QObject* parent = nullptr);

    void start();
    static MediaPendingAttachmentProcessResult processNext(const MediaPendingAttachmentRunnerPort& port);
    static void processAll(const MediaPendingAttachmentRunnerPort& port);

signals:
    void finished();

private:
    static void resetUploadState(const MediaPendingAttachmentRunnerPort& port);
    static void setProgressText(const MediaPendingAttachmentRunnerPort& port, const QString& text);

    void processNextAsync();
    void handleUploadFinished(const ChatPendingSendQueueSnapshot& queueSnapshot,
                              const MediaPendingAttachmentSessionSnapshot& session,
                              const QVariantMap& currentAttachment,
                              const MediaUploadResult& uploadResult);

    MediaPendingAttachmentRunnerPort _port;
    bool _running = false;
};

#endif // MEDIAPENDINGATTACHMENTRUNNER_H
