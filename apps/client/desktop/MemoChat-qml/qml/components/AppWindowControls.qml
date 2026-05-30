import QtQuick 2.15

Item {
    id: root
    width: controlsRow.implicitWidth
    height: controlsRow.implicitHeight

    property bool chatPageActive: false
    property real rowSpacing: 20
    property real hoverPadding: 0

    signal petRequested()
    signal minimizeRequested()
    signal maximizeRequested()
    signal closeRequested()

    implicitWidth: controlsRow.implicitWidth + hoverPadding * 2
    implicitHeight: controlsRow.implicitHeight + hoverPadding * 2

    Row {
        id: controlsRow
        anchors.centerIn: parent
        spacing: root.rowSpacing

        LoginIconButton {
            iconSource: "qrc:/icons/ai.png"
            visible: root.chatPageActive
            onClicked: root.petRequested()
        }

        LoginIconButton {
            iconSource: "qrc:/icons/minimize.png"
            onClicked: root.minimizeRequested()
        }

        LoginIconButton {
            iconSource: "qrc:/icons/maximize.png"
            onClicked: root.maximizeRequested()
        }

        LoginIconButton {
            iconSource: "qrc:/icons/close.png"
            onClicked: root.closeRequested()
        }
    }
}
