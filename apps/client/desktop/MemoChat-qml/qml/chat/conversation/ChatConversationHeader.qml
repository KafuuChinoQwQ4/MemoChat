import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Rectangle {
    id: root

    property string peerName: ""
    property bool hasCurrentChat: false
    property bool isGroupChat: false
    signal openGroupManageRequested()

    radius: 10
    color: Qt.rgba(1, 1, 1, 0.24)
    border.color: Qt.rgba(1, 1, 1, 0.46)

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 10
        spacing: 10

        Label {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            text: root.peerName.length > 0 ? root.peerName : "聊天"
            color: "#2a3649"
            font.pixelSize: 17
            font.bold: true
            elide: Text.ElideRight
        }

        Item { Layout.fillWidth: true }

        GlassButton {
            id: groupSettingsButton
            Layout.preferredWidth: 32
            Layout.preferredHeight: 28
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            visible: root.hasCurrentChat && root.isGroupChat
            text: "..."
            textPixelSize: 22
            cornerRadius: 9
            normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.22)
            hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.32)
            pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.40)
            disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
            onClicked: root.openGroupManageRequested()

            ToolTip.visible: hovering
            ToolTip.delay: 180
            ToolTip.text: "群设置"
        }
    }
}
