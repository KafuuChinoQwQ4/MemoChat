import QtQuick 2.15

Rectangle {
    id: root

    property string fileName: ""
    property real maxWidth: 240
    property bool hovering: fileArea.containsMouse
    property bool pressed: fileArea.pressed

    signal openRequested()

    width: implicitWidth
    height: implicitHeight
    color: root.pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                        : root.hovering ? Qt.rgba(0.35, 0.61, 0.90, 0.12)
                                        : "transparent"
    implicitWidth: Math.min(root.maxWidth, fileText.implicitWidth + 34)
    implicitHeight: fileText.implicitHeight + 8
    radius: 6
    scale: root.pressed ? 0.96 : (root.hovering ? 1.02 : 1.0)

    Behavior on color {
        ColorAnimation {
            duration: 110
            easing.type: Easing.OutQuad
        }
    }
    Behavior on scale {
        NumberAnimation {
            duration: 110
            easing.type: Easing.OutQuad
        }
    }

    Text {
        id: fileText
        anchors.verticalCenter: parent.verticalCenter
        text: "[FILE] " + (root.fileName.length > 0 ? root.fileName : "文件")
        color: root.pressed ? "#2d5f96" : (root.hovering ? "#366ea9" : "#233247")
        font.pixelSize: 14
        font.underline: root.hovering
        wrapMode: Text.Wrap

        Behavior on color {
            ColorAnimation {
                duration: 110
                easing.type: Easing.OutQuad
            }
        }
    }

    MouseArea {
        id: fileArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.openRequested()
    }
}
