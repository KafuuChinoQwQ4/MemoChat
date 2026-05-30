#include "AgentController.h"
#include "AgentMessageModel.h"
#include "httpmgr.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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

    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_http_finish, this, &AgentController::onHttpFinish);

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
    if (_currentStreamReply)
    {
        QNetworkReply* reply = _currentStreamReply;
        _currentStreamReply = nullptr;
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
}

void AgentController::onHttpFinish(ReqId id, const QString& res, ErrorCodes err, Modules mod)
{
    auto it = _pending_requests.find(id);
    if (it == _pending_requests.end())
        return;
    QString reqType = it.value();
    _pending_requests.erase(it);

    if (err != ErrorCodes::SUCCESS)
    {
        const QString errorText = QString("请求失败: error=%1").arg(static_cast<int>(err));
        if (reqType == "model_list")
        {
            setModelRefreshBusy(false);
        }
        else if (reqType == "api_provider_register")
        {
            setApiProviderBusy(false, errorText);
        }
        else if (reqType == "api_provider_delete")
        {
            setApiProviderBusy(false, errorText);
        }
        else if (reqType == "kb_upload" || reqType == "kb_search" || reqType == "kb_list" || reqType == "kb_delete")
        {
            setKnowledgeBusy(false);
            setKnowledgeError(errorText);
        }
        else if (reqType == "memory_list" || reqType == "memory_create" || reqType == "memory_delete")
        {
            setMemoryBusy(false);
            setMemoryError(errorText);
        }
        else if (reqType == "task_list" || reqType == "task_create" || reqType == "task_cancel" ||
                 reqType == "task_resume")
        {
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
    if (doc.isNull() || !doc.isObject())
    {
        if (reqType == "model_list")
        {
            setModelRefreshBusy(false);
        }
        else if (reqType == "api_provider_register")
        {
            setApiProviderBusy(false, "响应格式错误");
        }
        else if (reqType == "api_provider_delete")
        {
            setApiProviderBusy(false, "响应格式错误");
        }
        else if (reqType == "kb_upload" || reqType == "kb_search" || reqType == "kb_list" || reqType == "kb_delete")
        {
            setKnowledgeBusy(false);
            setKnowledgeError("响应格式错误");
        }
        else if (reqType == "memory_list" || reqType == "memory_create" || reqType == "memory_delete")
        {
            setMemoryBusy(false);
            setMemoryError("响应格式错误");
        }
        else if (reqType == "task_list" || reqType == "task_create" || reqType == "task_cancel" ||
                 reqType == "task_resume")
        {
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
    if (code != 0)
    {
        QString msg = root["message"].toString();
        const QString errorText = QString("AI服务错误: %1").arg(msg);
        if (reqType == "model_list")
        {
            setModelRefreshBusy(false);
        }
        else if (reqType == "api_provider_register")
        {
            setApiProviderBusy(false, errorText);
        }
        else if (reqType == "api_provider_delete")
        {
            setApiProviderBusy(false, errorText);
        }
        else if (reqType == "kb_upload" || reqType == "kb_search" || reqType == "kb_list" || reqType == "kb_delete")
        {
            setKnowledgeBusy(false);
            setKnowledgeError(errorText);
        }
        else if (reqType == "memory_list" || reqType == "memory_create" || reqType == "memory_delete")
        {
            setMemoryBusy(false);
            setMemoryError(errorText);
        }
        else if (reqType == "task_list" || reqType == "task_create" || reqType == "task_cancel" ||
                 reqType == "task_resume")
        {
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

    if (reqType == "list_sessions" || reqType == "create_session" || reqType == "delete_session")
    {
        handleSessionRsp(id, res, err, reqType);
    }
    else if (reqType == "history")
    {
        handleHistoryRsp(id, res, err);
    }
    else if (reqType == "model_list")
    {
        handleModelListRsp(id, res, err);
    }
    else if (reqType == "api_provider_register")
    {
        QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
        QJsonObject root = doc.object();
        const int modelCount = root["models"].toArray().size();
        setApiProviderBusy(false, QString("已接入 API，解析到 %1 个模型。").arg(modelCount));
        refreshModelList();
    }
    else if (reqType == "api_provider_delete")
    {
        setApiProviderBusy(false, "API 模型已删除。");
        refreshModelList();
    }
    else if (reqType == "summary" || reqType == "suggest" || reqType == "translate")
    {
        handleSmartRsp(id, res, err, reqType);
    }
    else if (reqType == "kb_upload" || reqType == "kb_search" || reqType == "kb_list" || reqType == "kb_delete")
    {
        handleKbRsp(id, res, err, reqType);
    }
    else if (reqType == "memory_list" || reqType == "memory_create" || reqType == "memory_delete")
    {
        handleMemoryRsp(id, res, err, reqType);
    }
    else if (reqType == "task_list" || reqType == "task_create" || reqType == "task_cancel" || reqType == "task_resume")
    {
        handleAgentTaskRsp(id, res, err, reqType);
    }
    else
    {
        handleChatRsp(id, res, err, reqType);
    }
}
