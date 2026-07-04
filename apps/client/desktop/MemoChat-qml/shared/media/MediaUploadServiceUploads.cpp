#include "MediaUploadServicePrivate.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QUrlQuery>

namespace MediaUploadServicePrivate
{
namespace
{

void appendGrantFields(QJsonObject& payload,
                       const QList<int>& grantUids,
                       qint64 grantGroupId,
                       bool grantPublic,
                       bool grantFriends)
{
    if (!grantUids.isEmpty())
    {
        QJsonArray grantees;
        for (int uid : grantUids)
        {
            if (uid > 0)
            {
                grantees.append(uid);
            }
        }
        if (!grantees.isEmpty())
        {
            payload["grant_uids"] = grantees;
        }
    }
    if (grantGroupId > 0)
    {
        payload["grant_group_id"] = grantGroupId;
    }
    if (grantPublic)
    {
        payload["grant_public"] = true;
    }
    if (grantFriends)
    {
        payload["grant_friends"] = true;
    }
}

} // namespace

bool uploadAvatarFile(QFile& file,
                      const QFileInfo& fileInfo,
                      const QString& mediaType,
                      const QString& mimeType,
                      int uid,
                      const QString& token,
                      const QList<int>& grantUids,
                      qint64 grantGroupId,
                      bool grantPublic,
                      bool grantFriends,
                      UploadedMediaInfo* outInfo,
                      QString* errorText,
                      const MediaUploadService::UploadProgressCallback& progress)
{
    if (progress)
    {
        progress(0, QStringLiteral("上传头像..."));
    }

    if (!file.seek(0))
    {
        if (errorText)
        {
            *errorText = "读取文件失败";
        }
        return false;
    }

    const QByteArray binary = file.readAll();
    if (binary.isEmpty())
    {
        if (errorText)
        {
            *errorText = "文件内容为空";
        }
        return false;
    }

    QJsonObject uploadPayload;
    uploadPayload["uid"] = uid;
    uploadPayload["token"] = token;
    uploadPayload["media_type"] = mediaType;
    uploadPayload["file_name"] = fileInfo.fileName();
    uploadPayload["mime"] = mimeType;
    uploadPayload["data_base64"] = QString::fromLatin1(binary.toBase64());
    appendGrantFields(uploadPayload, grantUids, grantGroupId, grantPublic, grantFriends);

    QJsonObject uploadRsp;
    if (!postJson(mediaUploadUrl(QStringLiteral("/upload_media")), uploadPayload, &uploadRsp, errorText))
    {
        return false;
    }
    if (!isApiSuccess(uploadRsp, errorText))
    {
        return false;
    }
    if (!populateUploadedMediaInfo(uploadRsp, fileInfo, mimeType, outInfo, errorText))
    {
        return false;
    }
    if (progress)
    {
        progress(100, QStringLiteral("上传完成"));
    }
    return true;
}

bool uploadChunkedFile(QFile& file,
                       const QFileInfo& fileInfo,
                       const QString& mediaType,
                       const QString& mimeType,
                       qint64 fileSize,
                       const ClientMediaConfig& mediaCfg,
                       int uid,
                       const QString& token,
                       const QList<int>& grantUids,
                       qint64 grantGroupId,
                       bool grantPublic,
                       bool grantFriends,
                       UploadedMediaInfo* outInfo,
                       QString* errorText,
                       const MediaUploadService::UploadProgressCallback& progress)
{
    const int requestedChunkSize = mediaCfg.chunkSizeBytes;
    if (progress)
    {
        progress(0, QStringLiteral("初始化上传..."));
    }

    QJsonObject initPayload;
    initPayload["uid"] = uid;
    initPayload["token"] = token;
    initPayload["media_type"] = mediaType;
    initPayload["file_name"] = fileInfo.fileName();
    initPayload["mime"] = mimeType;
    initPayload["file_size"] = static_cast<qint64>(fileSize);
    initPayload["chunk_size"] = requestedChunkSize;
    appendGrantFields(initPayload, grantUids, grantGroupId, grantPublic, grantFriends);

    QJsonObject initRsp;
    if (!postJson(mediaUploadUrl(QStringLiteral("/upload_media_init")), initPayload, &initRsp, errorText))
    {
        return false;
    }
    if (!isApiSuccess(initRsp, errorText))
    {
        return false;
    }

    const QString uploadId = initRsp.value("upload_id").toString();
    const int chunkSize = qBound(256 * 1024, initRsp.value("chunk_size").toInt(requestedChunkSize), 4 * 1024 * 1024);
    const int totalChunks =
        initRsp.value("total_chunks").toInt(static_cast<int>((fileSize + chunkSize - 1) / chunkSize));
    if (uploadId.isEmpty() || totalChunks <= 0)
    {
        if (errorText)
        {
            *errorText = "上传初始化失败";
        }
        return false;
    }

    QSet<int> uploadedChunks;
    QUrl statusUrl(mediaUploadUrl(QStringLiteral("/upload_media_status")));
    QUrlQuery statusQuery;
    statusQuery.addQueryItem("uid", QString::number(uid));
    statusQuery.addQueryItem("token", token);
    statusQuery.addQueryItem("upload_id", uploadId);
    statusUrl.setQuery(statusQuery);
    QJsonObject statusRsp;
    if (getJson(statusUrl, &statusRsp, nullptr) && isApiSuccess(statusRsp, nullptr))
    {
        const QJsonArray uploaded = statusRsp.value("uploaded_chunks").toArray();
        for (const QJsonValue& v : uploaded)
        {
            uploadedChunks.insert(v.toInt(-1));
        }
    }

    qint64 uploadedBytes = 0;
    for (int idx : uploadedChunks)
    {
        const qint64 remain = fileSize - static_cast<qint64>(idx) * chunkSize;
        if (remain > 0)
        {
            uploadedBytes += qMin<qint64>(chunkSize, remain);
        }
    }
    uploadedBytes = qMin(uploadedBytes, fileSize);
    if (progress)
    {
        const int percent = static_cast<int>((uploadedBytes * 100) / fileSize);
        progress(percent, QStringLiteral("上传分片..."));
    }

    for (int index = 0; index < totalChunks; ++index)
    {
        if (uploadedChunks.contains(index))
        {
            continue;
        }

        const qint64 offset = static_cast<qint64>(index) * chunkSize;
        if (!file.seek(offset))
        {
            if (errorText)
            {
                *errorText = "读取文件失败";
            }
            return false;
        }
        const QByteArray chunkBytes = file.read(chunkSize);
        if (chunkBytes.isEmpty())
        {
            if (errorText)
            {
                *errorText = "读取分片失败";
            }
            return false;
        }

        bool uploaded = false;
        QString lastErr;
        for (int attempt = 0; attempt < mediaCfg.chunkRetry; ++attempt)
        {
            QList<QPair<QByteArray, QByteArray>> headers;
            headers.append({QByteArrayLiteral("X-Uid"), QByteArray::number(uid)});
            headers.append({QByteArrayLiteral("X-Token"), token.toUtf8()});
            headers.append({QByteArrayLiteral("X-Upload-Id"), uploadId.toUtf8()});
            headers.append({QByteArrayLiteral("X-Chunk-Index"), QByteArray::number(index)});

            QJsonObject chunkRsp;
            if (!postBinary(
                    mediaUploadUrl(QStringLiteral("/upload_media_chunk")), chunkBytes, headers, &chunkRsp, &lastErr))
            {
                continue;
            }
            if (!isApiSuccess(chunkRsp, &lastErr))
            {
                continue;
            }
            uploaded = true;
            break;
        }

        if (!uploaded)
        {
            if (errorText)
            {
                *errorText = lastErr.isEmpty() ? "分片上传失败" : lastErr;
            }
            return false;
        }

        uploadedBytes += chunkBytes.size();
        uploadedBytes = qMin(uploadedBytes, fileSize);
        if (progress)
        {
            const int percent = static_cast<int>((uploadedBytes * 100) / fileSize);
            progress(percent, QStringLiteral("上传分片..."));
        }
    }

    QJsonObject completePayload;
    completePayload["uid"] = uid;
    completePayload["token"] = token;
    completePayload["upload_id"] = uploadId;
    QJsonObject completeRsp;
    if (!postJson(mediaUploadUrl(QStringLiteral("/upload_media_complete")), completePayload, &completeRsp, errorText))
    {
        return false;
    }
    if (!isApiSuccess(completeRsp, errorText))
    {
        return false;
    }
    if (!populateUploadedMediaInfo(completeRsp, fileInfo, mimeType, outInfo, errorText))
    {
        return false;
    }
    if (progress)
    {
        progress(100, QStringLiteral("上传完成"));
    }
    return true;
}

} // namespace MediaUploadServicePrivate
