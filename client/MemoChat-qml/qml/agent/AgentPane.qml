import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root
    color: "transparent"

    property var agentController: null
    property var messageModel: null
    property var sessions: []
    property string currentSessionId: ""
    property string currentModel: ""
    property var availableModels: []
    property bool loading: false
    property bool streaming: false
    property string errorMsg: ""
    property string selfAvatar: "qrc:/res/head_1.jpg"

    signal backRequested()

    function currentSessionTitle() {
        if (!sessions || currentSessionId.length === 0) {
            return "未选择会话"
        }
        for (var i = 0; i < sessions.length; ++i) {
            var session = sessions[i]
            if (session.session_id === currentSessionId) {
                return session.title && session.title.length > 0 ? session.title : "新会话"
            }
        }
        return "当前会话"
    }

    function sessionSummary() {
        if (currentSessionId.length === 0) {
            return "从左侧选择或新建会话开始。"
        }
        if (streaming) {
            return "AI 正在生成回复。"
        }
        if (loading) {
            return "AI 正在处理你的问题。"
        }
        return "当前会话已就绪，可以继续追问。"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 78
            radius: 12
            color: Qt.rgba(1, 1, 1, 0.22)
            border.color: Qt.rgba(1, 1, 1, 0.46)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "AI 助手"
                            color: "#2a3649"
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Rectangle {
                            Layout.preferredHeight: 24
                            Layout.preferredWidth: sessionTitleLabel.implicitWidth + 18
                            radius: 12
                            color: Qt.rgba(1, 1, 1, 0.32)
                            border.width: 1
                            border.color: Qt.rgba(1, 1, 1, 0.36)

                            Label {
                                id: sessionTitleLabel
                                anchors.centerIn: parent
                                text: root.currentSessionTitle()
                                color: "#4e5d74"
                                font.pixelSize: 11
                                font.bold: true
                                elide: Text.ElideRight
                            }
                        }

                        Rectangle {
                            Layout.preferredHeight: 24
                            Layout.preferredWidth: modelChipLabel.implicitWidth + 16
                            radius: 12
                            color: Qt.rgba(0.36, 0.62, 0.92, 0.16)
                            visible: root.currentModel.length > 0

                            Label {
                                id: modelChipLabel
                                anchors.centerIn: parent
                                text: root.currentModel
                                color: "#2d6fb4"
                                font.pixelSize: 11
                                font.bold: true
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.sessionSummary()
                        color: "#6a7b92"
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        maximumLineCount: 2
                    }
                }

                GlassButton {
                    Layout.preferredWidth: 84
                    Layout.preferredHeight: 34
                    text: root.streaming ? "停止" : "新会话"
                    textPixelSize: 13
                    cornerRadius: 9
                    normalColor: root.streaming ? Qt.rgba(0.89, 0.27, 0.27, 0.22) : Qt.rgba(0.35, 0.61, 0.90, 0.22)
                    hoverColor: root.streaming ? Qt.rgba(0.89, 0.27, 0.27, 0.32) : Qt.rgba(0.35, 0.61, 0.90, 0.32)
                    pressedColor: root.streaming ? Qt.rgba(0.89, 0.27, 0.27, 0.42) : Qt.rgba(0.35, 0.61, 0.90, 0.40)
                    onClicked: {
                        if (!root.agentController) {
                            return
                        }
                        if (root.streaming) {
                            root.agentController.cancelStream()
                        } else {
                            root.agentController.createSession()
                        }
                    }
                }

                GlassButton {
                    Layout.preferredWidth: 74
                    Layout.preferredHeight: 34
                    text: "模型"
                    textPixelSize: 13
                    cornerRadius: 9
                    normalColor: Qt.rgba(0.42, 0.56, 0.74, 0.22)
                    hoverColor: Qt.rgba(0.42, 0.56, 0.74, 0.32)
                    pressedColor: Qt.rgba(0.42, 0.56, 0.74, 0.40)
                    onClicked: modelSettingsLoader.active = !modelSettingsLoader.active
                }
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
                agentController: root.agentController
                messageModel: root.messageModel
                loading: root.loading
                streaming: root.streaming
                errorMsg: root.errorMsg
                sessions: root.sessions
                currentSessionId: root.currentSessionId
                selfAvatar: root.selfAvatar
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 148
            radius: 12
            color: Qt.rgba(1, 1, 1, 0.20)
            border.color: Qt.rgba(1, 1, 1, 0.44)

            AgentComposerBar {
                anchors.fill: parent
                enabledComposer: !root.loading && !root.streaming
                onSendComposer: function(text) {
                    if (root.agentController) {
                        root.agentController.sendMessage(text)
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
            Rectangle {
                anchors.fill: parent
                color: Qt.rgba(0, 0, 0, 0.28)

                MouseArea {
                    anchors.fill: parent
                    onClicked: modelSettingsLoader.active = false
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: 380
                    height: 320
                    radius: 14
                    color: Qt.rgba(1, 1, 1, 0.96)
                    border.color: Qt.rgba(1, 1, 1, 0.60)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Label {
                            text: "选择模型"
                            font.pixelSize: 16
                            font.bold: true
                            color: "#2a3649"
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.currentModel.length > 0 ? ("当前使用: " + root.currentModel) : "当前模型尚未加载"
                            color: "#6a7b92"
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: root.availableModels
                            spacing: 6

                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 48
                                radius: 8
                                color: {
                                    const fullName = (modelData.model_type || "") + ":" + (modelData.model_name || "")
                                    if (fullName === root.currentModel) {
                                        return Qt.rgba(0.54, 0.70, 0.93, 0.18)
                                    }
                                    return modelMouseArea.containsMouse ? Qt.rgba(0.54, 0.70, 0.93, 0.10) : "transparent"
                                }
                                border.color: Qt.rgba(1, 1, 1, 0.32)

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 8

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        Label {
                                            Layout.fillWidth: true
                                            text: modelData.display_name || modelData.model_name || ""
                                            color: "#2a3649"
                                            font.pixelSize: 14
                                            font.bold: true
                                            elide: Text.ElideRight
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: modelData.model_type || ""
                                            color: "#6a7b92"
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                        }
                                    }
                                }

                                MouseArea {
                                    id: modelMouseArea
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

                        RowLayout {
                            Layout.fillWidth: true

                            Item { Layout.fillWidth: true }

                            GlassButton {
                                Layout.preferredWidth: 96
                                Layout.preferredHeight: 32
                                text: "刷新模型"
                                textPixelSize: 13
                                cornerRadius: 8
                                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                                onClicked: {
                                    if (root.agentController) {
                                        root.agentController.refreshModelList()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
