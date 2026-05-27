#include "MomentsController.h"
#include "LocalFilePickerService.h"
#include "MediaUploadService.h"
#include "global.h"
#include "httpmgr.h"
#include "usermgr.h"

#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaObject>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>
#include <QtConcurrent>
#include <QtGlobal>

namespace {
struct MomentPublishTaskResult {
    bool ok = false;
    QString errorText;
    QVariantList items;
};

QVariantMap normalizeMomentAttachment(const QVariantMap& source)
{
    QVariantMap attachment = source;
    const QString type = attachment.value(QStringLiteral("type")).toString().trimmed();
    const QString fileUrl = attachment.value(QStringLiteral("fileUrl")).toString().trimmed();
    if ((type != QStringLiteral("image") && type != QStringLiteral("video")) || fileUrl.isEmpty()) {
        return {};
    }

    if (attachment.value(QStringLiteral("attachmentId")).toString().trimmed().isEmpty()) {
        attachment.insert(QStringLiteral("attachmentId"),
                          QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
    if (attachment.value(QStringLiteral("fileName")).toString().trimmed().isEmpty()) {
        attachment.insert(QStringLiteral("fileName"), QFileInfo(QUrl(fileUrl).toLocalFile()).fileName());
    }
    return attachment;
}
}

void MomentsController::publishMoment(const QString& location, int visibility, const QVariantList& items) {
    if (_loading) return;
    setLoading(true);
    setErrorText(QString());
    setProgressText(QStringLiteral("正在发布朋友圈..."));

    submitPublishRequest(location, visibility, items, false);
}

QVariantList MomentsController::pickMomentMedia()
{
    setErrorText(QString());

    QVariantList attachments;
    QString errorText;
    if (!LocalFilePickerService::pickMomentMediaUrls(&attachments, &errorText)) {
        if (!errorText.isEmpty()) {
            setErrorText(errorText);
        }
        return {};
    }

    QVariantList normalized;
    normalized.reserve(attachments.size());
    for (const QVariant& attachmentVar : attachments) {
        const QVariantMap normalizedAttachment = normalizeMomentAttachment(attachmentVar.toMap());
        if (!normalizedAttachment.isEmpty()) {
            normalized.push_back(normalizedAttachment);
        }
    }
    return normalized;
}

void MomentsController::publishDraftMoment(const QString& text, int visibility,
                                           const QVariantList& attachments)
{
    if (_loading) {
        return;
    }

    const QString trimmedText = text.trimmed();
    QVariantList normalizedAttachments;
    normalizedAttachments.reserve(attachments.size());
    for (const QVariant& attachmentVar : attachments) {
        const QVariantMap normalizedAttachment = normalizeMomentAttachment(attachmentVar.toMap());
        if (!normalizedAttachment.isEmpty()) {
            normalizedAttachments.push_back(normalizedAttachment);
        }
    }

    if (trimmedText.isEmpty() && normalizedAttachments.isEmpty()) {
        setErrorText(QStringLiteral("请输入内容或添加图片/视频"));
        emit publishError(_error_text);
        return;
    }

    auto um = UserMgr::GetInstance();
    if (!um || um->GetUid() <= 0 || um->GetToken().trimmed().isEmpty()) {
        setErrorText(QStringLiteral("登录态失效，请重新登录"));
        emit publishError(_error_text);
        return;
    }

    setLoading(true);
    setErrorText(QString());
    setProgressText(normalizedAttachments.isEmpty()
                        ? QStringLiteral("正在发布朋友圈...")
                        : QStringLiteral("正在准备素材 0/%1").arg(normalizedAttachments.size()));

    auto* watcher = new QFutureWatcher<MomentPublishTaskResult>(this);
    const int uid = um->GetUid();
    const QString token = um->GetToken();
    const int attachmentCount = normalizedAttachments.size();

    const auto future = QtConcurrent::run([this, trimmedText, normalizedAttachments, uid, token, attachmentCount]() {
        MomentPublishTaskResult result;
        QVariantList items;

        if (!trimmedText.isEmpty()) {
            items += MomentsController::buildTextItem(trimmedText);
        }

        for (int index = 0; index < normalizedAttachments.size(); ++index) {
            const QVariantMap attachment = normalizedAttachments.at(index).toMap();
            const QString uploadType = attachment.value(QStringLiteral("type")).toString();
            const QString fileUrl = attachment.value(QStringLiteral("fileUrl")).toString();

            UploadedMediaInfo uploaded;
            QString uploadError;
            const bool ok = MediaUploadService::uploadLocalFile(
                fileUrl,
                uploadType,
                uid,
                token,
                &uploaded,
                &uploadError,
                [this, index, attachmentCount](int percent, const QString& stage) {
                    QMetaObject::invokeMethod(this, [this, index, attachmentCount, percent, stage]() {
                        setProgressText(QStringLiteral("正在上传素材 %1/%2 %3 %4%")
                                            .arg(index + 1)
                                            .arg(qMax(1, attachmentCount))
                                            .arg(stage)
                                            .arg(qBound(0, percent, 100)));
                    }, Qt::QueuedConnection);
                });
            if (!ok) {
                result.errorText = uploadError.isEmpty()
                                       ? QStringLiteral("素材上传失败")
                                       : uploadError;
                return result;
            }

            QString mediaKey = uploaded.mediaKey;
            if (mediaKey.isEmpty()) {
                const QUrl uploadedUrl(uploaded.remoteUrl);
                const QUrlQuery query(uploadedUrl);
                mediaKey = query.queryItemValue(QStringLiteral("asset"));
            }
            if (mediaKey.isEmpty()) {
                result.errorText = QStringLiteral("上传完成，但未返回有效媒体标识");
                return result;
            }

            const int width = attachment.value(QStringLiteral("width")).toInt();
            const int height = attachment.value(QStringLiteral("height")).toInt();
            if (uploadType == QStringLiteral("video")) {
                items += MomentsController::buildVideoItem(
                    mediaKey,
                    QString(),
                    width,
                    height,
                    attachment.value(QStringLiteral("durationMs")).toInt());
            } else {
                items += MomentsController::buildImageItem(mediaKey, QString(), width, height);
            }
        }

        result.ok = true;
        result.items = items;
        return result;
    });

    connect(watcher, &QFutureWatcher<MomentPublishTaskResult>::finished, this,
            [this, watcher, visibility]() {
        const MomentPublishTaskResult result = watcher->result();
        watcher->deleteLater();

        if (!result.ok) {
            setLoading(false);
            setProgressText(QString());
            setErrorText(result.errorText.isEmpty()
                             ? QStringLiteral("发布失败")
                             : result.errorText);
            emit publishError(_error_text);
            return;
        }

        setProgressText(QStringLiteral("正在提交朋友圈..."));
        submitPublishRequest(QString(), visibility, result.items, false);
    });
    watcher->setFuture(future);
}

void MomentsController::submitPublishRequest(const QString& location, int visibility,
                                             const QVariantList& items, bool manageLoading) {
    if (manageLoading) {
        if (_loading) return;
        setLoading(true);
        setErrorText(QString());
        setProgressText(QStringLiteral("正在发布朋友圈..."));
    }

    QJsonObject payload = buildAuthJson();
    payload["location"] = location;
    payload["visibility"] = visibility;

    QJsonArray itemsArr;
    QString firstTextContent;
    for (const auto& itemVar : items) {
        const QVariantMap itemMap = itemVar.toMap();
        QJsonObject itemObj;
        for (auto it = itemMap.constBegin(); it != itemMap.constEnd(); ++it) {
            itemObj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        if (firstTextContent.isEmpty()
            && itemMap.value(QStringLiteral("media_type")).toString() == QStringLiteral("text")) {
            firstTextContent = itemMap.value(QStringLiteral("content")).toString();
        }
        itemsArr.append(itemObj);
    }
    payload["items"] = itemsArr;
    if (!firstTextContent.trimmed().isEmpty()) {
        payload["content"] = firstTextContent;
    }

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/api/moments/publish"),
        payload,
        ReqId::ID_MOMENTS_PUBLISH,
        Modules::MOMENTSMOD,
        QStringLiteral("moments"));
}

// Static helpers
QVariantList MomentsController::buildTextItem(const QString& content) {
    QVariantMap item;
    item["media_type"] = QStringLiteral("text");
    item["content"] = content;
    QVariantList list;
    list.append(item);
    return list;
}

QVariantList MomentsController::buildImageItem(const QString& mediaKey, const QString& thumbKey,
                                               int width, int height) {
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

QVariantList MomentsController::buildVideoItem(const QString& mediaKey, const QString& thumbKey,
                                               int width, int height, int durationMs) {
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
