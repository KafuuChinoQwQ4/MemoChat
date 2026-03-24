#include "httpmgr.h"
#include "TelemetryUtils.h"
#include <QDateTime>
#include <QTimer>
#include <QSslConfiguration>
#include <QSslSocket>

namespace {
constexpr int kHttpTimeoutMs = 10000;

bool isGateHttpsPrimaryPort(const QUrl &url)
{
    if (url.scheme().toLower() != QLatin1String("https")) {
        return false;
    }
    return url.port(443) == 8443;
}

QUrl gatePlaintextFallback(const QUrl &httpsUrl)
{
    QUrl fb = httpsUrl;
    fb.setScheme(QStringLiteral("http"));
    fb.setPort(8080);
    return fb;
}
}  // namespace

HttpMgr::~HttpMgr()
{
}

void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod, const QString &module)
{
    postHttpReqInternal(url, QJsonDocument(json).toJson(QJsonDocument::Compact), req_id, mod, module, true);
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
    request.setTransferTimeout(kHttpTimeoutMs);
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
    timeoutTimer->start(kHttpTimeoutMs);

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

        gate_url_prefix = url.scheme() + QStringLiteral("://") + url.authority();

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
