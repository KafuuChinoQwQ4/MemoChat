#include "AgentController.h"

#include "AgentNetworkRequestUtils.h"
#include "httpmgr.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QUrl>
#include <QUrlQuery>

namespace
{
QString aiHttpModule()
{
    return QStringLiteral("ai");
}
} // namespace

void AgentController::switchModel(const QString& backend, const QString& model)
{
    const QString nextBackend = backend.trimmed();
    const QString nextModel = model.trimmed();
    if (nextBackend.isEmpty() || nextModel.isEmpty())
    {
        return;
    }
    if (_current_model_backend == nextBackend && _current_model_name == nextModel)
    {
        saveCurrentModelSelection();
        return;
    }
    _current_model_backend = nextBackend;
    _current_model_name = nextModel;
    saveCurrentModelSelection();
    if (_thinking_enabled && !currentModelSupportsThinking())
    {
        _thinking_enabled = false;
    }
    emit modelChanged();
    emit thinkingChanged();
}

void AgentController::switchAgentSkillMode(const QString& mode)
{
    const QString normalized = mode.trimmed().toLower();
    const QString next =
        (normalized ==
         QStringLiteral("knowledge") ||
                        normalized == QStringLiteral("research") ||
                                                     normalized ==
                                                         QStringLiteral("graph") ||
                                                                            normalized == QStringLiteral("calculate"))
                                                         ? normalized
                                                         : QStringLiteral("auto");
    if (_agent_skill_mode == next)
    {
        return;
    }
    _agent_skill_mode = next;
    emit agentSkillModeChanged();
}

void AgentController::refreshModelList()
{
    ensureUserScope();
    if (_model_refresh_busy)
    {
        return;
    }
    clearErrorState();
    setModelRefreshBusy(true);
    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::ModelList, QString(), scopedUid());

    QUrl url = agentApiUrl(QStringLiteral("/ai/model/list"));
    QUrlQuery query;
    url.setQuery(query);

    HttpMgr::GetInstance()->GetHttpReq(url, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::discoverApiProvider(const QString& providerName, const QString& baseUrl, const QString& apiKey)
{
    ensureUserScope();
    const QString trimmedUrl = baseUrl.trimmed();
    const QString trimmedKey = apiKey.trimmed();
    if (trimmedUrl.isEmpty() || trimmedKey.isEmpty())
    {
        setApiProviderBusy(false, "请填写 API 地址和密钥。");
        setErrorState("请填写 API 地址和密钥。");
        return;
    }

    clearErrorState();
    _api_provider_candidates.clear();
    _pending_api_provider_name =
        providerName.trimmed().isEmpty() ? QStringLiteral("custom-api") : providerName.trimmed();
    _pending_api_provider_url = trimmedUrl;
    _pending_api_provider_key = trimmedKey;
    setApiProviderBusy(true, "正在检测 API 模型...");
    QJsonObject payload;
    payload["provider_name"] = _pending_api_provider_name;
    payload["base_url"] = trimmedUrl;
    payload["api_key"] = trimmedKey;
    payload["adapter"] = QStringLiteral("openai_compatible");

    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::ApiProviderDiscover, QString(), scopedUid());
    HttpMgr::GetInstance()->PostHttpReq(
        agentApiUrl(QStringLiteral("/ai/model/api/discover")), payload, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::registerDiscoveredApiModel(const QString& modelName)
{
    ensureUserScope();
    const QString selectedModel = modelName.trimmed();
    if (_pending_api_provider_url.isEmpty() || _pending_api_provider_key.isEmpty() || selectedModel.isEmpty())
    {
        setApiProviderBusy(false, "请先检测 API 并选择一个模型。");
        return;
    }

    bool candidateFound = false;
    for (const QVariant& candidateValue : _api_provider_candidates)
    {
        if (candidateValue.toMap().value(QStringLiteral("model_name")).toString() == selectedModel)
        {
            candidateFound = true;
            break;
        }
    }
    if (!candidateFound)
    {
        setApiProviderBusy(false, "所选模型不在本次检测结果中。");
        return;
    }

    clearErrorState();
    setApiProviderBusy(true, QString("正在接入模型 %1...").arg(selectedModel));
    QJsonObject payload;
    payload["provider_name"] = _pending_api_provider_name;
    payload["base_url"] = _pending_api_provider_url;
    payload["api_key"] = _pending_api_provider_key;
    payload["adapter"] = QStringLiteral("openai_compatible");
    payload["model_name"] = selectedModel;

    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::ApiProviderRegister, selectedModel, scopedUid());
    HttpMgr::GetInstance()->PostHttpReq(
        agentApiUrl(QStringLiteral("/ai/model/api/register")), payload, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::deleteApiProvider(const QString& providerId, const QString& modelName)
{
    ensureUserScope();
    const QString trimmedProviderId = providerId.trimmed();
    const QString trimmedModelName = modelName.trimmed();
    if (trimmedProviderId.isEmpty() || !trimmedProviderId.startsWith(QStringLiteral("api-")) ||
                                                                     trimmedModelName.isEmpty())
    {
        setApiProviderBusy(false, "只能删除通过 API 接入的模型。");
        return;
    }

    clearErrorState();
    setApiProviderBusy(true, "正在删除 API 模型...");
    QJsonObject payload;
    payload["provider_id"] = trimmedProviderId;
    payload["model_name"] = trimmedModelName;

    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::ApiProviderDelete, trimmedModelName, scopedUid());
    HttpMgr::GetInstance()->PostHttpReq(
        agentApiUrl(QStringLiteral("/ai/model/api/delete")), payload, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::handleModelListRsp(ReqId id, const QString& res, ErrorCodes err)
{
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
    for (const auto& m : models)
    {
        QJsonObject model = m.toObject();
        const QString modelType = model["model_type"].toString().trimmed();
        const QString modelName = model["model_name"].toString().trimmed();
        if (modelType.isEmpty() || modelName.isEmpty())
        {
            continue;
        }
        const QString dedupeKey = modelType + QStringLiteral(":") + modelName;
        if (seenModels.contains(dedupeKey))
        {
            continue;
        }
        seenModels.insert(dedupeKey);
        if (modelType == _current_model_backend && modelName == _current_model_name)
        {
            currentModelAvailable = true;
        }
        _available_models.append(model);
    }
    if (!currentModelAvailable && _available_models.isEmpty())
    {
        _current_model_backend.clear();
        _current_model_name.clear();
        clearPersistedModelSelection();
        emit modelChanged();
    }
    else if (!currentModelAvailable && !_available_models.isEmpty())
    {
        QJsonObject fallback = root["default_model"].toObject();
        if (fallback.isEmpty())
        {
            fallback = QJsonObject::fromVariantMap(_available_models.first().toMap());
        }
        const QString backend = fallback["model_type"].toString();
        const QString modelName = fallback["model_name"].toString();
        if (!backend.isEmpty() && !modelName.isEmpty())
        {
            _current_model_backend = backend;
            _current_model_name = modelName;
            saveCurrentModelSelection();
            emit modelChanged();
        }
    }
    if (_thinking_enabled && !currentModelSupportsThinking())
    {
        _thinking_enabled = false;
    }
    emit modelsChanged();
    emit thinkingChanged();
}
