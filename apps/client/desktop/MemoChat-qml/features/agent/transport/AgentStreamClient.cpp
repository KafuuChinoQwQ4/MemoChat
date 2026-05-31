#include "AgentStreamClient.h"
#include "AgentNetworkRequestUtils.h"

#include <QJsonDocument>
#include <QNetworkRequest>

AgentStreamClient::AgentStreamClient(QObject* parent)
    : QObject(parent)
    , _network(new QNetworkAccessManager(this))
{
}

AgentStreamClient::~AgentStreamClient()
{
    cancel();
}

void AgentStreamClient::start(const QUrl& url, const QJsonObject& payload)
{
    cancel();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "text/event-stream");
    request.setRawHeader("Cache-Control", "no-cache");
    request.setRawHeader("Connection", "keep-alive");
    configureAgentLocalGateRequest(request);

    _reply = _network->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(_reply, &QNetworkReply::readyRead, this, &AgentStreamClient::onReadyRead);
    connect(_reply, &QNetworkReply::finished, this, &AgentStreamClient::onFinished);
}

void AgentStreamClient::cancel()
{
    QNetworkReply* reply = _reply;
    _reply = nullptr;
    _buffer.clear();
    if (!reply)
    {
        return;
    }

    reply->disconnect(this);
    reply->abort();
    reply->deleteLater();
}

bool AgentStreamClient::isRunning() const
{
    return _reply != nullptr;
}

void AgentStreamClient::onReadyRead()
{
    if (!_reply)
    {
        return;
    }

    _buffer.append(QString::fromUtf8(_reply->readAll()));

    int linesEnd = 0;
    while (true)
    {
        const int newlinePos = _buffer.indexOf('\n', linesEnd);
        if (newlinePos == -1)
        {
            _buffer = _buffer.mid(linesEnd);
            break;
        }

        const QString line = _buffer.mid(linesEnd, newlinePos - linesEnd);
        linesEnd = newlinePos + 1;

        if (!line.isEmpty())
        {
            consumeLine(line);
        }
    }
}

void AgentStreamClient::onFinished()
{
    if (!_reply)
    {
        return;
    }

    QNetworkReply* reply = _reply;
    _reply = nullptr;
    const int networkError = static_cast<int>(reply->error());
    const QString errorString = reply->errorString();
    _buffer.clear();
    reply->deleteLater();

    if (networkError != static_cast<int>(QNetworkReply::NoError) &&
        networkError != static_cast<int>(QNetworkReply::OperationCanceledError))
    {
        emit errorOccurred(networkError, errorString);
    }
    emit finished(networkError, errorString);
}

void AgentStreamClient::consumeLine(const QString& line)
{
    if (!line.startsWith(QStringLiteral("data:")))
    {
        return;
    }

    const QString jsonText = line.mid(5).trimmed();
    if (jsonText.isEmpty() || jsonText == QStringLiteral("[DONE]"))
    {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8());
    if (doc.isNull() || !doc.isObject())
    {
        return;
    }

    const QJsonObject chunk = doc.object();
    emit chunkReceived(chunk);
    if (chunk.value(QStringLiteral("is_final")).toBool())
    {
        emit finalReceived(chunk);
    }
}
