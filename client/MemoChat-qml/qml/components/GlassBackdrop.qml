import QtQuick 2.15

Item {
    id: root
    clip: true

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(0.62, 0.79, 0.97, 0.11) }
            GradientStop { position: 1.0; color: Qt.rgba(0.96, 0.66, 0.90, 0.10) }
        }
    }

    Rectangle {
        id: orbA
        width: 230
        height: 230
        radius: 115
        color: Qt.rgba(0.66, 0.86, 1.0, 0.20)
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: -60
        anchors.topMargin: -34
        transform: Translate {
            id: orbAShift
            x: 0
            y: 0
        }
    }

    NumberAnimation {
        target: orbAShift
        property: "x"
        from: 0
        to: 76
        duration: 9200
        easing.type: Easing.InOutSine
        loops: Animation.Infinite
        running: true
    }
    NumberAnimation {
        target: orbAShift
        property: "y"
        from: 0
        to: 52
        duration: 8600
        easing.type: Easing.InOutSine
        loops: Animation.Infinite
        running: true
    }

    Rectangle {
        id: orbB
        width: 200
        height: 200
        radius: 100
        color: Qt.rgba(1.0, 0.78, 0.87, 0.15)
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: -24
        anchors.bottomMargin: -18
        transform: Translate {
            id: orbBShift
            x: 0
            y: 0
        }
    }

    NumberAnimation {
        target: orbBShift
        property: "x"
        from: 0
        to: -70
        duration: 9800
        easing.type: Easing.InOutSine
        loops: Animation.Infinite
        running: true
    }
    NumberAnimation {
        target: orbBShift
        property: "y"
        from: 0
        to: -56
        duration: 9000
        easing.type: Easing.InOutSine
        loops: Animation.Infinite
        running: true
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.14) }
            GradientStop { position: 0.45; color: Qt.rgba(1, 1, 1, 0.02) }
            GradientStop { position: 1.0; color: Qt.rgba(0.94, 0.96, 1.0, 0.10) }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(1, 1, 1, 0.04)
        border.width: 0
    }
}
