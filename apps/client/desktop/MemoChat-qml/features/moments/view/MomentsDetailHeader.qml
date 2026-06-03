import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "transparent"

    signal closeRequested()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 8
        spacing: 8

        Label {
            text: "朋友圈详情"
            font.pixelSize: 15
            font.weight: Font.Medium
            color: "#1a1a1a"
        }

        Item { Layout.fillWidth: true }

        ToolButton {
            icon.source: "qrc:/icons/close.png"
            icon.width: 18
            icon.height: 18
            onClicked: root.closeRequested()
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Qt.rgba(0.88, 0.88, 0.92, 0.6)
    }
}
