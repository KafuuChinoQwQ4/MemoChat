pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"
import "../runtime/AgentGameRuntime.js" as AgentGameRuntime

Rectangle {
    id: root
    radius: 14
    color: root.panelColor
    border.color: root.lineColor
    clip: true

    property var availableModels: []
    property string currentModel: ""
    property bool apiProviderBusy: false
    property bool gameSetupMode: false
    property var gameRulesets: []
    property var gameRolePresets: []
    property bool gameBusy: false
    property string gameStatusText: ""
    property string gameError: ""
    property var draftAgents: []
    property string customHumanRoleValue: "__custom_human_role__"
    property string testRulesetId: "multi_ai_chat.test"
    property color panelColor: Qt.rgba(0.96, 0.98, 1.0, 0.92)
    property color lineColor: Qt.rgba(1, 1, 1, 0.52)

    signal closeRequested()
    signal addDraftAgentRequested()
    signal draftAgentFieldChanged(int agentIndex, string key, var value)
    signal draftAgentRemoveRequested(int agentIndex)
    signal createRoomRequested()
    signal rulesetActivated(string rulesetId)
    signal registerApiProviderRequested(string name, string baseUrl, string apiKey)

    function firstModel() {
        return AgentGameRuntime.firstModel(root.availableModels)
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

    function selectedRulesetId() {
        return roomPane.selectedRulesetId()
    }

    function selectedRoleKey() {
        return roomPane.selectedRoleKey()
    }

    function roomTitle() {
        return roomPane.roomTitle()
    }

    function gameContent() {
        return roomPane.gameContent()
    }

    function hostConfig(model) {
        return roomPane.hostConfig(model)
    }

    function resetRoomDefaults() {
        roomPane.resetDefaults()
    }

    function agentRolePresets() {
        if (root.gameRolePresets && root.gameRolePresets.length > 0) {
            return root.gameRolePresets
        }
        if (selectedRulesetId() === "werewolf.basic") {
            return AgentGameRuntime.fallbackWerewolfRoles()
        }
        return []
    }

    function scrollSetup(delta) {
        var maxY = Math.max(0, setupFlick.contentHeight - setupFlick.height)
        setupFlick.contentY = Math.max(0, Math.min(maxY, setupFlick.contentY - delta))
    }

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

            AgentGameRoomPane {
                id: roomPane
                Layout.fillWidth: true
                gameSetupMode: root.gameSetupMode
                gameRulesets: root.gameRulesets
                gameRolePresets: root.gameRolePresets
                customHumanRoleValue: root.customHumanRoleValue
                testRulesetId: root.testRulesetId
                onRulesetActivated: function(rulesetId) {
                    root.rulesetActivated(rulesetId)
                }
            }

            AgentGameTemplatePane {
                Layout.fillWidth: true
                currentModelLabel: root.currentModel.length > 0 ? root.currentModel : AgentGameRuntime.modelLabel(root.firstModel())
                apiProviderBusy: root.apiProviderBusy
                onRegisterRequested: function(name, baseUrl, apiKey) {
                    root.registerApiProviderRequested(name, baseUrl, apiKey)
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
                    onClicked: root.addDraftAgentRequested()
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
                        rolePresets: root.agentRolePresets()
                        rulesetId: root.selectedRulesetId()
                        showRoleControls: root.gameSetupMode
                        onFieldChanged: function(agentIndex, key, value) {
                            root.draftAgentFieldChanged(agentIndex, key, value)
                        }
                        onRemoveRequested: function(agentIndex) {
                            root.draftAgentRemoveRequested(agentIndex)
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
                onClicked: root.createRoomRequested()
            }
        }
    }
}
