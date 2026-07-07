#include "httpmgr.h"
#include "HttpMgrRequestUtils.h"
#include "TelemetryUtils.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QTimer>
#include <QVector>

HttpMgr::~HttpMgr()
{
}

void HttpMgr::clearConnectionCache()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    _manager.clearConnectionCache();
#endif
}

void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod, const QString& module)
{
    QVector<QUrl> urls = gateProtocolFallbackUrls(url);
    const QUrl first = urls.takeFirst();
    postHttpReqInternal(first, QJsonDocument(json).toJson(QJsonDocument::Compact), req_id, mod, module, urls, true);
}

void HttpMgr::PostAnonymousHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod, const QString& module)
{
    QVector<QUrl> urls = gateProtocolFallbackUrls(url);
    const QUrl first = urls.takeFirst();
    postHttpReqInternal(first, QJsonDocument(json).toJson(QJsonDocument::Compact), req_id, mod, module, urls, false);
}

void HttpMgr::GetHttpReq(QUrl url, ReqId req_id, Modules mod, const QString& module)
{
    QVector<QUrl> urls = gateProtocolFallbackUrls(url);
    const QUrl first = urls.takeFirst();
    getHttpReqInternal(first, req_id, mod, module, urls);
}

void HttpMgr::postHttpReqInternal(const QUrl& url,
                                  const QByteArray& data,
                                  ReqId req_id,
                                  Modules mod,
                                  const QString& module,
                                  QVector<QUrl> fallbackUrls,
                                  bool withAuth)
{
    QNetworkRequest request(url);
    if (withAuth)
    {
        prepareJsonRequest(request, data);
    }
    else
    {
        prepareUnauthenticatedJsonRequest(request, data);
    }

    QString traceId;
    QString requestId;
    QString spanId;
    applyTraceHeaders(request, &traceId, &requestId, &spanId);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(httpTimeoutForRequest(url, module));
#endif

    auto self = shared_from_this();
    const qint64 startAtMs = QDateTime::currentMSecsSinceEpoch();
    QNetworkReply* reply = _manager.post(request, data);
    // VerifyNone on the SSL config suppresses socket-level verification, but
    // QNetworkAccessManager still aborts on self-signed cert errors unless we
    // explicitly opt out at the reply level too.
    if (url.scheme().compare(QLatin1String("https"), Qt::CaseInsensitive) == 0)
    {
        reply->ignoreSslErrors();
    }

    QTimer* timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    QObject::connect(timeoutTimer,
                     &QTimer::timeout,
                     reply,
                     [reply]()
                     {
                         if (reply->isRunning())
                         {
                             qWarning() << "HTTP request timeout, aborting:" << reply->url();
                             reply->abort();
                         }
                     });
    timeoutTimer->start(httpTimeoutForRequest(url, module));

    QObject::connect(
        reply,
        &QNetworkReply::finished,
        [reply,
         self,
         req_id,
         mod,
         timeoutTimer,
         traceId,
         requestId,
         spanId,
         startAtMs,
         module,
         data,
         fallbackUrls]() mutable
        {
            timeoutTimer->stop();
            const qint64 durationMs = qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startAtMs);
            QVariantMap attrs;
            attrs.insert("http.method", QStringLiteral("POST"));
            attrs.insert("http.url", redactedUrlForTelemetry(reply->url()));
            attrs.insert("module", module);
            attrs.insert("request.id", requestId);

            if (reply->error() != QNetworkReply::NoError)
            {
                qWarning() << "HTTP POST failed:" << reply->url() << reply->errorString();
                attrs.insert("error", reply->errorString());
                attrs.insert("error.type", QStringLiteral("network"));
                exportZipkinSpan(
                    QStringLiteral("HTTP POST %1").arg(reply->url().path()),
                                   QStringLiteral("CLIENT"), traceId, spanId, QString(), startAtMs, durationMs, attrs);

                if (!fallbackUrls.isEmpty())
                {
                    const QUrl fb = fallbackUrls.takeFirst();
                    qWarning() << "Retrying gate HTTP POST using fallback endpoint:" << fb.toString();
                    reply->deleteLater();
                    self->postHttpReqInternal(fb, data, req_id, mod, module, fallbackUrls);
                    return;
                }

                emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
                reply->deleteLater();
                return;
            }

            updateGatePrefixesFromReplyUrl(reply->url());

            const QByteArray body = reply->readAll();
            const QByteArray responseTrace = reply->rawHeader("X-Trace-Id");
            const QByteArray responseRequest = reply->rawHeader("X-Request-Id");
            attrs.insert("http.status_code", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
            exportZipkinSpan(QStringLiteral("HTTP POST %1").arg(reply->url().path()),
                                            QStringLiteral("CLIENT"),
                                                           !responseTrace.isEmpty() ? QString::fromUtf8(responseTrace)
                                                                                    : traceId,
                                                           spanId,
                                                           QString(),
                                                           startAtMs,
                                                           durationMs,
                                                           attrs);
            const QString res = responseWithTraceHeaders(body, responseTrace, responseRequest);

            emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);
            reply->deleteLater();
        });
}

void HttpMgr::getHttpReqInternal(const QUrl& url,
                                 ReqId req_id,
                                 Modules mod,
                                 const QString& module,
                                 QVector<QUrl> fallbackUrls)
{
    QNetworkRequest request(url);
    prepareGetRequest(request);

    QString traceId;
    QString requestId;
    QString spanId;
    applyTraceHeaders(request, &traceId, &requestId, &spanId);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(httpTimeoutForRequest(url, module));
#endif

    auto self = shared_from_this();
    const qint64 startAtMs = QDateTime::currentMSecsSinceEpoch();
    QNetworkReply* reply = _manager.get(request);
    if (url.scheme().compare(QLatin1String("https"), Qt::CaseInsensitive) == 0)
    {
        reply->ignoreSslErrors();
    }

    QTimer* timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    QObject::connect(timeoutTimer,
                     &QTimer::timeout,
                     reply,
                     [reply]()
                     {
                         if (reply->isRunning())
                         {
                             qWarning() << "HTTP request timeout, aborting:" << reply->url();
                             reply->abort();
                         }
                     });
    timeoutTimer->start(httpTimeoutForRequest(url, module));

    QObject::connect(
        reply,
        &QNetworkReply::finished,
        [reply, self, req_id, mod, timeoutTimer, traceId, requestId, spanId, startAtMs, module, fallbackUrls]() mutable
        {
            timeoutTimer->stop();
            const qint64 durationMs = qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startAtMs);
            QVariantMap attrs;
            attrs.insert("http.method", QStringLiteral("GET"));
            attrs.insert("http.url", redactedUrlForTelemetry(reply->url()));
            attrs.insert("module", module);
            attrs.insert("request.id", requestId);

            if (reply->error() != QNetworkReply::NoError)
            {
                qWarning() << "HTTP GET failed:" << reply->url() << reply->errorString();
                attrs.insert("error", reply->errorString());
                attrs.insert("error.type", QStringLiteral("network"));
                exportZipkinSpan(
                    QStringLiteral("HTTP GET %1").arg(reply->url().path()),
                                   QStringLiteral("CLIENT"), traceId, spanId, QString(), startAtMs, durationMs, attrs);

                if (!fallbackUrls.isEmpty())
                {
                    const QUrl fb = fallbackUrls.takeFirst();
                    qWarning() << "Retrying gate HTTP GET using fallback endpoint:" << fb.toString();
                    reply->deleteLater();
                    self->getHttpReqInternal(fb, req_id, mod, module, fallbackUrls);
                    return;
                }

                emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
                reply->deleteLater();
                return;
            }

            updateGatePrefixesFromReplyUrl(reply->url());

            const QByteArray body = reply->readAll();
            const QByteArray responseTrace = reply->rawHeader("X-Trace-Id");
            const QByteArray responseRequest = reply->rawHeader("X-Request-Id");
            attrs.insert("http.status_code", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
            exportZipkinSpan(QStringLiteral("HTTP GET %1").arg(reply->url().path()),
                                            QStringLiteral("CLIENT"),
                                                           !responseTrace.isEmpty() ? QString::fromUtf8(responseTrace)
                                                                                    : traceId,
                                                           spanId,
                                                           QString(),
                                                           startAtMs,
                                                           durationMs,
                                                           attrs);
            const QString res = responseWithTraceHeaders(body, responseTrace, responseRequest);

            emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);
            reply->deleteLater();
        });
}

HttpMgr::HttpMgr()
{
    connect(this, &HttpMgr::sig_http_finish, this, &HttpMgr::slot_http_finish);
}

void HttpMgr::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod)
{
    if (mod == Modules::REGISTERMOD)
    {
        emit sig_reg_mod_finish(id, res, err);
    }

    if (mod == Modules::RESETMOD)
    {
        emit sig_reset_mod_finish(id, res, err);
    }

    if (mod == Modules::LOGINMOD)
    {
        emit sig_login_mod_finish(id, res, err);
    }

    if (mod == Modules::SETTINGSMOD)
    {
        emit sig_settings_mod_finish(id, res, err);
    }

    if (mod == Modules::CALLMOD)
    {
        emit sig_call_mod_finish(id, res, err);
    }

    if (mod == Modules::MOMENTSMOD)
    {
        emit sig_moments_mod_finish(id, res, err);
    }

    if (mod == Modules::CONTACTMOD)
    {
        emit sig_contact_mod_finish(id, res, err);
    }
}
