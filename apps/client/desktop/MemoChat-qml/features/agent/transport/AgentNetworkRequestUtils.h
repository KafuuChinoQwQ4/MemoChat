#ifndef AGENTNETWORKREQUESTUTILS_H
#define AGENTNETWORKREQUESTUTILS_H

#include "global.h"

#include <QCoreApplication>
#include <QDir>
#include <QNetworkRequest>
#include <QSettings>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QString>
#include <QUrl>
#include <QtGlobal>

inline QString agentGatewayBaseUrl()
{
    const QString path = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config.ini"));
    QSettings settings(path, QSettings::IniFormat);
    QString baseUrl = settings.value(QStringLiteral("AI/BaseUrl")).toString().trimmed();
    if (baseUrl.isEmpty())
    {
        baseUrl = gate_url_prefix.trimmed();
    }
    if (baseUrl.endsWith(QLatin1Char('/')))
    {
        baseUrl.chop(1);
    }
    return baseUrl;
}

inline QUrl agentApiUrl(const QString& path)
{
    QString baseUrl = agentGatewayBaseUrl();
    QString normalizedPath = path.trimmed();
    if (!normalizedPath.startsWith(QLatin1Char('/')))
    {
        normalizedPath.prepend(QLatin1Char('/'));
    }
    return QUrl(baseUrl + normalizedPath);
}

inline void configureAgentLocalGateRequest(QNetworkRequest& request)
{
    const QUrl url = request.url();
    const QString scheme = url.scheme().toLower();
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(10000);
#endif
    if (scheme == QLatin1String("https"))
    {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
        request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
#endif
    }
}

#endif // AGENTNETWORKREQUESTUTILS_H
