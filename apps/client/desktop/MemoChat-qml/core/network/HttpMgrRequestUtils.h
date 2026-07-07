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
/** Like prepareJsonRequest but does NOT attach an Authorization header.
 *  Use for credential-exchange endpoints (login, register, verify-code) where
 *  attaching a stale session token can cause the server to reject the request. */
void prepareUnauthenticatedJsonRequest(QNetworkRequest& request, const QByteArray& data);
void prepareGetRequest(QNetworkRequest& request);
QString
responseWithTraceHeaders(const QByteArray& body, const QByteArray& responseTrace, const QByteArray& responseRequest);
void updateGatePrefixesFromReplyUrl(const QUrl& replyUrl);

#endif // HTTPMGRREQUESTUTILS_H
