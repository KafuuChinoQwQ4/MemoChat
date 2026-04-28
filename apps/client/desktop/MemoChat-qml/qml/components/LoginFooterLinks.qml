import QtQuick 2.15
import QtQuick.Controls 2.15

Row {
    id: root
    property Item moreOptionsAnchor: moreOptionsText
    signal scanLoginClicked()
    signal moreOptionsClicked()

    spacing: 14

    Label {
        id: scanLoginText
        property bool hovering: false
        text: "扫码登录"
        color: hovering ? "#1f63cb" : "#2c73df"
        font.pixelSize: 13

        Behavior on color {
            ColorAnimation {
                duration: 130
                easing.type: Easing.OutQuad
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.bottom
            anchors.topMargin: 2
            height: 1
            radius: 1
            color: Qt.rgba(0.17, 0.45, 0.88, 0.86)
            opacity: scanLoginText.hovering ? 1 : 0

            Behavior on opacity {
                NumberAnimation {
                    duration: 130
                    easing.type: Easing.OutQuad
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onEntered: scanLoginText.hovering = true
            onExited: scanLoginText.hovering = false
            onClicked: root.scanLoginClicked()
        }
    }

    Label {
        text: "|"
        color: "#b4b4b4"
        font.pixelSize: 13
    }

    Label {
        id: moreOptionsText
        property bool hovering: false
        text: "更多选项"
        color: hovering ? "#1f63cb" : "#2c73df"
        font.pixelSize: 13

        Behavior on color {
            ColorAnimation {
                duration: 130
                easing.type: Easing.OutQuad
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.bottom
            anchors.topMargin: 2
            height: 1
            radius: 1
            color: Qt.rgba(0.17, 0.45, 0.88, 0.86)
            opacity: moreOptionsText.hovering ? 1 : 0

            Behavior on opacity {
                NumberAnimation {
                    duration: 130
                    easing.type: Easing.OutQuad
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onEntered: moreOptionsText.hovering = true
            onExited: moreOptionsText.hovering = false
            onClicked: root.moreOptionsClicked()
        }
    }
}
