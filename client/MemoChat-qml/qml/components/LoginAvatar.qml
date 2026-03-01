import QtQuick 2.15
import Qt5Compat.GraphicalEffects

Item {
    id: root
    property url source: "qrc:/res/head_1.jpg"

    width: 96
    height: 96
    transform: Translate {
        id: floatShift
        x: 0
        y: 0
    }

    SequentialAnimation {
        loops: Animation.Infinite
        running: true
        NumberAnimation {
            target: floatShift
            property: "y"
            from: -1.5
            to: 1.5
            duration: 2600
            easing.type: Easing.InOutSine
        }
        NumberAnimation {
            target: floatShift
            property: "y"
            from: 1.5
            to: -1.5
            duration: 2600
            easing.type: Easing.InOutSine
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.color: "#ffffffcc"
        border.width: 2
        antialiasing: true
    }

    Rectangle {
        id: glowRing
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.width: 1
        border.color: Qt.rgba(0.76, 0.90, 1.0, 0.90)
        opacity: 0.24
    }

    SequentialAnimation {
        loops: Animation.Infinite
        running: true
        NumberAnimation {
            target: glowRing
            property: "opacity"
            from: 0.20
            to: 0.46
            duration: 1800
            easing.type: Easing.InOutSine
        }
        NumberAnimation {
            target: glowRing
            property: "opacity"
            from: 0.46
            to: 0.20
            duration: 1800
            easing.type: Easing.InOutSine
        }
    }

    Item {
        id: avatarClip
        anchors.fill: parent
        anchors.margins: 3

        Image {
            id: avatarImage
            anchors.fill: parent
            source: root.source
            fillMode: Image.PreserveAspectCrop
            smooth: true
            mipmap: true
            visible: false
        }

        OpacityMask {
            anchors.fill: parent
            source: avatarImage
            maskSource: Rectangle {
                width: avatarClip.width
                height: avatarClip.height
                radius: width / 2
                color: "black"
            }
        }
    }
}
