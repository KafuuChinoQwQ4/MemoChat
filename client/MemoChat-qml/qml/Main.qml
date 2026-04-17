import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import MemoChat 1.0
import "components"

ApplicationWindow {
    id: root
    visible: controller.page !== AppController.ChatPage
    title: appTitle
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    property bool isMaximized: visibility === Window.Maximized
    property int windowRadius: isMaximized ? 0 : 20
    property string appTitle: "MemoChat QML"
    property size loginWindowSize: Qt.size(300, 500)
    property size chatWindowSize: Qt.size(900, 640)
    property var chatWindowRef: null

    function centerWindow(win) {
        if (!win || win.visibility !== Window.Windowed) {
            return
        }
        const screenObj = win.screen
        if (!screenObj || !screenObj.availableGeometry) {
            return
        }
        const area = screenObj.availableGeometry
        if (area.x === undefined || area.y === undefined
                || area.width === undefined || area.height === undefined) {
            return
        }
        const centeredX = area.x + Math.round((area.width - win.width) / 2)
        const centeredY = area.y + Math.round((area.height - win.height) / 2)
        win.x = Math.max(area.x, centeredX)
        win.y = Math.max(area.y, centeredY)
    }

    function showLoginWindow() {
        if (root.visibility === Window.Maximized) {
            root.showNormal()
        }
        root.width = loginWindowSize.width
        root.height = loginWindowSize.height
        root.show()
        centerWindow(root)
        root.raise()
        root.requestActivate()
        Qt.callLater(function() {
            centerWindow(root)
        })
    }

    function showChatWindow() {
        const win = ensureChatWindow()
        if (!win) {
            console.warn("Failed to create chat window")
            return false
        }
        if (win.visibility === Window.Maximized) {
            win.showNormal()
        }
        win.width = chatWindowSize.width
        win.height = chatWindowSize.height
        win.show()
        centerWindow(win)
        win.raise()
        win.requestActivate()
        Qt.callLater(function() {
            centerWindow(win)
        })
        return true
    }

    function ensureChatWindow() {
        if (chatWindowRef) {
            return chatWindowRef
        }
        chatWindowRef = chatWindowComponent.createObject(null)
        if (!chatWindowRef) {
            return null
        }
        return chatWindowRef
    }

    function hideChatWindow() {
        if (!chatWindowRef) {
            return
        }
        chatWindowRef.hide()
    }

    function syncWindowsByPage() {
        if (controller.page === AppController.ChatPage) {
            root.hide()
            if (!showChatWindow()) {
                showLoginWindow()
            }
        } else {
            hideChatWindow()
            showLoginWindow()
        }
    }

    minimumWidth: loginWindowSize.width
    minimumHeight: loginWindowSize.height
    maximumWidth: loginWindowSize.width
    maximumHeight: loginWindowSize.height

    Component {
        id: loginPageComponent
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
        id: registerPageComponent
        RegisterPage { }
    }

    Component {
        id: resetPageComponent
        ResetPage { }
    }

    Component {
        id: chatShellPageComponent
        ChatShellPage {
            topInset: 24
        }
    }

    Component.onCompleted: syncWindowsByPage()
    Component.onDestruction: {
        if (chatWindowRef) {
            chatWindowRef.destroy()
            chatWindowRef = null
        }
    }

    Connections {
        target: controller
        function onPageChanged() {
            syncWindowsByPage()
        }
    }

    Rectangle {
        id: shell
        anchors.fill: parent
        radius: root.windowRadius
        antialiasing: !root.isMaximized
        clip: !root.isMaximized
        color: "transparent"

        StackLayout {
            id: pageStack
            anchors.fill: parent
            currentIndex: controller.page === AppController.RegisterPage
                          ? 1
                          : (controller.page === AppController.ResetPage ? 2 : 0)

            Item {
                Loader {
                    anchors.fill: parent
                    active: controller.page === AppController.LoginPage
                    asynchronous: true
                    sourceComponent: loginPageComponent
                }
            }

            Item {
                Loader {
                    anchors.fill: parent
                    active: controller.page === AppController.RegisterPage
                    asynchronous: true
                    sourceComponent: registerPageComponent
                }
            }

            Item {
                Loader {
                    anchors.fill: parent
                    active: controller.page === AppController.ResetPage
                    asynchronous: true
                    sourceComponent: resetPageComponent
                }
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
        id: loginWindowControls
        z: 200
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 20
        anchors.rightMargin: 14
        width: controlsRow.implicitWidth
        height: controlsRow.implicitHeight

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
                onClicked: Qt.quit()
            }
        }
    }

    Component {
        id: chatWindowComponent

        Window {
            id: chatWindow
            visible: false
            title: root.appTitle
            flags: Qt.Window | Qt.FramelessWindowHint
            color: "transparent"
            minimumWidth: 800
            minimumHeight: 600
            maximumWidth: 100000
            maximumHeight: 100000
            width: root.chatWindowSize.width
            height: root.chatWindowSize.height
            property bool isMaximized: visibility === Window.Maximized
            property int windowRadius: isMaximized ? 0 : 20

            onClosing: function(close) {
                close.accepted = false
                Qt.quit()
            }

            Rectangle {
                id: chatShell
                anchors.fill: parent
                radius: chatWindow.windowRadius
                antialiasing: !chatWindow.isMaximized
                clip: !chatWindow.isMaximized
                color: "transparent"

                Loader {
                    anchors.fill: parent
                    active: chatWindow.visible && controller.page === AppController.ChatPage
                    asynchronous: true
                    sourceComponent: chatShellPageComponent
                }
            }

            Item {
                id: chatWindowControls
                z: 200
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 10
                anchors.rightMargin: 10
                width: chatControlsRow.implicitWidth
                height: chatControlsRow.implicitHeight

                Rectangle {
                    anchors.centerIn: parent
                    width: chatControlsRow.implicitWidth + 20
                    height: chatControlsRow.implicitHeight + 12
                    radius: 12
                    color: Qt.rgba(1, 1, 1, 0.18)
                    border.color: Qt.rgba(1, 1, 1, 0.40)
                }

                Row {
                    id: chatControlsRow
                    spacing: 20

                    LoginIconButton {
                        iconSource: "qrc:/icons/minimize.png"
                        onClicked: chatWindow.showMinimized()
                    }

                    LoginIconButton {
                        iconSource: "qrc:/icons/maximize.png"
                        onClicked: {
                            if (chatWindow.visibility === Window.Maximized) {
                                chatWindow.showNormal()
                            } else {
                                chatWindow.showMaximized()
                            }
                        }
                    }

                    LoginIconButton {
                        iconSource: "qrc:/icons/close.png"
                        onClicked: Qt.quit()
                    }
                }
            }
        }
    }
}
