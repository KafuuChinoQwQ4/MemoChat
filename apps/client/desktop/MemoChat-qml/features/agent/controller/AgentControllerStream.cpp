#include "AgentController.h"
#include "AgentMessageModel.h"
#include "AgentNetworkRequestUtils.h"
#include "AgentStreamClient.h"
#include "ClientGateway.h"
#include "usermgr.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>
#include <QUuid>

namespace
{
QString makeUuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
} // namespace

void AgentController::sendStreamMessage(const QString& content)
{
    if (content.trimmed().isEmpty())
    {
        return;
    }

    ensureUserScope();
    auto uid = _gateway->userMgr()->GetUid();
    QString sessionId = _current_session_id;

    _model->appendUserMessage(content);
    clearTrace();
    clearErrorState();
    QString msgId = makeUuid();
    _model->appendAIMessage(msgId, _current_model_name);
    _streaming = true;
    setCurrentGeneratingMsgId(msgId);
    _currentStreamUid = uid;
    _accumulatedContent.clear();
    _streamFinalReceived = false;
    emit streamingChanged();

    QJsonObject payload;
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
    _streamClient->start(agentApiUrl(QStringLiteral("/ai/chat/stream")), payload);
}

void AgentController::cancelStream()
{
    const QString msgId = _currentStreamMsgId;
    const QString partialContent = _accumulatedContent;

    _streamClient->cancel();

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
    setCurrentGeneratingMsgId(QString());
    _currentStreamUid = 0;
    _accumulatedContent.clear();
    _streamFinalReceived = false;
    _model->finalizeAllStreamingMessages();
    clearErrorState();
    emit streamingChanged();
}

void AgentController::handleStreamChunk(const QJsonObject& chunk)
{
    if (_currentStreamUid != 0 && _currentStreamUid != currentUid())
    {
        ensureUserScope();
        return;
    }

    const QString chunkText = chunk["chunk"].toString();
    const bool isFinal = chunk["is_final"].toBool();

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

void AgentController::handleStreamFinished(int networkError, const QString& errorString)
{
    if (_currentStreamUid != 0 && _currentStreamUid != currentUid())
    {
        ensureUserScope();
        return;
    }

    const auto err = static_cast<QNetworkReply::NetworkError>(networkError);
    const bool hasUsefulContent = _streamFinalReceived || !_accumulatedContent.isEmpty();

    if (hasUsefulContent && !_streamFinalReceived && !_currentStreamMsgId.isEmpty())
    {
        finishStream(_currentStreamMsgId, _accumulatedContent);
    }

    const bool normalStreamClose = hasUsefulContent && err == QNetworkReply::RemoteHostClosedError;
    bool currentMessageFinalized = false;
    if (err != QNetworkReply::NoError && err != QNetworkReply::OperationCanceledError && !normalStreamClose)
    {
        qWarning() << "[AgentController] Stream error:" << err << errorString;
        if (!_currentStreamMsgId.isEmpty())
        {
            _model->setError(_currentStreamMsgId, errorString);
            _model->finalizeAIMessage(_currentStreamMsgId);
            emit streamingFinished(_currentStreamMsgId);
            currentMessageFinalized = true;
        }
        setErrorState(errorString);
    }

    if (!_currentStreamMsgId.isEmpty() && !currentMessageFinalized)
    {
        _model->finalizeAIMessage(_currentStreamMsgId);
        emit streamingFinished(_currentStreamMsgId);
    }

    _streaming = false;
    setCurrentGeneratingMsgId(QString());
    _currentStreamUid = 0;
    _accumulatedContent.clear();
    _streamFinalReceived = false;
    _model->finalizeAllStreamingMessages();
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

    _streaming = false;
    setCurrentGeneratingMsgId(QString());
    _model->finalizeAllStreamingMessages();
    if (_current_session_id.isEmpty())
    {
        _selectNewestSessionAfterList = true;
        loadSessions();
    }
    emit streamingChanged();
}
