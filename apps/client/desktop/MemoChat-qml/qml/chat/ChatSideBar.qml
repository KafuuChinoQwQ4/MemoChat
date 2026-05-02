import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

Rectangle {
    id: root
    color: "transparent"
    width: 72

    property int currentTab: 0
    property string userIcon: "qrc:/res/head_1.jpg"
    property bool hasPendingApply: false
    property bool minimalMode: false
    property color backgroundColor: "transparent"
    property color borderColor: "transparent"
    property color buttonHoverColor: Qt.rgba(0.56, 0.70, 0.86, 0.16)
    property color buttonPressedColor: Qt.rgba(0.56, 0.70, 0.86, 0.22)
    property color buttonSelectedColor: Qt.rgba(0.56, 0.70, 0.86, 0.26)
    property color buttonSelectedPressedColor: Qt.rgba(0.56, 0.70, 0.86, 0.34)
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
        radius: 14
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
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 12
        width: 52
        spacing: 14

        Rectangle {
            id: avatarButton
            width: 52
            height: 52
            radius: 13
            clip: true
            property bool hovering: avatarHover.containsMouse
            property bool pressed: avatarHover.pressed
            color: avatarButton.pressed ? root.buttonPressedColor
                                         : avatarButton.hovering ? root.buttonHoverColor
                                                                 : "transparent"
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
                anchors.centerIn: parent
                width: 34
                height: 34
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
                width: 52
                height: 52
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
                    radius: 13
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
                    opacity: root.currentTab === modelData ? 0.98 : 0.62
                    mipmap: true
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
        anchors.bottomMargin: 12
        spacing: 14

        Item {
            id: r18Button
            width: 52
            height: 52
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
                radius: 13
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
                opacity: 0.72
                mipmap: true
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
            width: 52
            height: 52
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
                radius: 13
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
                opacity: root.currentTab === 2 ? 0.98 : 0.62
                mipmap: true
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
