#include "httpmgr.h"
#include "TelemetryUtils.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QSettings>
#include <QTimer>
#include <QSslConfiguration>
#include <QSslSocket>

namespace {
constexpr int kHttpTimeoutMs = 10000;
constexpr int kAiHttpTimeoutMs = 300000;

int timeoutForModule(const QString &module)
{
    const QString normalized = module.trimmed().toLower();
    return normalized.startsWith(QStringLiteral("ai")) ? kAiHttpTimeoutMs : kHttpTimeoutMs;
}

bool isGateHttpsPrimaryPort(const QUrl &url)
{
    if (url.scheme().toLower() != QLatin1String("https")) {
        return false;
    }
    return url.port(443) == 8443;
}

int gatePlaintextFallbackPort()
{
    const QString path =
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config.ini"));
    QSettings settings(path, QSettings::IniFormat);
    QString p = settings.value(QStringLiteral("GateServer/http_port")).toString().trimmed();
    if (p.isEmpty()) {
        p = settings.value(QStringLiteral("GateServer/HttpPort")).toString().trimmed();
    }
    if (!p.isEmpty()) {
        bool ok = false;
        const int n = p.toInt(&ok);
        if (ok && n > 0) {
            return n;
        }
    }
    return 8080;
}

QUrl gatePlaintextFallback(const QUrl &httpsUrl)
{
    QUrl fb = httpsUrl;
    fb.setScheme(QStringLiteral("http"));
    fb.setPort(gatePlaintextFallbackPort());
    return fb;
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
    postHttpReqInternal(url, QJsonDocument(json).toJson(QJsonDocument::Compact), req_id, mod, module, true);
}

void HttpMgr::GetHttpReq(QUrl url, ReqId req_id, Modules mod, const QString &module)
{
    getHttpReqInternal(url, req_id, mod, module, true);
}

void HttpMgr::postHttpReqInternal(const QUrl &url,
    const QByteArray &data,
    ReqId req_id,
    Modules mod,
    const QString &module,
    bool allowPlaintextFallback)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));
    // Server closes the socket after each response (keep_alive false). Force fresh TCP for each
    // request so Qt does not reuse a half-dead connection from the pool (fixes "login once" issues).
    request.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("close"));

    if (url.scheme().toLower() == QLatin1String("https")) {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
    }

    QString traceId;
    QString requestId;
    QString spanId;
    applyTraceHeaders(request, &traceId, &requestId, &spanId);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(timeoutForModule(module));
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
    timeoutTimer->start(timeoutForModule(module));

    QObject::connect(reply, &QNetworkReply::finished, [reply, self, req_id, mod, timeoutTimer, traceId, requestId, spanId, startAtMs, module, url, data, allowPlaintextFallback]() {
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

            const bool canFallback = allowPlaintextFallback && isGateHttpsPrimaryPort(url);
            if (canFallback) {
                const QUrl fb = gatePlaintextFallback(url);
                qWarning() << "Retrying gate HTTP POST over plaintext:" << fb.toString();
                reply->deleteLater();
                self->postHttpReqInternal(fb, data, req_id, mod, module, false);
                return;
            }

            emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
            reply->deleteLater();
            return;
        }

        const QUrl replyBase = reply->url();
        if (replyBase.isValid() && !replyBase.scheme().isEmpty() && !replyBase.authority().isEmpty()) {
            gate_url_prefix = replyBase.scheme() + QStringLiteral("://") + replyBase.authority();
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
    bool allowPlaintextFallback)
{
    QNetworkRequest request(url);
    request.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("close"));

    if (url.scheme().toLower() == QLatin1String("https")) {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
    }

    QString traceId;
    QString requestId;
    QString spanId;
    applyTraceHeaders(request, &traceId, &requestId, &spanId);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(timeoutForModule(module));
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
    timeoutTimer->start(timeoutForModule(module));

    QObject::connect(reply, &QNetworkReply::finished, [reply, self, req_id, mod, timeoutTimer, traceId, requestId, spanId, startAtMs, module, url, allowPlaintextFallback]() {
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

            const bool canFallback = allowPlaintextFallback && isGateHttpsPrimaryPort(url);
            if (canFallback) {
                const QUrl fb = gatePlaintextFallback(url);
                qWarning() << "Retrying gate HTTP GET over plaintext:" << fb.toString();
                reply->deleteLater();
                self->getHttpReqInternal(fb, req_id, mod, module, false);
                return;
            }

            emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
            reply->deleteLater();
            return;
        }

        const QUrl replyBase = reply->url();
        if (replyBase.isValid() && !replyBase.scheme().isEmpty() && !replyBase.authority().isEmpty()) {
            gate_url_prefix = replyBase.scheme() + QStringLiteral("://") + replyBase.authority();
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
