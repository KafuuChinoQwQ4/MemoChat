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

    Image {
        anchors.fill: parent
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
        onClicked: root.clicked()
    }
}
