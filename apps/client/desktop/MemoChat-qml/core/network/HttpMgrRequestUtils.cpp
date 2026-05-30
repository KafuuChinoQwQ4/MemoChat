#include "HttpMgrRequestUtils.h"

#include "global.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSettings>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QString>
#include <QStringList>

namespace
{
constexpr int kHttpTimeoutMs = 10000;
constexpr int kAiHttpTimeoutMs = 300000;

bool isAiRequest(const QUrl& url, const QString& module)
{
    const QString normalized = module.trimmed().toLower();
    if (normalized.startsWith(QStringLiteral("ai")))
    {
        return true;
    }
    const QString path = url.path().trimmed().toLower();
    return path == QStringLiteral("/ai") || path.startsWith(QStringLiteral("/ai/"));
}

QString gateConfigValue(const QSettings& settings, const QString& lowerKey, const QString& upperKey)
{
    QString value = settings.value(lowerKey).toString().trimmed();
    if (value.isEmpty())
    {
        value = settings.value(upperKey).toString().trimmed();
    }
    return value;
}

int parsePort(const QString& value, int fallback)
{
    bool ok = false;
    const int port = value.trimmed().toInt(&ok);
    return ok && port > 0 ? port : fallback;
}

QUrl withGateEndpoint(const QUrl& source, const QString& scheme, const QString& host, int port)
{
    QUrl next = source;
    next.setScheme(scheme);
    next.setHost(host);
    next.setPort(port);
    return next;
}

void appendUniqueUrl(QVector<QUrl>& urls, const QUrl& url)
{
    if (!url.isValid() || url.host().isEmpty() || url.port() <= 0)
    {
        return;
    }
    const QString key = url.toString(QUrl::RemoveUserInfo);
    for (const auto& existing : urls)
    {
        if (existing.toString(QUrl::RemoveUserInfo) == key)
        {
            return;
        }
    }
    urls.push_back(url);
}

QStringList protocolOrder(const QString& preferred)
{
    const QString normalized = preferred.trimmed().toLower();
    if (normalized == QStringLiteral("http1") || normalized == QStringLiteral("http1.1") ||
                                                                              normalized == QStringLiteral("h1"))
    {
        return {QStringLiteral("http1")};
    }
    if (normalized == QStringLiteral("http2") || normalized == QStringLiteral("h2"))
    {
        return {QStringLiteral("http2"), QStringLiteral("http1")};
    }
    return {QStringLiteral("http3"), QStringLiteral("http2"), QStringLiteral("http1")};
}

void prepareRequestTransport(QNetworkRequest& request)
{
    const QUrl url = request.url();
    if (url.scheme().toLower() == QLatin1String("http"))
    {
        request.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("close"));
    }

    if (url.scheme().toLower() == QLatin1String("https"))
    {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
        request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
#endif
    }
}
} // namespace

int httpTimeoutForRequest(const QUrl& url, const QString& module)
{
    return isAiRequest(url, module) ? kAiHttpTimeoutMs : kHttpTimeoutMs;
}

QVector<QUrl> gateProtocolFallbackUrls(const QUrl& url)
{
    const QString path = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config.ini"));
    QSettings settings(path, QSettings::IniFormat);
    QString gateHost = gateConfigValue(settings, QStringLiteral("GateServer/host"), QStringLiteral("GateServer/Host"));
    if (gateHost.isEmpty())
    {
        gateHost = QStringLiteral("127.0.0.1");
    }
    if (gateHost.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0)
    {
        gateHost = QStringLiteral("127.0.0.1");
    }
    const int configuredPort = parsePort(
        gateConfigValue(settings, QStringLiteral("GateServer/port"), QStringLiteral("GateServer/Port")), 8080);
    const int h1DirectPort =
        parsePort(gateConfigValue(settings,
                                  QStringLiteral("GateServer/http_port"), QStringLiteral("GateServer/HttpPort")),
                                  configuredPort);
    const int h2Port = parsePort(
        gateConfigValue(settings, QStringLiteral("GateServer/http2_port"), QStringLiteral("GateServer/Http2Port")), 0);
    const int h3Port = parsePort(
        gateConfigValue(settings, QStringLiteral("GateServer/http3_port"), QStringLiteral("GateServer/Http3Port")), 0);
    const QString preferred = gateConfigValue(settings,
                                              QStringLiteral("GateServer/preferred_http_protocol"),
                                                             QStringLiteral("GateServer/PreferredHttpProtocol"));

    if (url.host().compare(gateHost, Qt::CaseInsensitive) != 0)
    {
        return {url};
    }

    QVector<QUrl> urls;
    const QStringList order = protocolOrder(preferred);
    for (const QString& protocol : order)
    {
        if (protocol == QStringLiteral("http3"))
        {
            if (h3Port > 0)
            {
                qInfo() << "HTTP/3 gate endpoint configured at" << gateHost << h3Port
                        << "but Qt Network has no HTTP/3 transport here; falling back to HTTP/2/HTTP/1.1";
            }
            continue;
        }
        if (protocol == QStringLiteral("http2") && h2Port > 0)
        {
            appendUniqueUrl(urls, withGateEndpoint(url, QStringLiteral("https"), gateHost, h2Port));
            continue;
        }
        if (protocol == QStringLiteral("http1"))
        {
            appendUniqueUrl(urls, withGateEndpoint(url, QStringLiteral("http"), gateHost, configuredPort));
            if (h1DirectPort != configuredPort)
            {
                appendUniqueUrl(urls, withGateEndpoint(url, QStringLiteral("http"), gateHost, h1DirectPort));
            }
        }
    }
    appendUniqueUrl(urls, url);
    if (urls.isEmpty())
    {
        urls.push_back(url);
    }
    return urls;
}

void prepareJsonRequest(QNetworkRequest& request, const QByteArray& data)
{
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));
    prepareRequestTransport(request);
}

void prepareGetRequest(QNetworkRequest& request)
{
    prepareRequestTransport(request);
}

QString
responseWithTraceHeaders(const QByteArray& body, const QByteArray& responseTrace, const QByteArray& responseRequest)
{
    QString res = QString::fromUtf8(body);
    QJsonParseError parseError;
    const QJsonDocument responseDoc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error == QJsonParseError::NoError && responseDoc.isObject())
    {
        QJsonObject responseObj = responseDoc.object();
        if (!responseTrace.isEmpty() && !responseObj.contains("trace_id"))
        {
            responseObj.insert("trace_id", QString::fromUtf8(responseTrace));
        }
        if (!responseRequest.isEmpty() && !responseObj.contains("request_id"))
        {
            responseObj.insert("request_id", QString::fromUtf8(responseRequest));
        }
        res = QString::fromUtf8(QJsonDocument(responseObj).toJson(QJsonDocument::Compact));
    }
    return res;
}

void updateGatePrefixesFromReplyUrl(const QUrl& replyUrl)
{
    if (replyUrl.isValid() && !replyUrl.scheme().isEmpty() && !replyUrl.authority().isEmpty())
    {
        gate_url_prefix = replyUrl.scheme() + QStringLiteral("://") + replyUrl.authority();
        if (gate_media_url_prefix.trimmed().isEmpty())
        {
            gate_media_url_prefix = gate_url_prefix;
        }
    }
}
