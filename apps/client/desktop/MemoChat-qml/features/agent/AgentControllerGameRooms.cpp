#include "AgentController.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include <QVariantList>
#include <QVariantMap>

namespace
{
QString gameBaseUrl()
{
    return gate_url_prefix + QStringLiteral("/ai/games");
}

QJsonObject hostConfigFromVariant(const QVariantMap& host)
{
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
} // namespace

void AgentController::loadGameRoom(const QString& roomId)
{
    const QString trimmedRoomId = roomId.trimmed();
    if (trimmedRoomId.isEmpty())
    {
        return;
    }
    QUrl url(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("uid"), QString::number(currentUid()));
    url.setQuery(query);
    if (_current_game_room_id != trimmedRoomId)
    {
        _current_game_room_id = trimmedRoomId;
        clearCurrentSession();
        emit gameStateChanged();
    }
    sendGameGet(url, QStringLiteral("room"), QStringLiteral("正在加载房间状态..."));
}

void AgentController::deleteGameRoom(const QString& roomId)
{
    const QString trimmedRoomId = roomId.trimmed();
    if (trimmedRoomId.isEmpty())
    {
        return;
    }
    QUrl url(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("uid"), QString::number(currentUid()));
    url.setQuery(query);
    _pendingDeleteGameRoomId = trimmedRoomId;
    sendGameDelete(url, QStringLiteral("delete_room"), QStringLiteral("正在清除游戏房间..."));
}

void AgentController::createGameRoom(const QString& title,
                                     const QString& rulesetId,
                                     const QVariantList& agents,
                                     const QVariantMap& host,
                                     const QString& displayName)
{
    const QString trimmedRuleset =
        rulesetId.trimmed().isEmpty() ? QStringLiteral("werewolf.basic") : rulesetId.trimmed();
    const QString trimmedDisplayName = displayName.trimmed();
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    if (!trimmedDisplayName.isEmpty())
    {
        payload[QStringLiteral("display_name")] = trimmedDisplayName;
    }
    payload[QStringLiteral("title")] = title.trimmed().isEmpty() ? QStringLiteral("MemoChat Game Room")
                                                                 : title.trimmed();
    payload[QStringLiteral("ruleset_id")] = trimmedRuleset;

    QJsonArray agentArray;
    for (const QVariant& agentVar : agents)
    {
        const QVariantMap agentMap = agentVar.toMap();
        const QString name = agentMap.value(QStringLiteral("display_name")).toString().trimmed();
        const QString modelType = agentMap.value(QStringLiteral("model_type")).toString().trimmed();
        const QString modelName = agentMap.value(QStringLiteral("model_name")).toString().trimmed();
        if (name.isEmpty() || modelType.isEmpty() || modelName.isEmpty())
        {
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
    const QString humanRoleKey = host.value(QStringLiteral("human_role_key")).toString().trimmed();
    if (!humanRoleKey.isEmpty())
    {
        config[QStringLiteral("human_role_key")] = humanRoleKey;
    }
    payload[QStringLiteral("config")] = config;

    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/rooms")),
                      payload,
                      QStringLiteral("create_room"), QStringLiteral("正在创建游戏房间..."));
}

void AgentController::createGameRoomFromTemplate(const QString& templateId,
                                                 const QString& title,
                                                 const QString& displayName)
{
    const QString trimmedTemplateId = templateId.trimmed();
    if (trimmedTemplateId.isEmpty())
    {
        return;
    }
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    const QString trimmedDisplayName = displayName.trimmed();
    if (!trimmedDisplayName.isEmpty())
    {
        payload[QStringLiteral("display_name")] = trimmedDisplayName;
    }
    payload[QStringLiteral("title")] = title.trimmed();

    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/templates/") + trimmedTemplateId + QStringLiteral("/rooms")),
                      payload, QStringLiteral("create_from_template"), QStringLiteral("正在从模板创建房间..."));
}

void AgentController::startGameRoom(const QString& roomId)
{
    const QString trimmedRoomId = roomId.trimmed();
    if (trimmedRoomId.isEmpty())
    {
        return;
    }
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId + QStringLiteral("/start")), payload,
                      QStringLiteral("start_room"), QStringLiteral("正在开始游戏..."));
}

void AgentController::restartGameRoom(const QString& roomId)
{
    const QString trimmedRoomId = roomId.trimmed();
    if (trimmedRoomId.isEmpty())
    {
        return;
    }
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId + QStringLiteral("/restart")), payload,
                      QStringLiteral("restart_room"), QStringLiteral("正在重新开始游戏..."));
}

void AgentController::tickGameRoom(const QString& roomId)
{
    const QString trimmedRoomId = roomId.trimmed();
    if (trimmedRoomId.isEmpty())
    {
        return;
    }
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId + QStringLiteral("/tick")), payload,
                      QStringLiteral("tick_room"), QStringLiteral("正在推进 Agent 回合..."));
}

void AgentController::autoTickGameRoom(const QString& roomId, int maxSteps)
{
    const QString trimmedRoomId = roomId.trimmed();
    if (trimmedRoomId.isEmpty())
    {
        return;
    }
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    payload[QStringLiteral("max_steps")] = qBound(1, maxSteps, 32);
    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId + QStringLiteral("/auto-tick")),
                      payload, QStringLiteral("auto_tick_room"), QStringLiteral("正在自动推进 Agent 回合..."));
}

void AgentController::submitGameAction(const QString& roomId,
                                       const QString& actorId,
                                       const QString& actionType,
                                       const QString& targetId,
                                       const QString& content)
{
    const QString trimmedRoomId = roomId.trimmed();
    if (trimmedRoomId.isEmpty())
    {
        return;
    }
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    payload[QStringLiteral("participant_id")] = actorId.trimmed();
    payload[QStringLiteral("action_type")] = actionType.trimmed().isEmpty() ? QStringLiteral("say")
                                                                            : actionType.trimmed();
    payload[QStringLiteral("target_participant_id")] = targetId.trimmed();
    payload[QStringLiteral("content")] = content.trimmed();

    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId + QStringLiteral("/actions")), payload,
                      QStringLiteral("action"), QStringLiteral("正在提交玩家行动..."));
}

void AgentController::updateGameParticipant(const QString& roomId,
                                            const QString& participantId,
                                            const QString& displayName,
                                            const QString& persona,
                                            const QString& strategy,
                                            const QString& skillName)
{
    const QString trimmedRoomId = roomId.trimmed();
    const QString trimmedParticipantId = participantId.trimmed();
    if (trimmedRoomId.isEmpty() || trimmedParticipantId.isEmpty())
    {
        return;
    }

    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    payload[QStringLiteral("display_name")] = displayName.trimmed();
    payload[QStringLiteral("persona")] = persona;
    payload[QStringLiteral("strategy")] = strategy.trimmed();
    payload[QStringLiteral("skill_name")] = skillName.trimmed();

    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId +
                                                     QStringLiteral("/participants/") + trimmedParticipantId +
                                                                    QStringLiteral("/update")),
                                                     payload,
                                                     QStringLiteral("update_participant"),
                                                                    QStringLiteral("正在保存 Agent 设置..."));
}
