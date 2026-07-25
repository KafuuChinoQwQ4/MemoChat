#include "AgentController.h"
#include "ClientGateway.h"
#include "AgentGameClient.h"
#include "AgentMessageModel.h"
#include "AgentStreamClient.h"
#include "httpmgr.h"
#include "usermgr.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
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

ReqId AgentController::nextAgentHttpRequestId()
{
    constexpr int kFirstAgentHttpRequestId = 2000000;
    constexpr int kLastAgentHttpRequestId = 2100000;
    if (_next_agent_http_request_id < kFirstAgentHttpRequestId ||
        _next_agent_http_request_id >= kLastAgentHttpRequestId)
    {
        _next_agent_http_request_id = kFirstAgentHttpRequestId;
    }
    return static_cast<ReqId>(_next_agent_http_request_id++);
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
            case AgentRequestKind::ApiProviderDiscover:
                _api_provider_candidates.clear();
                _pending_api_provider_name.clear();
                _pending_api_provider_url.clear();
                _pending_api_provider_key.clear();
                setApiProviderBusy(false, errorText);
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
        const QString errorText =
            (err == ErrorCodes::ERR_NETWORK) ? QStringLiteral("AI 服务连接失败，请检查服务是否已启动")
                                             : QString("请求失败: error=%1").arg(static_cast<int>(err));
        finishWithError(errorText);
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
        case AgentRequestKind::ApiProviderDiscover:
        {
            _api_provider_candidates.clear();
            const QJsonArray models = root["models"].toArray();
            for (const QJsonValue& model : models)
            {
                const QJsonObject modelObject = model.toObject();
                if (!modelObject["model_name"].toString().trimmed().isEmpty())
                {
                    _api_provider_candidates.push_back(modelObject.toVariantMap());
                }
            }
            setApiProviderBusy(false,
                               QString("检测到 %1 个模型，请选择一个接入。").arg(_api_provider_candidates.size()));
            break;
        }
        case AgentRequestKind::ApiProviderRegister:
        {
            setApiProviderBusy(false, QString("已接入模型 %1。").arg(record.messageId));
            _api_provider_candidates.clear();
            _pending_api_provider_name.clear();
            _pending_api_provider_url.clear();
            _pending_api_provider_key.clear();
            refreshModelList();
            break;
        }
        case AgentRequestKind::ApiProviderDelete:
            setApiProviderBusy(false, QString("模型 %1 已删除。").arg(record.messageId));
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
