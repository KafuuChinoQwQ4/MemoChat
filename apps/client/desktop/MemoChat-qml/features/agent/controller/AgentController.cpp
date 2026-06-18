#include "AgentController.h"
#include "AgentGameClient.h"
#include "AgentMessageModel.h"
#include "AgentStreamClient.h"
#include "httpmgr.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <optional>

AgentController::AgentController(ClientGateway* gateway, QObject* parent)
    : QObject(parent)
    , _gateway(gateway)
    , _model(new AgentMessageModel(this))
    , _current_model_backend("")
    , _current_model_name("")
    , _streamClient(new AgentStreamClient(this))
    , _gameClient(new AgentGameClient(this))
{
    loadPersistedModelSelection();

    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_http_finish, this, &AgentController::onHttpFinish);
    connect(_streamClient, &AgentStreamClient::chunkReceived, this, &AgentController::handleStreamChunk);
    connect(_streamClient, &AgentStreamClient::finished, this, &AgentController::handleStreamFinished);
    connect(_gameClient, &AgentGameClient::responseReady, this, &AgentController::handleGameResponse);
    connect(_gameClient, &AgentGameClient::networkError, this, &AgentController::handleGameNetworkError);
    connect(_gameClient, &AgentGameClient::formatError, this, &AgentController::handleGameFormatError);

    connect(this,
            &AgentController::errorOccurred,
            this,
            [](const QString& err)
            {
                qWarning() << "[AgentController]" << err;
            });
}

AgentController::~AgentController()
{
}

void AgentController::onHttpFinish(ReqId id, const QString& res, ErrorCodes err, Modules mod)
{
    Q_UNUSED(mod);
    std::optional<AgentRequestRecord> pending = _pending_requests.take(id);
    if (!pending.has_value())
    {
        return;
    }
    const AgentRequestRecord record = pending.value();
    if (record.uid != 0 && record.uid != currentUid())
    {
        return;
    }

    auto resetFeatureBusyForError = [this, &record](const QString& errorText)
    {
        switch (record.kind)
        {
            case AgentRequestKind::ModelList:
                setModelRefreshBusy(false);
                break;
            case AgentRequestKind::ApiProviderRegister:
            case AgentRequestKind::ApiProviderDelete:
                setApiProviderBusy(false, errorText);
                break;
            case AgentRequestKind::KnowledgeUpload:
            case AgentRequestKind::KnowledgeSearch:
            case AgentRequestKind::KnowledgeList:
            case AgentRequestKind::KnowledgeDelete:
                setKnowledgeBusy(false);
                setKnowledgeError(errorText);
                break;
            case AgentRequestKind::MemoryList:
            case AgentRequestKind::MemoryCreate:
            case AgentRequestKind::MemoryDelete:
                setMemoryBusy(false);
                setMemoryError(errorText);
                break;
            case AgentRequestKind::TaskList:
            case AgentRequestKind::TaskCreate:
            case AgentRequestKind::TaskCancel:
            case AgentRequestKind::TaskResume:
                setAgentTaskBusy(false);
                setAgentTaskError(errorText);
                break;
            default:
                break;
        }
    };

    auto finishWithError = [this, &resetFeatureBusyForError, &record](const QString& errorText)
    {
        resetFeatureBusyForError(errorText);
        if (record.kind == AgentRequestKind::ChatMessage && !record.messageId.isEmpty() && _model)
        {
            _model->setError(record.messageId, errorText);
            _model->finalizeAIMessage(record.messageId);
            emit streamingFinished(record.messageId);
            if (record.messageId == _currentStreamMsgId)
            {
                setCurrentGeneratingMsgId(QString());
            }
        }
        setErrorState(errorText);
        _loading = false;
        _streaming = false;
        if (_model)
        {
            _model->finalizeAllStreamingMessages();
        }
        emit loadingChanged();
        emit streamingChanged();
    };

    if (err != ErrorCodes::SUCCESS)
    {
        finishWithError(QString("请求失败: error=%1").arg(static_cast<int>(err)));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    if (doc.isNull() || !doc.isObject())
    {
        finishWithError("响应格式错误");
        return;
    }

    QJsonObject root = doc.object();
    int code = root.contains("error") ? root["error"].toInt() : root["code"].toInt();
    if (code != 0)
    {
        QString msg = root["message"].toString();
        finishWithError(QString("AI服务错误: %1").arg(msg));
        return;
    }

    switch (record.kind)
    {
        case AgentRequestKind::ListSessions:
        case AgentRequestKind::CreateSession:
        case AgentRequestKind::DeleteSession:
        case AgentRequestKind::RenameSession:
            handleSessionRsp(id, res, err, record.kind);
            break;
        case AgentRequestKind::History:
            handleHistoryRsp(id, res, err);
            break;
        case AgentRequestKind::ModelList:
            handleModelListRsp(id, res, err);
            break;
        case AgentRequestKind::ApiProviderRegister:
        {
            const int modelCount = root["models"].toArray().size();
            setApiProviderBusy(false, QString("已接入 API，解析到 %1 个模型。").arg(modelCount));
            refreshModelList();
            break;
        }
        case AgentRequestKind::ApiProviderDelete:
            setApiProviderBusy(false, "API 模型已删除。");
            refreshModelList();
            break;
        case AgentRequestKind::Summary:
        case AgentRequestKind::Suggest:
        case AgentRequestKind::Translate:
            handleSmartRsp(id, res, err, record.kind);
            break;
        case AgentRequestKind::KnowledgeUpload:
        case AgentRequestKind::KnowledgeSearch:
        case AgentRequestKind::KnowledgeList:
        case AgentRequestKind::KnowledgeDelete:
            handleKbRsp(id, res, err, record.kind);
            break;
        case AgentRequestKind::MemoryList:
        case AgentRequestKind::MemoryCreate:
        case AgentRequestKind::MemoryDelete:
            handleMemoryRsp(id, res, err, record.kind);
            break;
        case AgentRequestKind::TaskList:
        case AgentRequestKind::TaskCreate:
        case AgentRequestKind::TaskCancel:
        case AgentRequestKind::TaskResume:
            handleAgentTaskRsp(id, res, err, record.kind);
            break;
        case AgentRequestKind::ChatMessage:
            handleChatRsp(id, res, err, record.messageId);
            break;
    }
}
