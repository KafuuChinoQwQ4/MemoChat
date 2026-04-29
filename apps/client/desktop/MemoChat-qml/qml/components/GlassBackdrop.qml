import QtQuick 2.15

Item {
    id: root
    clip: true
    property real pinkProgress: 0.0

    function mixColor(fromColor, toColor) {
        var p = Math.max(0, Math.min(1, root.pinkProgress))
        return Qt.rgba(fromColor.r + (toColor.r - fromColor.r) * p,
                       fromColor.g + (toColor.g - fromColor.g) * p,
                       fromColor.b + (toColor.b - fromColor.b) * p,
                       fromColor.a + (toColor.a - fromColor.a) * p)
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: root.mixColor(Qt.rgba(0.92, 0.94, 0.97, 0.16),
                                     Qt.rgba(1.0, 0.76, 0.86, 0.30))
            }
            GradientStop {
                position: 1.0
                color: root.mixColor(Qt.rgba(0.94, 0.93, 0.95, 0.14),
                                     Qt.rgba(1.0, 0.82, 0.90, 0.26))
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: root.mixColor(Qt.rgba(1, 1, 1, 0.06),
                                     Qt.rgba(1.0, 0.72, 0.84, 0.14))
            }
            GradientStop {
                position: 0.45
                color: root.mixColor(Qt.rgba(1, 1, 1, 0.02),
                                     Qt.rgba(1.0, 0.68, 0.82, 0.12))
            }
            GradientStop {
                position: 1.0
                color: root.mixColor(Qt.rgba(1, 1, 1, 0.04),
                                     Qt.rgba(1.0, 0.78, 0.88, 0.12))
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: root.mixColor(Qt.rgba(1, 1, 1, 0.02),
                             Qt.rgba(1.0, 0.70, 0.84, 0.10))
        border.width: 0
    }
}
