#include "MomentsController.h"
#include "LocalFilePickerService.h"
#include "MomentsPublishPayload.h"
#include "global.h"
#include "httpmgr.h"
#include "usermgr.h"

#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMetaObject>
#include <QUrl>
#include <QtConcurrent>
#include <QtGlobal>

using memochat::moments::MomentPublishTaskResult;

void MomentsController::publishMoment(const QString& location, int visibility, const QVariantList& items)
{
    if (_loading)
        return;
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
    if (!LocalFilePickerService::pickMomentMediaUrls(&attachments, &errorText))
    {
        if (!errorText.isEmpty())
        {
            setErrorText(errorText);
        }
        return {};
    }

    QVariantList normalized;
    normalized.reserve(attachments.size());
    for (const QVariant& attachmentVar : attachments)
    {
        const QVariantMap normalizedAttachment = memochat::moments::normalizeMomentAttachment(attachmentVar.toMap());
        if (!normalizedAttachment.isEmpty())
        {
            normalized.push_back(normalizedAttachment);
        }
    }
    return normalized;
}

void MomentsController::publishDraftMoment(const QString& text, int visibility, const QVariantList& attachments)
{
    if (_loading)
    {
        return;
    }

    const QString trimmedText = text.trimmed();
    QVariantList normalizedAttachments;
    normalizedAttachments.reserve(attachments.size());
    for (const QVariant& attachmentVar : attachments)
    {
        const QVariantMap normalizedAttachment = memochat::moments::normalizeMomentAttachment(attachmentVar.toMap());
        if (!normalizedAttachment.isEmpty())
        {
            normalizedAttachments.push_back(normalizedAttachment);
        }
    }

    if (trimmedText.isEmpty() && normalizedAttachments.isEmpty())
    {
        setErrorText(QStringLiteral("请输入内容或添加图片/视频"));
        emit publishError(_error_text);
        return;
    }

    auto um = UserMgr::GetInstance();
    if (!um || um->GetUid() <= 0 || um->GetToken().trimmed().isEmpty())
    {
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

    const auto future = QtConcurrent::run(
        [this, trimmedText, normalizedAttachments, uid, token]()
        {
            return memochat::moments::buildUploadedDraftMomentItems(
                trimmedText,
                normalizedAttachments,
                uid,
                token,
                [this](int index, int attachmentCount, const QString& stage, int percent)
                {
                    QMetaObject::invokeMethod(
                        this,
                        [this, index, attachmentCount, percent, stage]()
                        {
                            setProgressText(QStringLiteral("正在上传素材 %1/%2 %3 %4%")
                                                               .arg(index + 1)
                                                               .arg(qMax(1, attachmentCount))
                                                               .arg(stage)
                                                               .arg(qBound(0, percent, 100)));
                        },
                        Qt::QueuedConnection);
                });
        });

    connect(watcher,
            &QFutureWatcher<MomentPublishTaskResult>::finished,
            this,
            [this, watcher, visibility]()
            {
                const MomentPublishTaskResult result = watcher->result();
                watcher->deleteLater();

                if (!result.ok)
                {
                    setLoading(false);
                    setProgressText(QString());
                    setErrorText(result.errorText.isEmpty() ? QStringLiteral("发布失败") : result.errorText);
                    emit publishError(_error_text);
                    return;
                }

                setProgressText(QStringLiteral("正在提交朋友圈..."));
                submitPublishRequest(QString(), visibility, result.items, false);
            });
    watcher->setFuture(future);
}

void MomentsController::publishDraftMomentJson(const QString& text, int visibility, const QString& attachmentsJson)
{
    QVariantList attachments;
    const QString trimmedJson = attachmentsJson.trimmed();
    if (!trimmedJson.isEmpty())
    {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(trimmedJson.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isArray())
        {
            setErrorText(QStringLiteral("素材列表解析失败"));
            emit publishError(_error_text);
            return;
        }

        const QJsonArray array = doc.array();
        attachments.reserve(array.size());
        for (const QJsonValue& value : array)
        {
            if (!value.isObject())
            {
                setErrorText(QStringLiteral("素材列表解析失败"));
                emit publishError(_error_text);
                return;
            }
            attachments.push_back(value.toObject().toVariantMap());
        }
    }

    publishDraftMoment(text, visibility, attachments);
}

void MomentsController::submitPublishRequest(const QString& location,
                                             int visibility,
                                             const QVariantList& items,
                                             bool manageLoading)
{
    if (manageLoading)
    {
        if (_loading)
            return;
        setLoading(true);
        setErrorText(QString());
        setProgressText(QStringLiteral("正在发布朋友圈..."));
    }

    const QJsonObject payload = memochat::moments::buildPublishPayload(buildAuthJson(), location, visibility, items);

    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/api/moments/publish"),
                                        payload,
                                        ReqId::ID_MOMENTS_PUBLISH,
                                        Modules::MOMENTSMOD,
                                        QStringLiteral("moments"));
}

// Static helpers
QVariantList MomentsController::buildTextItem(const QString& content)
{
    return memochat::moments::buildTextMomentItem(content);
}

QVariantList MomentsController::buildImageItem(const QString& mediaKey, const QString& thumbKey, int width, int height)
{
    return memochat::moments::buildImageMomentItem(mediaKey, thumbKey, width, height);
}

QVariantList MomentsController::buildVideoItem(const QString& mediaKey,
                                               const QString& thumbKey,
                                               int width,
                                               int height,
                                               int durationMs)
{
    return memochat::moments::buildVideoMomentItem(mediaKey, thumbKey, width, height, durationMs);
}
