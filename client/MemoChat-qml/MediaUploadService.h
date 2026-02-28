#ifndef MEDIAUPLOADSERVICE_H
#define MEDIAUPLOADSERVICE_H

#include <QString>

struct UploadedMediaInfo {
    QString remoteUrl;
    QString fileName;
    QString mimeType;
};

class MediaUploadService
{
public:
    static bool uploadLocalFile(const QString &localFileUrl,
                                const QString &mediaType,
                                int uid,
                                UploadedMediaInfo *outInfo,
                                QString *errorText);
};

#endif // MEDIAUPLOADSERVICE_H
