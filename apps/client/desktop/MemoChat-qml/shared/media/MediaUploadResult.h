#ifndef MEDIAUPLOADRESULT_H
#define MEDIAUPLOADRESULT_H

#include <QString>
#include <QtGlobal>

struct UploadedMediaInfo
{
    QString mediaKey;
    QString remoteUrl;
    QString fileName;
    QString mimeType;
    qint64 sizeBytes = 0;
};

struct MediaUploadResult
{
    UploadedMediaInfo info;
    QString errorText;
    bool ok = false;
};

#endif // MEDIAUPLOADRESULT_H
