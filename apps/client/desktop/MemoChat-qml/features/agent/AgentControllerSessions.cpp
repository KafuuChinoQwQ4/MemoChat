#include "AgentController.h"

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
    auto uid = _gateway->userMgr()->GetUid();
    ReqId reqId = ID_AI_SESSION_LIST;
    _pending_requests[reqId] = "list_sessions";
    QUrl url(gate_url_prefix + "/ai/session/list");
    QUrlQuery query;
    query.addQueryItem("uid", QString::number(uid));
    query.addQueryItem("model_type", _current_model_backend);
    query.addQueryItem("model_name", _current_model_name);
    url.setQuery(query);
    HttpMgr::GetInstance()->GetHttpReq(url, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::createSession()
{
    auto uid = _gateway->userMgr()->GetUid();
    if (!_current_game_room_id.isEmpty())
    {
        _current_game_room_id.clear();
        _game_state.clear();
        emit gameStateChanged();
    }
    QJsonObject payload;
    payload["uid"] = uid;
    payload["model_type"] = _current_model_backend;
    payload["model_name"] = _current_model_name;

    ReqId reqId = ID_AI_SESSION_CREATE;
    _pending_requests[reqId] = "create_session";
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/ai/session"),
                                        payload,
                                        reqId,
                                        Modules::LOGINMOD,
                                        aiHttpModule());
}

void AgentController::switchSession(const QString& sessionId)
{
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
    if (sessionId.trimmed().isEmpty())
    {
        return;
    }
    auto uid = _gateway->userMgr()->GetUid();
    QJsonObject payload;
    payload["uid"] = uid;
    payload["session_id"] = sessionId;

    ReqId reqId = ID_AI_SESSION_DELETE;
    _pending_requests[reqId] = "delete_session";
    _pendingDeleteSessionId = sessionId;
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/ai/session/delete"),
                                        payload,
                                        reqId,
                                        Modules::LOGINMOD,
                                        aiHttpModule());
}

void AgentController::loadHistory(const QString& sessionId)
{
    auto uid = _gateway->userMgr()->GetUid();
    QUrl url(gate_url_prefix + "/ai/history");
    QUrlQuery query;
    query.addQueryItem("uid", QString::number(uid));
    query.addQueryItem("session_id", sessionId);
    query.addQueryItem("limit", "50");
    query.addQueryItem("offset", "0");
    url.setQuery(query);

    ReqId reqId = ID_AI_HISTORY;
    _pending_requests[reqId] = "history";
    _model->clear();

    HttpMgr::GetInstance()->GetHttpReq(url, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::handleSessionRsp(ReqId id, const QString& res, ErrorCodes err, const QString& reqType)
{
    Q_UNUSED(id);
    Q_UNUSED(err);
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();

    if (reqType == "list_sessions")
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
    else if (reqType == "create_session")
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
    else if (reqType == "delete_session")
    {
        if (!_pendingDeleteSessionId.isEmpty() && _pendingDeleteSessionId == _current_session_id)
        {
            clearCurrentSession();
        }
        _pendingDeleteSessionId.clear();
        loadSessions();
    }
}

void AgentController::handleHistoryRsp(ReqId id, const QString& res, ErrorCodes err)
{
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

    emit loadingChanged();
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
        _model->clear();
    }
}
