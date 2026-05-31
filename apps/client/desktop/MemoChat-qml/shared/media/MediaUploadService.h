#ifndef MEDIAUPLOADSERVICE_H
#define MEDIAUPLOADSERVICE_H

#include "MediaUploadRequest.h"
#include "MediaUploadResult.h"

#include <QString>
#include <functional>

class MediaUploadService
{
public:
    using UploadProgressCallback = std::function<void(int, const QString&)>;

    static MediaUploadResult uploadLocalFile(const MediaUploadRequest& request,
                                             const UploadProgressCallback& progress = UploadProgressCallback());

    static bool uploadLocalFile(const QString& localFileUrl,
                                const QString& mediaType,
                                int uid,
                                const QString& token,
                                UploadedMediaInfo* outInfo,
                                QString* errorText,
                                const UploadProgressCallback& progress = UploadProgressCallback());
};

#endif // MEDIAUPLOADSERVICE_H
