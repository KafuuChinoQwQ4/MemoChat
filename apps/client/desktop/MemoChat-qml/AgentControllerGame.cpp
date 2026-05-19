#include "AgentController.h"
#include "ClientGateway.h"
#include "global.h"
#include "usermgr.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QStringList>
#include <QtGlobal>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

namespace {
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


void applyLocalGateRequestOptions(QNetworkRequest& request) {
    const QUrl url = request.url();
    const QString scheme = url.scheme().toLower();
    if (scheme == QLatin1String("https")) {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
        request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
#endif
    }
}
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

void AgentController::sendGameGet(const QUrl& url, const QString& op, const QString& statusText) {
    clearGameError();
    setGameBusy(true, statusText);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    applyLocalGateRequestOptions(request);
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
    applyLocalGateRequestOptions(request);
    QNetworkReply* reply = _gameNetwork->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, op]() {
        const QByteArray body = reply->readAll();
        const QString networkError = reply->error() == QNetworkReply::NoError
            ? QString()
            : reply->errorString();
        reply->deleteLater();

        if (!networkError.isEmpty()) {
            if (op == QStringLiteral("delete_room")) {
                _pendingDeleteGameRoomId.clear();
            }
            setGameBusy(false, QStringLiteral("请求失败"));
            setGameError(QStringLiteral("Game 服务请求失败: %1").arg(networkError));
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            if (op == QStringLiteral("delete_room")) {
                _pendingDeleteGameRoomId.clear();
            }
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
    applyLocalGateRequestOptions(request);
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
        clearCurrentSession();
        emit gameStateChanged();
    }
    sendGameGet(url, QStringLiteral("room"), QStringLiteral("正在加载房间状态..."));
}

void AgentController::deleteGameRoom(const QString& roomId) {
    const QString trimmedRoomId = roomId.trimmed();
    if (trimmedRoomId.isEmpty()) {
        return;
    }
    QUrl url(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("uid"), QString::number(currentUid()));
    url.setQuery(query);
    _pendingDeleteGameRoomId = trimmedRoomId;
    sendGameDelete(url, QStringLiteral("delete_room"), QStringLiteral("正在清除游戏房间..."));
}

void AgentController::createGameRoom(const QString& title, const QString& rulesetId, const QVariantList& agents, const QVariantMap& host, const QString& displayName) {
    const QString trimmedRuleset = rulesetId.trimmed().isEmpty() ? QStringLiteral("werewolf.basic") : rulesetId.trimmed();
    const QString trimmedDisplayName = displayName.trimmed();
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    if (!trimmedDisplayName.isEmpty()) {
        payload[QStringLiteral("display_name")] = trimmedDisplayName;
    }
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
    const QString humanRoleKey = host.value(QStringLiteral("human_role_key")).toString().trimmed();
    if (!humanRoleKey.isEmpty()) {
        config[QStringLiteral("human_role_key")] = humanRoleKey;
    }
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

void AgentController::createGameRoomFromTemplate(const QString& templateId, const QString& title, const QString& displayName) {
    const QString trimmedTemplateId = templateId.trimmed();
    if (trimmedTemplateId.isEmpty()) {
        return;
    }
    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    const QString trimmedDisplayName = displayName.trimmed();
    if (!trimmedDisplayName.isEmpty()) {
        payload[QStringLiteral("display_name")] = trimmedDisplayName;
    }
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

void AgentController::updateGameParticipant(const QString& roomId,
                                            const QString& participantId,
                                            const QString& displayName,
                                            const QString& persona,
                                            const QString& strategy,
                                            const QString& skillName) {
    const QString trimmedRoomId = roomId.trimmed();
    const QString trimmedParticipantId = participantId.trimmed();
    if (trimmedRoomId.isEmpty() || trimmedParticipantId.isEmpty()) {
        return;
    }

    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    payload[QStringLiteral("display_name")] = displayName.trimmed();
    payload[QStringLiteral("persona")] = persona;
    payload[QStringLiteral("strategy")] = strategy.trimmed();
    payload[QStringLiteral("skill_name")] = skillName.trimmed();

    sendGamePost(QUrl(gameBaseUrl() + QStringLiteral("/rooms/") + trimmedRoomId
                      + QStringLiteral("/participants/") + trimmedParticipantId
                      + QStringLiteral("/update")),
                 payload,
                 QStringLiteral("update_participant"),
                 QStringLiteral("正在保存 Agent 设置..."));
}

void AgentController::handleGameResponse(const QString& op, const QJsonObject& root) {
    const int code = root.contains(QStringLiteral("error"))
        ? root.value(QStringLiteral("error")).toInt()
        : root.value(QStringLiteral("code")).toInt();
    if (code != 0) {
        const QString message = root.value(QStringLiteral("message")).toString(QStringLiteral("未知错误"));
        if (op == QStringLiteral("delete_room")) {
            _pendingDeleteGameRoomId.clear();
        }
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
    } else if (op == QStringLiteral("delete_room")) {
        const QString deletedRoomId = _pendingDeleteGameRoomId;
        _pendingDeleteGameRoomId.clear();
        if (!deletedRoomId.isEmpty()) {
            QVariantList remainingRooms;
            for (const QVariant& roomVar : _game_rooms) {
                const QJsonObject room = jsonObjectFromVariant(roomVar);
                const QString roomId = room.value(QStringLiteral("room_id")).toString(
                    room.value(QStringLiteral("id")).toString());
                if (roomId != deletedRoomId) {
                    remainingRooms.append(roomVar);
                }
            }
            _game_rooms = remainingRooms;
            emit gameRoomsChanged();
            if (_current_game_room_id == deletedRoomId) {
                _current_game_room_id.clear();
                _game_state.clear();
                emit gameStateChanged();
            }
        }
        statusText = QStringLiteral("房间已清除，正在刷新列表...");
        refreshRooms = true;
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
            clearCurrentSession();
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
            const QString rulesetId = room.value(QStringLiteral("ruleset_id")).toString();
            if (rulesetId == QStringLiteral("multi_ai_chat.test") && !roomId.isEmpty()) {
                autoTickGameRoom(roomId, 8);
                return;
            }
        } else if (op == QStringLiteral("update_participant")) {
            statusText = QStringLiteral("Agent 设置已更新。");
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
