import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

GlassSurface {
    id: root
    property string groupName: ""
    property string statusText: ""
    property bool statusError: false

    signal refreshRequested()
    signal loadHistoryRequested()
    signal updateAnnouncementRequested(string announcement)
    signal quitRequested()

    cornerRadius: 10
    blurRadius: 26
    fillColor: Qt.rgba(1, 1, 1, 0.20)
    strokeColor: Qt.rgba(1, 1, 1, 0.42)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: root.groupName.length > 0 ? root.groupName : "群聊"
            color: "#2a3649"
            font.bold: true
            font.pixelSize: 15
            elide: Text.ElideRight
        }

        Label {
            Layout.fillWidth: true
            text: root.statusText
            visible: text.length > 0
            wrapMode: Text.Wrap
            color: root.statusError ? "#cc4a4a" : "#2a7f62"
            font.pixelSize: 12
        }

        GlassTextField {
            id: announcementInput
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            backdrop: root.backdrop
            placeholderText: "群公告（最多1000字）"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            GlassButton {
                Layout.fillWidth: true
                text: "刷新"
                implicitHeight: 30
                cornerRadius: 8
                normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
                hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
                pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.refreshRequested()
            }
            GlassButton {
                Layout.fillWidth: true
                text: "历史"
                implicitHeight: 30
                cornerRadius: 8
                normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
                hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
                pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.loadHistoryRequested()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            GlassButton {
                Layout.fillWidth: true
                text: "更新公告"
                implicitHeight: 30
                cornerRadius: 8
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.updateAnnouncementRequested(announcementInput.text)
            }
            GlassButton {
                Layout.fillWidth: true
                text: "退群"
                implicitHeight: 30
                cornerRadius: 8
                normalColor: Qt.rgba(0.82, 0.38, 0.38, 0.22)
                hoverColor: Qt.rgba(0.82, 0.38, 0.38, 0.32)
                pressedColor: Qt.rgba(0.82, 0.38, 0.38, 0.40)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.quitRequested()
            }
        }
    }
}
