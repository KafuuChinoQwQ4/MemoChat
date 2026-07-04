#include "AgentStreamClient.h"
#include "AgentNetworkRequestUtils.h"
#include "HttpMgrRequestUtils.h"

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
    _payload = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    _fallbackUrls = gateProtocolFallbackUrls(url);
    if (_fallbackUrls.isEmpty())
    {
        _fallbackUrls.push_back(url);
    }

    const QUrl first = _fallbackUrls.takeFirst();
    startRequest(first);
}

void AgentStreamClient::startRequest(const QUrl& url)
{
    _buffer.clear();
    _receivedAnyChunk = false;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "text/event-stream");
    request.setRawHeader("Cache-Control", "no-cache");
    request.setRawHeader("Connection", "keep-alive");
    configureAgentLocalGateRequest(request);

    _reply = _network->post(request, _payload);
    // Same as HttpMgr: VerifyNone alone is insufficient — the manager-level
    // SSL error handling also needs to be bypassed for self-signed certs.
    if (url.scheme().compare(QLatin1String("https"), Qt::CaseInsensitive) == 0)
    {
        _reply->ignoreSslErrors();
    }
    connect(_reply, &QNetworkReply::readyRead, this, &AgentStreamClient::onReadyRead);
    connect(_reply, &QNetworkReply::finished, this, &AgentStreamClient::onFinished);
}

void AgentStreamClient::cancel()
{
    QNetworkReply* reply = _reply;
    _reply = nullptr;
    _buffer.clear();
    _fallbackUrls.clear();
    _payload.clear();
    _receivedAnyChunk = false;
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
    const bool receivedAnyChunk = _receivedAnyChunk;
    const QUrl replyUrl = reply->url();
    _buffer.clear();

    if (networkError != static_cast<int>(QNetworkReply::NoError) &&
        networkError != static_cast<int>(QNetworkReply::OperationCanceledError) && !receivedAnyChunk &&
        !_fallbackUrls.isEmpty())
    {
        const QUrl fallback = _fallbackUrls.takeFirst();
        reply->deleteLater();
        startRequest(fallback);
        return;
    }

    if (networkError == static_cast<int>(QNetworkReply::NoError))
    {
        updateGatePrefixesFromReplyUrl(replyUrl);
    }

    _payload.clear();
    _fallbackUrls.clear();
    _receivedAnyChunk = false;
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
    _receivedAnyChunk = true;
    emit chunkReceived(chunk);
    if (chunk.value(QStringLiteral("is_final")).toBool())
    {
        emit finalReceived(chunk);
    }
}
