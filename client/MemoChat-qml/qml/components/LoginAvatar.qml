import QtQuick 2.15
import Qt5Compat.GraphicalEffects

Item {
    id: root
    property url source: "qrc:/res/head_1.jpg"

    width: 96
    height: 96

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.color: "#ffffffcc"
        border.width: 2
        antialiasing: true
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
