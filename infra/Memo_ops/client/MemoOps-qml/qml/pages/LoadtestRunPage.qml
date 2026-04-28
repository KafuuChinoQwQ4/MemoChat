import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ScrollView {
    id: root
    required property var opsApi
    signal openRun(string runId)
    signal openLoadtests()

    function rows(value) { return value ? value : [] }
    function n(value) { return value === undefined || value === null || value === "" ? 0 : Number(value) }
    function fmtPct(v) { return n(v).toFixed(1) + "%" }

    contentWidth: availableWidth

    ColumnLayout {
        x: 14; y: 14
        width: parent.width - 28
        spacing: 14

        // --- Control bar ---
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 110
            radius: 18; color: "#f4f1e6"; border.color: "#d3d8cf"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14
                spacing: 10
                Label { text: "Loadtest Runner"; font.pixelSize: 20; font.bold: true; color: "#203022" }
                RowLayout {
                    spacing: 12
                    Label { text: "Scenario:" }
                    ComboBox {
                        id: scenarioBox
                        Layout.preferredWidth: 200
                        model: ["all", "login", "tcp", "quic", "http", "auth", "friend", "group", "history", "media", "call", "postgresql", "mysql", "redis"]
                    }
                    Label { text: "Warmup:" }
                    TextField {
                        id: warmupField
                        Layout.preferredWidth: 80
                        placeholderText: "10"
                        text: "10"
                        validator: IntValidator { bottom: 0; top: 100 }
                    }
                    Label { text: "Pool Size:" }
                    TextField {
                        id: poolSizeField
                        Layout.preferredWidth: 80
                        placeholderText: "200"
                        text: "200"
                        validator: IntValidator { bottom: 1; top: 2000 }
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: isRunning ? "Running..." : "Start Loadtest"
                        enabled: !isRunning
                        onClicked: startLoadtest()
                        property bool isRunning: root.opsApi.loadtestRunStatus.status === "running"
                    }
                    Button { text: "View Reports"; onClicked: root.openLoadtests() }
                }
            }
        }

        // --- Run status card ---
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: statusCardHeight()
            radius: 18
            color: statusColor()
            border.color: statusBorderColor()
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 16
                RowLayout {
                    Label { text: "Run Status"; font.pixelSize: 18; font.bold: true; Layout.fillWidth: true }
                    Label { text: root.opsApi.loadtestRunStatus.started_at || ""; color: "#576155" }
                }
                Label {
                    text: "scenario: " + (root.opsApi.loadtestRunStatus.scenario || "-") +
                          "  status: " + (root.opsApi.loadtestRunStatus.status || "-") +
                          "  exit: " + (root.opsApi.loadtestRunStatus.exit_code !== null ? root.opsApi.loadtestRunStatus.exit_code : "-")
                    color: "#576155"
                }
                Label {
                    text: "completed: " + (root.opsApi.loadtestRunStatus.completed_scenarios || 0) +
                          " / " + (root.opsApi.loadtestRunStatus.total_scenarios || 12)
                    color: "#576155"
                }
                Label {
                    text: root.opsApi.loadtestRunStatus.error || ""
                    color: "#8c2f24"; visible: !!root.opsApi.loadtestRunStatus.error
                    wrapMode: Text.WrapAnywhere
                }
                // Report files list
                Label {
                    text: "Reports:"; font.bold: true; visible: rows(root.opsApi.loadtestRunStatus.reports).length > 0
                }
                Repeater {
                    model: rows(root.opsApi.loadtestRunStatus.reports)
                    Label {
                        text: "  " + modelData.file
                        color: "#576155"; font.pixelSize: 11; font.family: "Consolas"
                        visible: !!modelData.file
                    }
                }
            }
            function statusCardHeight() {
                var h = 120
                if (root.opsApi.loadtestRunStatus.error) h += 30
                if (rows(root.opsApi.loadtestRunStatus.reports).length > 0) h += 50
                return h
            }
            function statusColor() {
                var s = root.opsApi.loadtestRunStatus.status
                if (s === "running") return "#fef9e7"
                if (s === "done") return "#e0e8d8"
                if (s === "failed") return "#f0d8d1"
                return "#f4f1e6"
            }
            function statusBorderColor() {
                var s = root.opsApi.loadtestRunStatus.status
                if (s === "running") return "#e0c87a"
                if (s === "done") return "#a8c89a"
                if (s === "failed") return "#e0a090"
                return "#d3d8cf"
            }
        }

        // --- Scenario descriptions ---
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            radius: 18; color: "#f6f4ea"; border.color: "#d3d8cf"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14
                Label { text: "Scenario Reference"; font.pixelSize: 18; font.bold: true }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 4; rowSpacing: 6; columnSpacing: 12
                    Repeater {
                        model: [
                            { name: "all", desc: "Full suite" },
                            { name: "login", desc: "TCP + HTTP login" },
                            { name: "tcp", desc: "TCP persistent socket" },
                            { name: "quic", desc: "QUIC multi-threaded" },
                            { name: "http", desc: "WinHTTP HTTP/1.1" },
                            { name: "auth", desc: "Verify + Register + Reset" },
                            { name: "friend", desc: "Friend apply/auth" },
                            { name: "group", desc: "Group CRUD ops" },
                            { name: "history", desc: "History + ack" },
                            { name: "media", desc: "File upload/download" },
                            { name: "call", desc: "Call invite" },
                            { name: "postgresql", desc: "PG capacity test" },
                            { name: "mysql", desc: "MySQL capacity test" },
                            { name: "redis", desc: "Redis capacity test" },
                        ]
                        delegate: Column {
                            Layout.fillWidth: true
                            Label { text: modelData.name; font.bold: true; font.pixelSize: 12; color: "#203022" }
                            Label { text: modelData.desc; font.pixelSize: 10; color: "#576155" }
                        }
                    }
                }
            }
        }

        Component.onCompleted: {
            // Start polling status if there's an active run
            pollTimer.start()
        }

        Timer {
            id: pollTimer
            interval: 3000; repeat: true
            onTriggered: {
                var runId = root.opsApi.loadtestRunStatus.run_id
                if (runId) {
                    root.opsApi.refreshLoadtestStatus(runId)
                }
            }
        }

        function startLoadtest() {
            root.opsApi.startLoadtest(
                scenarioBox.currentText,
                parseInt(warmupField.text) || 10,
                parseInt(poolSizeField.text) || 200
            )
        }
    }
}
