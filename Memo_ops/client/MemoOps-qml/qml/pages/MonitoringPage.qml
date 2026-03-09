import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCharts 2.15

ScrollView {
    id: root
    required property var opsApi

    function rows(value) { return value ? value : [] }
    function fill(series, axis, items, valueKey) {
        series.clear()
        axis.clear()
        for (var i = 0; i < items.length; ++i) {
            series.append(i, Number(items[i][valueKey] || 0))
            axis.append(String(items[i].minute_utc), i)
        }
    }

    contentWidth: availableWidth

    ColumnLayout {
        x: 14
        y: 14
        width: parent.width - 28
        spacing: 14

        Rectangle {
            Layout.fillWidth: true
            radius: 18
            color: "#f4f1e6"
            border.color: "#d3d8cf"
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                Label { text: "Service Grid"; font.pixelSize: 20; font.bold: true }
                Flow {
                    width: parent.width
                    spacing: 10
                    Repeater {
                        model: root.opsApi.services
                        delegate: Rectangle {
                            width: 248
                            height: 108
                            radius: 14
                            color: modelData.status === "up" ? "#e0e8d8" : "#f0d8d1"
                            MouseArea { anchors.fill: parent; onClicked: root.opsApi.selectService(modelData.service_name, modelData.instance_name) }
                            Column {
                                anchors.fill: parent
                                anchors.margins: 12
                                Label { text: modelData.service_name + " / " + modelData.instance_name; font.bold: true }
                                Label { text: "cpu=" + Number(modelData.cpu_percent).toFixed(1) + "% qps=" + Number(modelData.qps).toFixed(2) }
                                Label { text: "err=" + Number(modelData.error_rate).toFixed(2) + " p95=" + Number(modelData.latency_p95_ms).toFixed(1) }
                            }
                        }
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 14
            rowSpacing: 14
            Repeater {
                model: [
                    { title: "CPU", key: "cpu_percent_avg" },
                    { title: "QPS", key: "qps_avg" },
                    { title: "Error Rate", key: "error_rate_avg" },
                    { title: "Latency P95", key: "latency_p95_ms_avg" }
                ]
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 240
                    radius: 18
                    color: "#f6f4ea"
                    border.color: "#d3d8cf"
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        Label { text: modelData.title; font.pixelSize: 20; font.bold: true }
                        ChartView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            antialiasing: true
                            backgroundColor: "#f6f4ea"
                            legend.visible: false
                            CategoryAxis { id: axisX }
                            ValuesAxis { id: axisY; min: 0 }
                            LineSeries { id: series; axisX: axisX; axisY: axisY }
                        }
                        Connections {
                            target: root.opsApi
                            function onServiceTrendChanged() { root.fill(series, axisX, root.rows(root.opsApi.serviceTrend.items), modelData.key) }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 240
            radius: 18
            color: "#f6f4ea"
            border.color: "#d3d8cf"
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                Label { text: "Chat Online Users"; font.pixelSize: 20; font.bold: true }
                ChartView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    antialiasing: true
                    backgroundColor: "#f6f4ea"
                    legend.visible: false
                    CategoryAxis { id: onlineAxisX }
                    ValuesAxis { id: onlineAxisY; min: 0 }
                    LineSeries { id: onlineSeries; axisX: onlineAxisX; axisY: onlineAxisY }
                }
                Connections {
                    target: root.opsApi
                    function onServiceTrendChanged() { root.fill(onlineSeries, onlineAxisX, root.rows(root.opsApi.serviceTrend.items), "online_users_avg") }
                }
            }
        }
    }
}
