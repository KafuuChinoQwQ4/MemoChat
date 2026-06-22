import QtQuick 2.15
import QtQuick.Controls 2.15

ScrollBar {
    id: root
    policy: ScrollBar.AsNeeded
    minimumSize: 0.08
    padding: 1
    width: 8

    // 常态隐藏，仅在滚动(active)、悬停或按下时淡入显现
    readonly property bool revealed: root.active || root.hovered || root.pressed

    contentItem: Rectangle {
        implicitWidth: 6
        radius: 3
        color: root.pressed ? Qt.rgba(0.41, 0.64, 0.91, 0.62)
                            : root.hovered ? Qt.rgba(0.53, 0.73, 0.95, 0.56)
                                           : Qt.rgba(0.56, 0.66, 0.79, 0.48)
        opacity: root.revealed ? 1.0 : 0.0

        Behavior on color {
            ColorAnimation {
                duration: 120
                easing.type: Easing.OutQuad
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutQuad
            }
        }
    }

    background: Rectangle {
        implicitWidth: 8
        radius: 4
        color: (root.active || root.hovered) ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(1, 1, 1, 0.08)
        opacity: root.revealed ? 1.0 : 0.0

        Behavior on color {
            ColorAnimation {
                duration: 120
                easing.type: Easing.OutQuad
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutQuad
            }
        }
    }
}
