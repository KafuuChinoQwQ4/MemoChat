#include "MediaUploadService.h"

#include "MediaUploadServicePrivate.h"

#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>

namespace
{

bool usesImageUploadLimit(const QString& mediaType)
{
    const QString normalized = mediaType.trimmed().toLower();
    return normalized == QStringLiteral("image") || normalized == QStringLiteral("moment_image");
}

} // namespace

MediaUploadResult MediaUploadService::uploadLocalFile(const MediaUploadRequest& request,
                                                      const UploadProgressCallback& progress)
{
    using namespace MediaUploadServicePrivate;

    MediaUploadResult result;
    if (request.uid <= 0 || request.token.trimmed().isEmpty())
    {
        result.errorText = "登录态失效，请重新登录";
        return result;
    }

    const QString localPath = resolveLocalPath(request.localFileUrl);
    QFile file(localPath);
    if (!file.exists())
    {
        result.errorText = "文件不存在";
        return result;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        result.errorText = "文件读取失败";
        return result;
    }

    const QFileInfo fileInfo(localPath);
    const qint64 fileSize = fileInfo.size();
    if (fileSize <= 0)
    {
        result.errorText = "文件内容为空";
        return result;
    }

    const ClientMediaConfig mediaCfg = loadMediaConfig();
    const qint64 limit = usesImageUploadLimit(request.mediaType) ? mediaCfg.maxImageBytes : mediaCfg.maxFileBytes;
    if (fileSize > limit)
    {
        result.errorText = QString("文件过大，请控制在 %1MB 以内").arg(limit / (1024 * 1024));
        return result;
    }

    const QMimeDatabase mimeDb;
    const QString mimeType = mimeDb.mimeTypeForFile(fileInfo).name();
    if (request.mediaType.compare(QStringLiteral("avatar"), Qt::CaseInsensitive) == 0)
    {
        result.ok = uploadAvatarFile(file,
                                     fileInfo,
                                     request.mediaType,
                                     mimeType,
                                     request.uid,
                                     request.token,
                                     request.grantUids,
                                     request.grantGroupId,
                                     request.grantPublic,
                                     request.grantFriends,
                                     &result.info,
                                     &result.errorText,
                                     progress);
    }
    else
    {
        result.ok = uploadChunkedFile(file,
                                      fileInfo,
                                      request.mediaType,
                                      mimeType,
                                      fileSize,
                                      mediaCfg,
                                      request.uid,
                                      request.token,
                                      request.grantUids,
                                      request.grantGroupId,
                                      request.grantPublic,
                                      request.grantFriends,
                                      &result.info,
                                      &result.errorText,
                                      progress);
    }
    if (result.ok && result.info.fileName.isEmpty() && !request.fallbackName.trimmed().isEmpty())
    {
        result.info.fileName = request.fallbackName.trimmed();
    }
    return result;
}

bool MediaUploadService::uploadLocalFile(const QString& localFileUrl,
                                         const QString& mediaType,
                                         int uid,
                                         const QString& token,
                                         UploadedMediaInfo* outInfo,
                                         QString* errorText,
                                         const UploadProgressCallback& progress)
{
    MediaUploadRequest request;
    request.localFileUrl = localFileUrl;
    request.mediaType = mediaType;
    request.uid = uid;
    request.token = token;

    const MediaUploadResult result = uploadLocalFile(request, progress);
    if (outInfo)
    {
        *outInfo = result.info;
    }
    if (errorText)
    {
        *errorText = result.errorText;
    }
    return result.ok;
}
