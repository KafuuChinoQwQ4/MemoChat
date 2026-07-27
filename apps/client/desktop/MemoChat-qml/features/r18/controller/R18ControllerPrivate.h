#pragma once

#include "HttpMgrRequestUtils.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QTimer>
#include <QUrl>

namespace memochat::r18
{
constexpr int kRequestTimeoutMs = 15000;

inline bool looksLikeWindowsDrivePath(const QString& path)
{
    return path.size() >= 2 && path.at(1) == QLatin1Char(':') && path.at(0).isLetter();
}

inline void applyRequestOptions(QNetworkRequest& request)
{
    request.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("close"));
    configureSecureNetworkRequest(request);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(kRequestTimeoutMs);
#endif
}

inline void armTimeout(QNetworkReply* reply)
{
    auto* timer = new QTimer(reply);
    timer->setSingleShot(true);
    QObject::connect(timer,
                     &QTimer::timeout,
                     reply,
                     [reply]()
                     {
                         if (reply->isRunning())
                         {
                             reply->abort();
                         }
                     });
    QObject::connect(reply, &QNetworkReply::finished, timer, &QTimer::stop);
    timer->start(kRequestTimeoutMs);
}
} // namespace memochat::r18
