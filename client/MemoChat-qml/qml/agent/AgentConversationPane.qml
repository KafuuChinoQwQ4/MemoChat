import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root
    color: "transparent"

    property var agentController: null
    property var messageModel: null
    property bool loading: false
    property bool streaming: false
    property string errorMsg: ""
    property var sessions: []
    property string currentSessionId: ""
    property string _draftText: ""

    signal reloadSessions()
    signal switchSession(string sessionId)
    signal deleteSession(string sessionId)

    RowLayout {
        anchors.fill: parent
        spacing: 10

        // 左侧会话列表
        Rectangle {
            Layout.preferredWidth: 200
            Layout.fillHeight: true
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.12)
            border.color: Qt.rgba(1, 1, 1, 0.3)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Label {
                    text: "会话列表"
                    font.pixelSize: 12
                    font.bold: true
                    color: "#6a7b92"
                    Layout.bottomMargin: 4
                }

                GlassButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    text: "+ 新会话"
                    textPixelSize: 13
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                    onClicked: {
                        if (root.agentController) root.agentController.createSession()
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.sessions
                    ScrollBar.vertical: GlassScrollBar { }
                    spacing: 4

                    delegate: Rectangle {
                        width: ListView.view.width
                        implicitHeight: 52
                        radius: 8
                        color: {
                            var isCurrent = (modelData.session_id === root.currentSessionId)
                            if (isCurrent) return Qt.rgba(0.54, 0.70, 0.93, 0.28)
                            return mouseArea.containsMouse ? Qt.rgba(1, 1, 1, 0.1) : "transparent"
                        }
                        border.color: Qt.rgba(1, 1, 1, 0.2)

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 2

                            Label {
                                Layout.fillWidth: true
                                text: modelData.title || "新会话"
                                font.pixelSize: 13
                                font.bold: true
                                color: "#2a3649"
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.model_name || ""
                                font.pixelSize: 11
                                color: "#6a7b92"
                                elide: Text.ElideRight
                            }
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.switchSession(modelData.session_id)
                            }
                        }

                        ToolTip.visible: mouseArea.containsMouse
                        ToolTip.delay: 500
                        ToolTip.text: "双击删除"
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton
                            onClicked: {
                                root.deleteSession(modelData.session_id)
                            }
                        }
                    }
                }
            }
        }

        // 右侧消息区域
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: "transparent"

            ListView {
                id: messageListView
                anchors.fill: parent
                clip: true
                model: root.messageModel
                ScrollBar.vertical: GlassScrollBar { }
                spacing: 0

                Rectangle {
                    anchors.centerIn: parent
                    width: 220
                    height: 86
                    radius: 10
                    color: Qt.rgba(1, 1, 1, 0.20)
                    border.color: Qt.rgba(1, 1, 1, 0.42)
                    visible: root.messageModel && root.messageModel.rowCount() === 0

                    Label {
                        anchors.centerIn: parent
                        text: root.currentSessionId.length > 0
                              ? "开始和 AI 聊天吧"
                              : "选择或创建会话"
                        color: "#6a7b92"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                delegate: AgentMessageDelegate {
                    width: messageListView.width
                    msgId: model.msgId || ""
                    content: model.content || ""
                    role: model.role || "assistant"
                    isUser: model.role === "user"
                    isAssistant: model.role === "assistant"
                    isStreaming: model.isStreaming || false
                    streamingContent: model.streamingContent || model.content || ""
                    createdAt: model.createdAt || 0
                    sourcesJson: model.sources || ""
                }

                onCountChanged: {
                    if (count > 0 && root.currentSessionId.length > 0) {
                        Qt.callLater(function() {
                            messageListView.positionViewAtEnd()
                        })
                    }
                }
            }

            // 加载指示器
            Rectangle {
                anchors.centerIn: parent
                width: 60
                height: 60
                radius: 30
                color: Qt.rgba(1, 1, 1, 0.8)
                visible: root.loading || root.streaming

                Label {
                    anchors.centerIn: parent
                    text: root.streaming ? "..." : ""
                    color: "#6a7b92"
                    font.pixelSize: 14
                }
            }

            // 错误提示
            Label {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 8
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.errorMsg
                color: "#e84141"
                font.pixelSize: 12
                visible: root.errorMsg.length > 0
                background: Rectangle {
                    anchors.fill: parent
                    anchors.margins: -4
                    radius: 4
                    color: Qt.rgba(1, 0, 0, 0.1)
                }
            }
        }
    }
}
