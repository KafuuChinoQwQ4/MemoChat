import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root
    color: "transparent"

    property var agentController: null
    property var messageModel: null
    property string currentSessionId: ""
    property var sessions: []
    property string currentModel: ""
    property var availableModels: []
    property bool loading: false
    property bool streaming: false
    property string errorMsg: ""
    property var kbList: []
    property var pendingSmartResult: null
    property int _viewMode: 0  // 0: chat, 1: kb management

    signal backRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 顶部标题栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.24)
            border.color: Qt.rgba(1, 1, 1, 0.46)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 18
                spacing: 8

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: root._viewMode === 0

                    RowLayout {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 12

                        Label {
                            text: "AI 助手"
                            color: "#2a3649"
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Label {
                            text: "• " + root.currentModel
                            color: "#6a7b92"
                            font.pixelSize: 12
                            visible: root.currentModel.length > 0
                        }

                        Label {
                            text: "..."
                            color: "#6a7b92"
                            font.pixelSize: 12
                            visible: root.loading || root.streaming
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: root._viewMode === 1

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "我的知识库"
                        color: "#2a3649"
                        font.pixelSize: 18
                        font.bold: true
                    }
                }

                GlassButton {
                    Layout.preferredWidth: 88
                    Layout.preferredHeight: 32
                    text: root._viewMode === 0 ? "知识库" : "返回聊天"
                    textPixelSize: 13
                    cornerRadius: 9
                    normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
                    hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
                    pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)
                    onClicked: {
                        if (root._viewMode === 0) {
                            root._viewMode = 1
                            root.reloadKnowledgeBases()
                        } else {
                            root._viewMode = 0
                        }
                    }
                }

                GlassButton {
                    Layout.preferredWidth: 88
                    Layout.preferredHeight: 32
                    text: "设置"
                    textPixelSize: 13
                    cornerRadius: 9
                    normalColor: Qt.rgba(0.42, 0.56, 0.74, 0.22)
                    hoverColor: Qt.rgba(0.42, 0.56, 0.74, 0.32)
                    pressedColor: Qt.rgba(0.42, 0.56, 0.74, 0.40)
                    onClicked: modelSettingsLoader.active = !modelSettingsLoader.active
                }
            }
        }

        // 主内容区
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.16)
            border.color: Qt.rgba(1, 1, 1, 0.42)

            Loader {
                anchors.fill: parent
                anchors.margins: 14
                active: root._viewMode === 0
                sourceComponent: Component {
                    AgentConversationPane {
                        agentController: root.agentController
                        messageModel: root.messageModel
                        loading: root.loading
                        streaming: root.streaming
                        errorMsg: root.errorMsg
                        sessions: root.sessions
                        currentSessionId: root.currentSessionId
                        onReloadSessions: {
                            if (root.agentController) root.agentController.loadSessions()
                        }
                        onSwitchSession: function(sid) {
                            if (root.agentController) root.agentController.switchSession(sid)
                        }
                        onDeleteSession: function(sid) {
                            if (root.agentController) root.agentController.deleteSession(sid)
                        }
                    }
                }
            }

            Loader {
                anchors.fill: parent
                anchors.margins: 14
                active: root._viewMode === 1
                sourceComponent: Component {
                    KnowledgeBasePane {
                        agentController: root.agentController
                        kbList: root.kbList
                        onReloadRequested: root.reloadKnowledgeBases()
                    }
                }
            }
        }

        // 智能功能工具栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.22)
            border.color: Qt.rgba(1, 1, 1, 0.46)
            visible: root._viewMode === 0

            SmartFeatureBar {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                smartResult: root.pendingSmartResult
                onSummaryRequested: {
                    var chatHistory = collectChatHistory()
                    if (root.agentController) root.agentController.summarizeChat("current", chatHistory)
                }
                onSuggestRequested: {
                    var chatHistory = collectChatHistory()
                    if (root.agentController) root.agentController.suggestReply("current", chatHistory)
                }
                onTranslateRequested: {
                    if (root.agentController) root.agentController.translateMessage(lastSelectedText, "中文")
                }
            }
        }

        // 输入框
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 156
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.22)
            border.color: Qt.rgba(1, 1, 1, 0.46)
            visible: root._viewMode === 0

            AgentComposerBar {
                id: composerBar
                anchors.fill: parent
                enabledComposer: !root.loading
                onSendComposer: function(text) {
                    if (root.agentController) root.agentController.sendMessage(text)
                }
                onNewSession: {
                    if (root.agentController) root.agentController.createSession()
                }
            }
        }
    }

    // 模型设置面板
    Loader {
        id: modelSettingsLoader
        anchors.fill: parent
        active: false
        sourceComponent: Component {
            Rectangle {
                anchors.fill: parent
                color: Qt.rgba(0, 0, 0, 0.3)
                MouseArea {
                    anchors.fill: parent
                    onClicked: modelSettingsLoader.active = false
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: 360
                    height: 280
                    radius: 12
                    color: Qt.rgba(1, 1, 1, 0.95)
                    border.color: Qt.rgba(1, 1, 1, 0.6)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Label {
                            text: "模型设置"
                            font.pixelSize: 16
                            font.bold: true
                            color: "#2a3649"
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: root.availableModels
                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 44
                                radius: 6
                                color: mouseArea.containsMouse ? Qt.rgba(0.54, 0.70, 0.93, 0.15) : "transparent"
                                border.color: Qt.rgba(1, 1, 1, 0.3)
                                Label {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.display_name || modelData.model_name || ""
                                    color: "#2a3649"
                                    font.pixelSize: 14
                                }
                                Label {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.model_type || ""
                                    color: "#6a7b92"
                                    font.pixelSize: 11
                                }
                                MouseArea {
                                    id: mouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (root.agentController && modelData.model_type && modelData.model_name) {
                                            root.agentController.switchModel(modelData.model_type, modelData.model_name)
                                        }
                                        modelSettingsLoader.active = false
                                    }
                                }
                            }
                        }

                        GlassButton {
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: 32
                            Layout.alignment: Qt.AlignRight
                            text: "刷新模型"
                            textPixelSize: 13
                            cornerRadius: 8
                            normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                            onClicked: {
                                if (root.agentController) root.agentController.refreshModelList()
                            }
                        }
                    }
                }
            }
        }
    }

    function collectChatHistory() {
        if (!root.messageModel) return "[]"
        var msgs = []
        var count = root.messageModel.rowCount()
        for (var i = 0; i < count; i++) {
            var item = root.messageModel.data(root.messageModel.index(i, 0), Qt.DisplayRole)
        }
        return JSON.stringify(msgs)
    }

    function reloadKnowledgeBases() {
        if (root.agentController) root.agentController.listKnowledgeBases()
    }

    function refreshSessions() {
        if (root.agentController) root.agentController.loadSessions()
    }
}
