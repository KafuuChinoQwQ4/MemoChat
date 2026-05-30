import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root

    property string cameraDiagnosticText: ""
    property string cloudVisionDiagnosticText: ""
    property string retentionDiagnosticText: ""
    property bool cloudVisionEnabled: false
    property bool debugRetentionEnabled: false

    Layout.fillWidth: true
    implicitHeight: privacyDiagnostics.implicitHeight + 18
    radius: 8
    antialiasing: true
    color: Qt.rgba(0.45, 0.70, 0.73, 0.10)
    border.color: Qt.rgba(0.45, 0.70, 0.73, 0.24)

    ColumnLayout {
        id: privacyDiagnostics
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 9
        spacing: 4

        Label {
            Layout.fillWidth: true
            text: "视觉隐私"
            color: "#4b3042"
            font.pixelSize: 12
            font.bold: true
            elide: Text.ElideRight
        }

        Label {
            Layout.fillWidth: true
            text: root.cameraDiagnosticText
            color: "#6a7b92"
            font.pixelSize: 11
            elide: Text.ElideRight
        }

        Label {
            Layout.fillWidth: true
            text: root.cloudVisionDiagnosticText
            color: root.cloudVisionEnabled ? "#4d7f5c" : "#8f7c88"
            font.pixelSize: 11
            elide: Text.ElideRight
        }

        Label {
            Layout.fillWidth: true
            text: root.retentionDiagnosticText
            color: root.debugRetentionEnabled ? "#b46d63" : "#6a7b92"
            font.pixelSize: 11
            elide: Text.ElideRight
        }
    }
}
