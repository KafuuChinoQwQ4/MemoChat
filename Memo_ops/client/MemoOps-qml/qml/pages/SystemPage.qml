import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCharts 2.15

ScrollView {
    id: root
    required property var opsApi
    signal openLogs(string serviceName, string instanceName, string level)

    function rows(value) { return value ? value : [] }
    function n(value) { return value === undefined || value === null || value === "" ? 0 : Number(value) }
    function fmtUptime(sec) {
        if (!sec) return "0s"
        var h = Math.floor(sec / 3600)
        var m = Math.floor((sec % 3600) / 60)
        var s = sec % 60
        return (h > 0 ? h + "h " : "") + (m > 0 ? m + "m " : "") + s + "s"
    }

    contentWidth: availableWidth

    ColumnLayout {
        x: 14; y: 14
        width: parent.width - 28
        spacing: 14

        // Header with refresh
        RowLayout {
            Layout.fillWidth: true
            Label { text: "System Metrics"; font.pixelSize: 24; font.bold: true; color: "#203022" }
            Item { Layout.fillWidth: true }
            Label { text: root.opsApi.systemMetrics.length + " services"; color: "#5c6659" }
            Button { text: "Refresh"; onClicked: root.opsApi.refreshSystemMetrics() }
            Button { text: "Auto Refresh"; checked: autoRefresh.checked; onClicked: autoRefresh.checked = !autoRefresh.checked }
            Timer { id: autoRefreshTimer; interval: 5000; repeat: true; running: autoRefresh.checked; onTriggered: root.opsApi.refreshSystemMetrics() }
        }

        // Service cards grid
        GridView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.ceil(root.rows(root.opsApi.systemMetrics).length / 4) * 130 + 20
            cellWidth: 280; cellHeight: 130
            clip: true
            model: root.opsApi.systemMetrics
            delegate: Rectangle {
                width: 268; height: 118
                radius: 14
                color: modelData.status === "up" ? "#e0e8d8" : "#f0d8d1"
                border.color: modelData.status === "up" ? "#a8c89a" : "#e0a090"
                Column {
                    anchors.fill: parent; anchors.margins: 12
                    Label { text: modelData.service || ""; font.bold: true; font.pixelSize: 15; color: "#203022" }
                    Label { text: "status=" + (modelData.status || "down") + "  pid=" + (modelData.pid || "-"); font.pixelSize: 11; color: "#576155" }
                    Label { text: "cpu=" + n(modelData.cpu_pct).toFixed(1) + "%  mem=" + n(modelData.rss_mb).toFixed(0) + "MB"; font.pixelSize: 11; color: "#576155" }
                    Label { text: "sys_cpu=" + n(modelData.cpu_system_pct).toFixed(1) + "%  uptime=" + root.fmtUptime(modelData.uptime_sec); font.pixelSize: 10; color: "#7a8a7b" }
                    Label { text: "port=" + (modelData.port || "-") + "  http_port=" + (modelData.http_port || "-"); font.pixelSize: 10; color: "#7a8a7b" }
                }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: root.openLogs(modelData.service || "", "", "")
                }
            }
        }

        // Memory breakdown chart
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 260
            radius: 18; color: "#f6f4ea"; border.color: "#d3d8cf"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 12
                Label { text: "Memory Overview"; font.pixelSize: 20; font.bold: true }
                ChartView {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    antialiasing: true; backgroundColor: "#f6f4ea"; legend.visible: true; legend.alignment: Qt.AlignBottom
                    PieSeries {
                        id: memSeries
                        slices: [
                            PieSlice { label: "Used"; value: 100 }
                        ]
                    }
                }
                Connections {
                    target: root.opsApi
                    function onSystemMetricsChanged() {
                        var items = root.rows(root.opsApi.systemMetrics)
                        var totalRss = items.reduce(function(acc, x) { return acc + n(x.rss_mb) }, 0)
                        var sysTotal = items.length > 0 ? n(items[0].mem_total_mb) : 0
                        var sysAvail = items.length > 0 ? n(items[0].mem_avail_mb) : 0
                        var usedSys = sysTotal - sysAvail
                        memSeries.clear()
                        var usedSlice = memSeries.append("Used by Services", totalRss)
                        usedSlice.color = "#d4a070"
                        var otherSlice = memSeries.append("System Free", Math.max(0, sysAvail - totalRss))
                        otherSlice.color = "#a0c8a0"
                    }
                }
            }
        }

        // CPU system chart
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 240
            radius: 18; color: "#f6f4ea"; border.color: "#d3d8cf"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 12
                Label { text: "System CPU"; font.pixelSize: 20; font.bold: true }
                ChartView {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    antialiasing: true; backgroundColor: "#f6f4ea"; legend.visible: false
                    CategoryAxis { id: sysCpuAxis; labelPosition: CategoryAxis.AxisLabelsValue }
                    ValuesAxis { id: sysCpuAxisY; min: 0; max: 100 }
                    LineSeries { id: sysCpuSeries; axisX: sysCpuAxis; axisY: sysCpuAxisY }
                }
                Connections {
                    target: root.opsApi
                    function onSystemMetricsChanged() {
                        var items = root.rows(root.opsApi.systemMetrics)
                        sysCpuSeries.clear()
                        sysCpuAxis.clear()
                        for (var i = 0; i < items.length; ++i) {
                            sysCpuSeries.append(i, n(items[i].cpu_pct))
                            sysCpuAxis.append(items[i].service || String(i), i)
                        }
                    }
                }
            }
        }

        // RSS per service chart
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 240
            radius: 18; color: "#f6f4ea"; border.color: "#d3d8cf"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 12
                Label { text: "Memory RSS per Service (MB)"; font.pixelSize: 20; font.bold: true }
                ChartView {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    antialiasing: true; backgroundColor: "#f6f4ea"; legend.visible: false
                    BarCategoryAxis { id: rssAxis }
                    ValuesAxis { id: rssAxisY; min: 0 }
                    BarSeries { id: rssSeries; axisX: rssAxis; axisY: rssAxisY }
                }
                Connections {
                    target: root.opsApi
                    function onSystemMetricsChanged() {
                        var items = root.rows(root.opsApi.systemMetrics)
                        rssSeries.clear()
                        rssAxis.clear()
                        var set = rssSeries.append("RSS MB")
                        for (var i = 0; i < items.length; ++i) {
                            set.append(n(items[i].rss_mb))
                            rssAxis.append(items[i].service || String(i))
                        }
                    }
                }
            }
        }

        Component.onCompleted: {
            root.opsApi.refreshSystemMetrics()
        }
    }
}
