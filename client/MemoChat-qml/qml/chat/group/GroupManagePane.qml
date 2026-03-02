import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

GlassSurface {
    id: root

    signal inviteRequested(int uid, string reason)
    signal setAdminRequested(int uid, bool isAdmin)
    signal muteRequested(int uid, int muteSeconds)
    signal kickRequested(int uid)

    cornerRadius: 10
    blurRadius: 26
    fillColor: Qt.rgba(1, 1, 1, 0.20)
    strokeColor: Qt.rgba(1, 1, 1, 0.42)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Label {
            text: "群管理"
            color: "#2a3649"
            font.bold: true
            font.pixelSize: 14
        }

        GlassTextField {
            id: uidInput
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            backdrop: root.backdrop
            placeholderText: "目标 UID"
        }

        GlassTextField {
            id: reasonInput
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            backdrop: root.backdrop
            placeholderText: "理由（可选）"
        }

        GlassTextField {
            id: muteInput
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            backdrop: root.backdrop
            placeholderText: "禁言秒数（0=解禁）"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            GlassButton {
                Layout.fillWidth: true
                text: "邀请"
                implicitHeight: 30
                cornerRadius: 8
                normalColor: Qt.rgba(0.28, 0.70, 0.58, 0.24)
                hoverColor: Qt.rgba(0.28, 0.70, 0.58, 0.34)
                pressedColor: Qt.rgba(0.28, 0.70, 0.58, 0.42)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.inviteRequested(parseInt(uidInput.text), reasonInput.text)
            }
            GlassButton {
                Layout.fillWidth: true
                text: "设管理员"
                implicitHeight: 30
                cornerRadius: 8
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.setAdminRequested(parseInt(uidInput.text), true)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            GlassButton {
                Layout.fillWidth: true
                text: "撤管理员"
                implicitHeight: 30
                cornerRadius: 8
                normalColor: Qt.rgba(0.48, 0.56, 0.66, 0.20)
                hoverColor: Qt.rgba(0.48, 0.56, 0.66, 0.30)
                pressedColor: Qt.rgba(0.48, 0.56, 0.66, 0.38)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.setAdminRequested(parseInt(uidInput.text), false)
            }
            GlassButton {
                Layout.fillWidth: true
                text: "禁言"
                implicitHeight: 30
                cornerRadius: 8
                normalColor: Qt.rgba(0.83, 0.61, 0.24, 0.24)
                hoverColor: Qt.rgba(0.83, 0.61, 0.24, 0.34)
                pressedColor: Qt.rgba(0.83, 0.61, 0.24, 0.42)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.muteRequested(parseInt(uidInput.text), parseInt(muteInput.text))
            }
        }

        GlassButton {
            Layout.fillWidth: true
            text: "踢出群聊"
            implicitHeight: 30
            cornerRadius: 8
            normalColor: Qt.rgba(0.82, 0.38, 0.38, 0.22)
            hoverColor: Qt.rgba(0.82, 0.38, 0.38, 0.32)
            pressedColor: Qt.rgba(0.82, 0.38, 0.38, 0.40)
            disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
            onClicked: root.kickRequested(parseInt(uidInput.text))
        }
    }
}
