#include "MediaUploadServicePrivate.h"

#include "TelemetryUtils.h"

#include <QDateTime>
#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QTimer>
#include <QVariantMap>

namespace MediaUploadServicePrivate
{

void configureSslIfNeeded(QNetworkRequest* request, const QUrl& url)
{
    if (url.scheme().toLower() == "https")
    {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request->setSslConfiguration(sslConfig);
    }
}

bool postJson(const QUrl& url, const QJsonObject& payload, QJsonObject* responseObj, QString* errorText)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    configureSslIfNeeded(&request, url);

    QString traceId;
    QString requestId;
    QString spanId;
    applyTraceHeaders(request, &traceId, &requestId, &spanId);
    const qint64 startAtMs = QDateTime::currentMSecsSinceEpoch();

    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    if (!reply)
    {
        if (errorText)
        {
            *errorText = "请求创建失败";
        }
        return false;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(30000);
    QObject::connect(&timer,
                     &QTimer::timeout,
                     [&]()
                     {
                         if (reply->isRunning())
                         {
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
    spanAttrs.insert("http.url", redactedUrlForTelemetry(url));
    spanAttrs.insert("module", QStringLiteral("media"));
    spanAttrs.insert("request.id", requestId);
    spanAttrs.insert("http.status_code", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    reply->deleteLater();
    if (netErr != QNetworkReply::NoError)
    {
        spanAttrs.insert("error", QStringLiteral("network"));
        exportZipkinSpan(QStringLiteral("HTTP POST %1").arg(url.path()),
                                        QStringLiteral("CLIENT"),
                                                       traceId,
                                                       spanId,
                                                       QString(),
                                                       startAtMs,
                                                       qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startAtMs),
                                                       spanAttrs);
        if (errorText)
        {
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
    if (!doc.isObject())
    {
        if (errorText)
        {
            const QString preview = responseBodyPreview(body);
            *errorText = preview.isEmpty() ? QStringLiteral("服务响应格式错误")
                                           : QStringLiteral("服务响应格式错误: %1").arg(preview);
        }
        return false;
    }
    if (responseObj)
    {
        *responseObj = normalizeMediaResponse(doc.object());
    }
    return true;
}

bool postBinary(const QUrl& url,
                const QByteArray& payload,
                const QList<QPair<QByteArray, QByteArray>>& headers,
                QJsonObject* responseObj,
                QString* errorText)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    configureSslIfNeeded(&request, url);

    QString traceId;
    QString requestId;
    QString spanId;
    applyTraceHeaders(request, &traceId, &requestId, &spanId);
    for (const auto& header : headers)
    {
        request.setRawHeader(header.first, header.second);
    }
    const qint64 startAtMs = QDateTime::currentMSecsSinceEpoch();

    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.post(request, payload);
    if (!reply)
    {
        if (errorText)
        {
            *errorText = "请求创建失败";
        }
        return false;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(30000);
    QObject::connect(&timer,
                     &QTimer::timeout,
                     [&]()
                     {
                         if (reply->isRunning())
                         {
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
    spanAttrs.insert("http.url", redactedUrlForTelemetry(url));
    spanAttrs.insert("module", QStringLiteral("media"));
    spanAttrs.insert("request.id", requestId);
    spanAttrs.insert("http.status_code", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    reply->deleteLater();
    if (netErr != QNetworkReply::NoError)
    {
        spanAttrs.insert("error", QStringLiteral("network"));
        exportZipkinSpan(QStringLiteral("HTTP POST %1").arg(url.path()),
                                        QStringLiteral("CLIENT"),
                                                       traceId,
                                                       spanId,
                                                       QString(),
                                                       startAtMs,
                                                       qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startAtMs),
                                                       spanAttrs);
        if (errorText)
        {
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
    if (!doc.isObject())
    {
        if (errorText)
        {
            const QString preview = responseBodyPreview(body);
            *errorText = preview.isEmpty() ? QStringLiteral("服务响应格式错误")
                                           : QStringLiteral("服务响应格式错误: %1").arg(preview);
        }
        return false;
    }
    if (responseObj)
    {
        *responseObj = normalizeMediaResponse(doc.object());
    }
    return true;
}

bool getJson(const QUrl& url, QJsonObject* responseObj, QString* errorText)
{
    QNetworkRequest request(url);
    configureSslIfNeeded(&request, url);

    QString traceId;
    QString requestId;
    QString spanId;
    applyTraceHeaders(request, &traceId, &requestId, &spanId);
    const qint64 startAtMs = QDateTime::currentMSecsSinceEpoch();

    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.get(request);
    if (!reply)
    {
        if (errorText)
        {
            *errorText = "请求创建失败";
        }
        return false;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(30000);
    QObject::connect(&timer,
                     &QTimer::timeout,
                     [&]()
                     {
                         if (reply->isRunning())
                         {
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
    spanAttrs.insert("http.url", redactedUrlForTelemetry(url));
    spanAttrs.insert("module", QStringLiteral("media"));
    spanAttrs.insert("request.id", requestId);
    spanAttrs.insert("http.status_code", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    reply->deleteLater();
    if (netErr != QNetworkReply::NoError)
    {
        spanAttrs.insert("error", QStringLiteral("network"));
        exportZipkinSpan(QStringLiteral("HTTP GET %1").arg(url.path()),
                                        QStringLiteral("CLIENT"),
                                                       traceId,
                                                       spanId,
                                                       QString(),
                                                       startAtMs,
                                                       qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startAtMs),
                                                       spanAttrs);
        if (errorText)
        {
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
    if (!doc.isObject())
    {
        if (errorText)
        {
            *errorText = "服务响应格式错误";
        }
        return false;
    }
    if (responseObj)
    {
        *responseObj = doc.object();
    }
    return true;
}

} // namespace MediaUploadServicePrivate
