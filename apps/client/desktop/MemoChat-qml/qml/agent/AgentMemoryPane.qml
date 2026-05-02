import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root
    color: "transparent"
    clip: true

    property var agentController: null
    property var memories: []
    property bool busy: false
    property string statusText: ""
    property string errorText: ""

    signal reloadRequested()

    function typeLabel(memoryType) {
        if (memoryType === "semantic") {
            return "语义"
        }
        if (memoryType === "episodic") {
            return "片段"
        }
        return memoryType && memoryType.length > 0 ? memoryType : "记忆"
    }

    function sourceLabel(source) {
        if (source === "manual") {
            return "手动记忆"
        }
        if (source === "ai_semantic_memory") {
            return "语义画像"
        }
        if (source === "ai_episodic_memory") {
            return "自动摘要"
        }
        return source || ""
    }

    function formatTime(value) {
        const n = Number(value || 0)
        if (n <= 0) {
            return ""
        }
        const d = new Date(n)
        return Qt.formatDateTime(d, "yyyy-MM-dd hh:mm")
    }

    function saveManualMemory() {
        const text = manualMemoryInput.text.trim()
        if (text.length === 0 || root.busy) {
            return
        }
        if (root.agentController) {
            root.agentController.createMemory(text)
            manualMemoryInput.text = ""
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? statusLabel.implicitHeight + 18 : 0
            radius: 10
            color: root.errorText.length > 0 ? Qt.rgba(0.89, 0.27, 0.27, 0.12) : Qt.rgba(0.35, 0.61, 0.90, 0.14)
            border.color: root.errorText.length > 0 ? Qt.rgba(0.89, 0.27, 0.27, 0.24) : Qt.rgba(0.35, 0.61, 0.90, 0.24)
            visible: root.busy || root.errorText.length > 0 || root.statusText.length > 0

            Label {
                id: statusLabel
                anchors.fill: parent
                anchors.margins: 9
                text: root.errorText.length > 0 ? root.errorText : root.statusText
                color: root.errorText.length > 0 ? "#c14d4d" : "#4e5d74"
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: manualMemoryColumn.implicitHeight + 18
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.18)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.32)

            ColumnLayout {
                id: manualMemoryColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 9
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        Layout.fillWidth: true
                        text: "手动记忆"
                        color: "#2a3649"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    GlassButton {
                        Layout.preferredWidth: 72
                        Layout.preferredHeight: 30
                        text: root.busy ? "保存中" : "保存"
                        textPixelSize: 12
                        cornerRadius: 10
                        enabled: !root.busy && manualMemoryInput.text.trim().length > 0
                        normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                        hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                        pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                        disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.14)
                        onClicked: root.saveManualMemory()
                    }
                }

                TextArea {
                    id: manualMemoryInput
                    Layout.fillWidth: true
                    Layout.preferredHeight: 72
                    placeholderText: "写下希望 AI 长期记住的偏好、背景或约束..."
                    placeholderTextColor: "#6a7b92"
                    color: "#253247"
                    font.pixelSize: 13
                    wrapMode: TextArea.Wrap
                    selectByMouse: true
                    enabled: !root.busy
                    background: Rectangle {
                        radius: 9
                        color: Qt.rgba(1, 1, 1, 0.34)
                        border.width: 1
                        border.color: manualMemoryInput.activeFocus
                                      ? Qt.rgba(0.35, 0.61, 0.90, 0.46)
                                      : Qt.rgba(1, 1, 1, 0.36)
                    }

                    Keys.onReturnPressed: function(event) {
                        if (event.modifiers & Qt.ControlModifier) {
                            root.saveManualMemory()
                            event.accepted = true
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
                text: "可见记忆"
                font.pixelSize: 12
                font.bold: true
                color: "#6a7b92"
            }

            GlassButton {
                Layout.preferredWidth: 72
                Layout.preferredHeight: 28
                text: root.busy ? "加载中" : "刷新"
                textPixelSize: 12
                cornerRadius: 7
                enabled: !root.busy
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                onClicked: root.reloadRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.12)
            border.color: Qt.rgba(1, 1, 1, 0.30)

            ListView {
                id: memoryList
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                model: root.memories
                spacing: 8
                ScrollBar.vertical: GlassScrollBar { }

                Rectangle {
                    anchors.centerIn: parent
                    width: Math.min(260, parent.width - 24)
                    height: 72
                    radius: 10
                    color: Qt.rgba(1, 1, 1, 0.20)
                    visible: !root.busy && root.errorText.length === 0 && root.memories.length === 0

                    Label {
                        anchors.centerIn: parent
                        width: parent.width - 24
                        text: "暂无可见记忆"
                        color: "#6a7b92"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                    }
                }

                delegate: Rectangle {
                    width: ListView.view.width
                    implicitHeight: Math.max(86, memoryContent.implicitHeight + 52)
                    radius: 8
                    color: memoryMouseArea.containsMouse ? Qt.rgba(1, 1, 1, 0.16) : "transparent"
                    border.color: Qt.rgba(1, 1, 1, 0.22)

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Rectangle {
                            Layout.preferredWidth: 8
                            Layout.fillHeight: true
                            radius: 4
                            color: modelData.type === "semantic" ? "#5f9a78" : "#4f82c4"
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            spacing: 5

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Label {
                                    text: root.typeLabel(modelData.type)
                                    color: "#2a3649"
                                    font.pixelSize: 13
                                    font.bold: true
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: root.sourceLabel(modelData.source)
                                    color: "#6a7b92"
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }

                            Label {
                                id: memoryContent
                                Layout.fillWidth: true
                                text: modelData.content || ""
                                color: "#253247"
                                font.pixelSize: 13
                                wrapMode: Text.Wrap
                                maximumLineCount: 4
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.formatTime(modelData.updated_at || modelData.created_at)
                                color: "#7d8ba0"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                visible: text.length > 0
                            }
                        }

                        GlassButton {
                            Layout.preferredWidth: 56
                            Layout.preferredHeight: 28
                            text: "删除"
                            textPixelSize: 11
                            cornerRadius: 7
                            enabled: !root.busy
                            normalColor: Qt.rgba(0.89, 0.27, 0.27, 0.20)
                            hoverColor: Qt.rgba(0.89, 0.27, 0.27, 0.30)
                            pressedColor: Qt.rgba(0.89, 0.27, 0.27, 0.40)
                            onClicked: {
                                if (root.agentController && modelData.memory_id) {
                                    root.agentController.deleteMemory(modelData.memory_id)
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: memoryMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }
                }
            }
        }
    }

}
