import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

Rectangle {
    id: root
    color: "transparent"
    width: 56

    property int currentTab: 0
    property string userIcon: "qrc:/res/head_1.jpg"
    property bool hasPendingApply: false
    property bool minimalMode: false
    property color backgroundColor: Qt.rgba(0.16, 0.22, 0.31, 0.44)
    property color borderColor: Qt.rgba(1, 1, 1, 0.16)
    property color buttonHoverColor: Qt.rgba(0.75, 0.87, 1.0, 0.18)
    property color buttonPressedColor: Qt.rgba(0.75, 0.87, 1.0, 0.26)
    property color buttonSelectedColor: Qt.rgba(0.75, 0.87, 1.0, 0.30)
    property color buttonSelectedPressedColor: Qt.rgba(0.75, 0.87, 1.0, 0.44)
    signal avatarClicked()
    signal tabSelected(int tab)
    signal r18Toggled()

    function iconForTab(tab, selected) {
        if (tab === 0) {
            return selected ? "qrc:/res/chat_icon_select_press.png" : "qrc:/res/chat_icon.png"
        }
        if (tab === 1) {
            return selected ? "qrc:/res/contact_list_press.png" : "qrc:/res/contact_list.png"
        }
        if (tab === 3) {
            return selected ? "qrc:/icons/moments.png" : "qrc:/icons/moments.png"
        }
        if (tab === 4) {
            return selected ? "qrc:/icons/ai.png" : "qrc:/icons/ai.png"
        }
        return selected ? "qrc:/res/settings_select_press.png" : "qrc:/res/settings.png"
    }

    function titleForTab(tab) {
        if (tab === 0) {
            return "聊天"
        }
        if (tab === 1) {
            return "联系人"
        }
        if (tab === 3) {
            return "朋友圈"
        }
        if (tab === 4) {
            return "AI助手"
        }
        return "更多"
    }

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: root.backgroundColor
        border.color: root.borderColor
    }

    // 全局拖动区域：覆盖整个侧边栏背景（底层）
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeAllCursor
        onPressed: function(mouse) {
            mouse.accepted = false
            if (Window.window) {
                Window.window.startSystemMove()
            }
        }
    }

    // 内容层（头像 + 标签图标）
    Column {
        z: 1
        visible: !root.minimalMode
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 10
        anchors.topMargin: 24
        spacing: 28

        Rectangle {
            id: avatarButton
            width: 35
            height: 35
            radius: 17
            clip: true
            property bool hovering: avatarHover.containsMouse
            property bool pressed: avatarHover.pressed
            color: Qt.rgba(0.47, 0.63, 0.83, 0.36)
            border.width: avatarButton.hovering ? 1 : 0
            border.color: Qt.rgba(1, 1, 1, 0.6)
            scale: avatarButton.pressed ? 0.96 : (avatarButton.hovering ? 1.02 : 1.0)

            Behavior on scale {
                NumberAnimation {
                    duration: 110
                    easing.type: Easing.OutQuad
                }
            }
            Behavior on border.width {
                NumberAnimation {
                    duration: 110
                    easing.type: Easing.OutQuad
                }
            }
            Image {
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                source: root.userIcon
                cache: true
                asynchronous: true
                opacity: (status === Image.Ready) ? 1.0 : 0.0
                Behavior on opacity { NumberAnimation { duration: 200 } }
            }

            MouseArea {
                id: avatarHover
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.avatarClicked()
            }

            ToolTip.visible: avatarHover.containsMouse
            ToolTip.delay: 120
            ToolTip.text: "个人中心"
        }

        Repeater {
            model: [0, 1, 3, 4]
            delegate: Item {
                width: 32
                height: 32
                property bool hovering: tabArea.containsMouse
                property bool pressed: tabArea.pressed
                scale: pressed ? 0.96 : (hovering ? 1.02 : 1.0)

                Behavior on scale {
                    NumberAnimation {
                        duration: 110
                        easing.type: Easing.OutQuad
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: {
                        if (root.currentTab === modelData) {
                            return tabArea.pressed ? root.buttonSelectedPressedColor : root.buttonSelectedColor
                        }
                        if (tabArea.pressed) {
                            return root.buttonPressedColor
                        }
                        if (tabArea.containsMouse) {
                            return root.buttonHoverColor
                        }
                        return "transparent"
                    }

                    Behavior on color {
                        ColorAnimation {
                            duration: 110
                            easing.type: Easing.OutQuad
                        }
                    }
                }

                Image {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: root.iconForTab(modelData, root.currentTab === modelData)
                    fillMode: Image.PreserveAspectFit
                }

                Rectangle {
                    visible: modelData === 1 && root.hasPendingApply
                    width: 8
                    height: 8
                    radius: 4
                    color: "#e84141"
                    anchors.right: parent.right
                    anchors.top: parent.top
                }

                MouseArea {
                    id: tabArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    propagateComposedEvents: true
                    onClicked: root.tabSelected(modelData)
                }

                ToolTip.visible: tabArea.containsMouse
                ToolTip.delay: 120
                ToolTip.text: root.titleForTab(modelData)
            }
        }
    }

    // 底部内容层（R18 + 设置）
    Column {
        z: 1
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16
        spacing: 10

        Item {
            id: r18Button
            width: 32
            height: 32
            property bool hovering: r18Hover.containsMouse
            property bool pressed: r18Hover.pressed
            scale: pressed ? 0.96 : (hovering ? 1.02 : 1.0)

            Behavior on scale {
                NumberAnimation {
                    duration: 110
                    easing.type: Easing.OutQuad
                }
            }

            Rectangle {
                anchors.fill: parent
                radius: 8
                color: r18Hover.pressed ? root.buttonPressedColor
                                        : r18Hover.containsMouse ? root.buttonHoverColor
                                                                 : "transparent"

                Behavior on color {
                    ColorAnimation {
                        duration: 110
                        easing.type: Easing.OutQuad
                    }
                }
            }

            Image {
                anchors.centerIn: parent
                width: 28
                height: 28
                source: "qrc:/icons/r18.png"
                fillMode: Image.PreserveAspectFit
            }

            MouseArea {
                id: r18Hover
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.r18Toggled()
            }

            ToolTip.visible: r18Hover.containsMouse
            ToolTip.delay: 120
            ToolTip.text: "R18"
        }

        Item {
            id: settingsButton
            width: 32
            height: 32
            property bool hovering: settingsArea.containsMouse
            property bool pressed: settingsArea.pressed
            scale: pressed ? 0.96 : (hovering ? 1.02 : 1.0)

            Behavior on scale {
                NumberAnimation {
                    duration: 110
                    easing.type: Easing.OutQuad
                }
            }

            Rectangle {
                anchors.fill: parent
                radius: 8
                color: {
                    if (root.currentTab === 2) {
                        return settingsArea.pressed ? root.buttonSelectedPressedColor : root.buttonSelectedColor
                    }
                    if (settingsArea.pressed) {
                        return root.buttonPressedColor
                    }
                    if (settingsArea.containsMouse) {
                        return root.buttonHoverColor
                    }
                    return "transparent"
                }

                Behavior on color {
                    ColorAnimation {
                        duration: 110
                        easing.type: Easing.OutQuad
                    }
                }
            }

            Image {
                anchors.centerIn: parent
                width: 24
                height: 24
                source: root.iconForTab(2, root.currentTab === 2)
                fillMode: Image.PreserveAspectFit
            }

            MouseArea {
                id: settingsArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                propagateComposedEvents: true
                onClicked: root.tabSelected(2)
            }

            ToolTip.visible: settingsArea.containsMouse
            ToolTip.delay: 120
            ToolTip.text: root.titleForTab(2)
        }
    }
}
