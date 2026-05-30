#ifndef MEDIAUPLOADSERVICEPRIVATE_H
#define MEDIAUPLOADSERVICEPRIVATE_H

#include "MediaUploadService.h"

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QString>
#include <QUrl>
#include <QtGlobal>

namespace MediaUploadServicePrivate
{

struct ClientMediaConfig
{
    qint64 maxImageBytes = 200LL * 1024 * 1024;
    qint64 maxFileBytes = 20480LL * 1024 * 1024;
    int chunkSizeBytes = 4 * 1024 * 1024;
    int chunkRetry = 3;
};

QString resolveLocalPath(const QString& localFileUrl);
QUrl mediaUploadUrl(const QString& path);
QJsonObject normalizeMediaResponse(QJsonObject responseObj);
QString responseBodyPreview(const QByteArray& body);
QString resolveUploadedMediaUrl(const QString& remoteUrl);
bool populateUploadedMediaInfo(const QJsonObject& responseObj,
                               const QFileInfo& fileInfo,
                               const QString& mimeType,
                               UploadedMediaInfo* outInfo,
                               QString* errorText);
ClientMediaConfig loadMediaConfig();
bool isApiSuccess(const QJsonObject& obj, QString* errorText);

bool postJson(const QUrl& url, const QJsonObject& payload, QJsonObject* responseObj, QString* errorText);
bool postBinary(const QUrl& url,
                const QByteArray& payload,
                const QList<QPair<QByteArray, QByteArray>>& headers,
                QJsonObject* responseObj,
                QString* errorText);
bool getJson(const QUrl& url, QJsonObject* responseObj, QString* errorText);

bool uploadAvatarFile(QFile& file,
                      const QFileInfo& fileInfo,
                      const QString& mediaType,
                      const QString& mimeType,
                      int uid,
                      const QString& token,
                      UploadedMediaInfo* outInfo,
                      QString* errorText,
                      const MediaUploadService::UploadProgressCallback& progress);
bool uploadChunkedFile(QFile& file,
                       const QFileInfo& fileInfo,
                       const QString& mediaType,
                       const QString& mimeType,
                       qint64 fileSize,
                       const ClientMediaConfig& mediaCfg,
                       int uid,
                       const QString& token,
                       UploadedMediaInfo* outInfo,
                       QString* errorText,
                       const MediaUploadService::UploadProgressCallback& progress);

} // namespace MediaUploadServicePrivate

#endif // MEDIAUPLOADSERVICEPRIVATE_H
