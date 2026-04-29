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

namespace {
QString makeUuid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
}

AgentController::AgentController(ClientGateway* gateway, QObject* parent)
    : QObject(parent)
    , _gateway(gateway)
    , _model(new AgentMessageModel(this))
    , _current_model_backend("ollama")
    , _current_model_name("qwen3:4b")
    , _streamManager(new QNetworkAccessManager(this))
{
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
QString AgentController::currentTraceId() const { return _current_trace_id; }
QString AgentController::currentSkill() const { return _current_skill; }
QString AgentController::currentFeedbackSummary() const { return _current_feedback_summary; }
QVariantList AgentController::traceEvents() const { return _trace_events; }
QVariantList AgentController::traceObservations() const { return _trace_observations; }

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
    return metadata;
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
    _current_model_backend = backend;
    _current_model_name = model;
    if (_thinking_enabled && !currentModelSupportsThinking()) {
        _thinking_enabled = false;
    }
    emit modelChanged();
    emit thinkingChanged();
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

void AgentController::summarizeChat(const QString& dialogUid, const QString& chatHistoryJson) {
    auto uid = _gateway->userMgr()->GetUid();

    QJsonObject payload;
    payload["uid"] = uid;
    payload["feature_type"] = "summary";
    payload["content"] = chatHistoryJson;

    ReqId reqId = ID_AI_SMART;
    _pending_requests[reqId] = "summary";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/smart"), payload, reqId, Modules::LOGINMOD);
}

void AgentController::suggestReply(const QString& dialogUid, const QString& chatHistoryJson) {
    auto uid = _gateway->userMgr()->GetUid();

    QJsonObject payload;
    payload["uid"] = uid;
    payload["feature_type"] = "suggest";
    payload["content"] = chatHistoryJson;

    ReqId reqId = ID_AI_SMART;
    _pending_requests[reqId] = "suggest";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/smart"), payload, reqId, Modules::LOGINMOD);
}

void AgentController::translateMessage(const QString& msgContent, const QString& targetLang) {
    auto uid = _gateway->userMgr()->GetUid();

    QJsonObject payload;
    payload["uid"] = uid;
    payload["feature_type"] = "translate";
    payload["content"] = msgContent;
    payload["target_lang"] = targetLang;

    ReqId reqId = ID_AI_SMART;
    _pending_requests[reqId] = "translate";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/smart"), payload, reqId, Modules::LOGINMOD);
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
        } else if (reqType == "kb_upload" || reqType == "kb_search" ||
                   reqType == "kb_list" || reqType == "kb_delete") {
            setKnowledgeBusy(false);
            setKnowledgeError(errorText);
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
        } else if (reqType == "kb_upload" || reqType == "kb_search" ||
                   reqType == "kb_list" || reqType == "kb_delete") {
            setKnowledgeBusy(false);
            setKnowledgeError("响应格式错误");
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
        } else if (reqType == "kb_upload" || reqType == "kb_search" ||
                   reqType == "kb_list" || reqType == "kb_delete") {
            setKnowledgeBusy(false);
            setKnowledgeError(errorText);
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
    } else if (reqType == "summary" || reqType == "suggest" || reqType == "translate") {
        handleSmartRsp(id, res, err, reqType);
    } else if (reqType == "kb_upload" || reqType == "kb_search" ||
               reqType == "kb_list" || reqType == "kb_delete") {
        handleKbRsp(id, res, err, reqType);
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
    if (!currentModelAvailable && !models.isEmpty()) {
        QJsonObject fallback = root["default_model"].toObject();
        if (fallback.isEmpty()) {
            fallback = models.first().toObject();
        }
        const QString backend = fallback["model_type"].toString();
        const QString modelName = fallback["model_name"].toString();
        if (!backend.isEmpty() && !modelName.isEmpty()) {
            _current_model_backend = backend;
            _current_model_name = modelName;
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
