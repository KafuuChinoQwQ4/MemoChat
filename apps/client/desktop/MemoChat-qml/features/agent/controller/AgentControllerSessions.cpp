#include "AgentController.h"

#include "AgentNetworkRequestUtils.h"
#include "AgentStreamClient.h"
#include "AgentMessageModel.h"
#include "ClientGateway.h"
#include "httpmgr.h"
#include "usermgr.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>

namespace
{
QString makeUuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString aiHttpModule()
{
    return QStringLiteral("ai");
}
} // namespace

void AgentController::loadSessions()
{
    ensureUserScope();
    auto uid = _gateway->userMgr()->GetUid();
    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::ListSessions, QString(), uid);
    QUrl url = agentApiUrl(QStringLiteral("/ai/session/list"));
    QUrlQuery query;
    query.addQueryItem("model_type", _current_model_backend);
    query.addQueryItem("model_name", _current_model_name);
    addAuthToQuery(query);
    url.setQuery(query);
    HttpMgr::GetInstance()->GetHttpReq(url, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::createSession()
{
    ensureUserScope();
    auto uid = _gateway->userMgr()->GetUid();
    if (!_current_game_room_id.isEmpty())
    {
        _current_game_room_id.clear();
        _game_state.clear();
        emit gameStateChanged();
    }
    QJsonObject payload;
    payload["model_type"] = _current_model_backend;
    payload["model_name"] = _current_model_name;
    addAuthToPayload(payload);

    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::CreateSession, QString(), uid);
    HttpMgr::GetInstance()->PostHttpReq(
        agentApiUrl(QStringLiteral("/ai/session")), payload, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::switchSession(const QString& sessionId)
{
    ensureUserScope();
    if (_current_session_id == sessionId)
        return;
    if (!_current_game_room_id.isEmpty())
    {
        _current_game_room_id.clear();
        _game_state.clear();
        emit gameStateChanged();
    }
    _current_session_id = sessionId;
    emit sessionChanged();
    loadHistory(sessionId);
}

void AgentController::deleteSession(const QString& sessionId)
{
    ensureUserScope();
    if (sessionId.trimmed().isEmpty())
    {
        return;
    }
    auto uid = _gateway->userMgr()->GetUid();
    QJsonObject payload;
    payload["session_id"] = sessionId;
    addAuthToPayload(payload);

    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::DeleteSession, QString(), uid);
    _pendingDeleteSessionId = sessionId;
    HttpMgr::GetInstance()->PostHttpReq(
        agentApiUrl(QStringLiteral("/ai/session/delete")), payload, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::renameSession(const QString& sessionId, const QString& title)
{
    ensureUserScope();
    const QString trimmedSessionId = sessionId.trimmed();
    const QString trimmedTitle = title.trimmed();
    if (trimmedSessionId.isEmpty())
    {
        return;
    }
    if (trimmedTitle.isEmpty())
    {
        setErrorState(QStringLiteral("请输入会话标题。"));
        return;
    }

    auto uid = _gateway->userMgr()->GetUid();
    QJsonObject payload;
    payload["session_id"] = trimmedSessionId;
    payload["title"] = trimmedTitle;
    addAuthToPayload(payload);

    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::RenameSession, QString(), uid);
    HttpMgr::GetInstance()->PostHttpReq(
        agentApiUrl(QStringLiteral("/ai/session/update")), payload, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::loadHistory(const QString& sessionId)
{
    ensureUserScope();
    auto uid = _gateway->userMgr()->GetUid();
    QUrl url = agentApiUrl(QStringLiteral("/ai/history"));
    QUrlQuery query;
    query.addQueryItem("session_id", sessionId);
    query.addQueryItem("limit", "50");
    query.addQueryItem("offset", "0");
    addAuthToQuery(query);
    url.setQuery(query);

    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::History, QString(), uid);
    setCurrentGeneratingMsgId(QString());
    _loading = true;
    _streaming = false;
    _model->finalizeAllStreamingMessages();
    _model->clear();
    emit loadingChanged();
    emit streamingChanged();

    HttpMgr::GetInstance()->GetHttpReq(url, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::handleSessionRsp(ReqId id, const QString& res, ErrorCodes err, AgentRequestKind kind)
{
    Q_UNUSED(id);
    Q_UNUSED(err);
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();

    if (kind == AgentRequestKind::ListSessions)
    {
        QJsonArray sessions = root["sessions"].toArray();
        _sessions.clear();
        for (const auto& s : sessions)
        {
            QJsonObject sess = s.toObject();
            _sessions.append(QJsonObject{
                {"session_id", sess["session_id"]},
                {"title", sess["title"]},
                {"model_type", sess["model_type"]},
                {"model_name", sess["model_name"]},
                {"created_at", sess["created_at"]},
                {"updated_at", sess["updated_at"]},
            });
        }
        emit sessionsChanged();
        if (_selectNewestSessionAfterList && _current_session_id.isEmpty() && !_sessions.isEmpty())
        {
            const QVariantMap firstSession = _sessions.first().toMap();
            const QString sessionId = firstSession.value("session_id").toString();
            if (!sessionId.isEmpty())
            {
                _current_session_id = sessionId;
                emit sessionChanged();
                loadHistory(sessionId);
            }
        }
        _selectNewestSessionAfterList = false;
    }
    else if (kind == AgentRequestKind::CreateSession)
    {
        QJsonObject sess = root["session"].toObject();
        QString newId = sess["session_id"].toString();
        if (!newId.isEmpty())
        {
            if (!_current_game_room_id.isEmpty())
            {
                _current_game_room_id.clear();
                _game_state.clear();
                emit gameStateChanged();
            }
            _current_session_id = newId;
            emit sessionChanged();
        }
        loadSessions();
    }
    else if (kind == AgentRequestKind::DeleteSession)
    {
        if (!_pendingDeleteSessionId.isEmpty() && _pendingDeleteSessionId == _current_session_id)
        {
            clearCurrentSession();
        }
        _pendingDeleteSessionId.clear();
        loadSessions();
    }
    else if (kind == AgentRequestKind::RenameSession)
    {
        QJsonObject sess = root["session"].toObject();
        const QString sessionId = sess["session_id"].toString();
        bool updated = false;
        if (!sessionId.isEmpty())
        {
            for (qsizetype i = 0; i < _sessions.size(); ++i)
            {
                QVariantMap item = _sessions.at(i).toMap();
                if (item.value(QStringLiteral("session_id")).toString() != sessionId)
                {
                    continue;
                }
                item[QStringLiteral("title")] = sess["title"].toString();
                item[QStringLiteral("model_type")] = sess["model_type"].toString();
                item[QStringLiteral("model_name")] = sess["model_name"].toString();
                item[QStringLiteral("updated_at")] = sess["updated_at"].toVariant();
                _sessions[i] = item;
                updated = true;
                break;
            }
        }
        if (updated)
        {
            emit sessionsChanged();
        }
        loadSessions();
    }
}

void AgentController::handleHistoryRsp(ReqId id, const QString& res, ErrorCodes err)
{
    Q_UNUSED(id);
    Q_UNUSED(err);
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();
    QJsonArray messages = root["messages"].toArray();

    for (const auto& m : messages)
    {
        QJsonObject msg = m.toObject();
        QString role = msg["role"].toString();
        QString content = msg["content"].toString();
        QString msgId = msg["msg_id"].toString();
        qint64 createdAt = msg["created_at"].toVariant().toLongLong();

        if (role == "user")
        {
            _model->appendUserMessage(content);
        }
        else
        {
            QString aiMsgId = msgId.isEmpty() ? makeUuid() : msgId;
            _model->appendAIMessage(aiMsgId, "");
            _model->updateStreamingContent(aiMsgId, content);
            _model->finalizeAIMessage(aiMsgId);
        }
    }

    _loading = false;
    _streaming = false;
    setCurrentGeneratingMsgId(QString());
    _model->finalizeAllStreamingMessages();
    emit loadingChanged();
    emit streamingChanged();
}

void AgentController::clearCurrentSession()
{
    if (!_current_session_id.isEmpty())
    {
        _current_session_id.clear();
        emit sessionChanged();
    }
    if (_model)
    {
        _model->finalizeAllStreamingMessages();
        _model->clear();
    }
}

void AgentController::resetForLogout()
{
    resetUserScopedRuntime();
    _scoped_uid = currentUid();
}

int AgentController::scopedUid()
{
    ensureUserScope();
    return _scoped_uid;
}

void AgentController::ensureUserScope()
{
    const int uid = currentUid();
    if (_scoped_uid == uid)
    {
        return;
    }
    resetUserScopedRuntime();
    _scoped_uid = uid;
}

void AgentController::resetUserScopedRuntime()
{
    if (_streaming)
    {
        _streamClient->cancel();
    }

    const bool sessionChangedNeeded = !_current_session_id.isEmpty();
    _current_session_id.clear();
    _sessions.clear();
    _pendingDeleteSessionId.clear();
    _selectNewestSessionAfterList = false;
    if (_model)
    {
        _model->finalizeAllStreamingMessages();
        _model->clear();
    }

    _loading = false;
    _streaming = false;
    _pending_requests.clear();
    setCurrentGeneratingMsgId(QString());
    _currentStreamUid = 0;
    _accumulatedContent.clear();
    _streamFinalReceived = false;
    _error.clear();
    clearTrace();

    _available_models.clear();
    _model_refresh_busy = false;
    _api_provider_busy = false;
    _api_provider_status.clear();
    _thinking_enabled = false;

    _knowledge_bases.clear();
    _knowledge_search_result.clear();
    _knowledge_busy = false;
    _knowledge_status_text.clear();
    _knowledge_error.clear();

    _memories.clear();
    _memory_busy = false;
    _memory_status_text.clear();
    _memory_error.clear();

    _agent_tasks.clear();
    _agent_task_busy = false;
    _agent_task_status_text.clear();
    _agent_task_error.clear();

    _pendingDeleteGameRoomId.clear();
    _current_game_room_id.clear();
    _game_rooms.clear();
    _game_templates.clear();
    _game_template_presets.clear();
    _game_state.clear();
    _game_rulesets.clear();
    _game_role_presets.clear();
    _game_busy = false;
    _game_status_text.clear();
    _game_error.clear();

    if (sessionChangedNeeded)
    {
        emit sessionChanged();
    }
    emit sessionsChanged();
    emit loadingChanged();
    emit streamingChanged();
    emit errorOccurred(QString());
    emit modelsChanged();
    emit modelStateChanged();
    emit thinkingChanged();
    emit knowledgeBasesChanged();
    emit knowledgeSearchResultChanged();
    emit knowledgeStateChanged();
    emit memoriesChanged();
    emit memoryStateChanged();
    emit agentTasksChanged();
    emit agentTaskStateChanged();
    emit gameRoomsChanged();
    emit gameTemplatesChanged();
    emit gameTemplatePresetsChanged();
    emit gameRulesetsChanged();
    emit gameRolePresetsChanged();
    emit gameStateChanged();
}
