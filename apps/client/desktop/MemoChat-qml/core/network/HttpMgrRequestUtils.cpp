#include "HttpMgrRequestUtils.h"

#include "global.h"
#include "usermgr.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSettings>
#include <QString>
#include <QStringList>

namespace
{
constexpr int kHttpTimeoutMs = 10000;
constexpr int kAiHttpTimeoutMs = 300000;

struct GateEndpointSettings
{
    QString host;
    int configuredPort = 8080;
    int h1DirectPort = 8080;
    int h2Port = 0;
    int h3Port = 0;
    QString configuredScheme;
    QString preferredProtocol;
};

bool isLongAiRequest(const QUrl& url, const QString& module)
{
    const QString normalized = module.trimmed().toLower();
    const QString path = url.path().trimmed().toLower();
    if (path == QStringLiteral("/ai/chat") || path == QStringLiteral("/ai/chat/stream"))
    {
        return true;
    }
    if (path == QStringLiteral("/ai/smart") || path == QStringLiteral("/ai/kb/upload"))
    {
        return true;
    }
    if (path == QStringLiteral("/ai/kb/search") || path == QStringLiteral("/ai/model/api/register"))
    {
        return true;
    }
    return normalized == QStringLiteral("ai-smart");
}

QString gateConfigValue(const QSettings& settings, const QString& key)
{
    return settings.value(key).toString().trimmed();
}

int parsePort(const QString& value, int fallback)
{
    bool ok = false;
    const int port = value.trimmed().toInt(&ok);
    return ok && port > 0 ? port : fallback;
}

GateEndpointSettings readGateEndpointSettings()
{
    const QString path = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config.ini"));
    QSettings settings(path, QSettings::IniFormat);

    GateEndpointSettings result;
    result.host = gateConfigValue(settings, QStringLiteral("GateServer/host"));
    if (result.host.isEmpty())
    {
        result.host = QStringLiteral("127.0.0.1");
    }
    if (result.host.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0)
    {
        result.host = QStringLiteral("127.0.0.1");
    }
    result.configuredPort = parsePort(gateConfigValue(settings, QStringLiteral("GateServer/port")), 8080);
    result.h1DirectPort =
        parsePort(gateConfigValue(settings, QStringLiteral("GateServer/http_port")), result.configuredPort);
    result.h2Port = parsePort(gateConfigValue(settings, QStringLiteral("GateServer/http2_port")), 0);
    result.h3Port = parsePort(gateConfigValue(settings, QStringLiteral("GateServer/http3_port")), 0);
    result.configuredScheme = gateConfigValue(settings, QStringLiteral("GateServer/scheme")).toLower();
    if (result.configuredScheme != QStringLiteral("https") && result.configuredScheme != QStringLiteral("http"))
    {
        result.configuredScheme = result.configuredPort == 8443 ? QStringLiteral("https") : QStringLiteral("http");
    }
    result.preferredProtocol = gateConfigValue(settings, QStringLiteral("GateServer/preferred_http_protocol"));
    return result;
}

bool isConfiguredGateEndpoint(const QUrl& url, const GateEndpointSettings& settings)
{
    if (url.host().compare(settings.host, Qt::CaseInsensitive) != 0)
    {
        return false;
    }

    const int port = url.port();
    if (port <= 0)
    {
        return true;
    }

    return port == settings.configuredPort || port == settings.h1DirectPort ||
           (settings.h2Port > 0 && port == settings.h2Port) || (settings.h3Port > 0 && port == settings.h3Port);
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

} // namespace

int httpTimeoutForRequest(const QUrl& url, const QString& module)
{
    return isLongAiRequest(url, module) ? kAiHttpTimeoutMs : kHttpTimeoutMs;
}

QVector<QUrl> gateProtocolFallbackUrls(const QUrl& url)
{
#if MEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD
    if (url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0)
    {
        qWarning() << "Distributable client rejected a non-HTTPS gate endpoint";
        return {};
    }
#endif
    const GateEndpointSettings settings = readGateEndpointSettings();
    if (!isConfiguredGateEndpoint(url, settings))
    {
        return {url};
    }

    QVector<QUrl> urls;
    const bool tlsConfigured = settings.configuredScheme == QStringLiteral("https") || settings.h2Port > 0;
    if (tlsConfigured)
    {
        if ((settings.preferredProtocol ==
             QStringLiteral("http3") || settings.preferredProtocol == QStringLiteral("h3")) && settings.h3Port > 0)
        {
            qInfo() << "HTTP/3 gate endpoint configured at" << settings.host << settings.h3Port
                    << "but Qt Network has no HTTP/3 transport here; using TLS with HTTP/2 negotiation";
        }
        const int tlsPort = settings.h2Port > 0 ? settings.h2Port : settings.configuredPort;
        appendUniqueUrl(urls, withGateEndpoint(url, QStringLiteral("https"), settings.host, tlsPort));
    }

    // Never downgrade a credential-bearing HTTPS request to plaintext during retry.
    if (url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0 || !tlsConfigured)
    {
        appendUniqueUrl(urls, url);
    }
    if (urls.isEmpty())
    {
        urls.push_back(url);
    }
    return urls;
}

void applyBearerAccessTokenHeader(QNetworkRequest& request)
{
    const auto userMgr = UserMgr::GetInstance();
    const QString accessToken = userMgr ? userMgr->GetToken().trimmed() : QString();
    if (!accessToken.isEmpty())
    {
        request.setRawHeader(QByteArrayLiteral("Authorization"), QByteArrayLiteral("Bearer ") + accessToken.toUtf8());
    }
}

bool prepareJsonRequest(QNetworkRequest& request, const QByteArray& data)
{
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));
    applyBearerAccessTokenHeader(request);
    return configureSecureNetworkRequest(request);
}

bool prepareUnauthenticatedJsonRequest(QNetworkRequest& request, const QByteArray& data)
{
    // No Authorization header — use for credential-exchange endpoints such as
    // /user_login, /user_register, and /get_varifycode where a stale Bearer
    // token must not be sent.
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));
    return configureSecureNetworkRequest(request);
}

bool prepareGetRequest(QNetworkRequest& request)
{
    applyBearerAccessTokenHeader(request);
    return configureSecureNetworkRequest(request);
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
    if (replyUrl.isValid() && !replyUrl.scheme().isEmpty() && !replyUrl.authority().isEmpty() &&
        isConfiguredGateEndpoint(replyUrl, readGateEndpointSettings()))
    {
        gate_url_prefix = replyUrl.scheme() + QStringLiteral("://") + replyUrl.authority();
        if (gate_media_url_prefix.trimmed().isEmpty())
        {
            gate_media_url_prefix = gate_url_prefix;
        }
    }
}
