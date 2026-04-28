import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "pages"

ApplicationWindow {
    id: window
    width: 1560
    height: 960
    visible: true
    title: "Memo_ops"
    color: "#eef1e8"

    header: Rectangle {
        color: "#1f3527"
        height: 74
        RowLayout {
            anchors.fill: parent
            anchors.margins: 18
            Label { text: "Memo_ops"; color: "#f6f4ea"; font.pixelSize: 28; font.bold: true }
            Label { text: "Observability + Loadtest"; color: "#c9d7ca" }
            Label { text: opsApi.baseUrl; color: "#a8baaa"; font.pixelSize: 12 }
            Item { Layout.fillWidth: true }
            Button { text: "Collect"; onClicked: opsApi.collectNow() }
            Button { text: "Import Reports"; onClicked: opsApi.importReports() }
            Button { text: "Import Logs"; onClicked: opsApi.importLogs() }
            Button { text: "Refresh"; onClicked: opsApi.refreshAll() }
            Button {
                text: loadtestRunning ? "Loadtest Running..." : "Start Loadtest"
                enabled: !loadtestRunning
                onClicked: {
                    tabBar.currentIndex = 4  // jump to Loadtests tab
                    stack.currentIndex = 4
                }
                property bool loadtestRunning: opsApi.loadtestRunStatus.status === "running"
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.topMargin: header.height
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#eef1e8" }
            GradientStop { position: 1.0; color: "#d8dfcf" }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 14

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 54
                radius: 16
                color: loadtestRunningColor()
                border.color: loadtestRunningBorderColor()
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    Label {
                        text: {
                            if (opsApi.busy) return "Syncing..."
                            if (loadtestRunning) return "Loadtest running: " + opsApi.loadtestRunStatus.scenario + "  (" + opsApi.loadtestRunStatus.completed_scenarios + "/" + opsApi.loadtestRunStatus.total_scenarios + " scenarios)"
                            return "Idle"
                        }
                        color: opsApi.busy ? "#9c5d14" : (loadtestRunning ? "#7a5a10" : "#2d5a3d")
                        font.bold: true
                    }
                    Label {
                        Layout.fillWidth: true
                        text: opsApi.lastError.length > 0 ? opsApi.lastError : "No client-side errors"
                        color: opsApi.lastError.length > 0 ? "#8c2f24" : "#5c6659"
                        elide: Text.ElideRight
                    }
                }
                property bool loadtestRunning: opsApi.loadtestRunStatus.status === "running"
                function loadtestRunningColor() {
                    if (loadtestRunning) return "#fef9e7"
                    if (opsApi.busy) return "#f4f1e6"
                    return "#faf8ef"
                }
                function loadtestRunningBorderColor() {
                    if (loadtestRunning) return "#e0c87a"
                    if (opsApi.busy) return "#d4dacd"
                    return "#d4dacd"
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 22
                color: "#fbfaf6"
                border.color: "#d2d8cd"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 0

                    TabBar {
                        id: tabBar
                        Layout.fillWidth: true
                        currentIndex: stack.currentIndex
                        onCurrentIndexChanged: stack.currentIndex = currentIndex
                        TabButton { text: "Overview" }
                        TabButton { text: "Monitoring" }
                        TabButton { text: "System" }
                        TabButton { text: "Logs" }
                        TabButton { text: "Loadtests" }
                        TabButton { text: "Config" }
                    }

                    StackLayout {
                        id: stack
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        OverviewPage {
                            opsApi: opsApi
                            onOpenRun: function(runId) {
                                opsApi.selectRun(runId)
                                tabBar.currentIndex = 4
                                stack.currentIndex = 4
                            }
                            onOpenService: function(serviceName, instanceName) {
                                opsApi.selectService(serviceName, instanceName)
                                tabBar.currentIndex = 1
                                stack.currentIndex = 1
                            }
                        }

                        MonitoringPage {
                            opsApi: opsApi
                            onOpenLogs: function(serviceName, instanceName, level) {
                                tabBar.currentIndex = 3
                                stack.currentIndex = 3
                            }
                        }

                        SystemPage {
                            opsApi: opsApi
                            onOpenLogs: function(serviceName, instanceName, level) {
                                tabBar.currentIndex = 3
                                stack.currentIndex = 3
                            }
                        }

                        LogsPage {
                            opsApi: opsApi
                            onOpenService: function(serviceName, instanceName) {
                                opsApi.selectService(serviceName, instanceName)
                                tabBar.currentIndex = 1
                                stack.currentIndex = 1
                            }
                        }

                        LoadtestsPage {
                            opsApi: opsApi
                            onOpenRunPage: function() {
                                tabBar.currentIndex = 4
                                stack.currentIndex = 4
                            }
                        }

                        ConfigPage {
                            opsApi: opsApi
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        opsApi.refreshLoadtestTrend()
        opsApi.refreshLogTrend()
    }
}
