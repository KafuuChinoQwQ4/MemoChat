#include "AgentController.h"

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

void AgentController::setAgentTaskBusy(bool busy, const QString& statusText)
{
    bool changed = _agent_task_busy != busy || _agent_task_status_text != statusText;
    _agent_task_busy = busy;
    _agent_task_status_text = statusText;
    if (changed)
    {
        emit agentTaskStateChanged();
    }
}

void AgentController::setAgentTaskError(const QString& error)
{
    if (_agent_task_error == error)
    {
        return;
    }
    _agent_task_error = error;
    emit agentTaskStateChanged();
}

void AgentController::clearAgentTaskError()
{
    if (_agent_task_error.isEmpty())
    {
        return;
    }
    _agent_task_error.clear();
    emit agentTaskStateChanged();
}

void AgentController::listAgentTasks()
{
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
    HttpMgr::GetInstance()->GetHttpReq(url, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::createAgentTask(const QString& content, const QString& title)
{
    const QString trimmed = content.trimmed();
    if (trimmed.isEmpty())
    {
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
    if (!skillName.isEmpty())
    {
        payload["skill_name"] = skillName;
    }
    payload["metadata"] = buildChatMetadata();

    ReqId reqId = ID_AI_TASK_CREATE;
    _pending_requests[reqId] = "task_create";
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/ai/tasks"),
                                        payload,
                                        reqId,
                                        Modules::LOGINMOD,
                                        aiHttpModule());
}

void AgentController::cancelAgentTask(const QString& taskId)
{
    const QString trimmed = taskId.trimmed();
    if (trimmed.isEmpty())
    {
        return;
    }
    clearErrorState();
    clearAgentTaskError();
    setAgentTaskBusy(true, "正在取消后台任务...");

    QJsonObject payload;
    payload["task_id"] = trimmed;

    ReqId reqId = ID_AI_TASK_CANCEL;
    _pending_requests[reqId] = "task_cancel";
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/ai/tasks/cancel"),
                                        payload,
                                        reqId,
                                        Modules::LOGINMOD,
                                        aiHttpModule());
}

void AgentController::resumeAgentTask(const QString& taskId)
{
    const QString trimmed = taskId.trimmed();
    if (trimmed.isEmpty())
    {
        return;
    }
    clearErrorState();
    clearAgentTaskError();
    setAgentTaskBusy(true, "正在恢复后台任务...");

    QJsonObject payload;
    payload["task_id"] = trimmed;

    ReqId reqId = ID_AI_TASK_RESUME;
    _pending_requests[reqId] = "task_resume";
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/ai/tasks/resume"),
                                        payload,
                                        reqId,
                                        Modules::LOGINMOD,
                                        aiHttpModule());
}

void AgentController::handleAgentTaskRsp(ReqId id, const QString& res, ErrorCodes err, const QString& reqType)
{
    Q_UNUSED(id);
    Q_UNUSED(err);
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();

    if (reqType == "task_list")
    {
        QJsonArray tasks = root["tasks"].toArray();
        _agent_tasks.clear();
        for (const auto& task : tasks)
        {
            if (task.isObject())
            {
                _agent_tasks.append(task.toObject());
            }
        }
        clearErrorState();
        clearAgentTaskError();
        setAgentTaskBusy(false,
                         _agent_tasks.isEmpty() ? "当前还没有后台任务。"
                                                : QString("已加载 %1 个后台任务。").arg(_agent_tasks.size()));
        emit agentTasksChanged();
    }
    else if (reqType == "task_create")
    {
        clearErrorState();
        clearAgentTaskError();
        setAgentTaskBusy(false, "任务已创建，正在刷新...");
        listAgentTasks();
    }
    else if (reqType == "task_cancel")
    {
        clearErrorState();
        clearAgentTaskError();
        setAgentTaskBusy(false, "任务已取消，正在刷新...");
        listAgentTasks();
    }
    else if (reqType == "task_resume")
    {
        clearErrorState();
        clearAgentTaskError();
        setAgentTaskBusy(false, "任务已恢复，正在刷新...");
        listAgentTasks();
    }
}
