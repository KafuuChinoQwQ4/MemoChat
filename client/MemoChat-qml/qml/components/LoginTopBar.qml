import QtQuick 2.15

Item {
    id: root
    signal settingsClicked()

    width: 0
    height: 24

    Row {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: -2
        anchors.verticalCenterOffset: -4
        spacing: 0

        LoginIconButton {
            iconSource: "qrc:/icons/settings.png"
            onClicked: root.settingsClicked()
        }
    }
}
