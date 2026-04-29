import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root
    color: "transparent"

    property string traceId: ""
    property string skill: ""
    property string feedbackSummary: ""
    property var observations: []
    property var events: []

    readonly property var layerSpecs: [
        { key: "orchestration", title: "编排层", color: Qt.rgba(0.20, 0.48, 0.78, 0.18) },
        { key: "memory", title: "记忆层", color: Qt.rgba(0.23, 0.58, 0.45, 0.18) },
        { key: "execution", title: "执行层", color: Qt.rgba(0.64, 0.46, 0.18, 0.18) },
        { key: "feedback", title: "反馈层", color: Qt.rgba(0.56, 0.36, 0.70, 0.18) }
    ]

    function eventsForLayer(layerKey) {
        var out = []
        if (!events) {
            return out
        }
        for (var i = 0; i < events.length; ++i) {
            var event = events[i]
            if ((event.layer || "") === layerKey) {
                out.push(event)
            }
        }
        return out
    }

    function durationText(event) {
        var started = Number(event.started_at || 0)
        var finished = Number(event.finished_at || 0)
        if (started <= 0 || finished <= started) {
            return ""
        }
        return Math.max(1, finished - started) + " ms"
    }

    function statusColor(status) {
        if (status === "ok" || status === "success" || status === "completed") {
            return "#2c8a5f"
        }
        if (status === "error" || status === "failed") {
            return "#bd3f3f"
        }
        return "#6a7b92"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                Label {
                    Layout.fillWidth: true
                    text: root.traceId.length > 0 ? root.traceId : "暂无执行轨迹"
                    color: "#2a3649"
                    font.pixelSize: 14
                    font.bold: true
                    elide: Text.ElideMiddle
                }

                Label {
                    Layout.fillWidth: true
                    text: root.skill.length > 0 ? ("技能: " + root.skill) : "等待下一次 AI Agent 调用"
                    color: "#6a7b92"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: traceColumn.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: GlassScrollBar { }

            Column {
                id: traceColumn
                width: parent.width
                spacing: 8

                Repeater {
                    model: root.layerSpecs

                    delegate: Rectangle {
                        width: traceColumn.width
                        implicitHeight: layerColumn.implicitHeight + 18
                        radius: 8
                        color: modelData.color
                        border.color: Qt.rgba(1, 1, 1, 0.38)

                        property var layerEvents: root.eventsForLayer(modelData.key)

                        Column {
                            id: layerColumn
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 9
                            spacing: 7

                            Row {
                                width: parent.width
                                spacing: 8

                                Label {
                                    width: parent.width - countLabel.width - 10
                                    text: modelData.title
                                    color: "#2a3649"
                                    font.pixelSize: 13
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Label {
                                    id: countLabel
                                    text: layerEvents.length + " 项"
                                    color: "#6a7b92"
                                    font.pixelSize: 11
                                }
                            }

                            Label {
                                width: parent.width
                                visible: layerEvents.length === 0
                                text: "本次调用没有该层事件"
                                color: "#6a7b92"
                                font.pixelSize: 12
                            }

                            Repeater {
                                model: layerEvents

                                delegate: Rectangle {
                                    width: layerColumn.width
                                    implicitHeight: eventColumn.implicitHeight + 14
                                    radius: 7
                                    color: Qt.rgba(1, 1, 1, 0.30)
                                    border.color: Qt.rgba(1, 1, 1, 0.30)

                                    Column {
                                        id: eventColumn
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 7
                                        spacing: 4

                                        Row {
                                            width: parent.width
                                            spacing: 8

                                            Label {
                                                width: parent.width - statusLabel.width - durationLabel.width - 20
                                                text: modelData.name || "event"
                                                color: "#253247"
                                                font.pixelSize: 12
                                                font.bold: true
                                                elide: Text.ElideRight
                                            }

                                            Label {
                                                id: statusLabel
                                                text: modelData.status || "-"
                                                color: root.statusColor(modelData.status || "")
                                                font.pixelSize: 11
                                                font.bold: true
                                            }

                                            Label {
                                                id: durationLabel
                                                text: root.durationText(modelData)
                                                color: "#6a7b92"
                                                font.pixelSize: 11
                                                visible: text.length > 0
                                            }
                                        }

                                        Label {
                                            width: parent.width
                                            text: modelData.summary || modelData.detail || ""
                                            visible: text.length > 0
                                            color: "#4e5d74"
                                            font.pixelSize: 12
                                            wrapMode: Text.Wrap
                                            maximumLineCount: 3
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    width: traceColumn.width
                    implicitHeight: feedbackColumn.implicitHeight + 18
                    radius: 8
                    color: Qt.rgba(1, 1, 1, 0.20)
                    border.color: Qt.rgba(1, 1, 1, 0.34)
                    visible: root.feedbackSummary.length > 0 || (root.observations && root.observations.length > 0)

                    Column {
                        id: feedbackColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 9
                        spacing: 7

                        Label {
                            width: parent.width
                            text: "观察与反馈"
                            color: "#2a3649"
                            font.pixelSize: 13
                            font.bold: true
                        }

                        Label {
                            width: parent.width
                            visible: root.feedbackSummary.length > 0
                            text: root.feedbackSummary
                            color: "#4e5d74"
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                        }

                        Repeater {
                            model: root.observations || []

                            delegate: Label {
                                width: feedbackColumn.width
                                text: typeof modelData === "string" ? modelData : JSON.stringify(modelData)
                                color: "#6a7b92"
                                font.pixelSize: 12
                                wrapMode: Text.Wrap
                                maximumLineCount: 3
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }
    }
}
