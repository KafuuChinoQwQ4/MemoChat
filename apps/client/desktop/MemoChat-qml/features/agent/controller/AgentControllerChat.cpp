#include "AgentController.h"

#include "AgentMessageModel.h"
#include "AgentNetworkRequestUtils.h"
#include "ClientGateway.h"
#include "httpmgr.h"
#include "usermgr.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
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

bool isGuardrailBlockedOutput(const QString& text)
{
    return text.trimmed().startsWith(QStringLiteral("Guardrail blocked"), Qt::CaseInsensitive);
}
} // namespace

void AgentController::sendMessage(const QString& content)
{
    if (content.trimmed().isEmpty())
        return;

    ensureUserScope();
    auto uid = _gateway->userMgr()->GetUid();
    QString sessionId = _current_session_id;

    _model->appendUserMessage(content);
    clearTrace();
    clearErrorState();
    _loading = true;
    emit loadingChanged();

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
    addAuthToPayload(payload);

    QString msgId = makeUuid();
    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::ChatMessage, msgId, uid);
    _model->appendAIMessage(msgId, _current_model_name);
    setCurrentGeneratingMsgId(msgId);

    HttpMgr::GetInstance()->PostHttpReq(
        agentApiUrl(QStringLiteral("/ai/chat")), payload, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::summarizeChat(const QString& dialogUid, const QString& chatHistoryJson)
{
    ensureUserScope();
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();

    QJsonObject payload;
    payload["uid"] = uid;
    payload["feature_type"] = "summary";
    payload["content"] = chatHistoryJson;
    payload["model_type"] = _current_model_backend;
    payload["model_name"] = _current_model_name;
    QJsonObject context;
    context["dialog"] = dialogUid;
    context["max_messages"] = 100;
    payload["context_json"] = QString::fromUtf8(QJsonDocument(context).toJson(QJsonDocument::Compact));
    addAuthToPayload(payload);

    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::Summary, QString(), uid);
    HttpMgr::GetInstance()->PostHttpReq(
        agentApiUrl(QStringLiteral("/ai/smart")), payload, reqId, Modules::LOGINMOD, QStringLiteral("ai-smart"));
}

void AgentController::suggestReply(const QString& dialogUid, const QString& chatHistoryJson)
{
    ensureUserScope();
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();

    QJsonObject payload;
    payload["uid"] = uid;
    payload["feature_type"] = "suggest";
    payload["content"] = chatHistoryJson;
    payload["model_type"] = _current_model_backend;
    payload["model_name"] = _current_model_name;
    QJsonObject context;
    context["dialog"] = dialogUid;
    context["format"] = "three_options";
    payload["context_json"] = QString::fromUtf8(QJsonDocument(context).toJson(QJsonDocument::Compact));
    addAuthToPayload(payload);

    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::Suggest, QString(), uid);
    HttpMgr::GetInstance()->PostHttpReq(
        agentApiUrl(QStringLiteral("/ai/smart")), payload, reqId, Modules::LOGINMOD, QStringLiteral("ai-smart"));
}

void AgentController::translateMessage(const QString& msgContent, const QString& targetLang)
{
    translateMessageWithSource(msgContent, QStringLiteral("自动检测"), targetLang);
}

void AgentController::translateMessageWithSource(const QString& msgContent,
                                                 const QString& sourceLang,
                                                 const QString& targetLang)
{
    ensureUserScope();
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();

    QJsonObject payload;
    payload["uid"] = uid;
    payload["feature_type"] = "translate";
    payload["content"] = msgContent;
    payload["target_lang"] = targetLang.trimmed().isEmpty() ? QStringLiteral("中文") : targetLang.trimmed();
    payload["model_type"] = _current_model_backend;
    payload["model_name"] = _current_model_name;
    QJsonObject context;
    context["source_lang"] = sourceLang.trimmed().isEmpty() ? QStringLiteral("自动检测") : sourceLang.trimmed();
    payload["context_json"] = QString::fromUtf8(QJsonDocument(context).toJson(QJsonDocument::Compact));
    addAuthToPayload(payload);

    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::Translate, QString(), uid);
    HttpMgr::GetInstance()->PostHttpReq(
        agentApiUrl(QStringLiteral("/ai/smart")), payload, reqId, Modules::LOGINMOD, QStringLiteral("ai-smart"));
}

void AgentController::handleChatRsp(ReqId id, const QString& res, ErrorCodes err, const QString& msgId)
{
    Q_UNUSED(id);
    Q_UNUSED(err);
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();

    bool wasStreaming = _streaming;
    _loading = false;
    _streaming = false;
    clearErrorState();
    emit loadingChanged();
    emit streamingChanged();

    QString content = root["content"].toString();
    const QString responseSessionId = root["session_id"].toString();
    if (!responseSessionId.isEmpty() && _current_session_id != responseSessionId)
    {
        _current_session_id = responseSessionId;
        emit sessionChanged();
        loadSessions();
    }
    updateTraceFromResponse(root);
    if (!msgId.isEmpty())
    {
        _model->updateStreamingContent(msgId, content);
        if (wasStreaming)
        {
            _model->finalizeAIMessage(msgId);
            emit streamingFinished(msgId);
        }
        else
        {
            _model->finalizeAIMessage(msgId);
        }
        setCurrentGeneratingMsgId(QString());
    }
    _model->finalizeAllStreamingMessages();
    emit aiResponseReceived(content);
}

void AgentController::handleSmartRsp(ReqId id, const QString& res, ErrorCodes err, AgentRequestKind kind)
{
    Q_UNUSED(id);
    Q_UNUSED(err);
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();
    QString result = root["result"].toString();
    if (isGuardrailBlockedOutput(result))
    {
        setErrorState(QStringLiteral("AI 没有生成有效内容，请检查模型/API Key、额度或模型服务状态。"));
        return;
    }

    emit smartResultReady(agentRequestFeatureType(kind), result);
}

void AgentController::clearTrace()
{
    _current_trace_id.clear();
    _current_skill.clear();
    _current_feedback_summary.clear();
    _trace_events.clear();
    _trace_observations.clear();
    emit traceChanged();
}

void AgentController::updateTraceFromResponse(const QJsonObject& root)
{
    _current_trace_id = root["trace_id"].toString();
    _current_skill = root["skill"].toString();
    _current_feedback_summary = root["feedback_summary"].toString();

    _trace_observations.clear();
    const QJsonArray observations = root["observations"].toArray();
    for (const auto& observation : observations)
    {
        if (observation.isString())
        {
            _trace_observations.append(observation.toString());
        }
        else if (observation.isObject())
        {
            _trace_observations.append(observation.toObject().toVariantMap());
        }
    }

    _trace_events.clear();
    const QJsonArray events = root["events"].toArray();
    for (const auto& event : events)
    {
        if (event.isObject())
        {
            _trace_events.append(event.toObject().toVariantMap());
        }
    }

    emit traceChanged();
}
