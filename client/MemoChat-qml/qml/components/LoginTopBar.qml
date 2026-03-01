import QtQuick 2.15

Item {
    id: root
    signal settingsClicked()
    signal closeClicked()

    width: 0
    height: 24

    Row {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: -8
        anchors.verticalCenterOffset: -4
        spacing: 22

        LoginIconButton {
            iconSource: "qrc:/icons/settings.png"
            onClicked: root.settingsClicked()
        }

        LoginIconButton {
            iconSource: "qrc:/icons/close.png"
            onClicked: root.closeClicked()
        }
    }
}
