#ifndef MOMENTSPUBLISHPAYLOAD_H
#define MOMENTSPUBLISHPAYLOAD_H

#include <QJsonObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <functional>

struct UploadedMediaInfo;

namespace memochat::moments
{

struct MomentPublishTaskResult
{
    bool ok = false;
    QString errorText;
    QVariantList items;
};

QVariantMap normalizeMomentAttachment(const QVariantMap& source);
QVariantList buildTextMomentItem(const QString& content);
QVariantList buildImageMomentItem(const QString& mediaKey, const QString& thumbKey, int width, int height);
QVariantList
buildVideoMomentItem(const QString& mediaKey, const QString& thumbKey, int width, int height, int durationMs);
MomentPublishTaskResult
buildUploadedDraftMomentItems(const QString& text,
                              const QVariantList& normalizedAttachments,
                              int uid,
                              const QString& token,
                              const std::function<void(int, int, const QString&, int)>& progressCallback);
QJsonObject
buildPublishPayload(QJsonObject authPayload, const QString& location, int visibility, const QVariantList& items);

} // namespace memochat::moments

#endif // MOMENTSPUBLISHPAYLOAD_H
