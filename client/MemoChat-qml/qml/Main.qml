import QtQuick 2.15
import QtQuick.Controls 2.15
import MemoChat 1.0

ApplicationWindow {
    id: root
    visible: true
    title: "MemoChat QML"
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    property int windowRadius: 20

    property bool chatMode: controller.page === AppController.ChatPage

    minimumWidth: chatMode ? 980 : 300
    minimumHeight: chatMode ? 760 : 500
    maximumWidth: chatMode ? 100000 : 300
    maximumHeight: chatMode ? 100000 : 500
    width: chatMode ? 1100 : 300
    height: chatMode ? 820 : 500

    onChatModeChanged: {
        if (chatMode) {
            width = 1100
            height = 820
        } else {
            width = 300
            height = 500
        }
    }

    Rectangle {
        id: shell
        anchors.fill: parent
        radius: root.windowRadius
        antialiasing: true
        clip: true
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
        ChatShellPage { }
    }
}
