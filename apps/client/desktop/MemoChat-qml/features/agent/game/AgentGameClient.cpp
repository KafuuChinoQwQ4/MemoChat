#include "AgentGameClient.h"
#include "AgentNetworkRequestUtils.h"

#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>

AgentGameClient::AgentGameClient(QObject* parent)
    : QObject(parent)
    , _network(new QNetworkAccessManager(this))
{
}

void AgentGameClient::get(const QUrl& url, const QString& op, const QString& statusText, int uid)
{
    Q_UNUSED(statusText);
    send(Verb::Get, url, QJsonObject(), op, uid);
}

void AgentGameClient::post(const QUrl& url,
                           const QJsonObject& payload,
                           const QString& op,
                           const QString& statusText,
                           int uid)
{
    Q_UNUSED(statusText);
    send(Verb::Post, url, payload, op, uid);
}

void AgentGameClient::deleteResource(const QUrl& url, const QString& op, const QString& statusText, int uid)
{
    Q_UNUSED(statusText);
    send(Verb::Delete, url, QJsonObject(), op, uid);
}

void AgentGameClient::send(Verb verb, const QUrl& url, const QJsonObject& payload, const QString& op, int uid)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    configureAgentLocalGateRequest(request);

    QNetworkReply* reply = nullptr;
    switch (verb)
    {
        case Verb::Get:
            reply = _network->get(request);
            break;
        case Verb::Post:
            reply = _network->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
            break;
        case Verb::Delete:
            reply = _network->deleteResource(request);
            break;
    }

    connect(reply,
            &QNetworkReply::finished,
            this,
            [this, reply, op, uid]()
            {
                handleReply(reply, op, uid);
            });
}

void AgentGameClient::handleReply(QNetworkReply* reply, const QString& op, int uid)
{
    const QByteArray body = reply->readAll();
    const QString errorText = reply->error() == QNetworkReply::NoError ? QString() : reply->errorString();
    reply->deleteLater();

    if (!errorText.isEmpty())
    {
        emit networkError(op, errorText, uid);
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject())
    {
        emit formatError(op, uid);
        return;
    }

    emit responseReady(op, doc.object(), uid);
}
