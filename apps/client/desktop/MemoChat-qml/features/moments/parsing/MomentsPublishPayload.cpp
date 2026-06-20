#include "MomentsPublishPayload.h"

#include "MediaUploadService.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>
#include <QtGlobal>

namespace memochat::moments
{
namespace
{
QString momentUploadMediaType(const QString& attachmentType)
{
    const QString normalized = attachmentType.trimmed().toLower();
    if (normalized == QStringLiteral("video"))
    {
        return QStringLiteral("moment_video");
    }
    return QStringLiteral("moment_image");
}
} // namespace

QVariantMap normalizeMomentAttachment(const QVariantMap& source)
{
    QVariantMap attachment = source;
    const QString type = attachment.value(QStringLiteral("type")).toString().trimmed();
    const QString fileUrl = attachment.value(QStringLiteral("fileUrl")).toString().trimmed();
    if ((type != QStringLiteral("image") && type != QStringLiteral("video")) || fileUrl.isEmpty())
    {
        return {};
    }

    if (attachment.value(QStringLiteral("attachmentId")).toString().trimmed().isEmpty())
    {
        attachment.insert(QStringLiteral("attachmentId"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
    if (attachment.value(QStringLiteral("fileName")).toString().trimmed().isEmpty())
    {
        attachment.insert(QStringLiteral("fileName"), QFileInfo(QUrl(fileUrl).toLocalFile()).fileName());
    }
    return attachment;
}

QVariantList buildTextMomentItem(const QString& content)
{
    QVariantMap item;
    item["media_type"] = QStringLiteral("text");
    item["content"] = content;
    QVariantList list;
    list.append(item);
    return list;
}

QVariantList buildImageMomentItem(const QString& mediaKey, const QString& thumbKey, int width, int height)
{
    QVariantMap item;
    item["media_type"] = QStringLiteral("image");
    item["media_key"] = mediaKey;
    item["thumb_key"] = thumbKey.isEmpty() ? mediaKey : thumbKey;
    item["width"] = width;
    item["height"] = height;
    QVariantList list;
    list.append(item);
    return list;
}

QVariantList
buildVideoMomentItem(const QString& mediaKey, const QString& thumbKey, int width, int height, int durationMs)
{
    QVariantMap item;
    item["media_type"] = QStringLiteral("video");
    item["media_key"] = mediaKey;
    item["thumb_key"] = thumbKey.isEmpty() ? mediaKey : thumbKey;
    item["width"] = width;
    item["height"] = height;
    item["duration_ms"] = durationMs;
    QVariantList list;
    list.append(item);
    return list;
}

MomentPublishTaskResult
buildUploadedDraftMomentItems(const QString& text,
                              const QVariantList& normalizedAttachments,
                              int uid,
                              const QString& token,
                              const std::function<void(int, int, const QString&, int)>& progressCallback)
{
    MomentPublishTaskResult result;
    QVariantList items;
    const QString trimmedText = text.trimmed();

    if (!trimmedText.isEmpty())
    {
        items += buildTextMomentItem(trimmedText);
    }

    const int attachmentCount = normalizedAttachments.size();
    for (int index = 0; index < normalizedAttachments.size(); ++index)
    {
        const QVariantMap attachment = normalizedAttachments.at(index).toMap();
        const QString attachmentType = attachment.value(QStringLiteral("type")).toString().trimmed().toLower();
        const QString uploadMediaType = momentUploadMediaType(attachmentType);
        const QString fileUrl = attachment.value(QStringLiteral("fileUrl")).toString();

        UploadedMediaInfo uploaded;
        QString uploadError;
        const bool ok = MediaUploadService::uploadLocalFile(
            fileUrl,
            uploadMediaType,
            uid,
            token,
            &uploaded,
            &uploadError,
            [index, attachmentCount, progressCallback](int percent, const QString& stage)
            {
                if (progressCallback)
                {
                    progressCallback(index, attachmentCount, stage, percent);
                }
            });
        if (!ok)
        {
            result.errorText = uploadError.isEmpty() ? QStringLiteral("素材上传失败") : uploadError;
            return result;
        }

        QString mediaKey = uploaded.mediaKey;
        if (mediaKey.isEmpty())
        {
            const QUrl uploadedUrl(uploaded.remoteUrl);
            const QUrlQuery query(uploadedUrl);
            mediaKey = query.queryItemValue(QStringLiteral("asset"));
        }
        if (mediaKey.isEmpty())
        {
            result.errorText = QStringLiteral("上传完成，但未返回有效媒体标识");
            return result;
        }

        const int width = attachment.value(QStringLiteral("width")).toInt();
        const int height = attachment.value(QStringLiteral("height")).toInt();
        if (attachmentType == QStringLiteral("video"))
        {
            items += buildVideoMomentItem(mediaKey,
                                          QString(),
                                          width,
                                          height,
                                          attachment.value(QStringLiteral("durationMs")).toInt());
        }
        else
        {
            items += buildImageMomentItem(mediaKey, QString(), width, height);
        }
    }

    result.ok = true;
    result.items = items;
    return result;
}

QJsonObject
buildPublishPayload(QJsonObject authPayload, const QString& location, int visibility, const QVariantList& items)
{
    QJsonObject payload = authPayload;
    payload["location"] = location;
    payload["visibility"] = visibility;

    QJsonArray itemsArr;
    for (const auto& itemVar : items)
    {
        const QVariantMap itemMap = itemVar.toMap();
        QJsonObject itemObj;
        for (auto it = itemMap.constBegin(); it != itemMap.constEnd(); ++it)
        {
            itemObj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        itemsArr.append(itemObj);
    }
    payload["items"] = itemsArr;

    return payload;
}

} // namespace memochat::moments
