#include "OpsApiTransport.h"

#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

OpsApiTransport::OpsApiTransport(QString baseUrl, QObject* parent)
    : QObject(parent)
    , m_baseUrl(std::move(baseUrl))
{
}

void OpsApiTransport::getJson(const QString& path, const JsonHandler& onObject)
{
    emit requestStarted();

    QNetworkRequest request(QUrl(urlForPath(path)));
    auto* reply = m_network.get(request);
    connect(reply,
            &QNetworkReply::finished,
            this,
            [this, reply, onObject]()
            {
                handleReply(reply, onObject);
            });
}

void OpsApiTransport::postJson(const QString& path, const JsonHandler& onObject)
{
    postJsonBody(path, QUrlQuery(), onObject);
}

void OpsApiTransport::postJsonWithQuery(const QString& path, const QUrlQuery& query, const JsonHandler& onObject)
{
    postJsonBody(path, query, onObject);
}

void OpsApiTransport::postJsonBody(const QString& path, const QUrlQuery& query, const JsonHandler& onObject)
{
    emit requestStarted();

    QNetworkRequest request(QUrl(urlForPath(path, query)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    auto* reply = m_network.post(request, QByteArray("{}"));
    connect(reply,
            &QNetworkReply::finished,
            this,
            [this, reply, onObject]()
            {
                handleReply(reply, onObject);
            });
}

QString OpsApiTransport::urlForPath(const QString& path, const QUrlQuery& query) const
{
    if (query.isEmpty())
    {
        return m_baseUrl + path;
    }
    return m_baseUrl + path + QStringLiteral("?") + query.toString(QUrl::FullyEncoded);
}

void OpsApiTransport::handleReply(QNetworkReply* reply, const JsonHandler& onObject)
{
    const QByteArray body = reply->readAll();
    if (reply->error() != QNetworkReply::NoError)
    {
        emit requestFailed(reply->errorString());
        emit requestFinished();
        reply->deleteLater();
        return;
    }

    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject())
    {
        emit requestFailed(QStringLiteral("Invalid JSON response"));
        emit requestFinished();
        reply->deleteLater();
        return;
    }

    emit requestSucceeded();
    onObject(doc.object());
    emit requestFinished();
    reply->deleteLater();
}
