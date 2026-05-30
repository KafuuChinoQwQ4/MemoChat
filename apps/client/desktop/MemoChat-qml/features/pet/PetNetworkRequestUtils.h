#ifndef PETNETWORKREQUESTUTILS_H
#define PETNETWORKREQUESTUTILS_H

#include <QtCore/QString>
#include <QtCore/QtGlobal>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslSocket>

namespace memochat::pet
{

inline void configurePetRequest(QNetworkRequest& request)
{
    const QString scheme = request.url().scheme().trimmed().toLower();
    if (scheme == QLatin1String("http"))
    {
        request.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("close"));
        return;
    }
    if (scheme != QLatin1String("https"))
    {
        return;
    }

    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
#endif
}

} // namespace memochat::pet

#endif // PETNETWORKREQUESTUTILS_H
