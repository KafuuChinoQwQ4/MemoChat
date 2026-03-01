import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    implicitWidth: 50
    implicitHeight: 50

    property string normalSource: ""
    property string hoverSource: ""
    property string pressedSource: ""

    signal clicked()

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: !root.enabled ? Qt.rgba(1, 1, 1, 0.08)
                             : mouse.pressed ? Qt.rgba(0.55, 0.72, 0.94, 0.34)
                                             : mouse.containsMouse ? Qt.rgba(0.55, 0.72, 0.94, 0.24)
                                                                  : Qt.rgba(1, 1, 1, 0.16)
        border.color: Qt.rgba(1, 1, 1, 0.40)

        Behavior on color {
            ColorAnimation {
                duration: 120
                easing.type: Easing.OutQuad
            }
        }
    }

    Image {
        anchors.centerIn: parent
        width: parent.width - 14
        height: parent.height - 14
        fillMode: Image.PreserveAspectFit
        source: {
            if (!root.enabled) {
                return root.normalSource
            }
            if (mouse.pressed && root.pressedSource.length > 0) {
                return root.pressedSource
            }
            if (mouse.containsMouse && root.hoverSource.length > 0) {
                return root.hoverSource
            }
            return root.normalSource
        }
        opacity: root.enabled ? 1.0 : 0.45
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: true
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: root.clicked()
    }
}
