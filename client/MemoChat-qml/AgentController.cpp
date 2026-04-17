#include "AgentController.h"
#include "ClientGateway.h"
#include "AgentMessageModel.h"
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
    , _current_model_name("qwen2.5:7b")
    , _streamManager(new QNetworkAccessManager(this))
{
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_http_finish,
            this, &AgentController::onHttpFinish);

    connect(this, &AgentController::errorOccurred, this, [](const QString& err) {
        qWarning() << "[AgentController]" << err;
    });
}

AgentController::~AgentController() = default;

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

void AgentController::loadSessions() {
    auto uid = _gateway->userMgr()->GetUid();
    QJsonObject payload;
    payload["uid"] = uid;
    payload["model_type"] = _current_model_backend;
    payload["model_name"] = _current_model_name;

    ReqId reqId = ID_AI_SESSION_LIST;
    _pending_requests[reqId] = "list_sessions";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/session/list"), payload, reqId, Modules::LOGINMOD);
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
    auto uid = _gateway->userMgr()->GetUid();
    QJsonObject payload;
    payload["uid"] = uid;
    payload["session_id"] = sessionId;

    ReqId reqId = ID_AI_SESSION_DELETE;
    _pending_requests[reqId] = "delete_session";
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

    QJsonObject dummy;
    HttpMgr::GetInstance()->PostHttpReq(url, dummy, reqId, Modules::LOGINMOD);
}

void AgentController::sendMessage(const QString& content) {
    if (content.trimmed().isEmpty()) return;

    auto uid = _gateway->userMgr()->GetUid();
    QString sessionId = _current_session_id;
    if (sessionId.isEmpty()) {
        createSession();
        sessionId = _current_session_id;
    }

    _model->appendUserMessage(content);
    emit loadingChanged();

    QJsonObject payload;
    payload["uid"] = uid;
    payload["session_id"] = sessionId;
    payload["content"] = content;
    payload["model_type"] = _current_model_backend;
    payload["model_name"] = _current_model_name;

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
    if (sessionId.isEmpty()) {
        createSession();
        sessionId = _current_session_id;
    }

    _model->appendUserMessage(content);
    QString msgId = makeUuid();
    _model->appendAIMessage(msgId, _current_model_name);
    _streaming = true;
    _currentStreamMsgId = msgId;
    _accumulatedContent.clear();
    _streamBuffer.clear();
    emit streamingChanged();

    // 构建 SSE 请求
    QJsonObject payload;
    payload["uid"] = uid;
    payload["session_id"] = sessionId;
    payload["content"] = content;
    payload["model_type"] = _current_model_backend;
    payload["model_name"] = _current_model_name;

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
    emit modelChanged();
}

void AgentController::refreshModelList() {
    ReqId reqId = ID_AI_MODEL_LIST;
    _pending_requests[reqId] = "model_list";

    QJsonObject dummy;
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/model/list"), dummy, reqId, Modules::LOGINMOD);
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

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(QString("无法打开文件: %1").arg(filePath));
        return;
    }
    QByteArray fileData = file.readAll();
    file.close();

    QFileInfo info(filePath);
    QString fileName = info.fileName();
    QString suffix = info.suffix().toLower();

    QString fileType;
    if (suffix == "pdf") fileType = "pdf";
    else if (suffix == "txt") fileType = "txt";
    else if (suffix == "md" || suffix == "markdown") fileType = "md";
    else if (suffix == "docx") fileType = "docx";
    else {
        emit errorOccurred(QString("不支持的文件类型: %1").arg(suffix));
        return;
    }

    QJsonObject payload;
    payload["uid"] = uid;
    payload["file_name"] = fileName;
    payload["file_type"] = fileType;
    payload["content"] = QString::fromLatin1(fileData.toBase64());

    emit kbUploadProgress(0);

    ReqId reqId = ID_AI_KB_UPLOAD;
    _pending_requests[reqId] = "kb_upload";
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/ai/kb/upload"), payload, reqId, Modules::LOGINMOD);
}

void AgentController::searchKnowledgeBase(const QString& query) {
    auto uid = _gateway->userMgr()->GetUid();

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
    QUrl url(gate_url_prefix + "/ai/kb/list");
    QUrlQuery query;
    query.addQueryItem("uid", QString::number(uid));
    url.setQuery(query);

    ReqId reqId = ID_AI_KB_LIST;
    _pending_requests[reqId] = "kb_list";

    QJsonObject dummy;
    HttpMgr::GetInstance()->PostHttpReq(url, dummy, reqId, Modules::LOGINMOD);
}

void AgentController::deleteKnowledgeBase(const QString& kbId) {
    auto uid = _gateway->userMgr()->GetUid();

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
        emit errorOccurred(QString("请求失败: error=%1").arg(static_cast<int>(err)));
        emit loadingChanged();
        emit streamingChanged();
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        emit errorOccurred("响应格式错误");
        emit loadingChanged();
        emit streamingChanged();
        return;
    }

    QJsonObject root = doc.object();
    int code = root["error"].toInt();
    if (code != 0) {
        QString msg = root["message"].toString();
        emit errorOccurred(QString("AI服务错误: %1").arg(msg));
        emit loadingChanged();
        emit streamingChanged();
        return;
    }

    if (reqType == "list_sessions" || reqType == "create_session" || reqType == "delete_session") {
        handleSessionRsp(id, res, err);
    } else if (reqType == "history") {
        handleHistoryRsp(id, res, err);
    } else if (reqType == "model_list") {
        handleModelListRsp(id, res, err);
    } else if (reqType == "summary" || reqType == "suggest" || reqType == "translate") {
        handleSmartRsp(id, res, err);
    } else if (reqType == "kb_upload" || reqType == "kb_search" ||
               reqType == "kb_list" || reqType == "kb_delete") {
        handleKbRsp(id, res, err);
    } else {
        handleChatRsp(id, res, err);
    }
}

void AgentController::handleChatRsp(ReqId id, const QString& res, ErrorCodes err) {
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();

    QString msgId = _pending_requests.value(id);
    if (msgId.isEmpty()) {
        for (auto it = _pending_requests.begin(); it != _pending_requests.end(); ++it) {
            msgId = it.value();
            break;
        }
    }

    bool wasStreaming = _streaming;
    _loading = false;
    _streaming = false;
    emit loadingChanged();
    emit streamingChanged();

    QString content = root["content"].toString();
    if (!msgId.isEmpty()) {
        if (wasStreaming) {
            _model->updateStreamingContent(msgId, content);
            _model->finalizeAIMessage(msgId);
            emit streamingFinished(msgId);
        } else {
            _model->finalizeAIMessage(msgId);
        }
    }
    emit aiResponseReceived(content);
}

void AgentController::handleSmartRsp(ReqId id, const QString& res, ErrorCodes err) {
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();
    QString result = root["result"].toString();

    QString reqType = _pending_requests.value(id);
    emit smartResultReady(reqType, result);
}

void AgentController::handleSessionRsp(ReqId id, const QString& res, ErrorCodes err) {
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();

    QString reqType = _pending_requests.value(id);

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
    } else if (reqType == "create_session") {
        QJsonObject sess = root["session"].toObject();
        QString newId = sess["session_id"].toString();
        if (!newId.isEmpty()) {
            _current_session_id = newId;
            emit sessionChanged();
        }
        loadSessions();
    } else if (reqType == "delete_session") {
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
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();
    QJsonArray models = root["models"].toArray();

    _available_models.clear();
    for (const auto& m : models) {
        QJsonObject model = m.toObject();
        _available_models.append(model);
    }
    emit modelsChanged();
}

void AgentController::handleKbRsp(ReqId id, const QString& res, ErrorCodes err) {
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();
    QString reqType = _pending_requests.value(id);

    if (reqType == "kb_upload") {
        int chunks = root["chunks"].toInt();
        emit kbUploadProgress(100);
        emit knowledgeBasesChanged();
    } else if (reqType == "kb_list") {
        emit knowledgeBasesChanged();
    } else if (reqType == "kb_search") {
        QJsonArray chunks = root["chunks"].toArray();
        QString summary;
        for (const auto& c : chunks) {
            QJsonObject chunk = c.toObject();
            summary += chunk["content"].toString() + "\n\n";
        }
        emit aiResponseReceived(summary);
    } else if (reqType == "kb_delete") {
        emit knowledgeBasesChanged();
    }
}

void AgentController::cancelStream() {
    if (_currentStreamReply) {
        _currentStreamReply->abort();
        _currentStreamReply->deleteLater();
        _currentStreamReply = nullptr;
    }
    _streaming = false;
    _streamBuffer.clear();
    _currentStreamMsgId.clear();
    _accumulatedContent.clear();
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

    if (!chunkText.isEmpty()) {
        _accumulatedContent += chunkText;
        if (!msgId.isEmpty() && msgId == _currentStreamMsgId) {
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

    _currentStreamReply->deleteLater();
    _currentStreamReply = nullptr;

    if (err != QNetworkReply::NoError && err != QNetworkReply::OperationCanceledError) {
        qWarning() << "[AgentController] Stream error:" << err << errorStr;
        if (!_currentStreamMsgId.isEmpty()) {
            _model->setError(_currentStreamMsgId, errorStr);
            _model->finalizeAIMessage(_currentStreamMsgId);
            emit streamingFinished(_currentStreamMsgId);
        }
    }

    // 清理状态
    _streaming = false;
    _streamBuffer.clear();
    _currentStreamMsgId.clear();
    _accumulatedContent.clear();
    emit streamingChanged();
}

void AgentController::finishStream(const QString& msgId, const QString& finalContent) {
    if (!msgId.isEmpty()) {
        _model->updateStreamingContent(msgId, finalContent);
        _model->finalizeAIMessage(msgId);
        emit streamingFinished(msgId);
    }

    emit aiResponseReceived(finalContent);

    // 清理状态
    _streaming = false;
    emit streamingChanged();
}
