#ifndef HTTPMGRREQUESTUTILS_H
#define HTTPMGRREQUESTUTILS_H

#include <QNetworkRequest>
#include <QUrl>
#include <QVector>

class QString;

int httpTimeoutForRequest(const QUrl& url, const QString& module);
QVector<QUrl> gateProtocolFallbackUrls(const QUrl& url);
void applyBearerAccessTokenHeader(QNetworkRequest& request);
void prepareJsonRequest(QNetworkRequest& request, const QByteArray& data);
void prepareGetRequest(QNetworkRequest& request);
QString
responseWithTraceHeaders(const QByteArray& body, const QByteArray& responseTrace, const QByteArray& responseRequest);
void updateGatePrefixesFromReplyUrl(const QUrl& replyUrl);

#endif // HTTPMGRREQUESTUTILS_H
