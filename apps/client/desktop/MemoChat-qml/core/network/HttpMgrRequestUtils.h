#ifndef HTTPMGRREQUESTUTILS_H
#define HTTPMGRREQUESTUTILS_H

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QSettings>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QString>
#include <QUrl>
#include <QVector>

int httpTimeoutForRequest(const QUrl& url, const QString& module);
QVector<QUrl> gateProtocolFallbackUrls(const QUrl& url);

inline QString deploymentCaFilePath()
{
    QString configuredPath = qEnvironmentVariable("MEMOCHAT_CLIENT_CA_FILE").trimmed();
    if (configuredPath.isEmpty())
    {
        const QString configPath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config.ini"));
        QSettings settings(configPath, QSettings::IniFormat);
        configuredPath = settings.value(QStringLiteral("GateServer/ca_file")).toString().trimmed();
    }
    if (configuredPath.isEmpty())
    {
        return {};
    }

    QFileInfo info(configuredPath);
    if (info.isRelative())
    {
        info.setFile(QDir(QCoreApplication::applicationDirPath()).filePath(configuredPath));
    }
    return info.absoluteFilePath();
}

inline bool configureSecureNetworkRequest(QNetworkRequest& request)
{
    const QString scheme = request.url().scheme().trimmed().toLower();
#if MEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD
    if (scheme != QLatin1String("https"))
    {
        qWarning() << "Distributable client rejected a non-HTTPS network request";
        request.setUrl(QUrl());
        return false;
    }
#else
    if (scheme == QLatin1String("http"))
    {
        request.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("close"));
        return true;
    }
    if (scheme != QLatin1String("https"))
    {
        return false;
    }
#endif

    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
    const QString caFile = deploymentCaFilePath();
    if (!caFile.isEmpty() && !sslConfig.addCaCertificates(caFile, QSsl::Pem))
    {
        qWarning() << "Configured deployment CA could not be loaded:" << caFile;
    }
    request.setSslConfiguration(sslConfig);
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
#endif
    return true;
}

void applyBearerAccessTokenHeader(QNetworkRequest& request);
bool prepareJsonRequest(QNetworkRequest& request, const QByteArray& data);
/** Like prepareJsonRequest but does NOT attach an Authorization header.
 *  Use for credential-exchange endpoints (login, register, verify-code) where
 *  attaching a stale session token can cause the server to reject the request. */
bool prepareUnauthenticatedJsonRequest(QNetworkRequest& request, const QByteArray& data);
bool prepareGetRequest(QNetworkRequest& request);
QString
responseWithTraceHeaders(const QByteArray& body, const QByteArray& responseTrace, const QByteArray& responseRequest);
void updateGatePrefixesFromReplyUrl(const QUrl& replyUrl);

#endif // HTTPMGRREQUESTUTILS_H
