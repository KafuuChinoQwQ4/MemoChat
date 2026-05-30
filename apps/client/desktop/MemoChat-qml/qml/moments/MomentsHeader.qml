import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

RowLayout {
    id: root
    spacing: 10

    property string userName: ""
    property string userNick: ""
    property string userIcon: "qrc:/res/head_1.jpg"
    property string locationText: ""
    property bool canDelete: false

    signal avatarClicked()
    signal deleteClicked()

    Rectangle {
        Layout.preferredWidth: 42
        Layout.preferredHeight: 42
        radius: 8
        clip: true
        color: Qt.rgba(0.8, 0.85, 0.95, 0.5)

        Image {
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            source: root.userIcon
            cache: true
            asynchronous: true

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.avatarClicked()
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        Label {
            text: root.userNick || root.userName || "用户"
            font.pixelSize: 14
            font.weight: Font.Medium
            color: "#1a1a1a"
        }

        Label {
            text: root.locationText
            font.pixelSize: 12
            color: "#888888"
        }
    }

    Item { Layout.fillWidth: true }

    ToolButton {
        id: moreButton
        visible: root.canDelete
        implicitWidth: 32
        implicitHeight: 32
        hoverEnabled: true
        ToolTip.visible: hovered
        ToolTip.delay: 120
        ToolTip.text: "更多"
        background: Rectangle {
            radius: 10
            color: moreButton.down ? Qt.rgba(0.16, 0.24, 0.36, 0.12)
                  : moreButton.hovered ? Qt.rgba(0.16, 0.24, 0.36, 0.08)
                  : "transparent"
            border.width: moreButton.hovered ? 1 : 0
            border.color: Qt.rgba(0.16, 0.24, 0.36, 0.12)
        }
        contentItem: Label {
            text: "..."
            color: "#4d5f73"
            font.pixelSize: 16
            font.weight: Font.Medium
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        onClicked: postMenu.open()

        Menu {
            id: postMenu
            y: moreButton.height + 4
            width: 116
            background: Rectangle {
                color: "#ffffff"
                radius: 10
                border.color: Qt.rgba(0.82, 0.84, 0.90, 0.75)
                layer.enabled: true
            }

            MenuItem {
                id: postDeleteAction
                height: 38
                text: "删除"
                contentItem: Label {
                    text: postDeleteAction.text
                    color: "#d64545"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 8
                    color: postDeleteAction.hovered ? Qt.rgba(0.86, 0.18, 0.18, 0.08) : "transparent"
                }
                onTriggered: root.deleteClicked()
            }
        }
    }
}
