#ifndef MEDIAUPLOADSERVICE_H
#define MEDIAUPLOADSERVICE_H

#include <QString>
#include <functional>

struct UploadedMediaInfo {
    QString mediaKey;
    QString remoteUrl;
    QString fileName;
    QString mimeType;
    qint64 sizeBytes = 0;
};

class MediaUploadService
{
public:
    using UploadProgressCallback = std::function<void(int, const QString &)>;

    static bool uploadLocalFile(const QString &localFileUrl,
                                const QString &mediaType,
                                int uid,
                                const QString &token,
                                UploadedMediaInfo *outInfo,
                                QString *errorText,
                                const UploadProgressCallback &progress = UploadProgressCallback());
};

#endif // MEDIAUPLOADSERVICE_H
