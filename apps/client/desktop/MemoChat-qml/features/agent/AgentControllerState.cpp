#include "AgentController.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>

namespace
{
constexpr auto kSettingsOrg = "MemoChat";
constexpr auto kSettingsApp = "MemoChatQml";
constexpr auto kModelBackendKey = "AI/CurrentModelBackend";
constexpr auto kModelNameKey = "AI/CurrentModelName";
} // namespace

AgentMessageModel* AgentController::model() const
{
    return _model;
}
QString AgentController::currentSessionId() const
{
    return _current_session_id;
}
QString AgentController::currentModel() const
{
    if (_current_model_backend.isEmpty() || _current_model_name.isEmpty())
    {
        return {};
    }
    return QString("%1:%2").arg(_current_model_backend, _current_model_name);
}
bool AgentController::loading() const
{
    return _loading;
}
QString AgentController::error() const
{
    return _error;
}
bool AgentController::streaming() const
{
    return _streaming;
}

QVariantList AgentController::sessions() const
{
    return _sessions;
}

QVariantList AgentController::availableModels() const
{
    return _available_models;
}
bool AgentController::modelRefreshBusy() const
{
    return _model_refresh_busy;
}
bool AgentController::apiProviderBusy() const
{
    return _api_provider_busy;
}
QString AgentController::apiProviderStatus() const
{
    return _api_provider_status;
}
bool AgentController::thinkingEnabled() const
{
    return _thinking_enabled;
}
bool AgentController::currentModelSupportsThinking() const
{
    return modelSupportsThinking(_current_model_backend, _current_model_name);
}
QVariantList AgentController::knowledgeBases() const
{
    return _knowledge_bases;
}
QString AgentController::knowledgeSearchResult() const
{
    return _knowledge_search_result;
}
bool AgentController::knowledgeBusy() const
{
    return _knowledge_busy;
}
QString AgentController::knowledgeStatusText() const
{
    return _knowledge_status_text;
}
QString AgentController::knowledgeError() const
{
    return _knowledge_error;
}
QVariantList AgentController::memories() const
{
    return _memories;
}
bool AgentController::memoryBusy() const
{
    return _memory_busy;
}
QString AgentController::memoryStatusText() const
{
    return _memory_status_text;
}
QString AgentController::memoryError() const
{
    return _memory_error;
}
QVariantList AgentController::agentTasks() const
{
    return _agent_tasks;
}
bool AgentController::agentTaskBusy() const
{
    return _agent_task_busy;
}
QString AgentController::agentTaskStatusText() const
{
    return _agent_task_status_text;
}
QString AgentController::agentTaskError() const
{
    return _agent_task_error;
}
QString AgentController::currentTraceId() const
{
    return _current_trace_id;
}
QString AgentController::currentSkill() const
{
    return _current_skill;
}
QString AgentController::currentFeedbackSummary() const
{
    return _current_feedback_summary;
}
QVariantList AgentController::traceEvents() const
{
    return _trace_events;
}
QVariantList AgentController::traceObservations() const
{
    return _trace_observations;
}
QString AgentController::agentSkillMode() const
{
    return _agent_skill_mode;
}
QString AgentController::agentSkillDisplay() const
{
    if (_agent_skill_mode == QStringLiteral("knowledge"))
        return QStringLiteral("知识库");
    if (_agent_skill_mode == QStringLiteral("research"))
        return QStringLiteral("联网搜索");
    if (_agent_skill_mode == QStringLiteral("graph"))
        return QStringLiteral("图谱关系");
    if (_agent_skill_mode == QStringLiteral("calculate"))
        return QStringLiteral("计算器");
    return QStringLiteral("自动");
}

QVariantList AgentController::gameRooms() const
{
    return _game_rooms;
}
QVariantList AgentController::gameTemplates() const
{
    return _game_templates;
}
QVariantList AgentController::gameTemplatePresets() const
{
    return _game_template_presets;
}
QVariantMap AgentController::gameState() const
{
    return _game_state;
}
QString AgentController::currentGameRoomId() const
{
    return _current_game_room_id;
}
QVariantList AgentController::gameRulesets() const
{
    return _game_rulesets;
}
QVariantList AgentController::gameRolePresets() const
{
    return _game_role_presets;
}
bool AgentController::gameBusy() const
{
    return _game_busy;
}
QString AgentController::gameStatusText() const
{
    return _game_status_text;
}
QString AgentController::gameError() const
{
    return _game_error;
}

void AgentController::setErrorState(const QString& error)
{
    _error = error;
    emit errorOccurred(_error);
}

void AgentController::clearErrorState()
{
    if (_error.isEmpty())
    {
        return;
    }
    _error.clear();
    emit errorOccurred(QString());
}

void AgentController::setModelRefreshBusy(bool busy)
{
    if (_model_refresh_busy == busy)
    {
        return;
    }
    _model_refresh_busy = busy;
    emit modelStateChanged();
}

void AgentController::setApiProviderBusy(bool busy, const QString& statusText)
{
    if (_api_provider_busy == busy && _api_provider_status == statusText)
    {
        return;
    }
    _api_provider_busy = busy;
    _api_provider_status = statusText;
    emit modelStateChanged();
}

void AgentController::setThinkingEnabled(bool enabled)
{
    const bool next = enabled && currentModelSupportsThinking();
    if (_thinking_enabled != next)
    {
        _thinking_enabled = next;
        emit thinkingChanged();
    }
}

bool AgentController::modelSupportsThinking(const QString& backend, const QString& modelName) const
{
    for (const QVariant& modelVar : _available_models)
    {
        const QVariantMap model = modelVar.toMap();
        if (model.value(QStringLiteral("model_type")).toString() == backend &&
                        model.value(QStringLiteral("model_name")).toString() == modelName)
        {
            return model.value(QStringLiteral("supports_thinking")).toBool();
        }
    }
    const QString normalized = (backend + QStringLiteral(":") + modelName).toLower();
    return normalized.contains(QStringLiteral("qwen3")) ||
                               normalized.contains(QStringLiteral("deepseek-reasoner")) ||
                                                   normalized.contains(QStringLiteral("reasoner"));
}

QJsonObject AgentController::buildChatMetadata() const
{
    QJsonObject metadata;
    metadata[QStringLiteral("think")] = _thinking_enabled && currentModelSupportsThinking();
    const QString skillName = resolvedSkillName();
    if (!skillName.isEmpty())
    {
        metadata[QStringLiteral("skill_name")] = skillName;
    }
    const QJsonArray requestedTools = requestedToolsForSkillMode();
    if (!requestedTools.isEmpty())
    {
        metadata[QStringLiteral("requested_tools")] = requestedTools;
    }
    return metadata;
}

QString AgentController::resolvedSkillName() const
{
    if (_agent_skill_mode == QStringLiteral("knowledge"))
        return QStringLiteral("knowledge_copilot");
    if (_agent_skill_mode == QStringLiteral("research"))
        return QStringLiteral("research_assistant");
    if (_agent_skill_mode == QStringLiteral("graph"))
        return QStringLiteral("graph_recommender");
    return QString();
}

QJsonArray AgentController::requestedToolsForSkillMode() const
{
    QJsonArray tools;
    if (_agent_skill_mode == QStringLiteral("calculate"))
    {
        tools.append(QStringLiteral("calculator"));
    }
    return tools;
}

void AgentController::loadPersistedModelSelection()
{
    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    const QString backend = settings.value(QString::fromLatin1(kModelBackendKey)).toString().trimmed();
    const QString modelName = settings.value(QString::fromLatin1(kModelNameKey)).toString().trimmed();
    if (!backend.isEmpty() && !modelName.isEmpty())
    {
        _current_model_backend = backend;
        _current_model_name = modelName;
    }
}

void AgentController::saveCurrentModelSelection() const
{
    if (_current_model_backend.trimmed().isEmpty() || _current_model_name.trimmed().isEmpty())
    {
        return;
    }
    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    settings.setValue(QString::fromLatin1(kModelBackendKey), _current_model_backend);
    settings.setValue(QString::fromLatin1(kModelNameKey), _current_model_name);
}

void AgentController::clearPersistedModelSelection() const
{
    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    settings.remove(QString::fromLatin1(kModelBackendKey));
    settings.remove(QString::fromLatin1(kModelNameKey));
}
