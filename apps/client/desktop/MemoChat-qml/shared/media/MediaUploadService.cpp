#include "MediaUploadService.h"

#include "MediaUploadServicePrivate.h"

#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>

bool MediaUploadService::uploadLocalFile(const QString& localFileUrl,
                                         const QString& mediaType,
                                         int uid,
                                         const QString& token,
                                         UploadedMediaInfo* outInfo,
                                         QString* errorText,
                                         const UploadProgressCallback& progress)
{
    using namespace MediaUploadServicePrivate;

    if (uid <= 0 || token.trimmed().isEmpty())
    {
        if (errorText)
        {
            *errorText = "登录态失效，请重新登录";
        }
        return false;
    }

    const QString localPath = resolveLocalPath(localFileUrl);
    QFile file(localPath);
    if (!file.exists())
    {
        if (errorText)
        {
            *errorText = "文件不存在";
        }
        return false;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        if (errorText)
        {
            *errorText = "文件读取失败";
        }
        return false;
    }

    const QFileInfo fileInfo(localPath);
    const qint64 fileSize = fileInfo.size();
    if (fileSize <= 0)
    {
        if (errorText)
        {
            *errorText = "文件内容为空";
        }
        return false;
    }

    const ClientMediaConfig mediaCfg = loadMediaConfig();
    const qint64 limit = (mediaType == "image") ? mediaCfg.maxImageBytes : mediaCfg.maxFileBytes;
    if (fileSize > limit)
    {
        if (errorText)
        {
            *errorText = QString("文件过大，请控制在 %1MB 以内").arg(limit / (1024 * 1024));
        }
        return false;
    }

    const QMimeDatabase mimeDb;
    const QString mimeType = mimeDb.mimeTypeForFile(fileInfo).name();
    if (mediaType.compare(QStringLiteral("avatar"), Qt::CaseInsensitive) == 0)
    {
        return uploadAvatarFile(file, fileInfo, mediaType, mimeType, uid, token, outInfo, errorText, progress);
    }

    return uploadChunkedFile(file,
                             fileInfo,
                             mediaType,
                             mimeType,
                             fileSize,
                             mediaCfg,
                             uid,
                             token,
                             outInfo,
                             errorText,
                             progress);
}
