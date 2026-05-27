#include "httpmgr.h"
#include "TelemetryUtils.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QSettings>
#include <QStringList>
#include <QTimer>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QVector>

namespace {
constexpr int kHttpTimeoutMs = 10000;
constexpr int kAiHttpTimeoutMs = 300000;

bool isAiRequest(const QUrl &url, const QString &module)
{
    const QString normalized = module.trimmed().toLower();
    if (normalized.startsWith(QStringLiteral("ai"))) {
        return true;
    }
    const QString path = url.path().trimmed().toLower();
    return path == QStringLiteral("/ai") || path.startsWith(QStringLiteral("/ai/"));
}

int timeoutForRequest(const QUrl &url, const QString &module)
{
    return isAiRequest(url, module) ? kAiHttpTimeoutMs : kHttpTimeoutMs;
}

QString gateConfigValue(const QSettings &settings, const QString &lowerKey, const QString &upperKey)
{
    QString value = settings.value(lowerKey).toString().trimmed();
    if (value.isEmpty()) {
        value = settings.value(upperKey).toString().trimmed();
    }
    return value;
}

int parsePort(const QString &value, int fallback)
{
    bool ok = false;
    const int port = value.trimmed().toInt(&ok);
    return ok && port > 0 ? port : fallback;
}

QUrl withGateEndpoint(const QUrl &source, const QString &scheme, const QString &host, int port)
{
    QUrl next = source;
    next.setScheme(scheme);
    next.setHost(host);
    next.setPort(port);
    return next;
}

void appendUniqueUrl(QVector<QUrl> &urls, const QUrl &url)
{
    if (!url.isValid() || url.host().isEmpty() || url.port() <= 0) {
        return;
    }
    const QString key = url.toString(QUrl::RemoveUserInfo);
    for (const auto &existing : urls) {
        if (existing.toString(QUrl::RemoveUserInfo) == key) {
            return;
        }
    }
    urls.push_back(url);
}

QStringList protocolOrder(const QString &preferred)
{
    const QString normalized = preferred.trimmed().toLower();
    if (normalized == QStringLiteral("http1") || normalized == QStringLiteral("http1.1") ||
        normalized == QStringLiteral("h1")) {
        return {QStringLiteral("http1")};
    }
    if (normalized == QStringLiteral("http2") || normalized == QStringLiteral("h2")) {
        return {QStringLiteral("http2"), QStringLiteral("http1")};
    }
    return {QStringLiteral("http3"), QStringLiteral("http2"), QStringLiteral("http1")};
}

QVector<QUrl> gateProtocolFallbackUrls(const QUrl &url)
{
    const QString path =
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config.ini"));
    QSettings settings(path, QSettings::IniFormat);
    QString gateHost = gateConfigValue(settings, QStringLiteral("GateServer/host"), QStringLiteral("GateServer/Host"));
    if (gateHost.isEmpty()) {
        gateHost = QStringLiteral("127.0.0.1");
    }
    if (gateHost.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0) {
        gateHost = QStringLiteral("127.0.0.1");
    }
    const int configuredPort = parsePort(
        gateConfigValue(settings, QStringLiteral("GateServer/port"), QStringLiteral("GateServer/Port")),
        8080);
    const int h1DirectPort = parsePort(
        gateConfigValue(settings, QStringLiteral("GateServer/http_port"), QStringLiteral("GateServer/HttpPort")),
        configuredPort);
    const int h2Port = parsePort(
        gateConfigValue(settings, QStringLiteral("GateServer/http2_port"), QStringLiteral("GateServer/Http2Port")),
        0);
    const int h3Port = parsePort(
        gateConfigValue(settings, QStringLiteral("GateServer/http3_port"), QStringLiteral("GateServer/Http3Port")),
        0);
    const QString preferred = gateConfigValue(
        settings,
        QStringLiteral("GateServer/preferred_http_protocol"),
        QStringLiteral("GateServer/PreferredHttpProtocol"));

    if (url.host().compare(gateHost, Qt::CaseInsensitive) != 0) {
        return {url};
    }

    QVector<QUrl> urls;
    const QStringList order = protocolOrder(preferred);
    for (const QString &protocol : order) {
        if (protocol == QStringLiteral("http3")) {
            if (h3Port > 0) {
                qInfo() << "HTTP/3 gate endpoint configured at" << gateHost << h3Port
                        << "but Qt Network has no HTTP/3 transport here; falling back to HTTP/2/HTTP/1.1";
            }
            continue;
        }
        if (protocol == QStringLiteral("http2") && h2Port > 0) {
            appendUniqueUrl(urls, withGateEndpoint(url, QStringLiteral("https"), gateHost, h2Port));
            continue;
        }
        if (protocol == QStringLiteral("http1")) {
            appendUniqueUrl(urls, withGateEndpoint(url, QStringLiteral("http"), gateHost, configuredPort));
            if (h1DirectPort != configuredPort) {
                appendUniqueUrl(urls, withGateEndpoint(url, QStringLiteral("http"), gateHost, h1DirectPort));
            }
        }
    }
    appendUniqueUrl(urls, url);
    if (urls.isEmpty()) {
        urls.push_back(url);
    }
    return urls;
}
}  // namespace

HttpMgr::~HttpMgr()
{
}

void HttpMgr::clearConnectionCache()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    _manager.clearConnectionCache();
#endif
}

void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod, const QString &module)
{
    QVector<QUrl> urls = gateProtocolFallbackUrls(url);
    const QUrl first = urls.takeFirst();
    postHttpReqInternal(first, QJsonDocument(json).toJson(QJsonDocument::Compact), req_id, mod, module, urls);
}

void HttpMgr::GetHttpReq(QUrl url, ReqId req_id, Modules mod, const QString &module)
{
    QVector<QUrl> urls = gateProtocolFallbackUrls(url);
    const QUrl first = urls.takeFirst();
    getHttpReqInternal(first, req_id, mod, module, urls);
}

void HttpMgr::postHttpReqInternal(const QUrl &url,
    const QByteArray &data,
    ReqId req_id,
    Modules mod,
    const QString &module,
    QVector<QUrl> fallbackUrls)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));
    // Server closes the socket after each response (keep_alive false). Force fresh TCP for each
    // request so Qt does not reuse a half-dead connection from the pool (fixes "login once" issues).
    if (url.scheme().toLower() == QLatin1String("http")) {
        request.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("close"));
    }

    if (url.scheme().toLower() == QLatin1String("https")) {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
        request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
#endif
    }

    QString traceId;
    QString requestId;
    QString spanId;
    applyTraceHeaders(request, &traceId, &requestId, &spanId);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(timeoutForRequest(url, module));
#endif

    auto self = shared_from_this();
    const qint64 startAtMs = QDateTime::currentMSecsSinceEpoch();
    QNetworkReply *reply = _manager.post(request, data);

    QTimer *timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    QObject::connect(timeoutTimer, &QTimer::timeout, reply, [reply]() {
        if (reply->isRunning()) {
            qWarning() << "HTTP request timeout, aborting:" << reply->url();
            reply->abort();
        }
    });
    timeoutTimer->start(timeoutForRequest(url, module));

    QObject::connect(reply, &QNetworkReply::finished, [reply, self, req_id, mod, timeoutTimer, traceId, requestId, spanId, startAtMs, module, data, fallbackUrls]() mutable {
        timeoutTimer->stop();
        const qint64 durationMs = qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startAtMs);
        QVariantMap attrs;
        attrs.insert("http.method", QStringLiteral("POST"));
        attrs.insert("http.url", reply->url().toString());
        attrs.insert("module", module);
        attrs.insert("request.id", requestId);

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "HTTP POST failed:" << reply->url() << reply->errorString();
            attrs.insert("error", reply->errorString());
            attrs.insert("error.type", QStringLiteral("network"));
            exportZipkinSpan(QStringLiteral("HTTP POST %1").arg(reply->url().path()),
                QStringLiteral("CLIENT"),
                traceId,
                spanId,
                QString(),
                startAtMs,
                durationMs,
                attrs);

            if (!fallbackUrls.isEmpty()) {
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

        const QUrl replyBase = reply->url();
        if (replyBase.isValid() && !replyBase.scheme().isEmpty() && !replyBase.authority().isEmpty()) {
            gate_url_prefix = replyBase.scheme() + QStringLiteral("://") + replyBase.authority();
            if (gate_media_url_prefix.trimmed().isEmpty()) {
                gate_media_url_prefix = gate_url_prefix;
            }
        }

        QString res = reply->readAll();
        const QByteArray responseTrace = reply->rawHeader("X-Trace-Id");
        const QByteArray responseRequest = reply->rawHeader("X-Request-Id");
        attrs.insert("http.status_code", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
        exportZipkinSpan(QStringLiteral("HTTP POST %1").arg(reply->url().path()),
            QStringLiteral("CLIENT"),
            !responseTrace.isEmpty() ? QString::fromUtf8(responseTrace) : traceId,
            spanId,
            QString(),
            startAtMs,
            durationMs,
            attrs);
        QJsonParseError parseError;
        const QJsonDocument responseDoc = QJsonDocument::fromJson(res.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && responseDoc.isObject()) {
            QJsonObject responseObj = responseDoc.object();
            if (!responseTrace.isEmpty() && !responseObj.contains("trace_id")) {
                responseObj.insert("trace_id", QString::fromUtf8(responseTrace));
            }
            if (!responseRequest.isEmpty() && !responseObj.contains("request_id")) {
                responseObj.insert("request_id", QString::fromUtf8(responseRequest));
            }
            res = QString::fromUtf8(QJsonDocument(responseObj).toJson(QJsonDocument::Compact));
        }

        emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);
        reply->deleteLater();
    });
}

void HttpMgr::getHttpReqInternal(const QUrl &url,
    ReqId req_id,
    Modules mod,
    const QString &module,
    QVector<QUrl> fallbackUrls)
{
    QNetworkRequest request(url);
    if (url.scheme().toLower() == QLatin1String("http")) {
        request.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("close"));
    }

    if (url.scheme().toLower() == QLatin1String("https")) {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
        request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
#endif
    }

    QString traceId;
    QString requestId;
    QString spanId;
    applyTraceHeaders(request, &traceId, &requestId, &spanId);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(timeoutForRequest(url, module));
#endif

    auto self = shared_from_this();
    const qint64 startAtMs = QDateTime::currentMSecsSinceEpoch();
    QNetworkReply *reply = _manager.get(request);

    QTimer *timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    QObject::connect(timeoutTimer, &QTimer::timeout, reply, [reply]() {
        if (reply->isRunning()) {
            qWarning() << "HTTP request timeout, aborting:" << reply->url();
            reply->abort();
        }
    });
    timeoutTimer->start(timeoutForRequest(url, module));

    QObject::connect(reply, &QNetworkReply::finished, [reply, self, req_id, mod, timeoutTimer, traceId, requestId, spanId, startAtMs, module, fallbackUrls]() mutable {
        timeoutTimer->stop();
        const qint64 durationMs = qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startAtMs);
        QVariantMap attrs;
        attrs.insert("http.method", QStringLiteral("GET"));
        attrs.insert("http.url", reply->url().toString());
        attrs.insert("module", module);
        attrs.insert("request.id", requestId);

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "HTTP GET failed:" << reply->url() << reply->errorString();
            attrs.insert("error", reply->errorString());
            attrs.insert("error.type", QStringLiteral("network"));
            exportZipkinSpan(QStringLiteral("HTTP GET %1").arg(reply->url().path()),
                QStringLiteral("CLIENT"),
                traceId,
                spanId,
                QString(),
                startAtMs,
                durationMs,
                attrs);

            if (!fallbackUrls.isEmpty()) {
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

        const QUrl replyBase = reply->url();
        if (replyBase.isValid() && !replyBase.scheme().isEmpty() && !replyBase.authority().isEmpty()) {
            gate_url_prefix = replyBase.scheme() + QStringLiteral("://") + replyBase.authority();
            if (gate_media_url_prefix.trimmed().isEmpty()) {
                gate_media_url_prefix = gate_url_prefix;
            }
        }

        QString res = reply->readAll();
        const QByteArray responseTrace = reply->rawHeader("X-Trace-Id");
        const QByteArray responseRequest = reply->rawHeader("X-Request-Id");
        attrs.insert("http.status_code", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
        exportZipkinSpan(QStringLiteral("HTTP GET %1").arg(reply->url().path()),
            QStringLiteral("CLIENT"),
            !responseTrace.isEmpty() ? QString::fromUtf8(responseTrace) : traceId,
            spanId,
            QString(),
            startAtMs,
            durationMs,
            attrs);
        QJsonParseError parseError;
        const QJsonDocument responseDoc = QJsonDocument::fromJson(res.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && responseDoc.isObject()) {
            QJsonObject responseObj = responseDoc.object();
            if (!responseTrace.isEmpty() && !responseObj.contains("trace_id")) {
                responseObj.insert("trace_id", QString::fromUtf8(responseTrace));
            }
            if (!responseRequest.isEmpty() && !responseObj.contains("request_id")) {
                responseObj.insert("request_id", QString::fromUtf8(responseRequest));
            }
            res = QString::fromUtf8(QJsonDocument(responseObj).toJson(QJsonDocument::Compact));
        }

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
    if (mod == Modules::REGISTERMOD) {
        emit sig_reg_mod_finish(id, res, err);
    }

    if (mod == Modules::RESETMOD) {
        emit sig_reset_mod_finish(id, res, err);
    }

    if (mod == Modules::LOGINMOD) {
        emit sig_login_mod_finish(id, res, err);
    }

    if (mod == Modules::SETTINGSMOD) {
        emit sig_settings_mod_finish(id, res, err);
    }

    if (mod == Modules::CALLMOD) {
        emit sig_call_mod_finish(id, res, err);
    }

    if (mod == Modules::MOMENTSMOD) {
        emit sig_moments_mod_finish(id, res, err);
    }
}
