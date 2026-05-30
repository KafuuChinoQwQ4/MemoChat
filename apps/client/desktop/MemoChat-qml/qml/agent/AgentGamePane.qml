pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"
import "AgentGameRuntime.js" as AgentGameRuntime

Rectangle {
    id: root
    color: Qt.rgba(15, 24, 35, 0.28)

    property var agentController: null
    property var availableModels: []
    property string currentModel: ""
    property bool apiProviderBusy: false
    property string apiProviderStatus: ""
    property var gameRooms: []
    property var gameTemplates: []
    property var gameTemplatePresets: []
    property var gameState: ({})
    property string currentGameRoomId: ""
    property int setupModeToken: 0
    property string setupKind: "multi"
    property var gameRulesets: []
    property var gameRolePresets: []
    property bool gameBusy: false
    property string gameStatusText: ""
    property string gameError: ""
    property string humanDisplayName: ""
    property var draftAgents: []
    property string initialRoomId: ""
    property bool setupReady: false

    readonly property string customHumanRoleValue: "__custom_human_role__"
    readonly property string testRulesetId: "multi_ai_chat.test"
    readonly property bool gameSetupMode: setupKind === "game"
    readonly property color panelColor: Qt.rgba(0.96, 0.98, 1.0, 0.92)
    readonly property color lineColor: Qt.rgba(1, 1, 1, 0.52)
    readonly property bool customHumanRoleSelected: root.gameSetupMode
        && roleCombo.currentIndex >= 0
        && root.gameRoleOptions()[roleCombo.currentIndex]
        && root.gameRoleOptions()[roleCombo.currentIndex].value === root.customHumanRoleValue

    signal closeRequested()

    function firstModel() {
        return AgentGameRuntime.firstModel(root.availableModels)
    }

    function modelLabel(model) {
        return AgentGameRuntime.modelLabel(model)
    }

    function formalRulesets() {
        return AgentGameRuntime.formalRulesets(root.gameRulesets, root.testRulesetId)
    }

    function selectedRulesetId() {
        if (!root.gameSetupMode) {
            return root.testRulesetId
        }
        var rows = root.formalRulesets()
        if (rulesetCombo.currentIndex >= 0 && rulesetCombo.currentIndex < rows.length) {
            return rows[rulesetCombo.currentIndex].ruleset_id || rows[rulesetCombo.currentIndex].id || "werewolf.basic"
        }
        return "werewolf.basic"
    }

    function fallbackWerewolfRoles() {
        return AgentGameRuntime.fallbackWerewolfRoles()
    }

    function gameRoleOptions() {
        return AgentGameRuntime.roleOptions(
                    root.gameRolePresets, root.selectedRulesetId(),
                    root.customHumanRoleValue,
                    "开始游戏时随机分配身份",
                    "未指定身份时，系统会在开局时随机分配角色。",
                    "使用你输入的 role_key 加入房间；开局时后端会保留该身份。")
    }

    function optionText(options, index) {
        return AgentGameRuntime.optionText(options, index, [root.customHumanRoleValue])
    }

    function selectedRoleKey() {
        if (!root.gameSetupMode || roleCombo.currentIndex < 0) {
            return ""
        }
        var rows = root.gameRoleOptions()
        if (roleCombo.currentIndex >= rows.length) {
            return ""
        }
        var value = rows[roleCombo.currentIndex].value || ""
        if (value === root.customHumanRoleValue) {
            return customHumanRoleField.text.trim()
        }
        return value
    }

    function selectedRoleRule() {
        if (!root.gameSetupMode || roleCombo.currentIndex < 0) {
            return ""
        }
        return AgentGameRuntime.optionRule(root.gameRoleOptions(), roleCombo.currentIndex)
    }

    function comboBackground(combo) {
        return combo.pressed ? Qt.rgba(0.88, 0.93, 1.0, 0.72)
                             : combo.hovered ? Qt.rgba(1, 1, 1, 0.62)
                                             : Qt.rgba(1, 1, 1, 0.46)
    }

    function setupTitle() {
        return root.gameSetupMode ? "Game 模式" : "多 AI 聊天"
    }

    function setupHint() {
        if (root.gameError.length > 0) {
            return root.gameError
        }
        if (root.gameStatusText.length > 0) {
            return root.gameStatusText
        }
        return root.gameSetupMode ? "先创建正式游戏房间，再在右侧时间线里推进和自定义行动。" : "只初始化参与对话的 AI。"
    }

    function resetDraftAgents() {
        draftAgents = []
        if (root.gameSetupMode) {
            roomTitleField.text = "自定义 Game 房间"
            gameContentField.text = "背景：\n目标：\n限制："
            addDraftAgent({ "display_name": "推理 AI", "persona": "谨慎分析线索，发言保持角色内。", "role_key": "" })
            addDraftAgent({ "display_name": "行动 AI", "persona": "积极推进游戏行动，但不跳出当前规则。", "role_key": "" })
            return
        }
        roomTitleField.text = "多 AI 测试房间"
        gameContentField.text = ""
        addDraftAgent({ "display_name": "灵感 AI", "persona": "负责提出开放想法，回复简短自然。" })
        addDraftAgent({ "display_name": "执行 AI", "persona": "负责把想法落到可测试步骤，避免长篇输出。" })
    }

    function addDraftAgent(seed) {
        var next = draftAgents.slice()
        next.push(AgentGameRuntime.createDraftAgent(seed, next.length, root.gameSetupMode, firstModel()))
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
        return AgentGameRuntime.normalizedAgent(
                    agent, firstModel(), root.gameSetupMode,
                    root.gameSetupMode ? gameContentField.text.trim() : "")
    }

    function roomAgents() {
        var rows = []
        for (var i = 0; i < draftAgents.length; ++i) {
            rows.push(normalizedAgent(draftAgents[i] || {}))
        }
        return rows
    }

    function disabledHostConfig() {
        return AgentGameRuntime.disabledHostConfig(root.gameSetupMode ? selectedRoleKey() : "")
    }

    function hostConfig() {
        if (!root.gameSetupMode || !hostEnabledCheck.checked) {
            return disabledHostConfig()
        }
        return AgentGameRuntime.hostConfig(
                    root.gameSetupMode, hostEnabledCheck.checked,
                    hostNameField.text.trim(), hostPersonaField.text.trim(),
                    gameContentField.text.trim(), firstModel(), selectedRoleKey())
    }

    function createRoom() {
        if (agentController) {
            agentController.createGameRoom(roomTitleField.text, selectedRulesetId(), roomAgents(), hostConfig(), root.humanDisplayName)
        }
    }

    function scrollSetup(delta) {
        var maxY = Math.max(0, setupFlick.contentHeight - setupFlick.height)
        setupFlick.contentY = Math.max(0, Math.min(maxY, setupFlick.contentY - delta))
    }

    Component.onCompleted: {
        initialRoomId = currentGameRoomId
        setupReady = true
        if (draftAgents.length === 0) {
            resetDraftAgents()
        }
        if (agentController) {
            agentController.refreshModelList()
            agentController.listGameRooms()
            agentController.listGameRulesets()
            agentController.listGameTemplates()
            agentController.listGameTemplatePresets(selectedRulesetId())
            if (root.gameSetupMode) {
                agentController.loadGameRolePresets(selectedRulesetId())
            }
        }
    }

    onSetupKindChanged: {
        if (setupReady) {
            initialRoomId = currentGameRoomId
            resetDraftAgents()
            if (agentController) {
                agentController.listGameTemplatePresets(selectedRulesetId())
                if (root.gameSetupMode) {
                    agentController.loadGameRolePresets(selectedRulesetId())
                }
            }
        }
    }

    onCurrentGameRoomIdChanged: {
        if (setupReady && currentGameRoomId.length > 0 && currentGameRoomId !== initialRoomId) {
            root.closeRequested()
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.closeRequested()
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(root.gameSetupMode ? 720 : 620, Math.max(0, root.width - 36))
        height: Math.min(760, Math.max(0, root.height - 36))
        radius: 14
        color: root.panelColor
        border.color: root.lineColor
        clip: true

        MouseArea {
            anchors.fill: parent
            onClicked: function(mouse) { mouse.accepted = true }
        }

        Flickable {
            id: setupFlick
            anchors.fill: parent
            anchors.margins: 16
            clip: true
            contentWidth: width
            contentHeight: setupColumn.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            ScrollBar.vertical: GlassScrollBar { }

            WheelHandler {
                onWheel: function(event) {
                    var delta = event.pixelDelta.y !== 0 ? event.pixelDelta.y : event.angleDelta.y / 120 * 54
                    if (delta !== 0 && setupFlick.contentHeight > setupFlick.height) {
                        root.scrollSetup(delta)
                        event.accepted = true
                    }
                }
            }

            ColumnLayout {
                id: setupColumn
                width: setupFlick.width
                height: implicitHeight
                spacing: 10

            AgentGameSetupHeader {
                Layout.fillWidth: true
                title: root.setupTitle()
                hint: root.setupHint()
                error: root.gameError.length > 0
                onCloseRequested: root.closeRequested()
            }

            TextField {
                id: roomTitleField
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                text: root.gameSetupMode ? "自定义 Game 房间" : "多 AI 测试房间"
                placeholderText: "房间标题"
                selectByMouse: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                visible: root.gameSetupMode

                ComboBox {
                    id: rulesetCombo
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    textRole: "label"
                    valueRole: "value"
                    model: {
                        var rows = []
                        var source = root.formalRulesets()
                        for (var i = 0; i < source.length; ++i) {
                            var item = source[i] || {}
                            rows.push({
                                "label": item.display_name || item.name || item.ruleset_id || "规则集",
                                "value": item.ruleset_id || item.id || ""
                            })
                        }
                        return rows
                    }
                    onActivated: {
                        roleCombo.currentIndex = 0
                        customHumanRoleField.text = ""
                        if (root.agentController) {
                            root.agentController.loadGameRolePresets(root.selectedRulesetId())
                            root.agentController.listGameTemplatePresets(root.selectedRulesetId())
                        }
                    }
                }

                ComboBox {
                    id: roleCombo
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    textRole: "label"
                    valueRole: "value"
                    model: root.gameRoleOptions()
                    displayText: root.optionText(root.gameRoleOptions(), currentIndex)
                    onActivated: {
                        if (root.gameRoleOptions()[currentIndex].value !== root.customHumanRoleValue) {
                            customHumanRoleField.text = ""
                        } else {
                            customHumanRoleField.forceActiveFocus()
                        }
                    }
                    delegate: ItemDelegate {
                        id: humanRoleDelegate
                        width: ListView.view ? ListView.view.width : 260
                        height: 44
                        highlighted: roleCombo.highlightedIndex === index

                        required property int index
                        required property var modelData

                        contentItem: Column {
                            spacing: 1
                            Text {
                                width: parent.width
                                text: root.optionText(root.gameRoleOptions(), humanRoleDelegate.index)
                                color: "#243145"
                                font.pixelSize: 12
                                font.bold: true
                                elide: Text.ElideRight
                            }
                            Text {
                                width: parent.width
                                text: humanRoleDelegate.modelData.hint
                                color: "#6c7a8e"
                                font.pixelSize: 10
                                elide: Text.ElideRight
                            }
                        }
                        background: Rectangle {
                            color: humanRoleDelegate.highlighted ? Qt.rgba(0.35, 0.61, 0.90, 0.16) : "transparent"
                            radius: 6
                        }
                    }
                    background: Rectangle {
                        radius: 8
                        color: root.comboBackground(roleCombo)
                        border.color: roleCombo.hovered || roleCombo.pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.42) : Qt.rgba(1, 1, 1, 0.38)
                    }
                    contentItem: Text {
                        leftPadding: 10
                        rightPadding: 30
                        text: "真人身份：" + roleCombo.displayText
                        color: "#243145"
                        font.pixelSize: 12
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    indicator: Image {
                        x: roleCombo.width - width - 10
                        y: (roleCombo.height - height) / 2
                        width: 12
                        height: 12
                        source: "qrc:/icons/dropdown.png"
                        fillMode: Image.PreserveAspectFit
                        rotation: roleCombo.popup.visible ? 180 : 0
                        opacity: roleCombo.enabled ? 0.78 : 0.38
                    }
                }
            }

            TextField {
                id: customHumanRoleField
                Layout.fillWidth: true
                Layout.preferredHeight: root.customHumanRoleSelected ? 32 : 0
                visible: root.gameSetupMode && root.customHumanRoleSelected
                placeholderText: "自定义真人 role_key"
                selectByMouse: true
            }

            Label {
                Layout.fillWidth: true
                Layout.preferredHeight: root.gameSetupMode ? implicitHeight : 0
                visible: root.gameSetupMode
                text: root.selectedRoleRule()
                color: "#65758b"
                font.pixelSize: 11
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            TextArea {
                id: gameContentField
                Layout.fillWidth: true
                Layout.preferredHeight: root.gameSetupMode ? 86 : 0
                visible: root.gameSetupMode
                text: "背景：\n目标：\n限制："
                placeholderText: "自定义游戏内容、背景、胜负目标或主持约束"
                wrapMode: TextArea.Wrap
                selectByMouse: true
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: hostColumn.implicitHeight + 18
                visible: root.gameSetupMode
                radius: 10
                color: Qt.rgba(1, 1, 1, 0.28)
                border.color: Qt.rgba(1, 1, 1, 0.36)

                ColumnLayout {
                    id: hostColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 9
                    spacing: 7

                    CheckBox {
                        id: hostEnabledCheck
                        text: "启用主持人"
                        checked: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        enabled: hostEnabledCheck.checked

                        TextField {
                            id: hostNameField
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 30
                            text: "主持人"
                            placeholderText: "主持名"
                            selectByMouse: true
                        }

                        TextField {
                            id: hostPersonaField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            placeholderText: "主持风格 / 开场约束"
                            selectByMouse: true
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: apiColumn.implicitHeight + 20
                radius: 10
                color: Qt.rgba(1, 1, 1, 0.30)
                border.color: Qt.rgba(1, 1, 1, 0.38)

                ColumnLayout {
                    id: apiColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 10
                    spacing: 7

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: "模型"
                            color: "#243145"
                            font.pixelSize: 13
                            font.bold: true
                        }

                        Label {
                            Layout.maximumWidth: 240
                            text: root.currentModel.length > 0 ? root.currentModel : root.modelLabel(root.firstModel())
                            color: "#65758b"
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        TextField {
                            id: apiProviderNameField
                            Layout.preferredWidth: 96
                            Layout.preferredHeight: 30
                            text: "chat-test"
                            placeholderText: "名称"
                            selectByMouse: true
                        }

                        TextField {
                            id: apiBaseUrlField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            text: "https://api.openai.com/v1"
                            placeholderText: "API 地址"
                            selectByMouse: true
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        TextField {
                            id: apiKeyField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            placeholderText: "API Key"
                            echoMode: TextInput.Password
                            selectByMouse: true
                        }

                        GlassButton {
                            Layout.preferredWidth: 72
                            Layout.preferredHeight: 30
                            text: root.apiProviderBusy ? "解析" : "接入"
                            textPixelSize: 12
                            cornerRadius: 8
                            enabled: !root.apiProviderBusy
                            normalColor: Qt.rgba(0.33, 0.56, 0.84, 0.22)
                            hoverColor: Qt.rgba(0.33, 0.56, 0.84, 0.32)
                            onClicked: {
                                if (root.agentController) {
                                    root.agentController.registerApiProvider(apiProviderNameField.text, apiBaseUrlField.text, apiKeyField.text)
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    text: root.gameSetupMode ? "AI 玩家" : "AI 成员"
                    color: "#243145"
                    font.pixelSize: 14
                    font.bold: true
                }

                GlassButton {
                    Layout.preferredWidth: 82
                    Layout.preferredHeight: 32
                    text: "添加 AI"
                    textPixelSize: 12
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.33, 0.56, 0.84, 0.22)
                    hoverColor: Qt.rgba(0.33, 0.56, 0.84, 0.32)
                    onClicked: root.addDraftAgent()
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: root.draftAgents

                    delegate: AgentGameAgentCard {
                        required property var modelData
                        required property int index

                        agentData: modelData
                        agentIndex: index
                        availableModels: root.availableModels
                        rolePresets: root.gameRolePresets && root.gameRolePresets.length > 0
                                     ? root.gameRolePresets
                                     : (root.selectedRulesetId() === "werewolf.basic" ? root.fallbackWerewolfRoles() : [])
                        rulesetId: root.selectedRulesetId()
                        showRoleControls: root.gameSetupMode
                        onFieldChanged: function(agentIndex, key, value) {
                            root.updateDraftAgent(agentIndex, key, value)
                        }
                        onRemoveRequested: function(agentIndex) {
                            root.removeDraftAgent(agentIndex)
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    visible: root.draftAgents.length === 0
                    radius: 8
                    color: Qt.rgba(1, 1, 1, 0.22)
                    border.color: Qt.rgba(1, 1, 1, 0.34)

                    Label {
                        anchors.centerIn: parent
                        text: "添加至少一个 AI"
                        color: "#65758b"
                        font.pixelSize: 12
                    }
                }
            }

            GlassButton {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                text: root.gameBusy ? "创建中" : (root.gameSetupMode ? "创建 Game 房间" : "创建多 AI 聊天")
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
}
