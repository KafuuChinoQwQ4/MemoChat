import QtQuick 2.15
import QtQuick.Window 2.15
import "qrc:/qml/components"

Item {
    id: root
    signal settingsClicked()
    signal dragMoveRequested()

    width: 0
    height: 24

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.OpenHandCursor
        onPressed: function(mouse) {
            root.dragMoveRequested()
            mouse.accepted = true
        }
    }

    Row {
        z: 1
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: -16
        anchors.verticalCenterOffset: -12
        spacing: 0

        LoginIconButton {
            iconSource: "qrc:/icons/settings.png"
            onClicked: root.settingsClicked()
        }
    }
}
