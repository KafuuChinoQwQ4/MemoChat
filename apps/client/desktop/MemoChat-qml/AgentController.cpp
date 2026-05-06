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

namespace {
constexpr auto kSettingsOrg = "MemoChat";
constexpr auto kSettingsApp = "MemoChatQml";
constexpr auto kModelBackendKey = "AI/CurrentModelBackend";
constexpr auto kModelNameKey = "AI/CurrentModelName";

QString makeUuid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString gameBaseUrl() {
    return gate_url_prefix + QStringLiteral("/ai/games");
}

QJsonValue sanitizedGameTemplateValue(const QJsonValue& value) {
    static const QStringList secretKeys = {
        QStringLiteral("api_key"),
        QStringLiteral("apikey"),
        QStringLiteral("apiKey"),
        QStringLiteral("authorization"),
        QStringLiteral("Authorization"),
        QStringLiteral("secret"),
        QStringLiteral("token"),
        QStringLiteral("password")
    };

    if (value.isArray()) {
        QJsonArray result;
        const QJsonArray source = value.toArray();
        for (const QJsonValue& item : source) {
            result.append(sanitizedGameTemplateValue(item));
        }
        return result;
    }
    if (!value.isObject()) {
        return value;
    }

    QJsonObject result;
    const QJsonObject source = value.toObject();
    for (auto it = source.constBegin(); it != source.constEnd(); ++it) {
        if (secretKeys.contains(it.key())) {
            continue;
        }
        result.insert(it.key(), sanitizedGameTemplateValue(it.value()));
    }
    return result;
}

QJsonObject jsonObjectFromVariant(const QVariant& value) {
    if (value.canConvert<QJsonObject>()) {
        return qvariant_cast<QJsonObject>(value);
    }
    return QJsonObject::fromVariantMap(value.toMap());
}

QJsonObject hostConfigFromVariant(const QVariantMap& host) {
    QJsonObject config;
    config[QStringLiteral("enabled")] = host.value(QStringLiteral("enabled")).toBool();
    config[QStringLiteral("display_name")] = host.value(QStringLiteral("display_name")).toString().trimmed().isEmpty()
        ? QStringLiteral("酒馆主持人")
        : host.value(QStringLiteral("display_name")).toString().trimmed();
    config[QStringLiteral("persona")] = host.value(QStringLiteral("persona")).toString().trimmed().isEmpty()
        ? QStringLiteral("你是游戏主持人。用简短、清晰、克制的中文主持当前阶段。")
        : host.value(QStringLiteral("persona")).toString().trimmed();
    config[QStringLiteral("model_type")] = host.value(QStringLiteral("model_type")).toString().trimmed();
    config[QStringLiteral("model_name")] = host.value(QStringLiteral("model_name")).toString().trimmed();
    config[QStringLiteral("skill_name")] = host.value(QStringLiteral("skill_name")).toString().trimmed().isEmpty()
        ? QStringLiteral("writer")
        : host.value(QStringLiteral("skill_name")).toString().trimmed();
    return config;
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

void AgentController::setKnowledgeBusy(bool busy, const QString& statusText) {
    bool changed = _knowledge_busy != busy || _knowledge_status_text != statusText;
    _knowledge_busy = busy;
    _knowledge_status_text = statusText;
    if (changed) {
        emit knowledgeStateChanged();
    }
}

void AgentController::setKnowledgeError(const QString& error) {
    if (_knowledge_error == error) {
        return;
    }
    _knowledge_error = error;
    emit knowledgeStateChanged();
}

void AgentController::clearKnowledgeError() {
    if (_knowledge_error.isEmpty()) {
        return;
    }
    _knowledge_error.clear();
    emit knowledgeStateChanged();
}

void AgentController::setMemoryBusy(bool busy, const QString& statusText) {
    bool changed = _memory_busy != busy || _memory_status_text != statusText;
    _memory_busy = busy;
    _memory_status_text = statusText;
    if (changed) {
        emit memoryStateChanged();
    }
}

void AgentController::setMemoryError(const QString& error) {
    if (_memory_error == error) {
        return;
    }
    _memory_error = error;
    emit memoryStateChanged();
}

void AgentController::clearMemoryError() {
    if (_memory_error.isEmpty()) {
        return;
    }
    _memory_error.clear();
    emit memoryStateChanged();
}

void AgentController::setAgentTaskBusy(bool busy, const QString& statusText) {
    bool changed = _agent_task_busy != busy || _agent_task_status_text != statusText;
    _agent_task_busy = busy;
    _agent_task_status_text = statusText;
    if (changed) {
        emit agentTaskStateChanged();
    }
}

void AgentController::setAgentTaskError(const QString& error) {
    if (_agent_task_error == error) {
        return;
    }
    _agent_task_error = error;
    emit agentTaskStateChanged();
}

void AgentController::clearAgentTaskError() {
    if (_agent_task_error.isEmpty()) {
        return;
    }
    _agent_task_error.clear();
    emit agentTaskStateChanged();
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

int AgentController::currentUid() const {
    return _gateway && _gateway->userMgr() ? _gateway->userMgr()->GetUid() : 0;
}

void AgentController::setGameBusy(bool busy, const QString& statusText) {
    if (_game_busy == busy && _game_status_text == statusText) {
        return;
    }
    _game_busy = busy;
    _game_status_text = statusText;
    emit gameStateChanged();
}

void AgentController::setGameError(const QString& error) {
    if (_game_error == error) {
        return;
    }
    _game_error = error;
    emit gameStateChanged();
}

void AgentController::clearGameError() {
    if (_game_error.isEmpty()) {
        return;
    }
    _game_error.clear();
    emit gameStateChanged();
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
    HttpMgr::GetInstance()->GetHttpReq(url, reqId, Modules::LOGINMOD);
}

void AgentController::createSession() {
    auto uid = _gateway->userMgr()->GetUid();
    QJsonObject payload;
    payload["uid"] = uid;
    payload["model_type"] = _current_model_backend;
    payload["model_name"] = _current_model_name;

    ReqId reqId = ID_AI_SESSION_CREATE;
    _pending_requests[reqId] = "create_session";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/session"), payload, reqId, Modules::LOGINMOD);
}

void AgentController::switchSession(const QString& sessionId) {
    if (_current_session_id == sessionId) return;
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
        QUrl(gate_url_prefix + "/ai/session/delete"), payload, reqId, Modules::LOGINMOD);
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

    HttpMgr::GetInstance()->GetHttpReq(url, reqId, Modules::LOGINMOD);
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
        QUrl(gate_url_prefix + "/ai/chat"), payload, reqId, Modules::LOGINMOD);
}

void AgentController::sendStreamMessage(const QString& content) {
    if (content.trimmed().isEmpty()) return;

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
    if (!skillName.isEmpty()) {
        payload["skill_name"] = skillName;
    }
    const QJsonArray requestedTools = requestedToolsForSkillMode();
    if (!requestedTools.isEmpty()) {
        payload["requested_tools"] = requestedTools;
    }

    QUrl url(gate_url_prefix + "/ai/chat/stream");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "text/event-stream");
    // 禁用缓存，保持连接
    request.setRawHeader("Cache-Control", "no-cache");
    request.setRawHeader("Connection", "keep-alive");

    // 发送请求并处理流式响应
    QByteArray data = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    _currentStreamReply = _streamManager->post(request, data);

    connect(_currentStreamReply, &QNetworkReply::readyRead,
            this, &AgentController::onStreamReadyRead);
    connect(_currentStreamReply, &QNetworkReply::finished,
            this, &AgentController::onStreamFinished);
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
        QUrl(gate_url_prefix + "/ai/model/list"), reqId, Modules::LOGINMOD);
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
        QUrl(gate_url_prefix + "/ai/model/api/register"), payload, reqId, Modules::LOGINMOD);
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
        QUrl(gate_url_prefix + "/ai/model/api/delete"), payload, reqId, Modules::LOGINMOD);
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

void AgentController::uploadDocument(const QString& filePath) {
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearKnowledgeError();
    setKnowledgeBusy(true, "正在准备上传文档...");

    QString localPath = filePath;
    const QUrl maybeUrl(filePath);
    if (maybeUrl.isLocalFile()) {
        localPath = maybeUrl.toLocalFile();
    }

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        const QString errorText = QString("无法打开文件: %1").arg(localPath);
        setKnowledgeBusy(false);
        setKnowledgeError(errorText);
        setErrorState(errorText);
        return;
    }
    QByteArray fileData = file.readAll();
    file.close();

    QFileInfo info(localPath);
    QString fileName = info.fileName();
    QString suffix = info.suffix().toLower();

    QString fileType;
    if (suffix == "pdf") fileType = "pdf";
    else if (suffix == "txt") fileType = "txt";
    else if (suffix == "md" || suffix == "markdown") fileType = "md";
    else if (suffix == "docx") fileType = "docx";
    else {
        const QString errorText = QString("不支持的文件类型: %1").arg(suffix);
        setKnowledgeBusy(false);
        setKnowledgeError(errorText);
        setErrorState(errorText);
        return;
    }

    QJsonObject payload;
    payload["uid"] = uid;
    payload["file_name"] = fileName;
    payload["file_type"] = fileType;
    payload["content"] = QString::fromLatin1(fileData.toBase64());

    emit kbUploadProgress(0);
    setKnowledgeBusy(true, QString("正在上传 %1...").arg(fileName));

    ReqId reqId = ID_AI_KB_UPLOAD;
    _pending_requests[reqId] = "kb_upload";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/kb/upload"), payload, reqId, Modules::LOGINMOD);
}

void AgentController::chooseAndUploadDocument() {
    QString fileUrl;
    QString displayName;
    QString errorText;
    if (!LocalFilePickerService::pickFileUrl(&fileUrl, &displayName, &errorText)) {
        if (!errorText.trimmed().isEmpty()) {
            setKnowledgeBusy(false);
            setKnowledgeError(errorText);
            setErrorState(errorText);
        }
        return;
    }
    uploadDocument(fileUrl);
}

void AgentController::searchKnowledgeBase(const QString& query) {
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearKnowledgeError();
    setKnowledgeBusy(true, "正在检索知识库...");
    _knowledge_search_result.clear();
    emit knowledgeSearchResultChanged();

    QJsonObject payload;
    payload["uid"] = uid;
    payload["query"] = query;
    payload["top_k"] = 5;

    ReqId reqId = ID_AI_KB_SEARCH;
    _pending_requests[reqId] = "kb_search";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/kb/search"), payload, reqId, Modules::LOGINMOD);
}

void AgentController::listKnowledgeBases() {
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearKnowledgeError();
    setKnowledgeBusy(true, "正在加载知识库...");
    QUrl url(gate_url_prefix + "/ai/kb/list");
    QUrlQuery query;
    query.addQueryItem("uid", QString::number(uid));
    url.setQuery(query);

    ReqId reqId = ID_AI_KB_LIST;
    _pending_requests[reqId] = "kb_list";

    HttpMgr::GetInstance()->GetHttpReq(url, reqId, Modules::LOGINMOD);
}

void AgentController::deleteKnowledgeBase(const QString& kbId) {
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearKnowledgeError();
    setKnowledgeBusy(true, "正在删除知识库...");

    QJsonObject payload;
    payload["uid"] = uid;
    payload["kb_id"] = kbId;

    ReqId reqId = ID_AI_KB_DELETE;
    _pending_requests[reqId] = "kb_delete";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/kb/delete"), payload, reqId, Modules::LOGINMOD);
}

void AgentController::listMemories() {
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearMemoryError();
    setMemoryBusy(true, "正在加载记忆...");
    QUrl url(gate_url_prefix + "/ai/memory/list");
    QUrlQuery query;
    query.addQueryItem("uid", QString::number(uid));
    url.setQuery(query);

    ReqId reqId = ID_AI_MEMORY_LIST;
    _pending_requests[reqId] = "memory_list";
    HttpMgr::GetInstance()->GetHttpReq(url, reqId, Modules::LOGINMOD);
}

void AgentController::createMemory(const QString& content) {
    const QString trimmed = content.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearMemoryError();
    setMemoryBusy(true, "正在保存记忆...");

    QJsonObject payload;
    payload["uid"] = uid;
    payload["content"] = trimmed;

    ReqId reqId = ID_AI_MEMORY_CREATE;
    _pending_requests[reqId] = "memory_create";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/memory"), payload, reqId, Modules::LOGINMOD);
}

void AgentController::deleteMemory(const QString& memoryId) {
    const QString trimmed = memoryId.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearMemoryError();
    setMemoryBusy(true, "正在删除记忆...");

    QJsonObject payload;
    payload["uid"] = uid;
    payload["memory_id"] = trimmed;

    ReqId reqId = ID_AI_MEMORY_DELETE;
    _pending_requests[reqId] = "memory_delete";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/memory/delete"), payload, reqId, Modules::LOGINMOD);
}

void AgentController::listAgentTasks() {
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearAgentTaskError();
    setAgentTaskBusy(true, "正在加载后台任务...");
    QUrl url(gate_url_prefix + "/ai/tasks");
    QUrlQuery query;
    query.addQueryItem("uid", QString::number(uid));
    query.addQueryItem("limit", "50");
    url.setQuery(query);

    ReqId reqId = ID_AI_TASK_LIST;
    _pending_requests[reqId] = "task_list";
    HttpMgr::GetInstance()->GetHttpReq(url, reqId, Modules::LOGINMOD);
}

void AgentController::createAgentTask(const QString& content, const QString& title) {
    const QString trimmed = content.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearAgentTaskError();
    setAgentTaskBusy(true, "正在创建后台任务...");

    QJsonObject payload;
    payload["uid"] = uid;
    payload["title"] = title.trimmed().isEmpty() ? trimmed.left(32) : title.trimmed();
    payload["content"] = trimmed;
    payload["session_id"] = _current_session_id;
    payload["model_type"] = _current_model_backend;
    payload["model_name"] = _current_model_name;
    const QString skillName = resolvedSkillName();
    if (!skillName.isEmpty()) {
        payload["skill_name"] = skillName;
    }
    payload["metadata"] = buildChatMetadata();

    ReqId reqId = ID_AI_TASK_CREATE;
    _pending_requests[reqId] = "task_create";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/tasks"), payload, reqId, Modules::LOGINMOD);
}

void AgentController::cancelAgentTask(const QString& taskId) {
    const QString trimmed = taskId.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    clearErrorState();
    clearAgentTaskError();
    setAgentTaskBusy(true, "正在取消后台任务...");

    QJsonObject payload;
    payload["task_id"] = trimmed;

    ReqId reqId = ID_AI_TASK_CANCEL;
    _pending_requests[reqId] = "task_cancel";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/tasks/cancel"), payload, reqId, Modules::LOGINMOD);
}

void AgentController::resumeAgentTask(const QString& taskId) {
    const QString trimmed = taskId.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    clearErrorState();
    clearAgentTaskError();
    setAgentTaskBusy(true, "正在恢复后台任务...");

    QJsonObject payload;
    payload["task_id"] = trimmed;

    ReqId reqId = ID_AI_TASK_RESUME;
    _pending_requests[reqId] = "task_resume";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/tasks/resume"), payload, reqId, Modules::LOGINMOD);
}

void AgentController::sendGameGet(const QUrl& url, const QString& op, const QString& statusText) {
    clearGameError();
    setGameBusy(true, statusText);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply* reply = _gameNetwork->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, op]() {
        const QByteArray body = reply->readAll();
        const QString networkError = reply->error() == QNetworkReply::NoError
            ? QString()
            : reply->errorString();
        reply->deleteLater();

        if (!networkError.isEmpty()) {
            setGameBusy(false, QStringLiteral("请求失败"));
            setGameError(QStringLiteral("Game 服务请求失败: %1").arg(networkError));
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            setGameBusy(false, QStringLiteral("响应格式错误"));
            setGameError(QStringLiteral("Game 服务响应格式错误"));
            return;
        }
        handleGameResponse(op, doc.object());
    });
}

void AgentController::sendGamePost(const QUrl& url, const QJsonObject& payload, const QString& op, const QString& statusText) {
    clearGameError();
    setGameBusy(true, statusText);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply* reply = _gameNetwork->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, op]() {
        const QByteArray body = reply->readAll();
        const QString networkError = reply->error() == QNetworkReply::NoError
            ? QString()
            : reply->errorString();
        reply->deleteLater();

        if (!networkError.isEmpty()) {
            setGameBusy(false, QStringLiteral("请求失败"));
            setGameError(QStringLiteral("Game 服务请求失败: %1").arg(networkError));
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            setGameBusy(false, QStringLiteral("响应格式错误"));
            setGameError(QStringLiteral("Game 服务响应格式错误"));
            return;
        }
        handleGameResponse(op, doc.object());
    });
}

void AgentController::sendGameDelete(const QUrl& url, const QString& op, const QString& statusText) {
    clearGameError();
    setGameBusy(true, statusText);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply* reply = _gameNetwork->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, op]() {
        const QByteArray body = reply->readAll();
        const QString networkError = reply->error() == QNetworkReply::NoError
            ? QString()
            : reply->errorString();
        reply->deleteLater();

        if (!networkError.isEmpty()) {
            setGameBusy(false, QStringLiteral("请求失败"));
            setGameError(QStringLiteral("Game 服务请求失败: %1").arg(networkError));
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            setGameBusy(false, QStringLiteral("响应格式错误"));
            setGameError(QStringLiteral("Game 服务响应格式错误"));
            return;
        }
        handleGameResponse(op, doc.object());
    });
}

void AgentController::listGameRulesets() {
    sendGameGet(QUrl(gameBaseUrl() + QStringLiteral("/rulesets")),
                QStringLiteral("rulesets"),
                QStringLiteral("正在加载游戏规则..."));
}

void AgentController::loadGameRolePresets(const QString& rulesetId) {
    QUrl url(gameBaseUrl() + QStringLiteral("/role-presets"));
    QUrlQuery query;
    const QString trimmedRuleset = rulesetId.trimmed();
    if (!trimmedRuleset.isEmpty()) {
        query.addQueryItem(QStringLiteral("ruleset_id"), trimmedRuleset);
    }
    url.setQuery(query);
    sendGameGet(url, QStringLiteral("role_presets"), QStringLiteral("正在加载角色预设..."));
}

void AgentController::listGameRooms() {
    QUrl url(gameBaseUrl() + QStringLiteral("/rooms"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("uid"), QString::number(currentUid()));
    url.setQuery(query);
    sendGameGet(url, QStringLiteral("rooms"), QStringLiteral("正在加载游戏房间..."));
}

void AgentController::listGameTemplates() {
    QUrl url(gameBaseUrl() + QStringLiteral("/templates"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("uid"), QString::number(currentUid()));
    url.setQuery(query);
    sendGameGet(url, QStringLiteral("templates"), QStringLiteral("正在加载游戏模板..."));
}

void AgentController::listGameTemplatePresets(const QString& rulesetId) {
    QUrl url(gameBaseUrl() + QStringLiteral("/template-presets"));
    QUrlQuery query;
    const QString trimmedRuleset = rulesetId.trimmed();
    if (!trimmedRuleset.isEmpty()) {
        query.addQueryItem(QStringLiteral("ruleset_id"), trimmedRuleset);
    }
    url.setQuery(query);
    sendGameGet(url, QStringLiteral("template_presets"), QStringLiteral("正在加载模板预设..."));
}

void AgentController::loadGameRoom(const QString& roomId) {
    const QString trimmedRoomId = roomId.trimmed();
    if (trimmedRoomId.isEmpty()) {
        return;
    }
    QUrl url(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("uid"), QString::number(currentUid()));
    url.setQuery(query);
    if (_current_game_room_id != trimmedRoomId) {
        _current_game_room_id = trimmedRoomId;
        emit gameStateChanged();
    }
    sendGameGet(url, QStringLiteral("room"), QStringLiteral("正在加载房间状态..."));
}

void AgentController::createGameRoom(const QString& title, const QString& rulesetId, const QVariantList& agents, const QVariantMap& host) {
    const QString trimmedRuleset = rulesetId.trimmed().isEmpty() ? QStringLiteral("werewolf.basic") : rulesetId.trimmed();
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    payload[QStringLiteral("title")] = title.trimmed().isEmpty() ? QStringLiteral("MemoChat Game Room") : title.trimmed();
    payload[QStringLiteral("ruleset_id")] = trimmedRuleset;

    QJsonArray agentArray;
    for (const QVariant& agentVar : agents) {
        const QVariantMap agentMap = agentVar.toMap();
        const QString name = agentMap.value(QStringLiteral("display_name")).toString().trimmed();
        const QString modelType = agentMap.value(QStringLiteral("model_type")).toString().trimmed();
        const QString modelName = agentMap.value(QStringLiteral("model_name")).toString().trimmed();
        if (name.isEmpty() || modelType.isEmpty() || modelName.isEmpty()) {
            continue;
        }
        QJsonObject agent;
        agent[QStringLiteral("display_name")] = name;
        agent[QStringLiteral("model_type")] = modelType;
        agent[QStringLiteral("model_name")] = modelName;
        agent[QStringLiteral("persona")] = agentMap.value(QStringLiteral("persona")).toString();
        agent[QStringLiteral("skill_name")] = agentMap.value(QStringLiteral("skill_name")).toString();
        agent[QStringLiteral("strategy")] = agentMap.value(QStringLiteral("strategy")).toString();
        agent[QStringLiteral("role_key")] = agentMap.value(QStringLiteral("role_key")).toString();
        QJsonObject metadata;
        metadata[QStringLiteral("source")] = QStringLiteral("memochat-qml");
        agent[QStringLiteral("metadata")] = metadata;
        agentArray.append(agent);
    }
    payload[QStringLiteral("agents")] = agentArray;
    QJsonObject config;
    config[QStringLiteral("host")] = hostConfigFromVariant(host);
    payload[QStringLiteral("config")] = config;

    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/rooms")),
                 payload,
                 QStringLiteral("create_room"),
                 QStringLiteral("正在创建游戏房间..."));
}

void AgentController::saveGameTemplate(const QString& title,
                                       const QString& description,
                                       const QString& rulesetId,
                                       const QVariantList& agents,
                                       const QVariantMap& host) {
    const QString trimmedRuleset = rulesetId.trimmed().isEmpty() ? QStringLiteral("werewolf.basic") : rulesetId.trimmed();
    const QString trimmedTitle = title.trimmed().isEmpty() ? QStringLiteral("MemoChat Game Template") : title.trimmed();

    QJsonObject payload;
    payload[QStringLiteral("template_id")] = QString();
    payload[QStringLiteral("uid")] = currentUid();
    payload[QStringLiteral("title")] = trimmedTitle;
    payload[QStringLiteral("description")] = description.trimmed();
    payload[QStringLiteral("ruleset_id")] = trimmedRuleset;

    QJsonArray agentArray;
    for (const QVariant& agentVar : agents) {
        const QVariantMap agentMap = agentVar.toMap();
        const QString name = agentMap.value(QStringLiteral("display_name")).toString().trimmed();
        const QString modelType = agentMap.value(QStringLiteral("model_type")).toString().trimmed();
        const QString modelName = agentMap.value(QStringLiteral("model_name")).toString().trimmed();
        if (name.isEmpty() || modelType.isEmpty() || modelName.isEmpty()) {
            continue;
        }
        QJsonObject agent;
        agent[QStringLiteral("display_name")] = name;
        agent[QStringLiteral("model_type")] = modelType;
        agent[QStringLiteral("model_name")] = modelName;
        agent[QStringLiteral("persona")] = agentMap.value(QStringLiteral("persona")).toString();
        agent[QStringLiteral("skill_name")] = agentMap.value(QStringLiteral("skill_name")).toString();
        agent[QStringLiteral("strategy")] = agentMap.value(QStringLiteral("strategy")).toString();
        agent[QStringLiteral("role_key")] = agentMap.value(QStringLiteral("role_key")).toString();
        QJsonObject metadata;
        metadata[QStringLiteral("source")] = QStringLiteral("memochat-qml");
        agent[QStringLiteral("metadata")] = metadata;
        agentArray.append(agent);
    }
    payload[QStringLiteral("max_players")] = qMax(1, agentArray.size() + 1);
    payload[QStringLiteral("agents")] = agentArray;

    QJsonObject config;
    config[QStringLiteral("source")] = QStringLiteral("agent-game-pane");
    config[QStringLiteral("host")] = hostConfigFromVariant(host);
    payload[QStringLiteral("config")] = config;

    QJsonObject metadata;
    metadata[QStringLiteral("source")] = QStringLiteral("memochat-qml");
    payload[QStringLiteral("metadata")] = metadata;

    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/templates")),
                 payload,
                 QStringLiteral("template"),
                 QStringLiteral("正在保存游戏模板..."));
}

void AgentController::deleteGameTemplate(const QString& templateId) {
    const QString trimmedTemplateId = templateId.trimmed();
    if (trimmedTemplateId.isEmpty()) {
        return;
    }
    QUrl url(gameBaseUrl() + QStringLiteral("/templates/") + trimmedTemplateId);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("uid"), QString::number(currentUid()));
    url.setQuery(query);
    sendGameDelete(url, QStringLiteral("delete_template"), QStringLiteral("正在删除游戏模板..."));
}

void AgentController::cloneGameTemplatePreset(const QString& presetId, const QString& title) {
    const QString trimmedPresetId = presetId.trimmed();
    if (trimmedPresetId.isEmpty()) {
        return;
    }

    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    payload[QStringLiteral("title")] = title.trimmed();

    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/template-presets/") + trimmedPresetId + QStringLiteral("/clone")),
                 payload,
                 QStringLiteral("clone_template_preset"),
                 QStringLiteral("正在克隆模板预设..."));
}

QString AgentController::exportGameTemplate(const QString& templateId) {
    const QString trimmedTemplateId = templateId.trimmed();
    if (trimmedTemplateId.isEmpty()) {
        setGameError(QStringLiteral("请选择要导出的模板。"));
        return {};
    }

    QJsonObject templ;
    const auto findTemplate = [&trimmedTemplateId](const QVariantList& source) -> QJsonObject {
        for (const QVariant& itemVar : source) {
            const QJsonObject item = jsonObjectFromVariant(itemVar);
            const QString itemId = item.value(QStringLiteral("template_id")).toString(
                item.value(QStringLiteral("id")).toString());
            if (itemId == trimmedTemplateId) {
                return item;
            }
        }
        return {};
    };

    templ = findTemplate(_game_templates);
    if (templ.isEmpty()) {
        templ = findTemplate(_game_template_presets);
    }
    if (templ.isEmpty()) {
        setGameError(QStringLiteral("未找到要导出的模板。"));
        return {};
    }

    QJsonObject exported = sanitizedGameTemplateValue(templ).toObject();
    exported[QStringLiteral("uid")] = currentUid();
    const QJsonDocument doc(exported);
    setGameBusy(false, QStringLiteral("模板 JSON 已导出。"));
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

bool AgentController::importGameTemplate(const QString& templateJson) {
    const QString trimmedJson = templateJson.trimmed();
    if (trimmedJson.isEmpty()) {
        setGameError(QStringLiteral("请输入模板 JSON。"));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(trimmedJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        setGameError(QStringLiteral("模板 JSON 格式错误: %1").arg(parseError.errorString()));
        return false;
    }

    QJsonObject payload = doc.object();
    if (!payload.value(QStringLiteral("agents")).isArray()) {
        setGameError(QStringLiteral("模板 JSON 缺少 agents 数组。"));
        return false;
    }
    payload[QStringLiteral("uid")] = currentUid();
    payload[QStringLiteral("template_id")] = payload.value(QStringLiteral("template_id")).toString();
    if (payload.value(QStringLiteral("title")).toString().trimmed().isEmpty()) {
        payload[QStringLiteral("title")] = QStringLiteral("Imported Game Template");
    }
    if (payload.value(QStringLiteral("ruleset_id")).toString().trimmed().isEmpty()) {
        payload[QStringLiteral("ruleset_id")] = QStringLiteral("werewolf.basic");
    }
    if (!payload.value(QStringLiteral("config")).isObject()) {
        payload[QStringLiteral("config")] = QJsonObject();
    }
    if (!payload.value(QStringLiteral("metadata")).isObject()) {
        payload[QStringLiteral("metadata")] = QJsonObject();
    }
    QJsonObject metadata = payload.value(QStringLiteral("metadata")).toObject();
    metadata[QStringLiteral("source")] = QStringLiteral("memochat-qml-import");
    payload[QStringLiteral("metadata")] = metadata;
    const int importedAgentCount = payload.value(QStringLiteral("agents")).toArray().size();
    const int importedMaxPlayers = payload.value(QStringLiteral("max_players")).toInt(0);
    payload[QStringLiteral("max_players")] = qMax(importedMaxPlayers, importedAgentCount + 1);

    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/templates")),
                 payload,
                 QStringLiteral("template"),
                 QStringLiteral("正在导入游戏模板..."));
    return true;
}

void AgentController::createGameRoomFromTemplate(const QString& templateId, const QString& title) {
    const QString trimmedTemplateId = templateId.trimmed();
    if (trimmedTemplateId.isEmpty()) {
        return;
    }
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    payload[QStringLiteral("title")] = title.trimmed();

    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/templates/") + trimmedTemplateId + QStringLiteral("/rooms")),
                 payload,
                 QStringLiteral("create_from_template"),
                 QStringLiteral("正在从模板创建房间..."));
}

void AgentController::startGameRoom(const QString& roomId) {
    const QString trimmedRoomId = roomId.trimmed();
    if (trimmedRoomId.isEmpty()) {
        return;
    }
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId + QStringLiteral("/start")),
                 payload,
                 QStringLiteral("start_room"),
                 QStringLiteral("正在开始游戏..."));
}

void AgentController::restartGameRoom(const QString& roomId) {
    const QString trimmedRoomId = roomId.trimmed();
    if (trimmedRoomId.isEmpty()) {
        return;
    }
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId + QStringLiteral("/restart")),
                 payload,
                 QStringLiteral("restart_room"),
                 QStringLiteral("正在重新开始游戏..."));
}

void AgentController::tickGameRoom(const QString& roomId) {
    const QString trimmedRoomId = roomId.trimmed();
    if (trimmedRoomId.isEmpty()) {
        return;
    }
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId + QStringLiteral("/tick")),
                 payload,
                 QStringLiteral("tick_room"),
                 QStringLiteral("正在推进 Agent 回合..."));
}

void AgentController::autoTickGameRoom(const QString& roomId, int maxSteps) {
    const QString trimmedRoomId = roomId.trimmed();
    if (trimmedRoomId.isEmpty()) {
        return;
    }
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    payload[QStringLiteral("max_steps")] = qBound(1, maxSteps, 32);
    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId + QStringLiteral("/auto-tick")),
                 payload,
                 QStringLiteral("auto_tick_room"),
                 QStringLiteral("正在自动推进 Agent 回合..."));
}

void AgentController::submitGameAction(const QString& roomId,
                                       const QString& actorId,
                                       const QString& actionType,
                                       const QString& targetId,
                                       const QString& content) {
    const QString trimmedRoomId = roomId.trimmed();
    if (trimmedRoomId.isEmpty()) {
        return;
    }
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    payload[QStringLiteral("participant_id")] = actorId.trimmed();
    payload[QStringLiteral("action_type")] = actionType.trimmed().isEmpty() ? QStringLiteral("say") : actionType.trimmed();
    payload[QStringLiteral("target_participant_id")] = targetId.trimmed();
    payload[QStringLiteral("content")] = content.trimmed();

    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId + QStringLiteral("/actions")),
                 payload,
                 QStringLiteral("action"),
                 QStringLiteral("正在提交玩家行动..."));
}

void AgentController::handleGameResponse(const QString& op, const QJsonObject& root) {
    const int code = root.contains(QStringLiteral("error"))
        ? root.value(QStringLiteral("error")).toInt()
        : root.value(QStringLiteral("code")).toInt();
    if (code != 0) {
        const QString message = root.value(QStringLiteral("message")).toString(QStringLiteral("未知错误"));
        setGameBusy(false, QStringLiteral("Game 服务错误"));
        setGameError(QStringLiteral("Game 服务错误: %1").arg(message));
        return;
    }

    clearGameError();
    QString statusText = QStringLiteral("游戏状态已更新。");
    bool refreshRooms = false;
    bool refreshTemplates = false;

    if (op == QStringLiteral("rulesets")) {
        _game_rulesets.clear();
        const QJsonArray rulesets = root.value(QStringLiteral("rulesets")).toArray();
        for (const QJsonValue& ruleset : rulesets) {
            if (ruleset.isObject()) {
                _game_rulesets.append(ruleset.toObject());
            }
        }
        emit gameRulesetsChanged();
        statusText = _game_rulesets.isEmpty()
            ? QStringLiteral("没有可用规则集。")
            : QStringLiteral("已加载 %1 个规则集。").arg(_game_rulesets.size());
    } else if (op == QStringLiteral("role_presets")) {
        _game_role_presets.clear();
        const QJsonArray presets = root.value(QStringLiteral("role_presets")).toArray();
        for (const QJsonValue& preset : presets) {
            if (preset.isObject()) {
                _game_role_presets.append(preset.toObject());
            }
        }
        emit gameRolePresetsChanged();
        statusText = _game_role_presets.isEmpty()
            ? QStringLiteral("当前规则没有角色预设。")
            : QStringLiteral("已加载 %1 个角色预设。").arg(_game_role_presets.size());
    } else if (op == QStringLiteral("rooms")) {
        _game_rooms.clear();
        const QJsonArray rooms = root.value(QStringLiteral("rooms")).toArray();
        for (const QJsonValue& room : rooms) {
            if (room.isObject()) {
                _game_rooms.append(room.toObject());
            }
        }
        emit gameRoomsChanged();
        statusText = _game_rooms.isEmpty()
            ? QStringLiteral("当前还没有游戏房间。")
            : QStringLiteral("已加载 %1 个游戏房间。").arg(_game_rooms.size());
    } else if (op == QStringLiteral("templates")) {
        _game_templates.clear();
        const QJsonArray templates = root.value(QStringLiteral("templates")).toArray();
        for (const QJsonValue& templ : templates) {
            if (templ.isObject()) {
                _game_templates.append(templ.toObject());
            }
        }
        emit gameTemplatesChanged();
        statusText = _game_templates.isEmpty()
            ? QStringLiteral("当前还没有游戏模板。")
            : QStringLiteral("已加载 %1 个游戏模板。").arg(_game_templates.size());
    } else if (op == QStringLiteral("template_presets")) {
        _game_template_presets.clear();
        const QJsonArray presets = root.value(QStringLiteral("template_presets")).toArray();
        for (const QJsonValue& preset : presets) {
            if (preset.isObject()) {
                _game_template_presets.append(preset.toObject());
            }
        }
        emit gameTemplatePresetsChanged();
        statusText = _game_template_presets.isEmpty()
            ? QStringLiteral("当前规则没有模板预设。")
            : QStringLiteral("已加载 %1 个模板预设。").arg(_game_template_presets.size());
    } else if (op == QStringLiteral("template")) {
        statusText = QStringLiteral("模板已保存，正在刷新列表...");
        const QJsonObject templ = root.value(QStringLiteral("template")).toObject();
        if (!templ.isEmpty()) {
            _game_templates.append(templ);
            emit gameTemplatesChanged();
        }
        refreshTemplates = true;
    } else if (op == QStringLiteral("clone_template_preset")) {
        statusText = QStringLiteral("预设已克隆，正在刷新模板...");
        const QJsonObject templ = root.value(QStringLiteral("template")).toObject();
        if (!templ.isEmpty()) {
            _game_templates.append(templ);
            emit gameTemplatesChanged();
        }
        refreshTemplates = true;
    } else if (op == QStringLiteral("delete_template")) {
        statusText = QStringLiteral("模板已删除，正在刷新列表...");
        refreshTemplates = true;
    }

    QJsonObject stateObject = root.value(QStringLiteral("state")).toObject();
    if (stateObject.isEmpty() && root.value(QStringLiteral("room")).isObject()) {
        stateObject[QStringLiteral("room")] = root.value(QStringLiteral("room")).toObject();
    }
    if (!stateObject.isEmpty()) {
        _game_state = stateObject.toVariantMap();
        const QJsonObject room = stateObject.value(QStringLiteral("room")).toObject();
        const QString roomId = room.value(QStringLiteral("room_id")).toString(room.value(QStringLiteral("id")).toString());
        if (!roomId.isEmpty()) {
            _current_game_room_id = roomId;
        }
        emit gameStateChanged();
        if (op == QStringLiteral("create_room")) {
            statusText = QStringLiteral("房间已创建。");
            refreshRooms = true;
        } else if (op == QStringLiteral("create_from_template")) {
            statusText = QStringLiteral("已从模板创建房间。");
            refreshRooms = true;
        } else if (op == QStringLiteral("start_room")) {
            statusText = QStringLiteral("游戏已开始。");
        } else if (op == QStringLiteral("restart_room")) {
            statusText = QStringLiteral("游戏已重新开始。");
            refreshRooms = true;
        } else if (op == QStringLiteral("tick_room")) {
            statusText = QStringLiteral("Agent 回合已推进。");
        } else if (op == QStringLiteral("auto_tick_room")) {
            const QJsonObject tick = root.value(QStringLiteral("tick")).toObject();
            const int steps = tick.value(QStringLiteral("steps")).toInt();
            const QString stopReason = tick.value(QStringLiteral("stop_reason")).toString();
            statusText = QStringLiteral("自动推进 %1 步，停止于 %2。").arg(steps).arg(stopReason.isEmpty() ? QStringLiteral("idle") : stopReason);
        } else if (op == QStringLiteral("action")) {
            statusText = QStringLiteral("玩家行动已提交。");
        }
    }

    setGameBusy(false, statusText);
    if (refreshRooms) {
        listGameRooms();
    }
    if (refreshTemplates) {
        listGameTemplates();
    }
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
    for (const auto& m : models) {
        QJsonObject model = m.toObject();
        if (model["model_type"].toString() == _current_model_backend &&
            model["model_name"].toString() == _current_model_name) {
            currentModelAvailable = true;
        }
        _available_models.append(model);
    }
    if (!currentModelAvailable && models.isEmpty()) {
        _current_model_backend.clear();
        _current_model_name.clear();
        clearPersistedModelSelection();
        emit modelChanged();
    } else if (!currentModelAvailable && !models.isEmpty()) {
        QJsonObject fallback = root["default_model"].toObject();
        if (fallback.isEmpty()) {
            fallback = models.first().toObject();
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

void AgentController::handleKbRsp(ReqId id, const QString& res, ErrorCodes err, const QString& reqType) {
    Q_UNUSED(id);
    Q_UNUSED(err);
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();

    if (reqType == "kb_upload") {
        Q_UNUSED(root["chunks"].toInt());
        emit kbUploadProgress(100);
        clearErrorState();
        setKnowledgeBusy(false, "文档上传完成，正在刷新知识库...");
        clearKnowledgeError();
        emit knowledgeBasesChanged();
        listKnowledgeBases();
    } else if (reqType == "kb_list") {
        QJsonArray bases = root["knowledge_bases"].toArray();
        _knowledge_bases.clear();
        for (const auto& base : bases) {
            _knowledge_bases.append(base.toObject());
        }
        clearErrorState();
        setKnowledgeBusy(false, _knowledge_bases.isEmpty() ? "当前还没有上传文档。" : QString("已加载 %1 个知识库条目。").arg(_knowledge_bases.size()));
        clearKnowledgeError();
        emit knowledgeBasesChanged();
    } else if (reqType == "kb_search") {
        QJsonArray chunks = root["chunks"].toArray();
        QString summary;
        for (const auto& c : chunks) {
            QJsonObject chunk = c.toObject();
            summary += chunk["content"].toString() + "\n\n";
        }
        _knowledge_search_result = summary.trimmed();
        clearErrorState();
        setKnowledgeBusy(false, _knowledge_search_result.isEmpty() ? "没有找到相关知识片段。" : QString("已返回 %1 条候选结果。").arg(chunks.size()));
        clearKnowledgeError();
        emit knowledgeSearchResultChanged();
        emit aiResponseReceived(summary);
    } else if (reqType == "kb_delete") {
        clearErrorState();
        setKnowledgeBusy(false, "知识库已删除，正在刷新列表...");
        clearKnowledgeError();
        emit knowledgeBasesChanged();
        listKnowledgeBases();
    }
}

void AgentController::handleMemoryRsp(ReqId id, const QString& res, ErrorCodes err, const QString& reqType) {
    Q_UNUSED(id);
    Q_UNUSED(err);
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();

    if (reqType == "memory_list") {
        QJsonArray memories = root["memories"].toArray();
        _memories.clear();
        for (const auto& memory : memories) {
            if (memory.isObject()) {
                _memories.append(memory.toObject());
            }
        }
        clearErrorState();
        clearMemoryError();
        setMemoryBusy(false, _memories.isEmpty() ? "当前还没有可见记忆。" : QString("已加载 %1 条记忆。").arg(_memories.size()));
        emit memoriesChanged();
    } else if (reqType == "memory_create") {
        clearErrorState();
        clearMemoryError();
        setMemoryBusy(false, "记忆已保存，正在刷新...");
        listMemories();
    } else if (reqType == "memory_delete") {
        clearErrorState();
        clearMemoryError();
        setMemoryBusy(false, "记忆已删除，正在刷新...");
        listMemories();
    }
}

void AgentController::handleAgentTaskRsp(ReqId id, const QString& res, ErrorCodes err, const QString& reqType) {
    Q_UNUSED(id);
    Q_UNUSED(err);
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();

    if (reqType == "task_list") {
        QJsonArray tasks = root["tasks"].toArray();
        _agent_tasks.clear();
        for (const auto& task : tasks) {
            if (task.isObject()) {
                _agent_tasks.append(task.toObject());
            }
        }
        clearErrorState();
        clearAgentTaskError();
        setAgentTaskBusy(false, _agent_tasks.isEmpty() ? "当前还没有后台任务。" : QString("已加载 %1 个后台任务。").arg(_agent_tasks.size()));
        emit agentTasksChanged();
    } else if (reqType == "task_create") {
        clearErrorState();
        clearAgentTaskError();
        setAgentTaskBusy(false, "任务已创建，正在刷新...");
        listAgentTasks();
    } else if (reqType == "task_cancel") {
        clearErrorState();
        clearAgentTaskError();
        setAgentTaskBusy(false, "任务已取消，正在刷新...");
        listAgentTasks();
    } else if (reqType == "task_resume") {
        clearErrorState();
        clearAgentTaskError();
        setAgentTaskBusy(false, "任务已恢复，正在刷新...");
        listAgentTasks();
    }
}

void AgentController::cancelStream() {
    const QString msgId = _currentStreamMsgId;
    const QString partialContent = _accumulatedContent;

    QNetworkReply* reply = _currentStreamReply;
    _currentStreamReply = nullptr;
    if (reply) {
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }

    if (!msgId.isEmpty()) {
        if (!partialContent.isEmpty()) {
            _model->updateStreamingContent(msgId, partialContent);
        } else {
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

void AgentController::onStreamReadyRead() {
    if (!_currentStreamReply) return;

    QByteArray data = _currentStreamReply->readAll();
    _streamBuffer.append(QString::fromUtf8(data));

    // 解析 SSE 数据：逐行处理
    int linesEnd = 0;
    while (true) {
        int newlinePos = _streamBuffer.indexOf('\n', linesEnd);
        if (newlinePos == -1) {
            // 没有完整的行，保留缓冲区
            _streamBuffer = _streamBuffer.mid(linesEnd);
            break;
        }

        QString line = _streamBuffer.mid(linesEnd, newlinePos - linesEnd);
        linesEnd = newlinePos + 1;

        // 处理空行（可能表示事件结束）
        if (line.isEmpty()) continue;

        // 解析 SSE 行
        parseSSEChunk(line);
    }
}

void AgentController::parseSSEChunk(const QString& line) {
    if (!line.startsWith("data:")) return;

    QString jsonStr = line.mid(5).trimmed();
    if (jsonStr.isEmpty() || jsonStr == "[DONE]") return;

    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    QJsonObject chunk = doc.object();
    QString chunkText = chunk["chunk"].toString();
    bool isFinal = chunk["is_final"].toBool();
    QString msgId = chunk["msg_id"].toString();

    if (isFinal) {
        updateTraceFromResponse(chunk);
    }

    if (!chunkText.isEmpty()) {
        _accumulatedContent += chunkText;
        if (!_currentStreamMsgId.isEmpty()) {
            _model->updateStreamingContent(_currentStreamMsgId, _accumulatedContent);
        }
        emit streamingChunkReceived(_currentStreamMsgId, chunkText);
    }

    if (isFinal) {
        finishStream(_currentStreamMsgId, _accumulatedContent);
    }
}

void AgentController::onStreamFinished() {
    if (!_currentStreamReply) return;

    QNetworkReply::NetworkError err = _currentStreamReply->error();
    QString errorStr = _currentStreamReply->errorString();
    const bool hasUsefulContent = _streamFinalReceived || !_accumulatedContent.isEmpty();

    _currentStreamReply->deleteLater();
    _currentStreamReply = nullptr;

    if (hasUsefulContent && !_streamFinalReceived && !_currentStreamMsgId.isEmpty()) {
        finishStream(_currentStreamMsgId, _accumulatedContent);
    }

    const bool normalStreamClose =
        hasUsefulContent && err == QNetworkReply::RemoteHostClosedError;
    if (err != QNetworkReply::NoError &&
        err != QNetworkReply::OperationCanceledError &&
        !normalStreamClose) {
        qWarning() << "[AgentController] Stream error:" << err << errorStr;
        if (!_currentStreamMsgId.isEmpty()) {
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

void AgentController::finishStream(const QString& msgId, const QString& finalContent) {
    if (_streamFinalReceived) {
        return;
    }
    _streamFinalReceived = true;
    if (!msgId.isEmpty()) {
        _model->updateStreamingContent(msgId, finalContent);
        _model->finalizeAIMessage(msgId);
        emit streamingFinished(msgId);
    }

    emit aiResponseReceived(finalContent);

    // 清理状态
    _streaming = false;
    if (_current_session_id.isEmpty()) {
        _selectNewestSessionAfterList = true;
        loadSessions();
    }
    emit streamingChanged();
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
