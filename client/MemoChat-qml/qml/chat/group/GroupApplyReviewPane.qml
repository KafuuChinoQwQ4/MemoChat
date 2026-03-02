import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

GlassSurface {
    id: root

    signal applyJoinRequested(var groupId, string reason)
    signal reviewRequested(var applyId, bool agree)

    cornerRadius: 10
    blurRadius: 26
    fillColor: Qt.rgba(1, 1, 1, 0.20)
    strokeColor: Qt.rgba(1, 1, 1, 0.42)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Label {
            text: "入群申请"
            color: "#2a3649"
            font.bold: true
            font.pixelSize: 14
        }

        GlassTextField {
            id: groupIdInput
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            backdrop: root.backdrop
            placeholderText: "申请入群 GroupId"
        }

        GlassTextField {
            id: reasonInput
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            backdrop: root.backdrop
            placeholderText: "申请理由（可选）"
        }

        GlassButton {
            Layout.fillWidth: true
            text: "提交入群申请"
            implicitHeight: 30
            cornerRadius: 8
            normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
            hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
            pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
            disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
            onClicked: root.applyJoinRequested(parseInt(groupIdInput.text), reasonInput.text)
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Qt.rgba(1, 1, 1, 0.28)
        }

        GlassTextField {
            id: applyIdInput
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            backdrop: root.backdrop
            placeholderText: "审核单 ApplyId"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            GlassButton {
                Layout.fillWidth: true
                text: "同意"
                implicitHeight: 30
                cornerRadius: 8
                normalColor: Qt.rgba(0.28, 0.70, 0.58, 0.24)
                hoverColor: Qt.rgba(0.28, 0.70, 0.58, 0.34)
                pressedColor: Qt.rgba(0.28, 0.70, 0.58, 0.42)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.reviewRequested(parseInt(applyIdInput.text), true)
            }
            GlassButton {
                Layout.fillWidth: true
                text: "拒绝"
                implicitHeight: 30
                cornerRadius: 8
                normalColor: Qt.rgba(0.82, 0.38, 0.38, 0.22)
                hoverColor: Qt.rgba(0.82, 0.38, 0.38, 0.32)
                pressedColor: Qt.rgba(0.82, 0.38, 0.38, 0.40)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.reviewRequested(parseInt(applyIdInput.text), false)
            }
        }
    }
}
