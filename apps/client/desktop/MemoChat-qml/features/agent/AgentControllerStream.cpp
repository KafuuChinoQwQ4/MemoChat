#include "AgentController.h"
#include "AgentMessageModel.h"
#include "ClientGateway.h"
#include "global.h"
#include "usermgr.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QUrl>
#include <QUuid>

namespace
{
QString makeUuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void applyLocalGateRequestOptions(QNetworkRequest& request)
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
} // namespace

void AgentController::sendStreamMessage(const QString& content)
{
    if (content.trimmed().isEmpty())
        return;

    auto uid = _gateway->userMgr()->GetUid();
    QString sessionId = _current_session_id;

    _model->appendUserMessage(content);
    clearTrace();
    clearErrorState();
    QString msgId = makeUuid();
    _model->appendAIMessage(msgId, _current_model_name);
    _streaming = true;
    _currentStreamMsgId = msgId;
    _accumulatedContent.clear();
    _streamBuffer.clear();
    _streamFinalReceived = false;
    emit streamingChanged();

    // 构建 SSE 请求
    QJsonObject payload;
    payload["uid"] = uid;
    payload["session_id"] = sessionId;
    payload["content"] = content;
    payload["model_type"] = _current_model_backend;
    payload["model_name"] = _current_model_name;
    payload["metadata"] = buildChatMetadata();
    const QString skillName = resolvedSkillName();
    if (!skillName.isEmpty())
    {
        payload["skill_name"] = skillName;
    }
    const QJsonArray requestedTools = requestedToolsForSkillMode();
    if (!requestedTools.isEmpty())
    {
        payload["requested_tools"] = requestedTools;
    }

    QUrl url(gate_url_prefix + "/ai/chat/stream");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "text/event-stream");
    // 禁用缓存，保持连接
    request.setRawHeader("Cache-Control", "no-cache");
    request.setRawHeader("Connection", "keep-alive");
    applyLocalGateRequestOptions(request);

    // 发送请求并处理流式响应
    QByteArray data = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    _currentStreamReply = _streamManager->post(request, data);

    connect(_currentStreamReply, &QNetworkReply::readyRead, this, &AgentController::onStreamReadyRead);
    connect(_currentStreamReply, &QNetworkReply::finished, this, &AgentController::onStreamFinished);
}

void AgentController::cancelStream()
{
    const QString msgId = _currentStreamMsgId;
    const QString partialContent = _accumulatedContent;

    QNetworkReply* reply = _currentStreamReply;
    _currentStreamReply = nullptr;
    if (reply)
    {
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }

    if (!msgId.isEmpty())
    {
        if (!partialContent.isEmpty())
        {
            _model->updateStreamingContent(msgId, partialContent);
        }
        else
        {
            _model->updateStreamingContent(msgId, QStringLiteral("已停止生成"));
        }
        _model->finalizeAIMessage(msgId);
        emit streamingFinished(msgId);
    }

    _streaming = false;
    _streamBuffer.clear();
    _currentStreamMsgId.clear();
    _accumulatedContent.clear();
    _streamFinalReceived = false;
    clearErrorState();
    emit streamingChanged();
}

void AgentController::onStreamReadyRead()
{
    if (!_currentStreamReply)
        return;

    QByteArray data = _currentStreamReply->readAll();
    _streamBuffer.append(QString::fromUtf8(data));

    // 解析 SSE 数据：逐行处理
    int linesEnd = 0;
    while (true)
    {
        int newlinePos = _streamBuffer.indexOf('\n', linesEnd);
        if (newlinePos == -1)
        {
            // 没有完整的行，保留缓冲区
            _streamBuffer = _streamBuffer.mid(linesEnd);
            break;
        }

        QString line = _streamBuffer.mid(linesEnd, newlinePos - linesEnd);
        linesEnd = newlinePos + 1;

        // 处理空行（可能表示事件结束）
        if (line.isEmpty())
            continue;

        // 解析 SSE 行
        parseSSEChunk(line);
    }
}

void AgentController::parseSSEChunk(const QString& line)
{
    if (!line.startsWith("data:"))
        return;

    QString jsonStr = line.mid(5).trimmed();
    if (jsonStr.isEmpty() || jsonStr == "[DONE]")
        return;

    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (doc.isNull() || !doc.isObject())
        return;

    QJsonObject chunk = doc.object();
    QString chunkText = chunk["chunk"].toString();
    bool isFinal = chunk["is_final"].toBool();
    QString msgId = chunk["msg_id"].toString();

    if (isFinal)
    {
        updateTraceFromResponse(chunk);
    }

    if (!chunkText.isEmpty())
    {
        _accumulatedContent += chunkText;
        if (!_currentStreamMsgId.isEmpty())
        {
            _model->updateStreamingContent(_currentStreamMsgId, _accumulatedContent);
        }
        emit streamingChunkReceived(_currentStreamMsgId, chunkText);
    }

    if (isFinal)
    {
        finishStream(_currentStreamMsgId, _accumulatedContent);
    }
}

void AgentController::onStreamFinished()
{
    if (!_currentStreamReply)
        return;

    QNetworkReply::NetworkError err = _currentStreamReply->error();
    QString errorStr = _currentStreamReply->errorString();
    const bool hasUsefulContent = _streamFinalReceived || !_accumulatedContent.isEmpty();

    _currentStreamReply->deleteLater();
    _currentStreamReply = nullptr;

    if (hasUsefulContent && !_streamFinalReceived && !_currentStreamMsgId.isEmpty())
    {
        finishStream(_currentStreamMsgId, _accumulatedContent);
    }

    const bool normalStreamClose = hasUsefulContent && err == QNetworkReply::RemoteHostClosedError;
    if (err != QNetworkReply::NoError && err != QNetworkReply::OperationCanceledError && !normalStreamClose)
    {
        qWarning() << "[AgentController] Stream error:" << err << errorStr;
        if (!_currentStreamMsgId.isEmpty())
        {
            _model->setError(_currentStreamMsgId, errorStr);
            _model->finalizeAIMessage(_currentStreamMsgId);
            emit streamingFinished(_currentStreamMsgId);
        }
        setErrorState(errorStr);
    }

    // 清理状态
    _streaming = false;
    _streamBuffer.clear();
    _currentStreamMsgId.clear();
    _accumulatedContent.clear();
    _streamFinalReceived = false;
    emit streamingChanged();
}

void AgentController::finishStream(const QString& msgId, const QString& finalContent)
{
    if (_streamFinalReceived)
    {
        return;
    }
    _streamFinalReceived = true;
    if (!msgId.isEmpty())
    {
        _model->updateStreamingContent(msgId, finalContent);
        _model->finalizeAIMessage(msgId);
        emit streamingFinished(msgId);
    }

    emit aiResponseReceived(finalContent);

    // 清理状态
    _streaming = false;
    if (_current_session_id.isEmpty())
    {
        _selectNewestSessionAfterList = true;
        loadSessions();
    }
    emit streamingChanged();
}
