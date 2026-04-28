import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCharts 2.15

ScrollView {
    id: root
    required property var opsApi
    signal openRun(string runId)
    signal openService(string serviceName, string instanceName)

    function rows(value) { return value ? value : [] }
    function n(value) { return value === undefined || value === null || value === "" ? 0 : Number(value) }
    function shortTs(value) { return value ? String(value).replace("T", " ").replace("+00:00", "").replace("Z", "") : "" }
    function fill(series, axis, items, labelKey, firstKey, secondKey) {
        series.clear()
        axis.clear()
        var first = series.append(firstKey)
        var second = series.append(secondKey)
        for (var i = 0; i < items.length; ++i) {
            first.append(n(items[i][firstKey]))
            second.append(n(items[i][secondKey]))
            axis.append(String(items[i][labelKey]))
        }
    }

    contentWidth: availableWidth

    ColumnLayout {
        x: 14
        y: 14
        width: parent.width - 28
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Repeater {
                model: [
                    { title: "Active Alerts", value: n(root.opsApi.overviewKpis.active_alerts), tone: "#f6d6cf" },
                    { title: "Online Services", value: n(root.opsApi.overviewKpis.online_services), tone: "#dcead6" },
                    { title: "Recent Runs", value: n(root.opsApi.overviewKpis.recent_runs), tone: "#e9e2cb" },
                    { title: "Error Logs / 1h", value: n(root.opsApi.overviewKpis.error_logs_1h), tone: "#e8d8cd" }
                ]
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 108
                    radius: 16
                    color: modelData.tone
                    border.color: "#c9cfc2"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 14
                        Label { text: modelData.title; color: "#576155" }
                        Label { text: modelData.value; font.pixelSize: 34; font.bold: true; color: "#203022" }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 330
            spacing: 14

            Rectangle {
                Layout.preferredWidth: 360
                Layout.fillHeight: true
                radius: 18
                color: "#f4f1e6"
                border.color: "#d3d8cf"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    Label { text: "Service Health"; font.pixelSize: 20; font.bold: true }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: rows(root.opsApi.overview.service_health)
                        spacing: 8
                        clip: true
                        delegate: Rectangle {
                            width: ListView.view.width
                            implicitHeight: 72
                            radius: 12
                            color: modelData.status === "up" ? "#dfe9d8" : "#f2d7d1"
                            MouseArea { anchors.fill: parent; onClicked: root.openService(modelData.service_name, modelData.instance_name) }
                            Column {
                                anchors.fill: parent
                                anchors.margins: 12
                                Label { text: modelData.service_name + " / " + modelData.instance_name; font.bold: true }
                                Label { text: "status=" + modelData.status + " qps=" + Number(modelData.qps).toFixed(2) }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 18
                color: "#f6f4ea"
                border.color: "#d3d8cf"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    Label { text: "Loadtest Pass Trend"; font.pixelSize: 20; font.bold: true }
                    ChartView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        antialiasing: true
                        backgroundColor: "#f6f4ea"
                        BarCategoryAxis { id: loadAxisX }
                        ValuesAxis { id: loadAxisY; min: 0 }
                        BarSeries { id: loadSeries; axisX: loadAxisX; axisY: loadAxisY }
                    }
                    Connections {
                        target: root.opsApi
                        function onOverviewChanged() { root.fill(loadSeries, loadAxisX, root.rows(root.opsApi.overview.loadtest_trend.items), "bucket", "pass_count", "fail_count") }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 18
                color: "#f6f4ea"
                border.color: "#d3d8cf"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    Label { text: "Log Error Trend"; font.pixelSize: 20; font.bold: true }
                    ChartView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        antialiasing: true
                        backgroundColor: "#f6f4ea"
                        BarCategoryAxis { id: logAxisX }
                        ValuesAxis { id: logAxisY; min: 0 }
                        BarSeries { id: logSeries; axisX: logAxisX; axisY: logAxisY }
                    }
                    Connections {
                        target: root.opsApi
                        function onOverviewChanged() { root.fill(logSeries, logAxisX, root.rows(root.opsApi.overview.log_trend.items), "bucket_utc", "total_count", "error_count") }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 250
            spacing: 14
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 18
                color: "#f4f1e6"
                border.color: "#d3d8cf"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    Label { text: "Recent Runs"; font.pixelSize: 20; font.bold: true }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: rows(root.opsApi.overview.recent_runs)
                        spacing: 8
                        clip: true
                        delegate: Rectangle {
                            width: ListView.view.width
                            implicitHeight: 72
                            radius: 12
                            color: modelData.status === "passed" ? "#e0e8d8" : "#efd8d1"
                            MouseArea { anchors.fill: parent; onClicked: root.openRun(modelData.run_id) }
                            Column {
                                anchors.fill: parent
                                anchors.margins: 12
                                Label { text: modelData.suite_name + " / " + modelData.scenario_name; font.bold: true }
                                Label { text: "ok=" + modelData.success_count + " fail=" + modelData.failure_count + " " + root.shortTs(modelData.finished_at) }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 18
                color: "#f4f1e6"
                border.color: "#d3d8cf"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    Label { text: "Alert Summary"; font.pixelSize: 20; font.bold: true }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: rows(root.opsApi.overview.alert_summary)
                        spacing: 8
                        clip: true
                        delegate: Rectangle {
                            width: ListView.view.width
                            implicitHeight: 60
                            radius: 12
                            color: modelData.severity === "critical" ? "#f0d2cb" : "#efe5c7"
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                Label { text: modelData.severity; font.bold: true; Layout.fillWidth: true }
                                Label { text: modelData.count_rows }
                            }
                        }
                    }
                }
            }
        }
    }
}
