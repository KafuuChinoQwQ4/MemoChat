#include "MediaUploadService.h"
#include "global.h"
#include "TelemetryUtils.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QtGlobal>
#include <QSslConfiguration>
#include <QSslSocket>

namespace {
QString resolveLocalPath(const QString &localFileUrl)
{
    QUrl url(localFileUrl);
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    return localFileUrl;
}

struct ClientMediaConfig {
    qint64 maxImageBytes = 200LL * 1024 * 1024;
    qint64 maxFileBytes = 20480LL * 1024 * 1024;
    int chunkSizeBytes = 4 * 1024 * 1024;
    int chunkRetry = 3;
};

ClientMediaConfig loadMediaConfig()
{
    ClientMediaConfig cfg;
    const QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    const qint64 maxImageMb = settings.value("Media/MaxImageMB", 200).toLongLong();
    const qint64 maxFileMb = settings.value("Media/MaxFileMB", 20480).toLongLong();
    const int chunkKb = settings.value("Media/ChunkSizeKB", 4096).toInt();
    const int retry = settings.value("Media/ChunkRetry", 3).toInt();
    if (maxImageMb > 0) {
        cfg.maxImageBytes = maxImageMb * 1024 * 1024;
    }
    if (maxFileMb > 0) {
        cfg.maxFileBytes = maxFileMb * 1024 * 1024;
    }
    if (chunkKb > 0) {
        cfg.chunkSizeBytes = qBound(256 * 1024, chunkKb * 1024, 4 * 1024 * 1024);
    }
    if (retry > 0) {
        cfg.chunkRetry = qBound(1, retry, 8);
    }
    return cfg;
}

bool postJson(const QUrl &url, const QJsonObject &payload, QJsonObject *responseObj, QString *errorText)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Enable HTTPS with self-signed cert support
    if (url.scheme().toLower() == "https") {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
    }

    QString traceId;
    QString requestId;
    QString spanId;
    applyTraceHeaders(request, &traceId, &requestId, &spanId);
    const qint64 startAtMs = QDateTime::currentMSecsSinceEpoch();

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    if (!reply) {
        if (errorText) {
            *errorText = "请求创建失败";
        }
        return false;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(30000);
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();

    const auto netErr = reply->error();
    const QByteArray body = reply->readAll();
    QVariantMap spanAttrs;
    spanAttrs.insert("http.method", QStringLiteral("POST"));
    spanAttrs.insert("http.url", url.toString());
    spanAttrs.insert("module", QStringLiteral("media"));
    spanAttrs.insert("request.id", requestId);
    spanAttrs.insert("http.status_code", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    reply->deleteLater();
    if (netErr != QNetworkReply::NoError) {
        spanAttrs.insert("error", QStringLiteral("network"));
        exportZipkinSpan(QStringLiteral("HTTP POST %1").arg(url.path()),
                         QStringLiteral("CLIENT"),
                         traceId,
                         spanId,
                         QString(),
                         startAtMs,
                         qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startAtMs),
                         spanAttrs);
        if (errorText) {
            *errorText = "网络请求失败";
        }
        return false;
    }
    exportZipkinSpan(QStringLiteral("HTTP POST %1").arg(url.path()),
                     QStringLiteral("CLIENT"),
                     traceId,
                     spanId,
                     QString(),
                     startAtMs,
                     qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startAtMs),
                     spanAttrs);

    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        if (errorText) {
            *errorText = "服务响应格式错误";
        }
        return false;
    }
    if (responseObj) {
        *responseObj = doc.object();
    }
    return true;
}

bool postBinary(const QUrl &url,
                const QByteArray &payload,
                const QList<QPair<QByteArray, QByteArray>> &headers,
                QJsonObject *responseObj,
                QString *errorText)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");

    // Enable HTTPS with self-signed cert support
    if (url.scheme().toLower() == "https") {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
    }

    QString traceId;
    QString requestId;
    QString spanId;
    applyTraceHeaders(request, &traceId, &requestId, &spanId);
    for (const auto &header : headers) {
        request.setRawHeader(header.first, header.second);
    }
    const qint64 startAtMs = QDateTime::currentMSecsSinceEpoch();

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(request, payload);
    if (!reply) {
        if (errorText) {
            *errorText = "请求创建失败";
        }
        return false;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(30000);
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();

    const auto netErr = reply->error();
    const QByteArray body = reply->readAll();
    QVariantMap spanAttrs;
    spanAttrs.insert("http.method", QStringLiteral("POST"));
    spanAttrs.insert("http.url", url.toString());
    spanAttrs.insert("module", QStringLiteral("media"));
    spanAttrs.insert("request.id", requestId);
    spanAttrs.insert("http.status_code", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    reply->deleteLater();
    if (netErr != QNetworkReply::NoError) {
        spanAttrs.insert("error", QStringLiteral("network"));
        exportZipkinSpan(QStringLiteral("HTTP POST %1").arg(url.path()),
                         QStringLiteral("CLIENT"),
                         traceId,
                         spanId,
                         QString(),
                         startAtMs,
                         qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startAtMs),
                         spanAttrs);
        if (errorText) {
            *errorText = "网络请求失败";
        }
        return false;
    }
    exportZipkinSpan(QStringLiteral("HTTP POST %1").arg(url.path()),
                     QStringLiteral("CLIENT"),
                     traceId,
                     spanId,
                     QString(),
                     startAtMs,
                     qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startAtMs),
                     spanAttrs);

    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        if (errorText) {
            *errorText = "服务响应格式错误";
        }
        return false;
    }
    if (responseObj) {
        *responseObj = doc.object();
    }
    return true;
}

bool getJson(const QUrl &url, QJsonObject *responseObj, QString *errorText)
{
    QNetworkRequest request(url);

    // Enable HTTPS with self-signed cert support
    if (url.scheme().toLower() == "https") {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
    }

    QString traceId;
    QString requestId;
    QString spanId;
    applyTraceHeaders(request, &traceId, &requestId, &spanId);
    const qint64 startAtMs = QDateTime::currentMSecsSinceEpoch();

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(request);
    if (!reply) {
        if (errorText) {
            *errorText = "请求创建失败";
        }
        return false;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(30000);
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();

    const auto netErr = reply->error();
    const QByteArray body = reply->readAll();
    QVariantMap spanAttrs;
    spanAttrs.insert("http.method", QStringLiteral("GET"));
    spanAttrs.insert("http.url", url.toString());
    spanAttrs.insert("module", QStringLiteral("media"));
    spanAttrs.insert("request.id", requestId);
    spanAttrs.insert("http.status_code", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    reply->deleteLater();
    if (netErr != QNetworkReply::NoError) {
        spanAttrs.insert("error", QStringLiteral("network"));
        exportZipkinSpan(QStringLiteral("HTTP GET %1").arg(url.path()),
                         QStringLiteral("CLIENT"),
                         traceId,
                         spanId,
                         QString(),
                         startAtMs,
                         qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startAtMs),
                         spanAttrs);
        if (errorText) {
            *errorText = "网络请求失败";
        }
        return false;
    }
    exportZipkinSpan(QStringLiteral("HTTP GET %1").arg(url.path()),
                     QStringLiteral("CLIENT"),
                     traceId,
                     spanId,
                     QString(),
                     startAtMs,
                     qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startAtMs),
                     spanAttrs);

    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        if (errorText) {
            *errorText = "服务响应格式错误";
        }
        return false;
    }
    if (responseObj) {
        *responseObj = doc.object();
    }
    return true;
}

bool isApiSuccess(const QJsonObject &obj, QString *errorText)
{
    const int error = obj.value("error").toInt(1);
    if (error == 0) {
        return true;
    }
    if (errorText) {
        *errorText = obj.value("message").toString("上传失败");
    }
    return false;
}
}

bool MediaUploadService::uploadLocalFile(const QString &localFileUrl,
                                         const QString &mediaType,
                                         int uid,
                                         const QString &token,
                                         UploadedMediaInfo *outInfo,
                                         QString *errorText,
                                         const UploadProgressCallback &progress)
{
    if (uid <= 0 || token.trimmed().isEmpty()) {
        if (errorText) {
            *errorText = "登录态失效，请重新登录";
        }
        return false;
    }

    const QString localPath = resolveLocalPath(localFileUrl);
    QFile file(localPath);
    if (!file.exists()) {
        if (errorText) {
            *errorText = "文件不存在";
        }
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (errorText) {
            *errorText = "文件读取失败";
        }
        return false;
    }

    const QFileInfo fileInfo(localPath);
    const qint64 fileSize = fileInfo.size();
    if (fileSize <= 0) {
        if (errorText) {
            *errorText = "文件内容为空";
        }
        return false;
    }

    const ClientMediaConfig mediaCfg = loadMediaConfig();
    const qint64 limit = (mediaType == "image") ? mediaCfg.maxImageBytes : mediaCfg.maxFileBytes;
    if (fileSize > limit) {
        if (errorText) {
            *errorText = QString("文件过大，请控制在 %1MB 以内").arg(limit / (1024 * 1024));
        }
        return false;
    }

    const QMimeDatabase mimeDb;
    const QString mimeType = mimeDb.mimeTypeForFile(fileInfo).name();
    const int requestedChunkSize = mediaCfg.chunkSizeBytes;

    if (progress) {
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

    QJsonObject initRsp;
    if (!postJson(QUrl(gate_url_prefix + "/upload_media_init"), initPayload, &initRsp, errorText)) {
        return false;
    }
    if (!isApiSuccess(initRsp, errorText)) {
        return false;
    }

    const QString uploadId = initRsp.value("upload_id").toString();
    const int chunkSize = qBound(256 * 1024, initRsp.value("chunk_size").toInt(requestedChunkSize), 4 * 1024 * 1024);
    const int totalChunks = initRsp.value("total_chunks").toInt(
        static_cast<int>((fileSize + chunkSize - 1) / chunkSize));
    if (uploadId.isEmpty() || totalChunks <= 0) {
        if (errorText) {
            *errorText = "上传初始化失败";
        }
        return false;
    }

    QSet<int> uploadedChunks;
    QUrl statusUrl(gate_url_prefix + "/upload_media_status");
    QUrlQuery statusQuery;
    statusQuery.addQueryItem("uid", QString::number(uid));
    statusQuery.addQueryItem("token", token);
    statusQuery.addQueryItem("upload_id", uploadId);
    statusUrl.setQuery(statusQuery);
    QJsonObject statusRsp;
    if (getJson(statusUrl, &statusRsp, nullptr) && isApiSuccess(statusRsp, nullptr)) {
        const QJsonArray uploaded = statusRsp.value("uploaded_chunks").toArray();
        for (const QJsonValue &v : uploaded) {
            uploadedChunks.insert(v.toInt(-1));
        }
    }

    qint64 uploadedBytes = 0;
    for (int idx : uploadedChunks) {
        const qint64 remain = fileSize - static_cast<qint64>(idx) * chunkSize;
        if (remain > 0) {
            uploadedBytes += qMin<qint64>(chunkSize, remain);
        }
    }
    uploadedBytes = qMin(uploadedBytes, fileSize);
    if (progress) {
        const int percent = static_cast<int>((uploadedBytes * 100) / fileSize);
        progress(percent, QStringLiteral("上传分片..."));
    }

    for (int index = 0; index < totalChunks; ++index) {
        if (uploadedChunks.contains(index)) {
            continue;
        }

        const qint64 offset = static_cast<qint64>(index) * chunkSize;
        if (!file.seek(offset)) {
            if (errorText) {
                *errorText = "读取文件失败";
            }
            return false;
        }
        const QByteArray chunkBytes = file.read(chunkSize);
        if (chunkBytes.isEmpty()) {
            if (errorText) {
                *errorText = "读取分片失败";
            }
            return false;
        }

        bool uploaded = false;
        QString lastErr;
        for (int attempt = 0; attempt < mediaCfg.chunkRetry; ++attempt) {
            QList<QPair<QByteArray, QByteArray>> headers;
            headers.append({QByteArrayLiteral("X-Uid"), QByteArray::number(uid)});
            headers.append({QByteArrayLiteral("X-Token"), token.toUtf8()});
            headers.append({QByteArrayLiteral("X-Upload-Id"), uploadId.toUtf8()});
            headers.append({QByteArrayLiteral("X-Chunk-Index"), QByteArray::number(index)});

            QJsonObject chunkRsp;
            if (!postBinary(QUrl(gate_url_prefix + "/upload_media_chunk"), chunkBytes, headers, &chunkRsp, &lastErr)) {
                continue;
            }
            if (!isApiSuccess(chunkRsp, &lastErr)) {
                continue;
            }
            uploaded = true;
            break;
        }

        if (!uploaded) {
            if (errorText) {
                *errorText = lastErr.isEmpty() ? "分片上传失败" : lastErr;
            }
            return false;
        }

        uploadedBytes += chunkBytes.size();
        uploadedBytes = qMin(uploadedBytes, fileSize);
        if (progress) {
            const int percent = static_cast<int>((uploadedBytes * 100) / fileSize);
            progress(percent, QStringLiteral("上传分片..."));
        }
    }

    QJsonObject completePayload;
    completePayload["uid"] = uid;
    completePayload["token"] = token;
    completePayload["upload_id"] = uploadId;
    QJsonObject completeRsp;
    if (!postJson(QUrl(gate_url_prefix + "/upload_media_complete"), completePayload, &completeRsp, errorText)) {
        return false;
    }
    if (!isApiSuccess(completeRsp, errorText)) {
        return false;
    }

    QString remoteUrl = completeRsp.value("url").toString();
    if (remoteUrl.startsWith("/")) {
        remoteUrl = gate_url_prefix + remoteUrl;
    }
    if (remoteUrl.isEmpty()) {
        if (errorText) {
            *errorText = "上传失败：无可用地址";
        }
        return false;
    }

    if (outInfo) {
        outInfo->remoteUrl = remoteUrl;
        outInfo->fileName = completeRsp.value("file_name").toString(fileInfo.fileName());
        outInfo->mimeType = completeRsp.value("mime").toString(mimeType);
        outInfo->sizeBytes = completeRsp.value("size").toVariant().toLongLong();
    }
    if (progress) {
        progress(100, QStringLiteral("上传完成"));
    }
    return true;
}
