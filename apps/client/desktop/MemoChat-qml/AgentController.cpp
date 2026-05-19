#include "AgentController.h"
#include "ClientGateway.h"
#include "AgentMessageModel.h"
#include "LocalFilePickerService.h"
#include "httpmgr.h"
#include "usermgr.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QBuffer>
#include <QHttpMultiPart>
#include <QUuid>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QSettings>
#include <QJsonParseError>
#include <QSet>
#include <QSslConfiguration>
#include <QSslSocket>

namespace {
constexpr auto kSettingsOrg = "MemoChat";
constexpr auto kSettingsApp = "MemoChatQml";
constexpr auto kModelBackendKey = "AI/CurrentModelBackend";
constexpr auto kModelNameKey = "AI/CurrentModelName";

QString makeUuid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString aiHttpModule() {
    return QStringLiteral("ai");
}

}

AgentController::AgentController(ClientGateway* gateway, QObject* parent)
    : QObject(parent)
    , _gateway(gateway)
    , _model(new AgentMessageModel(this))
    , _current_model_backend("")
    , _current_model_name("")
    , _streamManager(new QNetworkAccessManager(this))
    , _gameNetwork(new QNetworkAccessManager(this))
{
    loadPersistedModelSelection();

    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_http_finish,
            this, &AgentController::onHttpFinish);

    connect(this, &AgentController::errorOccurred, this, [](const QString& err) {
        qWarning() << "[AgentController]" << err;
    });
}

AgentController::~AgentController() {
    if (_currentStreamReply) {
        QNetworkReply* reply = _currentStreamReply;
        _currentStreamReply = nullptr;
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
}

AgentMessageModel* AgentController::model() const { return _model; }
QString AgentController::currentSessionId() const { return _current_session_id; }
QString AgentController::currentModel() const {
    if (_current_model_backend.isEmpty() || _current_model_name.isEmpty()) {
        return {};
    }
    return QString("%1:%2").arg(_current_model_backend, _current_model_name);
}
bool AgentController::loading() const { return _loading; }
QString AgentController::error() const { return _error; }
bool AgentController::streaming() const { return _streaming; }

QVariantList AgentController::sessions() const { return _sessions; }

QVariantList AgentController::availableModels() const { return _available_models; }
bool AgentController::modelRefreshBusy() const { return _model_refresh_busy; }
bool AgentController::apiProviderBusy() const { return _api_provider_busy; }
QString AgentController::apiProviderStatus() const { return _api_provider_status; }
bool AgentController::thinkingEnabled() const { return _thinking_enabled; }
bool AgentController::currentModelSupportsThinking() const {
    return modelSupportsThinking(_current_model_backend, _current_model_name);
}
QVariantList AgentController::knowledgeBases() const { return _knowledge_bases; }
QString AgentController::knowledgeSearchResult() const { return _knowledge_search_result; }
bool AgentController::knowledgeBusy() const { return _knowledge_busy; }
QString AgentController::knowledgeStatusText() const { return _knowledge_status_text; }
QString AgentController::knowledgeError() const { return _knowledge_error; }
QVariantList AgentController::memories() const { return _memories; }
bool AgentController::memoryBusy() const { return _memory_busy; }
QString AgentController::memoryStatusText() const { return _memory_status_text; }
QString AgentController::memoryError() const { return _memory_error; }
QVariantList AgentController::agentTasks() const { return _agent_tasks; }
bool AgentController::agentTaskBusy() const { return _agent_task_busy; }
QString AgentController::agentTaskStatusText() const { return _agent_task_status_text; }
QString AgentController::agentTaskError() const { return _agent_task_error; }
QString AgentController::currentTraceId() const { return _current_trace_id; }
QString AgentController::currentSkill() const { return _current_skill; }
QString AgentController::currentFeedbackSummary() const { return _current_feedback_summary; }
QVariantList AgentController::traceEvents() const { return _trace_events; }
QVariantList AgentController::traceObservations() const { return _trace_observations; }
QString AgentController::agentSkillMode() const { return _agent_skill_mode; }
QString AgentController::agentSkillDisplay() const {
    if (_agent_skill_mode == QStringLiteral("knowledge")) return QStringLiteral("知识库");
    if (_agent_skill_mode == QStringLiteral("research")) return QStringLiteral("联网搜索");
    if (_agent_skill_mode == QStringLiteral("graph")) return QStringLiteral("图谱关系");
    if (_agent_skill_mode == QStringLiteral("calculate")) return QStringLiteral("计算器");
    return QStringLiteral("自动");
}

QVariantList AgentController::gameRooms() const { return _game_rooms; }
QVariantList AgentController::gameTemplates() const { return _game_templates; }
QVariantList AgentController::gameTemplatePresets() const { return _game_template_presets; }
QVariantMap AgentController::gameState() const { return _game_state; }
QString AgentController::currentGameRoomId() const { return _current_game_room_id; }
QVariantList AgentController::gameRulesets() const { return _game_rulesets; }
QVariantList AgentController::gameRolePresets() const { return _game_role_presets; }
bool AgentController::gameBusy() const { return _game_busy; }
QString AgentController::gameStatusText() const { return _game_status_text; }
QString AgentController::gameError() const { return _game_error; }

void AgentController::setErrorState(const QString& error) {
    _error = error;
    emit errorOccurred(_error);
}

void AgentController::clearErrorState() {
    if (_error.isEmpty()) {
        return;
    }
    _error.clear();
    emit errorOccurred(QString());
}

void AgentController::setModelRefreshBusy(bool busy) {
    if (_model_refresh_busy == busy) {
        return;
    }
    _model_refresh_busy = busy;
    emit modelStateChanged();
}

void AgentController::setApiProviderBusy(bool busy, const QString& statusText) {
    if (_api_provider_busy == busy && _api_provider_status == statusText) {
        return;
    }
    _api_provider_busy = busy;
    _api_provider_status = statusText;
    emit modelStateChanged();
}

void AgentController::setThinkingEnabled(bool enabled) {
    const bool next = enabled && currentModelSupportsThinking();
    if (_thinking_enabled != next) {
        _thinking_enabled = next;
        emit thinkingChanged();
    }
}

bool AgentController::modelSupportsThinking(const QString& backend, const QString& modelName) const {
    for (const QVariant& modelVar : _available_models) {
        const QVariantMap model = modelVar.toMap();
        if (model.value(QStringLiteral("model_type")).toString() == backend
            && model.value(QStringLiteral("model_name")).toString() == modelName) {
            return model.value(QStringLiteral("supports_thinking")).toBool();
        }
    }
    const QString normalized = (backend + QStringLiteral(":") + modelName).toLower();
    return normalized.contains(QStringLiteral("qwen3"))
        || normalized.contains(QStringLiteral("deepseek-reasoner"))
        || normalized.contains(QStringLiteral("reasoner"));
}

QJsonObject AgentController::buildChatMetadata() const {
    QJsonObject metadata;
    metadata[QStringLiteral("think")] = _thinking_enabled && currentModelSupportsThinking();
    const QString skillName = resolvedSkillName();
    if (!skillName.isEmpty()) {
        metadata[QStringLiteral("skill_name")] = skillName;
    }
    const QJsonArray requestedTools = requestedToolsForSkillMode();
    if (!requestedTools.isEmpty()) {
        metadata[QStringLiteral("requested_tools")] = requestedTools;
    }
    return metadata;
}

QString AgentController::resolvedSkillName() const {
    if (_agent_skill_mode == QStringLiteral("knowledge")) return QStringLiteral("knowledge_copilot");
    if (_agent_skill_mode == QStringLiteral("research")) return QStringLiteral("research_assistant");
    if (_agent_skill_mode == QStringLiteral("graph")) return QStringLiteral("graph_recommender");
    return QString();
}

QJsonArray AgentController::requestedToolsForSkillMode() const {
    QJsonArray tools;
    if (_agent_skill_mode == QStringLiteral("calculate")) {
        tools.append(QStringLiteral("calculator"));
    }
    return tools;
}

void AgentController::loadSessions() {
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

void AgentController::createSession() {
    auto uid = _gateway->userMgr()->GetUid();
    if (!_current_game_room_id.isEmpty()) {
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
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/session"), payload, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::switchSession(const QString& sessionId) {
    if (_current_session_id == sessionId) return;
    if (!_current_game_room_id.isEmpty()) {
        _current_game_room_id.clear();
        _game_state.clear();
        emit gameStateChanged();
    }
    _current_session_id = sessionId;
    emit sessionChanged();
    loadHistory(sessionId);
}

void AgentController::deleteSession(const QString& sessionId) {
    if (sessionId.trimmed().isEmpty()) {
        return;
    }
    auto uid = _gateway->userMgr()->GetUid();
    QJsonObject payload;
    payload["uid"] = uid;
    payload["session_id"] = sessionId;

    ReqId reqId = ID_AI_SESSION_DELETE;
    _pending_requests[reqId] = "delete_session";
    _pendingDeleteSessionId = sessionId;
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/session/delete"), payload, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::loadHistory(const QString& sessionId) {
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

void AgentController::sendMessage(const QString& content) {
    if (content.trimmed().isEmpty()) return;

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
    if (!skillName.isEmpty()) {
        payload["skill_name"] = skillName;
    }
    const QJsonArray requestedTools = requestedToolsForSkillMode();
    if (!requestedTools.isEmpty()) {
        payload["requested_tools"] = requestedTools;
    }

    QString msgId = makeUuid();
    ReqId reqId = ID_AI_CHAT;
    _pending_requests[reqId] = msgId;
    _model->appendAIMessage(msgId, _current_model_name);

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/chat"), payload, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::switchModel(const QString& backend, const QString& model) {
    const QString nextBackend = backend.trimmed();
    const QString nextModel = model.trimmed();
    if (nextBackend.isEmpty() || nextModel.isEmpty()) {
        return;
    }
    if (_current_model_backend == nextBackend && _current_model_name == nextModel) {
        saveCurrentModelSelection();
        return;
    }
    _current_model_backend = nextBackend;
    _current_model_name = nextModel;
    saveCurrentModelSelection();
    if (_thinking_enabled && !currentModelSupportsThinking()) {
        _thinking_enabled = false;
    }
    emit modelChanged();
    emit thinkingChanged();
}

void AgentController::switchAgentSkillMode(const QString& mode) {
    const QString normalized = mode.trimmed().toLower();
    const QString next = (normalized == QStringLiteral("knowledge")
                          || normalized == QStringLiteral("research")
                          || normalized == QStringLiteral("graph")
                          || normalized == QStringLiteral("calculate"))
        ? normalized
        : QStringLiteral("auto");
    if (_agent_skill_mode == next) {
        return;
    }
    _agent_skill_mode = next;
    emit agentSkillModeChanged();
}

void AgentController::refreshModelList() {
    clearErrorState();
    setModelRefreshBusy(true);
    ReqId reqId = ID_AI_MODEL_LIST;
    _pending_requests[reqId] = "model_list";

    HttpMgr::GetInstance()->GetHttpReq(
        QUrl(gate_url_prefix + "/ai/model/list"), reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::registerApiProvider(const QString& providerName, const QString& baseUrl, const QString& apiKey) {
    const QString trimmedUrl = baseUrl.trimmed();
    const QString trimmedKey = apiKey.trimmed();
    if (trimmedUrl.isEmpty() || trimmedKey.isEmpty()) {
        setApiProviderBusy(false, "请填写 API 地址和密钥。");
        setErrorState("请填写 API 地址和密钥。");
        return;
    }

    clearErrorState();
    setApiProviderBusy(true, "正在连接 API 并解析模型...");
    QJsonObject payload;
    payload["provider_name"] = providerName.trimmed().isEmpty() ? QStringLiteral("custom-api") : providerName.trimmed();
    payload["base_url"] = trimmedUrl;
    payload["api_key"] = trimmedKey;
    payload["adapter"] = QStringLiteral("openai_compatible");

    ReqId reqId = ID_AI_MODEL_API_REGISTER;
    _pending_requests[reqId] = "api_provider_register";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/model/api/register"), payload, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::deleteApiProvider(const QString& providerId) {
    const QString trimmedProviderId = providerId.trimmed();
    if (trimmedProviderId.isEmpty() || !trimmedProviderId.startsWith(QStringLiteral("api-"))) {
        setApiProviderBusy(false, "只能删除通过 API 接入的模型。");
        return;
    }

    clearErrorState();
    setApiProviderBusy(true, "正在删除 API 模型...");
    QJsonObject payload;
    payload["provider_id"] = trimmedProviderId;

    ReqId reqId = ID_AI_MODEL_API_DELETE;
    _pending_requests[reqId] = "api_provider_delete";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/model/api/delete"), payload, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::summarizeChat(const QString& dialogUid, const QString& chatHistoryJson) {
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

    ReqId reqId = ID_AI_SMART;
    _pending_requests[reqId] = "summary";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/smart"), payload, reqId, Modules::LOGINMOD, QStringLiteral("ai-smart"));
}

void AgentController::suggestReply(const QString& dialogUid, const QString& chatHistoryJson) {
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

    ReqId reqId = ID_AI_SMART;
    _pending_requests[reqId] = "suggest";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/smart"), payload, reqId, Modules::LOGINMOD, QStringLiteral("ai-smart"));
}

void AgentController::translateMessage(const QString& msgContent, const QString& targetLang) {
    translateMessageWithSource(msgContent, QStringLiteral("自动检测"), targetLang);
}

void AgentController::translateMessageWithSource(const QString& msgContent,
                                                 const QString& sourceLang,
                                                 const QString& targetLang) {
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

    ReqId reqId = ID_AI_SMART;
    _pending_requests[reqId] = "translate";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/smart"), payload, reqId, Modules::LOGINMOD, QStringLiteral("ai-smart"));
}

void AgentController::onHttpFinish(ReqId id, const QString& res, ErrorCodes err, Modules mod) {
    auto it = _pending_requests.find(id);
    if (it == _pending_requests.end()) return;
    QString reqType = it.value();
    _pending_requests.erase(it);

    if (err != ErrorCodes::SUCCESS) {
        const QString errorText = QString("请求失败: error=%1").arg(static_cast<int>(err));
        if (reqType == "model_list") {
            setModelRefreshBusy(false);
        } else if (reqType == "api_provider_register") {
            setApiProviderBusy(false, errorText);
        } else if (reqType == "api_provider_delete") {
            setApiProviderBusy(false, errorText);
        } else if (reqType == "kb_upload" || reqType == "kb_search" ||
                   reqType == "kb_list" || reqType == "kb_delete") {
            setKnowledgeBusy(false);
            setKnowledgeError(errorText);
        } else if (reqType == "memory_list" || reqType == "memory_create" || reqType == "memory_delete") {
            setMemoryBusy(false);
            setMemoryError(errorText);
        } else if (reqType == "task_list" || reqType == "task_create" ||
                   reqType == "task_cancel" || reqType == "task_resume") {
            setAgentTaskBusy(false);
            setAgentTaskError(errorText);
        }
        setErrorState(errorText);
        _loading = false;
        _streaming = false;
        emit loadingChanged();
        emit streamingChanged();
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        if (reqType == "model_list") {
            setModelRefreshBusy(false);
        } else if (reqType == "api_provider_register") {
            setApiProviderBusy(false, "响应格式错误");
        } else if (reqType == "api_provider_delete") {
            setApiProviderBusy(false, "响应格式错误");
        } else if (reqType == "kb_upload" || reqType == "kb_search" ||
                   reqType == "kb_list" || reqType == "kb_delete") {
            setKnowledgeBusy(false);
            setKnowledgeError("响应格式错误");
        } else if (reqType == "memory_list" || reqType == "memory_create" || reqType == "memory_delete") {
            setMemoryBusy(false);
            setMemoryError("响应格式错误");
        } else if (reqType == "task_list" || reqType == "task_create" ||
                   reqType == "task_cancel" || reqType == "task_resume") {
            setAgentTaskBusy(false);
            setAgentTaskError("响应格式错误");
        }
        setErrorState("响应格式错误");
        _loading = false;
        _streaming = false;
        emit loadingChanged();
        emit streamingChanged();
        return;
    }

    QJsonObject root = doc.object();
    int code = root.contains("error") ? root["error"].toInt() : root["code"].toInt();
    if (code != 0) {
        QString msg = root["message"].toString();
        const QString errorText = QString("AI服务错误: %1").arg(msg);
        if (reqType == "model_list") {
            setModelRefreshBusy(false);
        } else if (reqType == "api_provider_register") {
            setApiProviderBusy(false, errorText);
        } else if (reqType == "api_provider_delete") {
            setApiProviderBusy(false, errorText);
        } else if (reqType == "kb_upload" || reqType == "kb_search" ||
                   reqType == "kb_list" || reqType == "kb_delete") {
            setKnowledgeBusy(false);
            setKnowledgeError(errorText);
        } else if (reqType == "memory_list" || reqType == "memory_create" || reqType == "memory_delete") {
            setMemoryBusy(false);
            setMemoryError(errorText);
        } else if (reqType == "task_list" || reqType == "task_create" ||
                   reqType == "task_cancel" || reqType == "task_resume") {
            setAgentTaskBusy(false);
            setAgentTaskError(errorText);
        }
        setErrorState(errorText);
        _loading = false;
        _streaming = false;
        emit loadingChanged();
        emit streamingChanged();
        return;
    }

    if (reqType == "list_sessions" || reqType == "create_session" || reqType == "delete_session") {
        handleSessionRsp(id, res, err, reqType);
    } else if (reqType == "history") {
        handleHistoryRsp(id, res, err);
    } else if (reqType == "model_list") {
        handleModelListRsp(id, res, err);
    } else if (reqType == "api_provider_register") {
        QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
        QJsonObject root = doc.object();
        const int modelCount = root["models"].toArray().size();
        setApiProviderBusy(false, QString("已接入 API，解析到 %1 个模型。").arg(modelCount));
        refreshModelList();
    } else if (reqType == "api_provider_delete") {
        setApiProviderBusy(false, "API 模型已删除。");
        refreshModelList();
    } else if (reqType == "summary" || reqType == "suggest" || reqType == "translate") {
        handleSmartRsp(id, res, err, reqType);
    } else if (reqType == "kb_upload" || reqType == "kb_search" ||
               reqType == "kb_list" || reqType == "kb_delete") {
        handleKbRsp(id, res, err, reqType);
    } else if (reqType == "memory_list" || reqType == "memory_create" || reqType == "memory_delete") {
        handleMemoryRsp(id, res, err, reqType);
    } else if (reqType == "task_list" || reqType == "task_create" ||
               reqType == "task_cancel" || reqType == "task_resume") {
        handleAgentTaskRsp(id, res, err, reqType);
    } else {
        handleChatRsp(id, res, err, reqType);
    }
}

void AgentController::handleChatRsp(ReqId id, const QString& res, ErrorCodes err, const QString& msgId) {
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
    if (!responseSessionId.isEmpty() && _current_session_id != responseSessionId) {
        _current_session_id = responseSessionId;
        emit sessionChanged();
        loadSessions();
    }
    updateTraceFromResponse(root);
    if (!msgId.isEmpty()) {
        _model->updateStreamingContent(msgId, content);
        if (wasStreaming) {
            _model->finalizeAIMessage(msgId);
            emit streamingFinished(msgId);
        } else {
            _model->finalizeAIMessage(msgId);
        }
    }
    emit aiResponseReceived(content);
}

void AgentController::handleSmartRsp(ReqId id, const QString& res, ErrorCodes err, const QString& reqType) {
    Q_UNUSED(id);
    Q_UNUSED(err);
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();
    QString result = root["result"].toString();

    emit smartResultReady(reqType, result);
}

void AgentController::clearTrace() {
    _current_trace_id.clear();
    _current_skill.clear();
    _current_feedback_summary.clear();
    _trace_events.clear();
    _trace_observations.clear();
    emit traceChanged();
}

void AgentController::updateTraceFromResponse(const QJsonObject& root) {
    _current_trace_id = root["trace_id"].toString();
    _current_skill = root["skill"].toString();
    _current_feedback_summary = root["feedback_summary"].toString();

    _trace_observations.clear();
    const QJsonArray observations = root["observations"].toArray();
    for (const auto& observation : observations) {
        if (observation.isString()) {
            _trace_observations.append(observation.toString());
        } else if (observation.isObject()) {
            _trace_observations.append(observation.toObject().toVariantMap());
        }
    }

    _trace_events.clear();
    const QJsonArray events = root["events"].toArray();
    for (const auto& event : events) {
        if (event.isObject()) {
            _trace_events.append(event.toObject().toVariantMap());
        }
    }

    emit traceChanged();
}

void AgentController::handleSessionRsp(ReqId id, const QString& res, ErrorCodes err, const QString& reqType) {
    Q_UNUSED(id);
    Q_UNUSED(err);
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();

    if (reqType == "list_sessions") {
        QJsonArray sessions = root["sessions"].toArray();
        _sessions.clear();
        for (const auto& s : sessions) {
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
        if (_selectNewestSessionAfterList && _current_session_id.isEmpty() && !_sessions.isEmpty()) {
            const QVariantMap firstSession = _sessions.first().toMap();
            const QString sessionId = firstSession.value("session_id").toString();
            if (!sessionId.isEmpty()) {
                _current_session_id = sessionId;
                emit sessionChanged();
                loadHistory(sessionId);
            }
        }
        _selectNewestSessionAfterList = false;
    } else if (reqType == "create_session") {
        QJsonObject sess = root["session"].toObject();
        QString newId = sess["session_id"].toString();
        if (!newId.isEmpty()) {
            if (!_current_game_room_id.isEmpty()) {
                _current_game_room_id.clear();
                _game_state.clear();
                emit gameStateChanged();
            }
            _current_session_id = newId;
            emit sessionChanged();
        }
        loadSessions();
    } else if (reqType == "delete_session") {
        if (!_pendingDeleteSessionId.isEmpty() && _pendingDeleteSessionId == _current_session_id) {
            clearCurrentSession();
        }
        _pendingDeleteSessionId.clear();
        loadSessions();
    }
}

void AgentController::handleHistoryRsp(ReqId id, const QString& res, ErrorCodes err) {
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();
    QJsonArray messages = root["messages"].toArray();

    for (const auto& m : messages) {
        QJsonObject msg = m.toObject();
        QString role = msg["role"].toString();
        QString content = msg["content"].toString();
        QString msgId = msg["msg_id"].toString();
        qint64 createdAt = msg["created_at"].toVariant().toLongLong();

        if (role == "user") {
            _model->appendUserMessage(content);
        } else {
            QString aiMsgId = msgId.isEmpty() ? makeUuid() : msgId;
            _model->appendAIMessage(aiMsgId, "");
            _model->updateStreamingContent(aiMsgId, content);
            _model->finalizeAIMessage(aiMsgId);
        }
    }

    emit loadingChanged();
}

void AgentController::handleModelListRsp(ReqId id, const QString& res, ErrorCodes err) {
    Q_UNUSED(id);
    Q_UNUSED(err);
    setModelRefreshBusy(false);
    clearErrorState();
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();
    QJsonArray models = root["models"].toArray();

    _available_models.clear();
    bool currentModelAvailable = false;
    QSet<QString> seenModels;
    for (const auto& m : models) {
        QJsonObject model = m.toObject();
        const QString modelType = model["model_type"].toString().trimmed();
        const QString modelName = model["model_name"].toString().trimmed();
        if (modelType.isEmpty() || modelName.isEmpty()) {
            continue;
        }
        const QString dedupeKey = modelType + QStringLiteral(":") + modelName;
        if (seenModels.contains(dedupeKey)) {
            continue;
        }
        seenModels.insert(dedupeKey);
        if (modelType == _current_model_backend && modelName == _current_model_name) {
            currentModelAvailable = true;
        }
        _available_models.append(model);
    }
    if (!currentModelAvailable && _available_models.isEmpty()) {
        _current_model_backend.clear();
        _current_model_name.clear();
        clearPersistedModelSelection();
        emit modelChanged();
    } else if (!currentModelAvailable && !_available_models.isEmpty()) {
        QJsonObject fallback = root["default_model"].toObject();
        if (fallback.isEmpty()) {
            fallback = QJsonObject::fromVariantMap(_available_models.first().toMap());
        }
        const QString backend = fallback["model_type"].toString();
        const QString modelName = fallback["model_name"].toString();
        if (!backend.isEmpty() && !modelName.isEmpty()) {
            _current_model_backend = backend;
            _current_model_name = modelName;
            saveCurrentModelSelection();
            emit modelChanged();
        }
    }
    if (_thinking_enabled && !currentModelSupportsThinking()) {
        _thinking_enabled = false;
    }
    emit modelsChanged();
    emit thinkingChanged();
}

void AgentController::clearCurrentSession() {
    if (!_current_session_id.isEmpty()) {
        _current_session_id.clear();
        emit sessionChanged();
    }
    if (_model) {
        _model->clear();
    }
}

void AgentController::loadPersistedModelSelection() {
    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    const QString backend = settings.value(QString::fromLatin1(kModelBackendKey)).toString().trimmed();
    const QString modelName = settings.value(QString::fromLatin1(kModelNameKey)).toString().trimmed();
    if (!backend.isEmpty() && !modelName.isEmpty()) {
        _current_model_backend = backend;
        _current_model_name = modelName;
    }
}

void AgentController::saveCurrentModelSelection() const {
    if (_current_model_backend.trimmed().isEmpty() || _current_model_name.trimmed().isEmpty()) {
        return;
    }
    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    settings.setValue(QString::fromLatin1(kModelBackendKey), _current_model_backend);
    settings.setValue(QString::fromLatin1(kModelNameKey), _current_model_name);
}

void AgentController::clearPersistedModelSelection() const {
    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    settings.remove(QString::fromLatin1(kModelBackendKey));
    settings.remove(QString::fromLatin1(kModelNameKey));
}
