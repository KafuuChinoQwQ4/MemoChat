#include "MediaUploadServicePrivate.h"

#include "global.h"

#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonValue>
#include <QSettings>
#include <QUrlQuery>

namespace MediaUploadServicePrivate
{

QString resolveLocalPath(const QString& localFileUrl)
{
    QUrl url(localFileUrl);
    if (url.isLocalFile())
    {
        return url.toLocalFile();
    }
    return localFileUrl;
}

QString mediaUploadBaseUrl()
{
    const QString mediaBase = gate_media_url_prefix.trimmed();
    if (!mediaBase.isEmpty())
    {
        return mediaBase;
    }
    return gate_url_prefix.trimmed();
}

QUrl mediaUploadUrl(const QString& path)
{
    return QUrl(mediaUploadBaseUrl() + path);
}

QJsonObject normalizeMediaResponse(QJsonObject responseObj)
{
    const QJsonValue dataValue = responseObj.value(QStringLiteral("data"));
    if (dataValue.isObject())
    {
        const QJsonObject dataObject = dataValue.toObject();
        for (auto it = dataObject.begin(); it != dataObject.end(); ++it)
        {
            if (!responseObj.contains(it.key()))
            {
                responseObj.insert(it.key(), it.value());
            }
        }
    }
    return responseObj;
}

QString responseBodyPreview(const QByteArray& body)
{
    QString preview = QString::fromUtf8(body.left(160)).trimmed();
    preview.replace('\n', QLatin1Char(' '));
    preview.replace('\r', QLatin1Char(' '));
    return preview;
}

QString resolveUploadedMediaUrl(const QString& remoteUrl)
{
    if (remoteUrl.startsWith(QLatin1Char('/')))
    {
        const QString mediaBaseUrl =
            gate_media_url_prefix.trimmed().isEmpty() ? gate_url_prefix.trimmed() : gate_media_url_prefix.trimmed();
        return mediaBaseUrl + remoteUrl;
    }
    return remoteUrl;
}

bool populateUploadedMediaInfo(const QJsonObject& responseObj,
                               const QFileInfo& fileInfo,
                               const QString& mimeType,
                               UploadedMediaInfo* outInfo,
                               QString* errorText)
{
    const QString remoteUrl = resolveUploadedMediaUrl(responseObj.value(QStringLiteral("url")).toString());
    if (remoteUrl.isEmpty())
    {
        if (errorText)
        {
            *errorText = QStringLiteral("上传失败：无可用地址");
        }
        return false;
    }

    QString mediaKey = responseObj.value(QStringLiteral("media_key")).toString();
    if (mediaKey.isEmpty())
    {
        const QUrl remoteUrlObj(remoteUrl);
        const QUrlQuery remoteQuery(remoteUrlObj);
        mediaKey = remoteQuery.queryItemValue(QStringLiteral("asset"));
    }

    if (outInfo)
    {
        outInfo->mediaKey = mediaKey;
        outInfo->remoteUrl = remoteUrl;
        outInfo->fileName = responseObj.value(QStringLiteral("file_name")).toString(fileInfo.fileName());
        outInfo->mimeType = responseObj.value(QStringLiteral("mime")).toString(mimeType);
        outInfo->sizeBytes = responseObj.value(QStringLiteral("size")).toVariant().toLongLong();
    }
    return true;
}

ClientMediaConfig loadMediaConfig()
{
    ClientMediaConfig cfg;
    const QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    const qint64 maxImageMb = settings.value("Media/MaxImageMB", 200).toLongLong();
    const qint64 maxFileMb = settings.value("Media/MaxFileMB", 20480).toLongLong();
    const int chunkKb = settings.value("Media/ChunkSizeKB", 4096).toInt();
    const int retry = settings.value("Media/ChunkRetry", 3).toInt();
    if (maxImageMb > 0)
    {
        cfg.maxImageBytes = maxImageMb * 1024 * 1024;
    }
    if (maxFileMb > 0)
    {
        cfg.maxFileBytes = maxFileMb * 1024 * 1024;
    }
    if (chunkKb > 0)
    {
        cfg.chunkSizeBytes = qBound(256 * 1024, chunkKb * 1024, 4 * 1024 * 1024);
    }
    if (retry > 0)
    {
        cfg.chunkRetry = qBound(1, retry, 8);
    }
    return cfg;
}

bool isApiSuccess(const QJsonObject& obj, QString* errorText)
{
    const int error = obj.value("error").toInt(1);
    if (error == 0)
    {
        return true;
    }
    if (errorText)
    {
        *errorText = obj.value("message").toString("上传失败");
    }
    return false;
}

} // namespace MediaUploadServicePrivate
