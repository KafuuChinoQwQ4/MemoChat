import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ToolButton {
    id: control

    property string iconSource: ""
    property bool iconActive: enabled

    Layout.preferredWidth: 30
    Layout.preferredHeight: 30
    hoverEnabled: true
    scale: down ? 0.96 : (hovered ? 1.02 : 1.0)

    Behavior on scale {
        NumberAnimation {
            duration: 110
            easing.type: Easing.OutQuad
        }
    }

    HoverHandler {
        cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }

    contentItem: Image {
        source: control.iconSource
        fillMode: Image.PreserveAspectFit
        sourceSize.width: 16
        sourceSize.height: 16
        opacity: control.iconActive ? 0.95 : 0.45
    }

    background: Rectangle {
        radius: 8
        color: !control.enabled ? Qt.rgba(1, 1, 1, 0.08)
                                : control.down ? Qt.rgba(0.35, 0.61, 0.90, 0.30)
                                               : control.hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                                                                 : "transparent"

        Behavior on color {
            ColorAnimation {
                duration: 110
                easing.type: Easing.OutQuad
            }
        }
    }
}
