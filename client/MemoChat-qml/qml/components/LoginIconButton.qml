import QtQuick 2.15

Item {
    id: root
    property url iconSource: ""
    signal clicked()
    property bool hovering: false
    property bool pressed: false

    width: 14
    height: 14

    Rectangle {
        width: 28
        height: 28
        radius: 14
        anchors.centerIn: parent
        color: Qt.rgba(0.65, 0.78, 0.93, 0.22)
        opacity: root.hovering ? 0.95 : 0.0

        Behavior on opacity {
            NumberAnimation {
                duration: 130
                easing.type: Easing.OutQuad
            }
        }
    }

    Image {
        id: iconImage
        anchors.fill: parent
        source: root.iconSource
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        opacity: root.pressed ? 0.78 : 1.0
        scale: root.pressed ? 0.92 : (root.hovering ? 1.06 : 1.0)

        Behavior on scale {
            NumberAnimation {
                duration: 120
                easing.type: Easing.OutQuad
            }
        }
        Behavior on opacity {
            NumberAnimation {
                duration: 120
                easing.type: Easing.OutQuad
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: root.hovering = true
        onExited: {
            root.hovering = false
            root.pressed = false
        }
        onPressed: root.pressed = true
        onReleased: root.pressed = false
        onClicked: root.clicked()
    }
}
