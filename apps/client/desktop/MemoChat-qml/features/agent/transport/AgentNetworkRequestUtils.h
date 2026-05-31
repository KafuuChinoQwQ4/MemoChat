#ifndef AGENTNETWORKREQUESTUTILS_H
#define AGENTNETWORKREQUESTUTILS_H

#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QUrl>
#include <QtGlobal>

inline void configureAgentLocalGateRequest(QNetworkRequest& request)
{
    const QUrl url = request.url();
    const QString scheme = url.scheme().toLower();
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
