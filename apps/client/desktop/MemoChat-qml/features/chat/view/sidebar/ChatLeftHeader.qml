import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "qrc:/qml/components"

Rectangle {
    id: root

    property Item backdrop: null
    property int currentTab: 0
    property string title: "更多"
    property string subtitle: ""
    property bool agentBusy: false

    signal createGroupRequested()
    signal addFriendRequested()
    signal joinGroupRequested()
    signal openApplyRequested()

    // 背景由外层圆角玻璃板提供，此处保持透明，避免直角底板盖住圆角
    color: "transparent"

    // 顶栏与下方列表之间的细分隔线（两侧内缩，不触及圆角）
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        height: 1
        color: Qt.rgba(1, 1, 1, 0.22)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 8
        visible: root.currentTab === ShellViewModel.ChatTabPage

        GlassButton {
            id: addMenuButton
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30
            text: "+"
            textPixelSize: 21
            cornerRadius: 15
            normalColor: Qt.rgba(0.55, 0.70, 0.92, 0.24)
            hoverColor: Qt.rgba(0.55, 0.70, 0.92, 0.34)
            pressedColor: Qt.rgba(0.55, 0.70, 0.92, 0.42)
            disabledColor: Qt.rgba(0.52, 0.56, 0.62, 0.18)
            onClicked: addMenuPopup.open()

            ToolTip.visible: hovering
            ToolTip.delay: 180
            ToolTip.text: "添加"
        }

        Item { Layout.fillWidth: true }

        Popup {
            id: addMenuPopup
            width: 152
            height: addMenuColumn.implicitHeight + 16
            x: addMenuButton.x
            y: addMenuButton.y + addMenuButton.height + 6
            padding: 0
            modal: false
            focus: true
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

            background: Rectangle {
                color: "transparent"
            }

            contentItem: GlassSurface {
                backdrop: root.backdrop !== null ? root.backdrop : root
                cornerRadius: 12
                blurRadius: 20
                fillColor: Qt.rgba(0.98, 0.99, 1.0, 0.86)
                strokeColor: Qt.rgba(1, 1, 1, 0.58)
                glowTopColor: Qt.rgba(1, 1, 1, 0.28)
                glowBottomColor: Qt.rgba(1, 1, 1, 0.06)

                ColumnLayout {
                    id: addMenuColumn
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    GlassButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        text: "创建群聊"
                        textPixelSize: 13
                        cornerRadius: 8
                        normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.18)
                        hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.28)
                        pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.36)
                        onClicked: {
                            addMenuPopup.close()
                            root.createGroupRequested()
                        }
                    }

                    GlassButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        text: "添加好友"
                        textPixelSize: 13
                        cornerRadius: 8
                        normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.14)
                        hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                        pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.32)
                        onClicked: {
                            addMenuPopup.close()
                            root.addFriendRequested()
                        }
                    }

                    GlassButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        text: "加入群聊"
                        textPixelSize: 13
                        cornerRadius: 8
                        normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.14)
                        hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                        pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.32)
                        onClicked: {
                            addMenuPopup.close()
                            root.joinGroupRequested()
                        }
                    }

                    GlassButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        text: "好友申请"
                        textPixelSize: 13
                        cornerRadius: 8
                        normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.16)
                        hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.24)
                        pressedColor: Qt.rgba(0.54, 0.60, 0.68, 0.32)
                        onClicked: {
                            addMenuPopup.close()
                            root.openApplyRequested()
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 4
        visible: root.currentTab !== ShellViewModel.ChatTabPage

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: root.title
                color: "#2a3649"
                font.pixelSize: 18
                font.bold: true
            }

            Rectangle {
                Layout.preferredHeight: 22
                Layout.preferredWidth: busyLabel.implicitWidth + 14
                radius: 11
                color: Qt.rgba(0.36, 0.62, 0.92, 0.16)
                visible: root.currentTab === ShellViewModel.AgentTabPage && root.agentBusy

                Label {
                    id: busyLabel
                    anchors.centerIn: parent
                    text: "处理中"
                    color: "#2d6fb4"
                    font.pixelSize: 11
                    font.bold: true
                }
            }

            Item { Layout.fillWidth: true }
        }

        Label {
            Layout.fillWidth: true
            text: root.subtitle
            color: "#6a7b92"
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
    }
}
