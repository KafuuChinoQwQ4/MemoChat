#include "AgentController.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QVariant>
#include <QVariantList>

namespace
{
QJsonObject jsonObjectFromVariant(const QVariant& value)
{
    if (value.canConvert<QJsonObject>())
    {
        return qvariant_cast<QJsonObject>(value);
    }
    return QJsonObject::fromVariantMap(value.toMap());
}
} // namespace

void AgentController::handleGameResponse(const QString& op, const QJsonObject& root, int uid)
{
    if (uid != 0 && uid != currentUid())
    {
        return;
    }

    const int code = root.contains(QStringLiteral("error")) ? root.value(QStringLiteral("error")).toInt()
                                                            : root.value(QStringLiteral("code")).toInt();
    if (code != 0)
    {
        const QString message = root.value(QStringLiteral("message")).toString(QStringLiteral("未知错误"));
        if (op == QStringLiteral("delete_room"))
        {
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

    if (op == QStringLiteral("rulesets"))
    {
        _game_rulesets.clear();
        const QJsonArray rulesets = root.value(QStringLiteral("rulesets")).toArray();
        for (const QJsonValue& ruleset : rulesets)
        {
            if (ruleset.isObject())
            {
                _game_rulesets.append(ruleset.toObject());
            }
        }
        emit gameRulesetsChanged();
        statusText = _game_rulesets.isEmpty() ? QStringLiteral("没有可用规则集。")
                                              : QStringLiteral("已加载 %1 个规则集。").arg(_game_rulesets.size());
    }
    else if (op == QStringLiteral("role_presets"))
    {
        _game_role_presets.clear();
        const QJsonArray presets = root.value(QStringLiteral("role_presets")).toArray();
        for (const QJsonValue& preset : presets)
        {
            if (preset.isObject())
            {
                _game_role_presets.append(preset.toObject());
            }
        }
        emit gameRolePresetsChanged();
        statusText =
            _game_role_presets.isEmpty() ? QStringLiteral("当前规则没有角色预设。")
                                         : QStringLiteral("已加载 %1 个角色预设。").arg(_game_role_presets.size());
    }
    else if (op == QStringLiteral("rooms"))
    {
        _game_rooms.clear();
        const QJsonArray rooms = root.value(QStringLiteral("rooms")).toArray();
        for (const QJsonValue& room : rooms)
        {
            if (room.isObject())
            {
                _game_rooms.append(room.toObject());
            }
        }
        emit gameRoomsChanged();
        statusText = _game_rooms.isEmpty() ? QStringLiteral("当前还没有游戏房间。")
                                           : QStringLiteral("已加载 %1 个游戏房间。").arg(_game_rooms.size());
    }
    else if (op == QStringLiteral("templates"))
    {
        _game_templates.clear();
        const QJsonArray templates = root.value(QStringLiteral("templates")).toArray();
        for (const QJsonValue& templ : templates)
        {
            if (templ.isObject())
            {
                _game_templates.append(templ.toObject());
            }
        }
        emit gameTemplatesChanged();
        statusText = _game_templates.isEmpty() ? QStringLiteral("当前还没有游戏模板。")
                                               : QStringLiteral("已加载 %1 个游戏模板。").arg(_game_templates.size());
    }
    else if (op == QStringLiteral("template_presets"))
    {
        _game_template_presets.clear();
        const QJsonArray presets = root.value(QStringLiteral("template_presets")).toArray();
        for (const QJsonValue& preset : presets)
        {
            if (preset.isObject())
            {
                _game_template_presets.append(preset.toObject());
            }
        }
        emit gameTemplatePresetsChanged();
        statusText = _game_template_presets.isEmpty()
            ? QStringLiteral("当前规则没有模板预设。")
            : QStringLiteral("已加载 %1 个模板预设。").arg(_game_template_presets.size());
    }
    else if (op == QStringLiteral("template"))
    {
        statusText = QStringLiteral("模板已保存，正在刷新列表...");
        const QJsonObject templ = root.value(QStringLiteral("template")).toObject();
        if (!templ.isEmpty())
        {
            _game_templates.append(templ);
            emit gameTemplatesChanged();
        }
        refreshTemplates = true;
    }
    else if (op == QStringLiteral("clone_template_preset"))
    {
        statusText = QStringLiteral("预设已克隆，正在刷新模板...");
        const QJsonObject templ = root.value(QStringLiteral("template")).toObject();
        if (!templ.isEmpty())
        {
            _game_templates.append(templ);
            emit gameTemplatesChanged();
        }
        refreshTemplates = true;
    }
    else if (op == QStringLiteral("delete_template"))
    {
        statusText = QStringLiteral("模板已删除，正在刷新列表...");
        refreshTemplates = true;
    }
    else if (op == QStringLiteral("delete_room"))
    {
        const QString deletedRoomId = _pendingDeleteGameRoomId;
        _pendingDeleteGameRoomId.clear();
        if (!deletedRoomId.isEmpty())
        {
            QVariantList remainingRooms;
            for (const QVariant& roomVar : _game_rooms)
            {
                const QJsonObject room = jsonObjectFromVariant(roomVar);
                const QString roomId =
                    room.value(QStringLiteral("room_id")).toString(room.value(QStringLiteral("id")).toString());
                if (roomId != deletedRoomId)
                {
                    remainingRooms.append(roomVar);
                }
            }
            _game_rooms = remainingRooms;
            emit gameRoomsChanged();
            if (_current_game_room_id == deletedRoomId)
            {
                _current_game_room_id.clear();
                _game_state.clear();
                emit gameStateChanged();
            }
        }
        statusText = QStringLiteral("房间已清除，正在刷新列表...");
        refreshRooms = true;
    }

    QJsonObject stateObject = root.value(QStringLiteral("state")).toObject();
    if (stateObject.isEmpty() && root.value(QStringLiteral("room")).isObject())
    {
        stateObject[QStringLiteral("room")] = root.value(QStringLiteral("room")).toObject();
    }
    if (!stateObject.isEmpty())
    {
        _game_state = stateObject.toVariantMap();
        const QJsonObject room = stateObject.value(QStringLiteral("room")).toObject();
        const QString roomId =
            room.value(QStringLiteral("room_id")).toString(room.value(QStringLiteral("id")).toString());
        if (!roomId.isEmpty())
        {
            _current_game_room_id = roomId;
            clearCurrentSession();
        }
        emit gameStateChanged();
        if (op == QStringLiteral("create_room"))
        {
            statusText = QStringLiteral("房间已创建。");
            refreshRooms = true;
        }
        else if (op == QStringLiteral("create_from_template"))
        {
            statusText = QStringLiteral("已从模板创建房间。");
            refreshRooms = true;
        }
        else if (op == QStringLiteral("start_room"))
        {
            statusText = QStringLiteral("游戏已开始。");
        }
        else if (op == QStringLiteral("restart_room"))
        {
            statusText = QStringLiteral("游戏已重新开始。");
            refreshRooms = true;
        }
        else if (op == QStringLiteral("tick_room"))
        {
            statusText = QStringLiteral("Agent 回合已推进。");
        }
        else if (op == QStringLiteral("auto_tick_room"))
        {
            const QJsonObject tick = root.value(QStringLiteral("tick")).toObject();
            const int steps = tick.value(QStringLiteral("steps")).toInt();
            const QString stopReason = tick.value(QStringLiteral("stop_reason")).toString();
            statusText = QStringLiteral("自动推进 %1 步，停止于 %2。").arg(steps).arg(stopReason.isEmpty()
                                                                                      ? QStringLiteral("idle")
                                                                                      : stopReason);
        }
        else if (op == QStringLiteral("action"))
        {
            statusText = QStringLiteral("玩家行动已提交。");
            const QString rulesetId = room.value(QStringLiteral("ruleset_id")).toString();
            if (rulesetId == QStringLiteral("multi_ai_chat.test") && !roomId.isEmpty())
            {
                autoTickGameRoom(roomId, 8);
                return;
            }
        }
        else if (op == QStringLiteral("update_participant"))
        {
            statusText = QStringLiteral("Agent 设置已更新。");
        }
    }

    setGameBusy(false, statusText);
    if (refreshRooms)
    {
        listGameRooms();
    }
    if (refreshTemplates)
    {
        listGameTemplates();
    }
}
