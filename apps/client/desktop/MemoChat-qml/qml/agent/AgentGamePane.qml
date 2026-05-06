pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root
    color: Qt.rgba(15, 24, 35, 0.24)

    property var agentController: null
    property var availableModels: []
    property string currentModel: ""
    property bool apiProviderBusy: false
    property string apiProviderStatus: ""
    property var gameRooms: []
    property var gameTemplates: agentController ? agentController.gameTemplates : []
    property var gameTemplatePresets: agentController ? agentController.gameTemplatePresets : []
    property var gameState: ({})
    property string currentGameRoomId: ""
    property int setupModeToken: 0
    property var gameRulesets: []
    property var gameRolePresets: []
    property bool gameBusy: false
    property string gameStatusText: ""
    property string gameError: ""
    property var draftAgents: []
    property color panelColor: Qt.rgba(0.96, 0.98, 1.0, 0.88)
    property color lineColor: Qt.rgba(1, 1, 1, 0.50)
    property bool setupMode: false
    property string setupModeRoomId: ""
    readonly property bool compactLayout: width < 980
    readonly property string selectedRoomId: roomIdFromState().length > 0 ? roomIdFromState() : currentGameRoomId
    readonly property bool hasCurrentRoom: !setupMode && selectedRoomId.length > 0

    signal closeRequested()

    function roomObject() {
        return gameState && gameState.room ? gameState.room : ({})
    }

    function roomIdFromState() {
        var room = roomObject()
        return room.room_id || room.id || ""
    }

    function gameStatusValue() {
        var room = roomObject()
        return (room.status || gameState.status || gameState.phase || "").toString().toLowerCase()
    }

    function isGameEnded() {
        var value = gameStatusValue()
        return value === "ended" || value === "finished" || value === "complete" || value === "completed"
    }

    function participants() {
        return gameState && gameState.participants ? gameState.participants : []
    }

    function events() {
        return gameState && gameState.events ? gameState.events : []
    }

    function availableActions() {
        return gameState && gameState.available_actions ? gameState.available_actions : []
    }

    function firstModel() {
        if (availableModels && availableModels.length > 0) {
            return availableModels[0]
        }
        return { "model_type": "ollama", "model_name": "qwen2.5:7b" }
    }

    function modelLabel(model) {
        if (!model) {
            return "ollama:qwen2.5:7b"
        }
        return (model.model_type || "") + ":" + (model.model_name || "")
    }

    function selectedRulesetId() {
        if (rulesetCombo.currentIndex >= 0 && gameRulesets && rulesetCombo.currentIndex < gameRulesets.length) {
            return gameRulesets[rulesetCombo.currentIndex].ruleset_id || gameRulesets[rulesetCombo.currentIndex].id || "werewolf.basic"
        }
        return "werewolf.basic"
    }

    function setRulesetComboById(rulesetId) {
        var target = rulesetId && rulesetId.length > 0 ? rulesetId : "werewolf.basic"
        if (gameRulesets && gameRulesets.length > 0) {
            for (var i = 0; i < gameRulesets.length; ++i) {
                var item = gameRulesets[i]
                var value = item.ruleset_id || item.id || ""
                if (value === target) {
                    rulesetCombo.currentIndex = i
                    if (agentController) {
                        agentController.loadGameRolePresets(target)
                        agentController.listGameTemplatePresets(target)
                    }
                    return
                }
            }
        }
        rulesetCombo.currentIndex = 0
    }

    function selectedRoleKey() {
        if (roleCombo.currentIndex > 0 && gameRolePresets && roleCombo.currentIndex - 1 < gameRolePresets.length) {
            var item = gameRolePresets[roleCombo.currentIndex - 1]
            return item.role_key || item.key || ""
        }
        return ""
    }

    function addDraftAgent(seed) {
        var model = firstModel()
        var next = draftAgents.slice()
        var item = seed || {}
        next.push({
            "display_name": item.display_name || item.name || ("Agent " + (next.length + 1)),
            "role_key": item.role_key || selectedRoleKey(),
            "environment": item.environment || "",
            "persona": item.persona || "",
            "skill_name": item.skill_name || "writer",
            "strategy": item.strategy || "react_reflection",
            "model_type": item.model_type || model.model_type || "ollama",
            "model_name": item.model_name || model.model_name || "qwen2.5:7b"
        })
        draftAgents = next
    }

    function removeDraftAgent(index) {
        var next = draftAgents.slice()
        next.splice(index, 1)
        draftAgents = next
    }

    function updateDraftAgent(index, key, value) {
        var next = draftAgents.slice()
        var source = next[index] || {}
        var item = {}
        for (var prop in source) {
            item[prop] = source[prop]
        }
        item[key] = value
        next[index] = item
        draftAgents = next
    }

    function normalizedAgent(agent) {
        var model = firstModel()
        var env = agent.environment || ""
        var persona = agent.persona || ""
        if (env.length > 0) {
            persona = "环境:\n" + env + (persona.length > 0 ? "\n\n角色:\n" + persona : "")
        }
        return {
            "display_name": agent.display_name || "Agent",
            "role_key": agent.role_key || "",
            "persona": persona,
            "skill_name": agent.skill_name || "writer",
            "strategy": agent.strategy || "react_reflection",
            "model_type": agent.model_type || model.model_type || "ollama",
            "model_name": agent.model_name || model.model_name || "qwen2.5:7b"
        }
    }

    function roomAgents() {
        var rows = []
        for (var i = 0; i < draftAgents.length; ++i) {
            rows.push(normalizedAgent(draftAgents[i] || {}))
        }
        return rows
    }

    function disabledHostConfig() {
        return {
            "enabled": false,
            "display_name": "",
            "persona": "",
            "model_type": "",
            "model_name": "",
            "skill_name": ""
        }
    }

    function roomIdAt(index) {
        if (!gameRooms || index < 0 || index >= gameRooms.length) {
            return ""
        }
        return gameRooms[index].room_id || gameRooms[index].id || ""
    }

    function templateAt(index) {
        if (!gameTemplates || index < 0 || index >= gameTemplates.length) {
            return ({})
        }
        return gameTemplates[index]
    }

    function presetAt(index) {
        if (!gameTemplatePresets || index < 0 || index >= gameTemplatePresets.length) {
            return ({})
        }
        return gameTemplatePresets[index]
    }

    function templateIdAt(index) {
        var item = templateAt(index)
        return item.template_id || item.id || ""
    }

    function presetIdAt(index) {
        var item = presetAt(index)
        return item.template_id || item.id || ""
    }

    function applyTemplateObject(item) {
        if (!item) {
            return
        }
        roomTitleField.text = item.title || roomTitleField.text
        templateDescriptionField.text = item.description || templateDescriptionField.text
        setRulesetComboById(item.ruleset_id || "werewolf.basic")

        var agents = item.agents || []
        var next = []
        for (var i = 0; i < agents.length; ++i) {
            var agent = agents[i] || {}
            next.push({
                "display_name": agent.display_name || agent.name || ("Agent " + (i + 1)),
                "role_key": agent.role_key || "",
                "environment": agent.environment || (agent.metadata && agent.metadata.environment ? agent.metadata.environment : ""),
                "persona": agent.persona || "",
                "skill_name": agent.skill_name || "writer",
                "strategy": agent.strategy || "react_reflection",
                "model_type": agent.model_type || firstModel().model_type || "ollama",
                "model_name": agent.model_name || firstModel().model_name || "qwen2.5:7b"
            })
        }
        draftAgents = next
    }

    function applyTemplate(index) {
        applyTemplateObject(templateAt(index))
    }

    function applyPreset(index) {
        applyTemplateObject(presetAt(index))
    }

    function parsePlainAgents(text) {
        var blocks = text.split(/\n\s*\n/)
        var rows = []
        for (var i = 0; i < blocks.length; ++i) {
            var block = blocks[i].trim()
            if (block.length === 0) {
                continue
            }
            var agent = {
                "display_name": "Agent " + (rows.length + 1),
                "role_key": "",
                "environment": "",
                "persona": block,
                "skill_name": "writer",
                "strategy": "react_reflection"
            }
            var lines = block.split(/\n/)
            var personaLines = []
            for (var j = 0; j < lines.length; ++j) {
                var line = lines[j].trim()
                var sep = line.indexOf(":")
                if (sep < 0) sep = line.indexOf("：")
                if (sep > 0) {
                    var key = line.slice(0, sep).trim().toLowerCase()
                    var value = line.slice(sep + 1).trim()
                    if (key === "name" || key === "名称" || key === "agent") agent.display_name = value
                    else if (key === "role" || key === "role_key" || key === "角色") agent.role_key = value
                    else if (key === "env" || key === "environment" || key === "环境") agent.environment = value
                    else if (key === "skill" || key === "skill_name" || key === "技能") agent.skill_name = value
                    else if (key === "strategy" || key === "策略") agent.strategy = value
                    else if (key === "persona" || key === "prompt" || key === "人设") personaLines.push(value)
                    else personaLines.push(line)
                } else {
                    personaLines.push(line)
                }
            }
            agent.persona = personaLines.join("\n").trim()
            rows.push(agent)
        }
        return rows
    }

    function importDraftTemplate() {
        var text = templateTextArea.text.trim()
        if (text.length === 0) {
            return
        }
        try {
            var parsed = JSON.parse(text)
            if (parsed.agents && parsed.agents.length !== undefined) {
                applyTemplateObject(parsed)
                return
            }
            if (parsed.length !== undefined) {
                applyTemplateObject({ "agents": parsed })
                return
            }
        } catch (e) {
        }
        draftAgents = parsePlainAgents(text)
    }

    function saveCurrentTemplate() {
        if (agentController) {
            agentController.saveGameTemplate(roomTitleField.text, templateDescriptionField.text, selectedRulesetId(), roomAgents(), disabledHostConfig())
        }
    }

    function importTemplateJson() {
        if (agentController) {
            agentController.importGameTemplate(templateTextArea.text)
        }
    }

    function createRoomFromSelectedTemplate() {
        var templateId = templateIdAt(templateCombo.currentIndex)
        if (agentController && templateId.length > 0) {
            agentController.createGameRoomFromTemplate(templateId, roomTitleField.text)
        }
    }

    function deleteSelectedTemplate() {
        var templateId = templateIdAt(templateCombo.currentIndex)
        if (agentController && templateId.length > 0) {
            agentController.deleteGameTemplate(templateId)
        }
    }

    function cloneSelectedPreset() {
        var presetId = presetIdAt(presetCombo.currentIndex)
        if (agentController && presetId.length > 0) {
            agentController.cloneGameTemplatePreset(presetId, "")
        }
    }

    function exportSelectedTemplate() {
        var templateId = templateIdAt(templateCombo.currentIndex)
        if (agentController && templateId.length > 0) {
            templateTextArea.text = agentController.exportGameTemplate(templateId)
        }
    }

    function exportSelectedPreset() {
        var presetId = presetIdAt(presetCombo.currentIndex)
        if (agentController && presetId.length > 0) {
            templateTextArea.text = agentController.exportGameTemplate(presetId)
        }
    }

    function createRoom() {
        if (agentController) {
            agentController.createGameRoom(roomTitleField.text, selectedRulesetId(), roomAgents(), disabledHostConfig())
        }
    }

    function actorIdAt(index) {
        var ps = participants()
        if (index < 0 || index >= ps.length) {
            return ""
        }
        return ps[index].participant_id || ps[index].id || ""
    }

    function actionTypeAt(index) {
        var acts = availableActions()
        if (index < 0 || index >= acts.length) {
            return "say"
        }
        if (typeof acts[index] === "string") {
            return acts[index]
        }
        return acts[index].action_type || acts[index].type || "say"
    }

    Component.onCompleted: {
        if (setupModeToken > 0) {
            setupMode = true
            setupModeRoomId = currentGameRoomId
        }
        if (agentController) {
            agentController.listGameRulesets()
            agentController.listGameRooms()
            agentController.listGameTemplates()
            agentController.listGameTemplatePresets(selectedRulesetId())
            agentController.refreshModelList()
        }
    }

    onSetupModeTokenChanged: {
        setupMode = true
        setupModeRoomId = currentGameRoomId
    }

    onCurrentGameRoomIdChanged: {
        if (setupMode && currentGameRoomId.length > 0 && currentGameRoomId !== setupModeRoomId) {
            setupMode = false
        }
    }

    onGameRulesetsChanged: {
        if (agentController) {
            agentController.loadGameRolePresets(selectedRulesetId())
            agentController.listGameTemplatePresets(selectedRulesetId())
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 8
        radius: 14
        color: root.panelColor
        border.color: root.lineColor
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: "A2A Game"
                        color: "#243145"
                        font.pixelSize: 20
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.gameError.length > 0 ? root.gameError : (root.gameStatusText.length > 0 ? root.gameStatusText : "自定义 Agent，创建房间，然后像群聊一样推进。")
                        color: root.gameError.length > 0 ? "#b64a4a" : "#65758b"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }

                GlassButton {
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 32
                    text: "关闭"
                    textPixelSize: 12
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.20)
                    hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.30)
                    onClicked: root.closeRequested()
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: root.compactLayout ? 1 : 2
                columnSpacing: 12
                rowSpacing: 12

                Rectangle {
                    visible: !root.hasCurrentRoom
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: root.compactLayout || !root.hasCurrentRoom ? parent.width : 430
                    Layout.maximumWidth: root.compactLayout || !root.hasCurrentRoom ? parent.width : 500
                    Layout.minimumHeight: root.compactLayout || !root.hasCurrentRoom ? 540 : 0
                    Layout.preferredHeight: root.compactLayout || !root.hasCurrentRoom ? 620 : 0
                    radius: 10
                    color: Qt.rgba(1, 1, 1, 0.34)
                    border.color: Qt.rgba(1, 1, 1, 0.42)
                    clip: true

                    ScrollView {
                        id: setupScroll
                        anchors.fill: parent
                        anchors.margins: 12
                        clip: true
                        contentWidth: setupScroll.availableWidth

                        ColumnLayout {
                            width: Math.max(0, setupScroll.availableWidth)
                            spacing: 10

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                TextField {
                                    id: roomTitleField
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 34
                                    text: "A2A Room"
                                    placeholderText: "房间标题"
                                    selectByMouse: true
                                }

                                ComboBox {
                                    id: rulesetCombo
                                    Layout.preferredWidth: 150
                                    Layout.preferredHeight: 34
                                    textRole: "label"
                                    valueRole: "value"
                                    model: {
                                        var rows = []
                                        if (root.gameRulesets && root.gameRulesets.length > 0) {
                                            for (var i = 0; i < root.gameRulesets.length; ++i) {
                                                var item = root.gameRulesets[i]
                                                rows.push({ "label": item.display_name || item.name || item.title || item.ruleset_id || "规则集", "value": item.ruleset_id || item.id || "" })
                                            }
                                        } else {
                                            rows.push({ "label": "Werewolf", "value": "werewolf.basic" })
                                        }
                                        return rows
                                    }
                                    onActivated: {
                                        if (root.agentController) {
                                            root.agentController.loadGameRolePresets(root.selectedRulesetId())
                                            root.agentController.listGameTemplatePresets(root.selectedRulesetId())
                                        }
                                    }
                                }
                            }

                            ComboBox {
                                id: roomCombo
                                Layout.fillWidth: true
                                Layout.preferredHeight: 34
                                textRole: "label"
                                valueRole: "value"
                                model: {
                                    var rows = []
                                    if (root.gameRooms) {
                                        for (var i = 0; i < root.gameRooms.length; ++i) {
                                            var room = root.gameRooms[i]
                                            rows.push({ "label": (room.title || room.name || "Game Room") + " · " + (room.status || "draft"), "value": room.room_id || room.id || "" })
                                        }
                                    }
                                    if (rows.length === 0) rows.push({ "label": "暂无房间", "value": "" })
                                    return rows
                                }
                                onActivated: {
                                    var roomId = root.roomIdAt(currentIndex)
                                    if (root.agentController && roomId.length > 0) {
                                        root.agentController.loadGameRoom(roomId)
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                ComboBox {
                                    id: templateCombo
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    textRole: "label"
                                    valueRole: "value"
                                    model: {
                                        var rows = []
                                        if (root.gameTemplates) {
                                            for (var i = 0; i < root.gameTemplates.length; ++i) {
                                                var item = root.gameTemplates[i]
                                                rows.push({ "label": item.title || item.template_id || "Template", "value": item.template_id || item.id || "" })
                                            }
                                        }
                                        if (rows.length === 0) rows.push({ "label": "暂无模板", "value": "" })
                                        return rows
                                    }
                                    onActivated: root.applyTemplate(currentIndex)
                                }

                                GlassButton {
                                    Layout.preferredWidth: 52
                                    Layout.preferredHeight: 32
                                    text: "套用"
                                    textPixelSize: 11
                                    cornerRadius: 7
                                    enabled: root.templateIdAt(templateCombo.currentIndex).length > 0
                                    normalColor: Qt.rgba(0.33, 0.56, 0.84, 0.18)
                                    hoverColor: Qt.rgba(0.33, 0.56, 0.84, 0.28)
                                    onClicked: root.applyTemplate(templateCombo.currentIndex)
                                }

                                GlassButton {
                                    Layout.preferredWidth: 52
                                    Layout.preferredHeight: 32
                                    text: "导出"
                                    textPixelSize: 11
                                    cornerRadius: 7
                                    enabled: root.templateIdAt(templateCombo.currentIndex).length > 0
                                    normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.18)
                                    hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.28)
                                    onClicked: root.exportSelectedTemplate()
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                ComboBox {
                                    id: presetCombo
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    textRole: "label"
                                    valueRole: "value"
                                    model: {
                                        var rows = []
                                        if (root.gameTemplatePresets) {
                                            for (var i = 0; i < root.gameTemplatePresets.length; ++i) {
                                                var item = root.gameTemplatePresets[i]
                                                rows.push({ "label": item.title || item.template_id || "Preset", "value": item.template_id || item.id || "" })
                                            }
                                        }
                                        if (rows.length === 0) rows.push({ "label": "暂无预设", "value": "" })
                                        return rows
                                    }
                                    onActivated: root.applyPreset(currentIndex)
                                }

                                GlassButton {
                                    Layout.preferredWidth: 52
                                    Layout.preferredHeight: 32
                                    text: "套用"
                                    textPixelSize: 11
                                    cornerRadius: 7
                                    enabled: root.presetIdAt(presetCombo.currentIndex).length > 0
                                    normalColor: Qt.rgba(0.33, 0.56, 0.84, 0.18)
                                    hoverColor: Qt.rgba(0.33, 0.56, 0.84, 0.28)
                                    onClicked: root.applyPreset(presetCombo.currentIndex)
                                }

                                GlassButton {
                                    Layout.preferredWidth: 52
                                    Layout.preferredHeight: 32
                                    text: "克隆"
                                    textPixelSize: 11
                                    cornerRadius: 7
                                    enabled: root.presetIdAt(presetCombo.currentIndex).length > 0
                                    normalColor: Qt.rgba(0.30, 0.58, 0.36, 0.18)
                                    hoverColor: Qt.rgba(0.30, 0.58, 0.36, 0.28)
                                    onClicked: root.cloneSelectedPreset()
                                }
                            }

                            TextField {
                                id: templateDescriptionField
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                placeholderText: "模板描述，可留空"
                                selectByMouse: true
                            }

                            TextArea {
                                id: templateTextArea
                                Layout.fillWidth: true
                                Layout.preferredHeight: 92
                                placeholderText: "粘贴 JSON 或普通文字模板"
                                wrapMode: TextArea.Wrap
                                selectByMouse: true
                                font.pixelSize: 12
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                GlassButton {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    text: "导入草稿"
                                    textPixelSize: 12
                                    cornerRadius: 8
                                    enabled: templateTextArea.text.trim().length > 0
                                    normalColor: Qt.rgba(0.33, 0.56, 0.84, 0.20)
                                    hoverColor: Qt.rgba(0.33, 0.56, 0.84, 0.30)
                                    onClicked: root.importDraftTemplate()
                                }

                                GlassButton {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    text: "保存模板"
                                    textPixelSize: 12
                                    cornerRadius: 8
                                    enabled: !root.gameBusy && root.draftAgents.length > 0
                                    normalColor: Qt.rgba(0.30, 0.58, 0.36, 0.20)
                                    hoverColor: Qt.rgba(0.30, 0.58, 0.36, 0.30)
                                    onClicked: root.saveCurrentTemplate()
                                }

                                GlassButton {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    text: "JSON入库"
                                    textPixelSize: 12
                                    cornerRadius: 8
                                    enabled: templateTextArea.text.trim().length > 0
                                    normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.18)
                                    hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.28)
                                    onClicked: root.importTemplateJson()
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 142
                                radius: 8
                                color: Qt.rgba(1, 1, 1, 0.28)
                                border.color: Qt.rgba(1, 1, 1, 0.36)

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 7

                                    Label {
                                        Layout.fillWidth: true
                                        text: "API"
                                        color: "#243145"
                                        font.pixelSize: 13
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }

                                    TextField { id: apiProviderNameField; Layout.fillWidth: true; Layout.preferredHeight: 28; text: "game-api"; placeholderText: "名称"; selectByMouse: true }
                                    TextField { id: apiBaseUrlField; Layout.fillWidth: true; Layout.preferredHeight: 28; text: "https://api.openai.com/v1"; placeholderText: "API 地址"; selectByMouse: true }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        TextField { id: apiKeyField; Layout.fillWidth: true; Layout.preferredHeight: 28; placeholderText: "API Key"; echoMode: TextInput.Password; selectByMouse: true }

                                        GlassButton {
                                            Layout.preferredWidth: 64
                                            Layout.preferredHeight: 28
                                            text: root.apiProviderBusy ? "解析" : "接入"
                                            textPixelSize: 11
                                            cornerRadius: 7
                                            enabled: !root.apiProviderBusy
                                            normalColor: Qt.rgba(0.33, 0.56, 0.84, 0.22)
                                            hoverColor: Qt.rgba(0.33, 0.56, 0.84, 0.32)
                                            onClicked: if (root.agentController) root.agentController.registerApiProvider(apiProviderNameField.text, apiBaseUrlField.text, apiKeyField.text)
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                ComboBox {
                                    id: roleCombo
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    textRole: "label"
                                    valueRole: "value"
                                    model: {
                                        var rows = [{ "label": "自定义角色", "value": "" }]
                                        if (root.gameRolePresets) {
                                            for (var i = 0; i < root.gameRolePresets.length; ++i) {
                                                var item = root.gameRolePresets[i]
                                                rows.push({ "label": item.display_name || item.name || item.role_key || "角色", "value": item.role_key || item.key || "" })
                                            }
                                        }
                                        return rows
                                    }
                                }

                                GlassButton {
                                    Layout.preferredWidth: 44
                                    Layout.preferredHeight: 32
                                    text: "+"
                                    textPixelSize: 18
                                    cornerRadius: 8
                                    normalColor: Qt.rgba(0.33, 0.56, 0.84, 0.22)
                                    hoverColor: Qt.rgba(0.33, 0.56, 0.84, 0.32)
                                    onClicked: root.addDraftAgent()
                                }
                            }

                            Repeater {
                                model: root.draftAgents

                                delegate: AgentGameAgentCard {
                                    required property var modelData
                                    required property int index

                                    agentData: modelData
                                    agentIndex: index
                                    availableModels: root.availableModels
                                    onFieldChanged: function(agentIndex, key, value) {
                                        root.updateDraftAgent(agentIndex, key, value)
                                    }
                                    onRemoveRequested: function(agentIndex) {
                                        root.removeDraftAgent(agentIndex)
                                    }
                                }
                            }

                            GlassButton {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                text: root.gameBusy ? "处理中" : "创建房间"
                                textPixelSize: 13
                                cornerRadius: 9
                                enabled: !root.gameBusy && root.draftAgents.length > 0
                                normalColor: Qt.rgba(0.24, 0.54, 0.72, 0.28)
                                hoverColor: Qt.rgba(0.24, 0.54, 0.72, 0.38)
                                pressedColor: Qt.rgba(0.24, 0.54, 0.72, 0.46)
                                onClicked: root.createRoom()
                            }
                        }
                    }
                }

                AgentGamePlayPane {
                    visible: root.hasCurrentRoom
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredHeight: root.compactLayout ? 620 : 0
                    participants: root.participants()
                    events: root.events()
                    availableActions: root.availableActions()
                    gameBusy: root.gameBusy
                    roomId: root.selectedRoomId
                    roomLabel: root.selectedRoomId.length > 0 ? root.selectedRoomId : "未选择房间"
                    roomEnded: root.isGameEnded()
                    onRefreshRequested: if (root.agentController) root.agentController.listGameRooms()
                    onStartRequested: if (root.agentController) root.agentController.startGameRoom(root.selectedRoomId)
                    onRestartRequested: if (root.agentController) root.agentController.restartGameRoom(root.selectedRoomId)
                    onTickRequested: if (root.agentController) root.agentController.tickGameRoom(root.selectedRoomId)
                    onAutoTickRequested: if (root.agentController) root.agentController.autoTickGameRoom(root.selectedRoomId, 8)
                    onSubmitActionRequested: function(actorId, actionType, targetId, content) {
                        if (root.agentController) {
                            root.agentController.submitGameAction(root.selectedRoomId, actorId, actionType, targetId, content)
                        }
                    }
                }
            }
        }
    }
}
