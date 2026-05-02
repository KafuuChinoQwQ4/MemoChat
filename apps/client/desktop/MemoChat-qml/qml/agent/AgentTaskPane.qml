import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root
    color: "transparent"
    clip: true

    property var agentController: null
    property var tasks: []
    property bool busy: false
    property string statusText: ""
    property string errorText: ""

    signal reloadRequested()

    function statusLabel(status) {
        if (status === "queued") return "排队中"
        if (status === "running") return "运行中"
        if (status === "completed") return "已完成"
        if (status === "failed") return "失败"
        if (status === "canceled") return "已取消"
        if (status === "paused") return "已暂停"
        return status && status.length > 0 ? status : "未知"
    }

    function statusColor(status) {
        if (status === "completed") return "#5f9a78"
        if (status === "running" || status === "queued") return "#4f82c4"
        if (status === "failed") return "#c14d4d"
        return "#7d8ba0"
    }

    function formatTime(value) {
        const n = Number(value || 0)
        if (n <= 0) return ""
        return Qt.formatDateTime(new Date(n), "yyyy-MM-dd hh:mm")
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 128
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.22)
            border.color: Qt.rgba(1, 1, 1, 0.36)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                TextArea {
                    id: taskInput
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    placeholderText: "输入要后台执行的任务..."
                    placeholderTextColor: Qt.rgba(106, 123, 146, 0.6)
                    color: "#253247"
                    font.pixelSize: 13
                    wrapMode: TextEdit.Wrap
                    selectByMouse: true
                    background: Rectangle {
                        radius: 8
                        color: Qt.rgba(1, 1, 1, 0.28)
                        border.color: Qt.rgba(1, 1, 1, 0.30)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        Layout.fillWidth: true
                        text: root.errorText.length > 0 ? root.errorText : root.statusText
                        color: root.errorText.length > 0 ? "#c14d4d" : "#6a7b92"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }

                    GlassButton {
                        Layout.preferredWidth: 72
                        Layout.preferredHeight: 28
                        text: root.busy ? "处理中" : "刷新"
                        textPixelSize: 12
                        cornerRadius: 7
                        enabled: !root.busy
                        normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.24)
                        onClicked: root.reloadRequested()
                    }

                    GlassButton {
                        Layout.preferredWidth: 88
                        Layout.preferredHeight: 28
                        text: "创建任务"
                        textPixelSize: 12
                        cornerRadius: 7
                        enabled: !root.busy && taskInput.text.trim().length > 0
                        normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                        hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                        pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                        onClicked: {
                            if (root.agentController) {
                                root.agentController.createAgentTask(taskInput.text, taskInput.text.trim().slice(0, 32))
                                taskInput.text = ""
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.12)
            border.color: Qt.rgba(1, 1, 1, 0.30)

            ListView {
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                model: root.tasks
                spacing: 8
                ScrollBar.vertical: GlassScrollBar { }

                Rectangle {
                    anchors.centerIn: parent
                    width: Math.min(260, parent.width - 24)
                    height: 72
                    radius: 10
                    color: Qt.rgba(1, 1, 1, 0.20)
                    visible: !root.busy && root.errorText.length === 0 && root.tasks.length === 0

                    Label {
                        anchors.centerIn: parent
                        width: parent.width - 24
                        text: "暂无后台任务"
                        color: "#6a7b92"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                delegate: Rectangle {
                    width: ListView.view.width
                    implicitHeight: Math.max(92, taskTitle.implicitHeight + resultPreview.implicitHeight + 56)
                    radius: 8
                    color: taskMouseArea.containsMouse ? Qt.rgba(1, 1, 1, 0.16) : "transparent"
                    border.color: Qt.rgba(1, 1, 1, 0.22)

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Rectangle {
                            Layout.preferredWidth: 8
                            Layout.fillHeight: true
                            radius: 4
                            color: root.statusColor(modelData.status)
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            spacing: 5

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    id: taskTitle
                                    Layout.fillWidth: true
                                    text: modelData.title || modelData.task_id || "后台任务"
                                    color: "#2a3649"
                                    font.pixelSize: 13
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Label {
                                    text: root.statusLabel(modelData.status)
                                    color: root.statusColor(modelData.status)
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }

                            Label {
                                id: resultPreview
                                Layout.fillWidth: true
                                text: {
                                    if (modelData.error && modelData.error.length > 0) return modelData.error
                                    if (modelData.result && modelData.result.content) return modelData.result.content
                                    return modelData.description || ""
                                }
                                color: modelData.error && modelData.error.length > 0 ? "#c14d4d" : "#253247"
                                font.pixelSize: 12
                                wrapMode: Text.Wrap
                                maximumLineCount: 3
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.formatTime(modelData.updated_at || modelData.created_at)
                                color: "#7d8ba0"
                                font.pixelSize: 11
                                visible: text.length > 0
                            }
                        }

                        ColumnLayout {
                            Layout.preferredWidth: 62
                            spacing: 6

                            GlassButton {
                                Layout.preferredWidth: 62
                                Layout.preferredHeight: 26
                                text: "取消"
                                textPixelSize: 11
                                cornerRadius: 7
                                enabled: !root.busy && (modelData.status === "queued" || modelData.status === "running" || modelData.status === "paused")
                                normalColor: Qt.rgba(0.89, 0.27, 0.27, 0.20)
                                hoverColor: Qt.rgba(0.89, 0.27, 0.27, 0.30)
                                pressedColor: Qt.rgba(0.89, 0.27, 0.27, 0.40)
                                onClicked: {
                                    if (root.agentController && modelData.task_id) {
                                        root.agentController.cancelAgentTask(modelData.task_id)
                                    }
                                }
                            }

                            GlassButton {
                                Layout.preferredWidth: 62
                                Layout.preferredHeight: 26
                                text: "恢复"
                                textPixelSize: 11
                                cornerRadius: 7
                                enabled: !root.busy && (modelData.status === "failed" || modelData.status === "canceled" || modelData.status === "paused")
                                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                                onClicked: {
                                    if (root.agentController && modelData.task_id) {
                                        root.agentController.resumeAgentTask(modelData.task_id)
                                    }
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: taskMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }
                }
            }
        }
    }
}
