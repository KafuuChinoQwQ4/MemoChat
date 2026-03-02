import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import MemoChat 1.0
import "components"

ApplicationWindow {
    id: root
    visible: true
    title: "MemoChat QML"
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    property bool isMaximized: visibility === Window.Maximized
    property int windowRadius: isMaximized ? 0 : 20

    property bool chatMode: controller.page === AppController.ChatPage
    property size loginWindowSize: Qt.size(300, 500)
    property size chatWindowSize: Qt.size(1100, 820)

    minimumWidth: chatMode ? 980 : 300
    minimumHeight: chatMode ? 760 : 500
    maximumWidth: chatMode ? 100000 : 300
    maximumHeight: chatMode ? 100000 : 500

    Component.onCompleted: {
        width = chatMode ? chatWindowSize.width : loginWindowSize.width
        height = chatMode ? chatWindowSize.height : loginWindowSize.height
    }

    onChatModeChanged: {
        if (visibility === Window.Windowed) {
            width = chatMode ? chatWindowSize.width : loginWindowSize.width
            height = chatMode ? chatWindowSize.height : loginWindowSize.height
        }
    }

    Rectangle {
        id: shell
        anchors.fill: parent
        radius: root.windowRadius
        antialiasing: !root.isMaximized
        clip: !root.isMaximized
        color: "transparent"

        Loader {
            id: pageLoader
            anchors.fill: parent
            sourceComponent: {
                if (controller.page === AppController.LoginPage) return loginPage
                if (controller.page === AppController.RegisterPage) return registerPage
                if (controller.page === AppController.ResetPage) return resetPage
                return chatPage
            }
        }

        DragHandler {
            target: null
            onActiveChanged: {
                if (active) {
                    root.startSystemMove()
                }
            }
        }
    }

    Item {
        id: windowControls
        z: 200
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: chatMode ? 10 : 20
        anchors.rightMargin: chatMode ? 10 : 14
        width: controlsRow.implicitWidth
        height: controlsRow.implicitHeight

        Rectangle {
            anchors.centerIn: parent
            width: controlsRow.implicitWidth + (chatMode ? 20 : 0)
            height: controlsRow.implicitHeight + (chatMode ? 12 : 0)
            radius: 12
            color: chatMode ? Qt.rgba(1, 1, 1, 0.18) : "transparent"
            border.color: chatMode ? Qt.rgba(1, 1, 1, 0.40) : "transparent"
            visible: chatMode
        }

        Row {
            id: controlsRow
            spacing: 20

            LoginIconButton {
                iconSource: "qrc:/icons/minimize.png"
                onClicked: root.showMinimized()
            }

            LoginIconButton {
                iconSource: "qrc:/icons/maximize.png"
                onClicked: {
                    if (root.visibility === Window.Maximized) {
                        root.showNormal()
                    } else {
                        root.showMaximized()
                    }
                }
            }

            LoginIconButton {
                iconSource: "qrc:/icons/close.png"
                onClicked: root.close()
            }
        }
    }

    Component {
        id: loginPage
        LoginPage {
            tipText: controller.tipText
            tipError: controller.tipError
            busy: controller.busy

            onClearTipRequested: controller.clearTip()
            onSwitchToRegisterRequested: controller.switchToRegister()
            onSwitchToResetRequested: controller.switchToReset()
            onLoginRequested: function(email, password) {
                controller.login(email, password)
            }
        }
    }

    Component {
        id: registerPage
        RegisterPage { }
    }

    Component {
        id: resetPage
        ResetPage { }
    }

    Component {
        id: chatPage
        ChatShellPage {
            topInset: 24
        }
    }
}
