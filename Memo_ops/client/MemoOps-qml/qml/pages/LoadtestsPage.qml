import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCharts 2.15

SplitView {
    id: root
    required property var opsApi

    function rows(value) { return value ? value : [] }
    function clipText(value, limit) {
        var text = value ? String(value) : ""
        return text.length <= limit ? text : text.slice(0, limit - 1) + "…"
    }
    function fill(series, axis, items) {
        series.clear()
        axis.clear()
        var first = series.append("pass_count")
        var second = series.append("fail_count")
        for (var i = 0; i < items.length; ++i) {
            first.append(Number(items[i].pass_count || 0))
            second.append(Number(items[i].fail_count || 0))
            axis.append(String(items[i].bucket))
        }
    }

    orientation: Qt.Horizontal

    Rectangle {
        SplitView.preferredWidth: 380
        color: "#f4f1e6"
        radius: 18
        border.color: "#d3d8cf"
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            Label { text: "Runs"; font.pixelSize: 20; font.bold: true }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: root.opsApi.runs
                spacing: 8
                clip: true
                delegate: Rectangle {
                    width: ListView.view.width
                    implicitHeight: 96
                    radius: 12
                    color: modelData.status === "passed" ? "#dde7d5" : "#f0d9d2"
                    MouseArea { anchors.fill: parent; onClicked: root.opsApi.selectRun(modelData.run_id) }
                    Column {
                        anchors.fill: parent
                        anchors.margins: 12
                        Label { text: modelData.suite_name + " / " + modelData.scenario_name; font.bold: true }
                        Label { text: "status=" + modelData.status + " ok=" + modelData.success_count + " fail=" + modelData.failure_count }
                        Label { text: root.clipText(modelData.file_path, 48); color: "#657060" }
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
                    Label { text: "Run Trend"; font.pixelSize: 20; font.bold: true }
                    ChartView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        antialiasing: true
                        backgroundColor: "#f6f4ea"
                        BarCategoryAxis { id: trendAxisX }
                        ValuesAxis { id: trendAxisY; min: 0 }
                        BarSeries { id: trendSeries; axisX: trendAxisX; axisY: trendAxisY }
                    }
                    Connections {
                        target: root.opsApi
                        function onLoadtestTrendChanged() { root.fill(trendSeries, trendAxisX, root.rows(root.opsApi.loadtestTrend.items)) }
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
                    Label { text: "Selected Run"; font.pixelSize: 20; font.bold: true }
                    Label { text: JSON.stringify(root.opsApi.selectedRun.run || {}, null, 2); font.family: "Consolas"; wrapMode: Text.WrapAnywhere }
                    Label { text: "Cases"; font.pixelSize: 18; font.bold: true }
                    Label { text: JSON.stringify(root.opsApi.selectedRun.cases || [], null, 2); font.family: "Consolas"; wrapMode: Text.WrapAnywhere }
                    Label { text: "Errors"; font.pixelSize: 18; font.bold: true }
                    Label { text: JSON.stringify(root.opsApi.selectedRun.errors || [], null, 2); font.family: "Consolas"; wrapMode: Text.WrapAnywhere }
                }
            }
        }
    }
}
