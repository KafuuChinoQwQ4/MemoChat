#include "AgentController.h"

#include "AgentNetworkRequestUtils.h"
#include "ClientGateway.h"
#include "global.h"
#include "httpmgr.h"
#include "usermgr.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

namespace
{
QString aiHttpModule()
{
    return QStringLiteral("ai");
}
} // namespace

void AgentController::setMemoryBusy(bool busy, const QString& statusText)
{
    bool changed = _memory_busy != busy || _memory_status_text != statusText;
    _memory_busy = busy;
    _memory_status_text = statusText;
    if (changed)
    {
        emit memoryStateChanged();
    }
}

void AgentController::setMemoryError(const QString& error)
{
    if (_memory_error == error)
    {
        return;
    }
    _memory_error = error;
    emit memoryStateChanged();
}

void AgentController::clearMemoryError()
{
    if (_memory_error.isEmpty())
    {
        return;
    }
    _memory_error.clear();
    emit memoryStateChanged();
}

void AgentController::listMemories()
{
    ensureUserScope();
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearMemoryError();
    setMemoryBusy(true, "正在加载记忆...");
    QUrl url = agentApiUrl(QStringLiteral("/ai/memory/list"));
    QUrlQuery query;
    url.setQuery(query);

    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::MemoryList, QString(), uid);
    HttpMgr::GetInstance()->GetHttpReq(url, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::createMemory(const QString& content)
{
    const QString trimmed = content.trimmed();
    if (trimmed.isEmpty())
    {
        return;
    }
    ensureUserScope();
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearMemoryError();
    setMemoryBusy(true, "正在保存记忆...");

    QJsonObject payload;
    payload["content"] = trimmed;

    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::MemoryCreate, QString(), uid);
    HttpMgr::GetInstance()->PostHttpReq(
        agentApiUrl(QStringLiteral("/ai/memory")), payload, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::deleteMemory(const QString& memoryId)
{
    const QString trimmed = memoryId.trimmed();
    if (trimmed.isEmpty())
    {
        return;
    }
    ensureUserScope();
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearMemoryError();
    setMemoryBusy(true, "正在删除记忆...");

    QJsonObject payload;
    payload["memory_id"] = trimmed;

    ReqId reqId = nextAgentHttpRequestId();
    _pending_requests.track(reqId, AgentRequestKind::MemoryDelete, QString(), uid);
    HttpMgr::GetInstance()->PostHttpReq(
        agentApiUrl(QStringLiteral("/ai/memory/delete")), payload, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::handleMemoryRsp(ReqId id, const QString& res, ErrorCodes err, AgentRequestKind kind)
{
    Q_UNUSED(id);
    Q_UNUSED(err);
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();

    if (kind == AgentRequestKind::MemoryList)
    {
        QJsonArray memories = root["memories"].toArray();
        _memories.clear();
        for (const auto& memory : memories)
        {
            if (memory.isObject())
            {
                _memories.append(memory.toObject());
            }
        }
        clearErrorState();
        clearMemoryError();
        setMemoryBusy(false,
                      _memories.isEmpty() ? "当前还没有可见记忆。"
                                          : QString("已加载 %1 条记忆。").arg(_memories.size()));
        emit memoriesChanged();
    }
    else if (kind == AgentRequestKind::MemoryCreate)
    {
        clearErrorState();
        clearMemoryError();
        setMemoryBusy(false, "记忆已保存，正在刷新...");
        listMemories();
    }
    else if (kind == AgentRequestKind::MemoryDelete)
    {
        clearErrorState();
        clearMemoryError();
        setMemoryBusy(false, "记忆已删除，正在刷新...");
        listMemories();
    }
}
