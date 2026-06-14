pragma ComponentBehavior: Bound

import QtQuick 2.15
import "../runtime/AgentGameRuntime.js" as AgentGameRuntime

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

    signal closeRequested()

    function firstModel() {
        return AgentGameRuntime.firstModel(root.availableModels)
    }

    function selectedRulesetId() {
        if (setupPane) {
            return setupPane.selectedRulesetId()
        }
        return root.gameSetupMode ? "werewolf.basic" : root.testRulesetId
    }

    function selectedRoleKey() {
        return setupPane ? setupPane.selectedRoleKey() : ""
    }

    function resetDraftAgents() {
        draftAgents = []
        if (setupPane) {
            setupPane.resetRoomDefaults()
        }
        if (root.gameSetupMode) {
            addDraftAgent({ "display_name": "推理 AI", "persona": "谨慎分析线索，发言保持角色内。", "role_key": "" })
            addDraftAgent({ "display_name": "行动 AI", "persona": "积极推进游戏行动，但不跳出当前规则。", "role_key": "" })
            return
        }
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
                    root.gameSetupMode && setupPane ? setupPane.gameContent() : "")
    }

    function roomAgents() {
        var rows = []
        for (var i = 0; i < draftAgents.length; ++i) {
            rows.push(normalizedAgent(draftAgents[i] || {}))
        }
        return rows
    }

    function hostConfig() {
        if (setupPane) {
            return setupPane.hostConfig(firstModel())
        }
        return AgentGameRuntime.disabledHostConfig(root.gameSetupMode ? selectedRoleKey() : "")
    }

    function createRoom() {
        if (agentController && setupPane) {
            agentController.createGameRoom(setupPane.roomTitle(), selectedRulesetId(), roomAgents(), hostConfig(), root.humanDisplayName)
        }
    }

    function refreshRulesetData() {
        if (!agentController) {
            return
        }
        agentController.listGameTemplatePresets(selectedRulesetId())
        if (root.gameSetupMode) {
            agentController.loadGameRolePresets(selectedRulesetId())
        }
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
            refreshRulesetData()
        }
    }

    onSetupKindChanged: {
        if (setupReady) {
            initialRoomId = currentGameRoomId
            resetDraftAgents()
            refreshRulesetData()
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

    AgentGameSetupPane {
        id: setupPane
        anchors.centerIn: parent
        width: Math.min(root.gameSetupMode ? 720 : 620, Math.max(0, root.width - 36))
        height: Math.min(760, Math.max(0, root.height - 36))

        availableModels: root.availableModels
        currentModel: root.currentModel
        apiProviderBusy: root.apiProviderBusy
        gameSetupMode: root.gameSetupMode
        gameRulesets: root.gameRulesets
        gameRolePresets: root.gameRolePresets
        gameBusy: root.gameBusy
        gameStatusText: root.gameStatusText
        gameError: root.gameError
        draftAgents: root.draftAgents
        customHumanRoleValue: root.customHumanRoleValue
        testRulesetId: root.testRulesetId
        panelColor: root.panelColor
        lineColor: root.lineColor

        onCloseRequested: root.closeRequested()
        onAddDraftAgentRequested: root.addDraftAgent()
        onDraftAgentFieldChanged: function(agentIndex, key, value) {
            root.updateDraftAgent(agentIndex, key, value)
        }
        onDraftAgentRemoveRequested: function(agentIndex) {
            root.removeDraftAgent(agentIndex)
        }
        onCreateRoomRequested: root.createRoom()
        onRulesetActivated: root.refreshRulesetData()
        onRegisterApiProviderRequested: function(name, baseUrl, apiKey) {
            if (root.agentController) {
                root.agentController.registerApiProvider(name, baseUrl, apiKey)
            }
        }
    }
}
