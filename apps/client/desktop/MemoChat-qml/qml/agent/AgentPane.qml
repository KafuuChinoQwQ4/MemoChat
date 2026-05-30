pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "AgentPaneRuntime.js" as AgentPaneRuntime

Rectangle {
    id: root
    color: "transparent"

    property var agentController: null
    property var messageModel: null
    property var sessions: []
    property string currentSessionId: ""
    property string currentModel: ""
    property var availableModels: []
    property bool modelRefreshBusy: false
    property bool apiProviderBusy: false
    property string apiProviderStatus: ""
    property bool thinkingEnabled: root.agentController ? root.agentController.thinkingEnabled : false
    property bool currentModelSupportsThinking: root.agentController ? root.agentController.currentModelSupportsThinking : false
    property var knowledgeBases: root.agentController ? root.agentController.knowledgeBases : []
    property string knowledgeSearchResult: root.agentController ? root.agentController.knowledgeSearchResult : ""
    property bool knowledgeBusy: false
    property string knowledgeStatusText: ""
    property string knowledgeError: ""
    property var memories: root.agentController ? root.agentController.memories : []
    property bool memoryBusy: root.agentController ? root.agentController.memoryBusy : false
    property string memoryStatusText: root.agentController ? root.agentController.memoryStatusText : ""
    property string memoryError: root.agentController ? root.agentController.memoryError : ""
    property var agentTasks: root.agentController ? root.agentController.agentTasks : []
    property bool agentTaskBusy: root.agentController ? root.agentController.agentTaskBusy : false
    property string agentTaskStatusText: root.agentController ? root.agentController.agentTaskStatusText : ""
    property string agentTaskError: root.agentController ? root.agentController.agentTaskError : ""
    property string currentTraceId: root.agentController ? root.agentController.currentTraceId : ""
    property string currentSkill: root.agentController ? root.agentController.currentSkill : ""
    property string currentFeedbackSummary: root.agentController ? root.agentController.currentFeedbackSummary : ""
    property var traceEvents: root.agentController ? root.agentController.traceEvents : []
    property var traceObservations: root.agentController ? root.agentController.traceObservations : []
    property string agentSkillMode: root.agentController ? root.agentController.agentSkillMode : "auto"
    property string agentSkillDisplay: root.agentController ? root.agentController.agentSkillDisplay : "自动"
    property bool loading: false
    property bool streaming: false
    property string errorMsg: ""
    property string selfName: ""
    property string selfAvatar: "qrc:/res/head_1.jpg"
    property var gameState: root.agentController ? root.agentController.gameState : ({})
    property string currentGameRoomId: root.agentController ? root.agentController.currentGameRoomId : ""
    property bool gameBusy: root.agentController ? root.agentController.gameBusy : false
    property string gameStatusText: root.agentController ? root.agentController.gameStatusText : ""
    property string gameError: root.agentController ? root.agentController.gameError : ""
    property bool multiAiDetailsExpanded: false
    readonly property color glassPanelColor: Qt.rgba(0.96, 0.98, 1.0, 0.62)
    readonly property color glassPanelHoverColor: Qt.rgba(1, 1, 1, 0.76)
    readonly property color glassPanelPressedColor: Qt.rgba(0.86, 0.92, 1.0, 0.70)
    readonly property color glassBorderColor: Qt.rgba(1, 1, 1, 0.58)
    readonly property var skillModes: [
        { "key": "auto", "label": "自动" },
        { "key": "knowledge", "label": "知识库" },
        { "key": "research", "label": "联网" },
        { "key": "graph", "label": "图谱" },
        { "key": "calculate", "label": "计算" }
    ]
    readonly property bool gameRoomActive: currentGameRoomId.length > 0
    readonly property bool multiAiRoomActive: gameRoomActive && currentRulesetId() === "multi_ai_chat.test"
    readonly property bool formalGameRoomActive: gameRoomActive && currentRulesetId().length > 0 && !multiAiRoomActive

    signal backRequested()
    signal gameModeRequested()

    onCurrentGameRoomIdChanged: multiAiDetailsExpanded = false

    function currentSessionTitle() {
        return AgentPaneRuntime.currentSessionTitle({
            "gameRoomActive": root.gameRoomActive,
            "gameState": root.gameState,
            "multiAiRoomActive": root.multiAiRoomActive,
            "sessions": root.sessions,
            "currentSessionId": root.currentSessionId
        })
    }

    function sessionSummary() {
        return AgentPaneRuntime.sessionSummary({
            "gameRoomActive": root.gameRoomActive,
            "multiAiRoomActive": root.multiAiRoomActive,
            "gameBusy": root.gameBusy,
            "gameError": root.gameError,
            "gameStatusText": root.gameStatusText,
            "currentSessionId": root.currentSessionId,
            "streaming": root.streaming,
            "loading": root.loading
        })
    }

    function roomObject() {
        return AgentPaneRuntime.roomObject(root.gameState, root.currentGameRoomId)
    }

    function currentRulesetId() {
        return AgentPaneRuntime.rulesetId(root.gameState, root.currentGameRoomId)
    }

    function participants() {
        return AgentPaneRuntime.participants(root.gameState)
    }

    function events() {
        return AgentPaneRuntime.events(root.gameState)
    }

    function availableActions() {
        return AgentPaneRuntime.availableActions(root.gameState)
    }

    function roomStatusValue() {
        return AgentPaneRuntime.roomStatusValue(root.gameState, root.currentGameRoomId)
    }

    function isGameEnded() {
        return AgentPaneRuntime.isGameEnded(root.gameState, root.currentGameRoomId)
    }

    function skillModeHint() {
        return AgentPaneRuntime.skillModeHint(root.agentSkillMode)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        AgentPaneHeader {
            Layout.fillWidth: true
            Layout.preferredHeight: 78
            gameRoomActive: root.gameRoomActive
            multiAiRoomActive: root.multiAiRoomActive
            multiAiDetailsExpanded: root.multiAiDetailsExpanded
            sessionTitle: root.currentSessionTitle()
            sessionSummary: root.sessionSummary()
            currentModel: root.currentModel
            glassPanelColor: root.glassPanelColor
            glassPanelHoverColor: root.glassPanelHoverColor
            glassPanelPressedColor: root.glassPanelPressedColor
            glassBorderColor: root.glassBorderColor
            onDetailsToggled: root.multiAiDetailsExpanded = !root.multiAiDetailsExpanded
            onModeMenuRequested: modeMenu.open()
            onMoreMenuRequested: moreMenu.open()
        }

        Menu {
            id: moreMenu
            y: 48
            x: Math.max(0, root.width - width - 18)
            implicitWidth: 174
            padding: 8
            background: Rectangle {
                radius: 14
                color: root.glassPanelColor
                border.width: 1
                border.color: root.glassBorderColor
            }

            delegate: Rectangle {
                implicitWidth: 158
                implicitHeight: 36
                radius: 10
                color: hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.14)
                               : pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                                         : "transparent"
                border.width: 0

                required property string text
                required property bool highlighted
                required property bool hovered
                required property bool pressed
                required property var action

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    text: parent.text
                    color: "#2a3649"
                    font.pixelSize: 13
                    font.bold: parent.hovered
                    elide: Text.ElideRight
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: parent.action.trigger()
                }
            }

            MenuItem {
                text: root.streaming ? "停止生成" : "停止"
                enabled: root.streaming
                onTriggered: {
                    if (root.agentController) {
                        root.agentController.cancelStream()
                    }
                }
            }

            MenuItem {
                text: "刷新房间"
                visible: root.gameRoomActive
                enabled: root.currentGameRoomId.length > 0 && !root.gameBusy
                onTriggered: {
                    if (root.agentController) {
                        root.agentController.loadGameRoom(root.currentGameRoomId)
                    }
                }
            }

            MenuItem {
                text: root.formalGameRoomActive ? "新建 Game 房间" : "新建多 AI 房间"
                visible: root.gameRoomActive
                enabled: !root.loading && !root.streaming && !root.gameBusy
                onTriggered: root.gameModeRequested()
            }

            MenuItem {
                text: "模型"
                onTriggered: {
                    const nextActive = !modelSettingsLoader.active
                    modelSettingsLoader.active = nextActive
                    if (nextActive && root.agentController) {
                        root.agentController.refreshModelList()
                    }
                }
            }

            MenuItem {
                text: "知识库"
                onTriggered: {
                    if (root.agentController) {
                        root.agentController.listKnowledgeBases()
                    }
                    knowledgeBaseLoader.active = true
                }
            }

            MenuItem {
                text: "轨迹"
                onTriggered: traceLoader.active = true
            }

            MenuItem {
                text: "记忆"
                onTriggered: {
                    if (root.agentController) {
                        root.agentController.listMemories()
                    }
                    memoryLoader.active = true
                }
            }

            MenuItem {
                text: "任务"
                onTriggered: {
                    if (root.agentController) {
                        root.agentController.listAgentTasks()
                    }
                    taskLoader.active = true
                }
            }
        }

        Menu {
            id: modeMenu
            y: 48
            x: Math.max(0, root.width - width - 64)
            implicitWidth: 180
            padding: 8
            background: Rectangle {
                radius: 14
                color: root.glassPanelColor
                border.width: 1
                border.color: root.glassBorderColor
            }

            delegate: Rectangle {
                id: modeMenuDelegate
                implicitWidth: 164
                implicitHeight: 36
                radius: 10
                color: hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.14)
                               : pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                                         : "transparent"
                border.width: 0

                required property string text
                required property bool highlighted
                required property bool hovered
                required property bool pressed
                required property var action

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Label {
                        Layout.fillWidth: true
                        text: modeMenuDelegate.text
                        color: "#2a3649"
                        font.pixelSize: 13
                        font.bold: modeMenuDelegate.hovered
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        Layout.preferredWidth: 7
                        Layout.preferredHeight: 7
                        radius: 4
                        color: modeMenuDelegate.text === root.agentSkillDisplay ? "#3f8fe5" : "transparent"
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: parent.action ? parent.action.enabled : true
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: parent.action.trigger()
                }
            }

            Repeater {
                model: root.skillModes

                delegate: MenuItem {
                    text: modelData.label
                    enabled: !root.loading && !root.streaming
                    onTriggered: {
                        if (root.agentController) {
                            root.agentController.switchAgentSkillMode(modelData.key)
                        }
                    }
                }
            }

            MenuItem {
                text: "Game 模式"
                enabled: !root.loading && !root.streaming
                onTriggered: root.gameModeRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: Qt.rgba(1, 1, 1, 0.16)
            border.color: Qt.rgba(1, 1, 1, 0.42)

            AgentConversationPane {
                anchors.fill: parent
                anchors.margins: 12
                visible: !root.gameRoomActive
                agentController: root.agentController
                messageModel: root.messageModel
                loading: root.loading
                streaming: root.streaming
                errorMsg: root.errorMsg
                sessions: root.sessions
                currentSessionId: root.currentSessionId
                selfAvatar: root.selfAvatar
            }

            AgentMultiAiChatPane {
                anchors.fill: parent
                anchors.margins: 12
                visible: root.multiAiRoomActive
                agentController: root.agentController
                gameState: root.gameState
                currentGameRoomId: root.currentGameRoomId
                gameBusy: root.gameBusy
                selfName: root.selfName
                selfAvatar: root.selfAvatar
                detailsExpanded: root.multiAiDetailsExpanded
            }

            AgentGamePlayPane {
                anchors.fill: parent
                anchors.margins: 12
                visible: root.formalGameRoomActive
                agentController: root.agentController
                participants: root.participants()
                events: root.events()
                availableActions: root.availableActions()
                gameBusy: root.gameBusy
                roomId: root.currentGameRoomId
                roomLabel: root.currentSessionTitle()
                roomEnded: root.isGameEnded()
                onRefreshRequested: {
                    if (root.agentController && root.currentGameRoomId.length > 0) {
                        root.agentController.loadGameRoom(root.currentGameRoomId)
                    }
                }
                onStartRequested: {
                    if (root.agentController) {
                        root.agentController.startGameRoom(root.currentGameRoomId)
                    }
                }
                onRestartRequested: {
                    if (root.agentController) {
                        root.agentController.restartGameRoom(root.currentGameRoomId)
                    }
                }
                onTickRequested: {
                    if (root.agentController) {
                        root.agentController.tickGameRoom(root.currentGameRoomId)
                    }
                }
                onAutoTickRequested: {
                    if (root.agentController) {
                        root.agentController.autoTickGameRoom(root.currentGameRoomId, 8)
                    }
                }
                onSubmitActionRequested: function(actorId, actionType, targetId, content) {
                    if (root.agentController) {
                        root.agentController.submitGameAction(root.currentGameRoomId, actorId, actionType, targetId, content)
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 148
            visible: !root.gameRoomActive
            radius: 12
            color: Qt.rgba(1, 1, 1, 0.20)
            border.color: Qt.rgba(1, 1, 1, 0.44)

            AgentComposerBar {
                anchors.fill: parent
                enabledComposer: !root.loading && !root.streaming
                onSendComposer: function(text) {
                    if (root.agentController) {
                        root.agentController.sendStreamMessage(text)
                    }
                }
            }
        }
    }

    Loader {
        id: modelSettingsLoader
        anchors.fill: parent
        active: false
        sourceComponent: Component {
            AgentModelControlBar {
                currentModel: root.currentModel
                availableModels: root.availableModels
                modelRefreshBusy: root.modelRefreshBusy
                apiProviderBusy: root.apiProviderBusy
                apiProviderStatus: root.apiProviderStatus
                thinkingEnabled: root.thinkingEnabled
                currentModelSupportsThinking: root.currentModelSupportsThinking
                onCloseRequested: modelSettingsLoader.active = false
                onThinkingToggled: function(checked) {
                    if (root.agentController) {
                        root.agentController.thinkingEnabled = checked
                    }
                }
                onApiProviderRegistered: function(name, baseUrl, apiKey) {
                    if (root.agentController) {
                        root.agentController.registerApiProvider(name, baseUrl, apiKey)
                    }
                }
                onModelSelected: function(modelType, modelName) {
                    if (root.agentController && modelType.length > 0 && modelName.length > 0) {
                        root.agentController.switchModel(modelType, modelName)
                    }
                }
                onModelDeleted: function(modelType) {
                    if (root.agentController && modelType.length > 0) {
                        root.agentController.deleteApiProvider(modelType)
                    }
                }
                onRefreshModelsRequested: {
                    if (root.agentController) {
                        root.agentController.refreshModelList()
                    }
                }
            }
        }
    }

    AgentStatusOverlay {
        id: knowledgeBaseLoader
        anchors.fill: parent
        active: false
        title: "知识库"
        maxPanelWidth: 560
        maxPanelHeight: 640
        onCloseRequested: active = false
        contentComponent: Component {
            KnowledgeBasePane {
                agentController: root.agentController
                kbList: root.knowledgeBases
                searchResult: root.knowledgeSearchResult
                busy: root.knowledgeBusy
                statusText: root.knowledgeStatusText
                errorText: root.knowledgeError
                onReloadRequested: {
                    if (root.agentController) {
                        root.agentController.listKnowledgeBases()
                    }
                }
            }
        }
    }

    AgentStatusOverlay {
        id: traceLoader
        anchors.fill: parent
        active: false
        title: "AI Agent 轨迹"
        maxPanelWidth: 720
        maxPanelHeight: 660
        onCloseRequested: active = false
        contentComponent: Component {
            AgentTracePane {
                traceId: root.currentTraceId
                skill: root.currentSkill
                feedbackSummary: root.currentFeedbackSummary
                observations: root.traceObservations
                events: root.traceEvents
            }
        }
    }

    AgentStatusOverlay {
        id: memoryLoader
        anchors.fill: parent
        active: false
        title: "AI 记忆"
        maxPanelWidth: 620
        maxPanelHeight: 640
        onCloseRequested: active = false
        contentComponent: Component {
            AgentMemoryPane {
                agentController: root.agentController
                memories: root.memories
                busy: root.memoryBusy
                statusText: root.memoryStatusText
                errorText: root.memoryError
                onReloadRequested: {
                    if (root.agentController) {
                        root.agentController.listMemories()
                    }
                }
            }
        }
    }

    AgentStatusOverlay {
        id: taskLoader
        anchors.fill: parent
        active: false
        title: "AI 任务"
        maxPanelWidth: 660
        maxPanelHeight: 640
        onCloseRequested: active = false
        contentComponent: Component {
            AgentTaskPane {
                agentController: root.agentController
                tasks: root.agentTasks
                busy: root.agentTaskBusy
                statusText: root.agentTaskStatusText
                errorText: root.agentTaskError
                onReloadRequested: {
                    if (root.agentController) {
                        root.agentController.listAgentTasks()
                    }
                }
            }
        }
    }
}
