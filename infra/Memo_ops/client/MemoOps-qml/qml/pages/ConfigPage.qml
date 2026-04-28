import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ScrollView {
    id: root
    required property var opsApi

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
                Label { text: "Platform"; font.pixelSize: 20; font.bold: true }
                Label { text: "OpsServer: " + root.opsApi.baseUrl }
                Label { text: "Last Error: " + (root.opsApi.lastError.length > 0 ? root.opsApi.lastError : "none") }
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
                Label { text: "Data Sources"; font.pixelSize: 20; font.bold: true }
                Label { text: JSON.stringify(root.opsApi.dataSources || {}, null, 2); font.family: "Consolas"; wrapMode: Text.WrapAnywhere }
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
                Label { text: "Debug Payload"; font.pixelSize: 20; font.bold: true }
                Label { text: JSON.stringify(root.opsApi.overview || {}, null, 2); font.family: "Consolas"; wrapMode: Text.WrapAnywhere }
            }
        }
    }
}
