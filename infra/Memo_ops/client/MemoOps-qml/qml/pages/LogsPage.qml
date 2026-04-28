import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCharts 2.15

SplitView {
    id: root
    required property var opsApi
    signal openService(string serviceName, string instanceName)
    property var selectedLogItem: ({})

    function rows(value) { return value ? value : [] }
    function clipText(value, limit) {
        var text = value ? String(value) : ""
        return text.length <= limit ? text : text.slice(0, limit - 1) + "…"
    }
    function fill(series, axis, items) {
        series.clear()
        axis.clear()
        var first = series.append("total_count")
        var second = series.append("error_count")
        for (var i = 0; i < items.length; ++i) {
            first.append(Number(items[i].total_count || 0))
            second.append(Number(items[i].error_count || 0))
            axis.append(String(items[i].bucket_utc))
        }
    }

    orientation: Qt.Horizontal

    Rectangle {
        SplitView.preferredWidth: 420
        color: "#f4f1e6"
        radius: 18
        border.color: "#d3d8cf"
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            Label { text: "Filters"; font.pixelSize: 20; font.bold: true }
            TextField { id: serviceField; placeholderText: "service" }
            TextField { id: instanceField; placeholderText: "instance" }
            RowLayout {
                Layout.fillWidth: true
                ComboBox { id: levelBox; Layout.fillWidth: true; model: ["", "info", "warn", "error"] }
                TextField { id: eventField; Layout.fillWidth: true; placeholderText: "event" }
            }
            TextField { id: traceField; placeholderText: "trace_id" }
            TextField { id: requestField; placeholderText: "request_id" }
            TextField { id: keywordField; placeholderText: "keyword" }
            RowLayout {
                Layout.fillWidth: true
                TextField { id: fromField; Layout.fillWidth: true; placeholderText: "from_utc" }
                TextField { id: toField; Layout.fillWidth: true; placeholderText: "to_utc" }
            }
            RowLayout {
                Layout.fillWidth: true
                Button {
                    text: "Search"
                    onClicked: {
                        root.opsApi.refreshLogSearch(serviceField.text, instanceField.text, levelBox.currentText,
                                                     eventField.text, traceField.text, requestField.text,
                                                     keywordField.text, fromField.text, toField.text, 1, 100, "ts_desc")
                        root.opsApi.refreshLogTrend(serviceField.text, instanceField.text, levelBox.currentText,
                                                    eventField.text, traceField.text, requestField.text,
                                                    keywordField.text, fromField.text, toField.text)
                    }
                }
                Button {
                    text: "Clear"
                    onClicked: {
                        serviceField.text = ""
                        instanceField.text = ""
                        levelBox.currentIndex = 0
                        eventField.text = ""
                        traceField.text = ""
                        requestField.text = ""
                        keywordField.text = ""
                        fromField.text = ""
                        toField.text = ""
                        root.opsApi.refreshLogSearch()
                        root.opsApi.refreshLogTrend()
                    }
                }
            }
            // Tail controls
    property bool tailing: false
    RowLayout {
        Layout.fillWidth: true
        Button {
            text: root.tailing ? "Stop Tail" : "Live Tail"
            onClicked: {
                if (root.tailing) {
                    root.opsApi.stopTailLogs()
                    tailTimer.stop()
                    root.tailing = false
                } else {
                    root.tailing = true
                    tailTimer.restart()
                }
            }
        }
        ComboBox {
            id: tailLevelBox
            Layout.preferredWidth: 80
            model: ["", "info", "warn", "error"]
        }
        Label { text: "tail:" + root.opsApi.tailLogItems.length + " lines"; color: root.tailing ? "#9c5d14" : "#5c6659"; font.pixelSize: 11 }
        Timer { id: tailTimer; interval: 3000; repeat: true; triggeredOnStart: true
            onTriggered: {
                root.opsApi.fetchTailLogs(serviceField.text, tailLevelBox.currentText, 50)
            }
        }
    }
            Label { text: "Total " + Number(root.opsApi.logSearchResult.total || 0) + " logs"; color: "#5c6659" }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: root.opsApi.logs
                spacing: 8
                clip: true
                delegate: Rectangle {
                    width: ListView.view.width
                    implicitHeight: 90
                    radius: 12
                    color: modelData.level === "error" ? "#f0d8d2" : "#e4eadf"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.selectedLogItem = modelData
                            if (modelData.trace_id && modelData.trace_id.length > 0)
                                root.opsApi.refreshTrace(modelData.trace_id)
                        }
                    }
                    Column {
                        anchors.fill: parent
                        anchors.margins: 12
                        Label { text: String(modelData.ts_utc || "") + "  " + modelData.service_name + " / " + modelData.level; font.bold: true }
                        Label { text: root.clipText(modelData.message, 110); color: "#4f5b4a" }
                        Label { text: root.clipText(modelData.trace_id || "", 42); color: "#687265" }
                    }
                }
            }
        }
    }

    ScrollView {
        SplitView.fillWidth: true
        contentWidth: availableWidth
        ColumnLayout {
            x: 14
            y: 14
            width: parent.width - 28
            spacing: 14

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 280
                radius: 18
                color: "#f6f4ea"
                border.color: "#d3d8cf"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    Label { text: "Log Trend"; font.pixelSize: 20; font.bold: true }
                    ChartView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        antialiasing: true
                        backgroundColor: "#f6f4ea"
                        BarCategoryAxis { id: axisX }
                        ValuesAxis { id: axisY; min: 0 }
                        BarSeries { id: series; axisX: axisX; axisY: axisY }
                    }
                    Connections {
                        target: root.opsApi
                        function onLogTrendChanged() { root.fill(series, axisX, root.rows(root.opsApi.logTrend.items)) }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                radius: 18
                color: "#f4f1e6"
                border.color: "#d3d8cf"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    Label { text: "Log Detail"; font.pixelSize: 20; font.bold: true }
                    Label { text: JSON.stringify(root.selectedLogItem || {}, null, 2); font.family: "Consolas"; wrapMode: Text.WrapAnywhere; font.pixelSize: 12 }
                    RowLayout {
                        Button {
                            text: "Open Related Service Trend"
                            enabled: !!root.selectedLogItem.service_name
                            onClicked: root.openService(root.selectedLogItem.service_name || "", root.selectedLogItem.service_instance || "")
                        }
                        Button {
                            text: "Trace Logs"
                            enabled: !!root.selectedLogItem.trace_id
                            onClicked: {
                                if (root.selectedLogItem.trace_id) {
                                    traceField.text = root.selectedLogItem.trace_id
                                    root.opsApi.refreshLogSearch(serviceField.text, instanceField.text, levelBox.currentText,
                                                                 eventField.text, root.selectedLogItem.trace_id, requestField.text,
                                                                 keywordField.text, fromField.text, toField.text, 1, 100, "ts_desc")
                                }
                            }
                        }
                    }
                    Label { text: "Trace"; font.pixelSize: 18; font.bold: true }
                    Label { text: JSON.stringify(root.opsApi.selectedTrace || {}, null, 2); font.family: "Consolas"; wrapMode: Text.WrapAnywhere; font.pixelSize: 12 }
                }
            }

            // Live tail panel
            Rectangle {
                Layout.fillWidth: true
                radius: 18
                color: root.tailing ? "#fef9e7" : "#f4f1e6"
                border.color: root.tailing ? "#e0c87a" : "#d3d8cf"
                visible: root.opsApi.tailLogItems.length > 0
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    Label { text: "Live Tail (" + root.opsApi.tailLogItems.length + " lines)"; font.pixelSize: 18; font.bold: true }
                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 200
                        clip: true
                        model: root.opsApi.tailLogItems
                        delegate: Rectangle {
                            width: ListView.view.width
                            implicitHeight: 50
                            color: modelData.level === "error" ? "#f0d8d2" : "#e4eadf"
                            radius: 8
                            Column {
                                anchors.fill: parent; anchors.margins: 6
                                Label { text: String(modelData.ts_utc || "").slice(0, 23) + " " + (modelData.service_name || "") + " [" + (modelData.level || "") + "]"; font.pixelSize: 10; font.bold: true }
                                Label { text: root.clipText(modelData.message || "", 120); font.pixelSize: 10; color: "#4f5b4a"; font.family: "Consolas" }
                            }
                        }
                    }
                }
            }
        }
    }
}
