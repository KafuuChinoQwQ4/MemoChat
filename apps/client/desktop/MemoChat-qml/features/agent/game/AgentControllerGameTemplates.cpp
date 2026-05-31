#include "AgentController.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

namespace
{
QString gameBaseUrl()
{
    return gate_url_prefix + QStringLiteral("/ai/games");
}

QJsonValue sanitizedGameTemplateValue(const QJsonValue& value)
{
    static const QStringList secretKeys = {QStringLiteral("api_key"),
        QStringLiteral("apikey"),
            QStringLiteral("apiKey"),
                           QStringLiteral("authorization"),
                                          QStringLiteral("Authorization"),
                                                         QStringLiteral("secret"),
                                                                        QStringLiteral("token"),
                                                                                       QStringLiteral("password") };

    if (value.isArray())
    {
        QJsonArray result;
        const QJsonArray source = value.toArray();
        for (const QJsonValue& item : source)
        {
            result.append(sanitizedGameTemplateValue(item));
        }
        return result;
    }
    if (!value.isObject())
    {
        return value;
    }

    QJsonObject result;
    const QJsonObject source = value.toObject();
    for (auto it = source.constBegin(); it != source.constEnd(); ++it)
    {
        if (secretKeys.contains(it.key()))
        {
            continue;
        }
        result.insert(it.key(), sanitizedGameTemplateValue(it.value()));
    }
    return result;
}

QJsonObject jsonObjectFromVariant(const QVariant& value)
{
    if (value.canConvert<QJsonObject>())
    {
        return qvariant_cast<QJsonObject>(value);
    }
    return QJsonObject::fromVariantMap(value.toMap());
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

void AgentController::listGameRulesets()
{
    sendGameGet(QUrl(gameBaseUrl() + QStringLiteral("/rulesets")),
                     QStringLiteral("rulesets"), QStringLiteral("正在加载游戏规则..."));
}

void AgentController::loadGameRolePresets(const QString& rulesetId)
{
    QUrl url(gameBaseUrl() + QStringLiteral("/role-presets"));
    QUrlQuery query;
    const QString trimmedRuleset = rulesetId.trimmed();
    if (!trimmedRuleset.isEmpty())
    {
        query.addQueryItem(QStringLiteral("ruleset_id"), trimmedRuleset);
    }
    url.setQuery(query);
    sendGameGet(url, QStringLiteral("role_presets"), QStringLiteral("正在加载角色预设..."));
}

void AgentController::listGameRooms()
{
    QUrl url(gameBaseUrl() + QStringLiteral("/rooms"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("uid"), QString::number(currentUid()));
    url.setQuery(query);
    sendGameGet(url, QStringLiteral("rooms"), QStringLiteral("正在加载游戏房间..."));
}

void AgentController::listGameTemplates()
{
    QUrl url(gameBaseUrl() + QStringLiteral("/templates"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("uid"), QString::number(currentUid()));
    url.setQuery(query);
    sendGameGet(url, QStringLiteral("templates"), QStringLiteral("正在加载游戏模板..."));
}

void AgentController::listGameTemplatePresets(const QString& rulesetId)
{
    QUrl url(gameBaseUrl() + QStringLiteral("/template-presets"));
    QUrlQuery query;
    const QString trimmedRuleset = rulesetId.trimmed();
    if (!trimmedRuleset.isEmpty())
    {
        query.addQueryItem(QStringLiteral("ruleset_id"), trimmedRuleset);
    }
    url.setQuery(query);
    sendGameGet(url, QStringLiteral("template_presets"), QStringLiteral("正在加载模板预设..."));
}

void AgentController::saveGameTemplate(const QString& title,
                                       const QString& description,
                                       const QString& rulesetId,
                                       const QVariantList& agents,
                                       const QVariantMap& host)
{
    const QString trimmedRuleset =
        rulesetId.trimmed().isEmpty() ? QStringLiteral("werewolf.basic") : rulesetId.trimmed();
    const QString trimmedTitle = title.trimmed().isEmpty() ? QStringLiteral("MemoChat Game Template") : title.trimmed();

    QJsonObject payload;
    payload[QStringLiteral("template_id")] = QString();
    payload[QStringLiteral("uid")] = currentUid();
    payload[QStringLiteral("title")] = trimmedTitle;
    payload[QStringLiteral("description")] = description.trimmed();
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
                      QStringLiteral("template"), QStringLiteral("正在保存游戏模板..."));
}

void AgentController::deleteGameTemplate(const QString& templateId)
{
    const QString trimmedTemplateId = templateId.trimmed();
    if (trimmedTemplateId.isEmpty())
    {
        return;
    }
    QUrl url(gameBaseUrl() + QStringLiteral("/templates/") + trimmedTemplateId);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("uid"), QString::number(currentUid()));
    url.setQuery(query);
    sendGameDelete(url, QStringLiteral("delete_template"), QStringLiteral("正在删除游戏模板..."));
}

void AgentController::cloneGameTemplatePreset(const QString& presetId, const QString& title)
{
    const QString trimmedPresetId = presetId.trimmed();
    if (trimmedPresetId.isEmpty())
    {
        return;
    }

    QJsonObject payload;
    payload[QStringLiteral("uid")] = currentUid();
    payload[QStringLiteral("title")] = title.trimmed();

    sendGamePost(QUrl(gameBaseUrl() +
                      QStringLiteral("/template-presets/") + trimmedPresetId + QStringLiteral("/clone")), payload,
                      QStringLiteral("clone_template_preset"), QStringLiteral("正在克隆模板预设..."));
}

QString AgentController::exportGameTemplate(const QString& templateId)
{
    const QString trimmedTemplateId = templateId.trimmed();
    if (trimmedTemplateId.isEmpty())
    {
        setGameError(QStringLiteral("请选择要导出的模板。"));
        return {};
    }

    QJsonObject templ;
    const auto findTemplate = [&trimmedTemplateId](const QVariantList& source) -> QJsonObject
    {
        for (const QVariant& itemVar : source)
        {
            const QJsonObject item = jsonObjectFromVariant(itemVar);
            const QString itemId =
                item.value(QStringLiteral("template_id")).toString(item.value(QStringLiteral("id")).toString());
            if (itemId == trimmedTemplateId)
            {
                return item;
            }
        }
        return {};
    };

    templ = findTemplate(_game_templates);
    if (templ.isEmpty())
    {
        templ = findTemplate(_game_template_presets);
    }
    if (templ.isEmpty())
    {
        setGameError(QStringLiteral("未找到要导出的模板。"));
        return {};
    }

    QJsonObject exported = sanitizedGameTemplateValue(templ).toObject();
    exported[QStringLiteral("uid")] = currentUid();
    const QJsonDocument doc(exported);
    setGameBusy(false, QStringLiteral("模板 JSON 已导出。"));
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

bool AgentController::importGameTemplate(const QString& templateJson)
{
    const QString trimmedJson = templateJson.trimmed();
    if (trimmedJson.isEmpty())
    {
        setGameError(QStringLiteral("请输入模板 JSON。"));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(trimmedJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        setGameError(QStringLiteral("模板 JSON 格式错误: %1").arg(parseError.errorString()));
        return false;
    }

    QJsonObject payload = doc.object();
    if (!payload.value(QStringLiteral("agents")).isArray())
    {
        setGameError(QStringLiteral("模板 JSON 缺少 agents 数组。"));
        return false;
    }
    payload[QStringLiteral("uid")] = currentUid();
    payload[QStringLiteral("template_id")] = payload.value(QStringLiteral("template_id")).toString();
    if (payload.value(QStringLiteral("title")).toString().trimmed().isEmpty())
    {
        payload[QStringLiteral("title")] = QStringLiteral("Imported Game Template");
    }
    if (payload.value(QStringLiteral("ruleset_id")).toString().trimmed().isEmpty())
    {
        payload[QStringLiteral("ruleset_id")] = QStringLiteral("werewolf.basic");
    }
    if (!payload.value(QStringLiteral("config")).isObject())
    {
        payload[QStringLiteral("config")] = QJsonObject();
    }
    if (!payload.value(QStringLiteral("metadata")).isObject())
    {
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
                      QStringLiteral("template"), QStringLiteral("正在导入游戏模板..."));
    return true;
}
