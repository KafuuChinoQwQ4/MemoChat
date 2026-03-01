import QtQuick 2.15

Item {
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(0.62, 0.79, 0.97, 0.11) }
            GradientStop { position: 1.0; color: Qt.rgba(0.96, 0.66, 0.90, 0.10) }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(1, 1, 1, 0.04)
        border.width: 0
    }
}
