pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import MemoChat 1.0
import "components"
import "pet"

Item {
    id: root
    visible: false
    property string appTitle: "MemoChat QML"
    property size loginWindowSize: Qt.size(300, 500)
    property size chatWindowSize: Qt.size(900, 640)
    property var loginWindowRef: null
    property var chatWindowRef: null
    property var petWindowRef: null
    property bool memochatStartupCenter: true
    readonly property bool chatPageActive: controller.page === AppController.ChatPage

    PetAssetSettings {
        id: startupPetSettings
    }

    function centerWindow(win) {
        if (!win || win.visibility !== Window.Windowed) {
            return false
        }
        const screenObj = win.screen
        if (!screenObj || !screenObj.availableGeometry) {
            return false
        }
        const area = screenObj.availableGeometry
        if (area.x === undefined || area.y === undefined
                || area.width === undefined || area.height === undefined) {
            return false
        }
        const centeredX = area.x + Math.round((area.width - win.width) / 2)
        const centeredY = area.y + Math.round((area.height - win.height) / 2)
        win.x = Math.max(area.x, centeredX)
        win.y = Math.max(area.y, centeredY)
        return true
    }

    function logWindowState(label, win) {
        if (!win) {
            console.info(label + " window=null")
            return
        }
        console.info(label
                     + " visible=" + win.visible
                     + " visibility=" + win.visibility
                     + " x=" + win.x
                     + " y=" + win.y
                     + " w=" + win.width
                     + " h=" + win.height
                     + " page=" + controller.page)
    }

    function destroyLoginWindow() {
        if (!loginWindowRef) {
            return
        }
        const win = loginWindowRef
        loginWindowRef = null
        win.hide()
        win.destroy()
    }

    function destroyChatWindow() {
        if (!chatWindowRef) {
            return
        }
        const win = chatWindowRef
        chatWindowRef = null
        win.hide()
        win.destroy()
    }

    function ensureLoginWindow() {
        if (loginWindowRef) {
            return loginWindowRef
        }
        loginWindowRef = loginWindowComponent.createObject(null)
        if (!loginWindowRef) {
            console.warn("Failed to create login window")
        }
        return loginWindowRef
    }

    function ensureChatWindow() {
        if (chatWindowRef) {
            return chatWindowRef
        }
        chatWindowRef = chatWindowComponent.createObject(null)
        if (!chatWindowRef) {
            console.warn("Failed to create chat window")
        }
        return chatWindowRef
    }

    function showLoginWindow() {
        destroyChatWindow()
        const win = ensureLoginWindow()
        if (!win) {
            return
        }
        if (win.visibility === Window.Maximized) {
            win.showNormal()
        }
        win.width = loginWindowSize.width
        win.height = loginWindowSize.height
        win.show()
        centerWindow(win)
        win.raise()
        win.requestActivate()
    }

    function ensureLoginWindowVisible(reason) {
        if (controller.page === AppController.ChatPage) {
            return
        }
        showLoginWindow()
        logWindowState("login-window " + reason, loginWindowRef)
    }

    function showChatWindow() {
        destroyLoginWindow()
        const win = ensureChatWindow()
        if (!win) {
            return
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
            if (controller.page === AppController.ChatPage) {
                controller.beginPostLoginBootstrap()
            }
        })
        if (startupPetSettings.autoStartPetOnClientStart) {
            startupPetTimer.start()
        }
    }

    function ensurePetWindow(petAssetSettings) {
        const settings = petAssetSettings || startupPetSettings
        if (petWindowRef) {
            petWindowRef.petAssetSettings = settings
            petWindowRef.selfAvatar = controller.currentUserIcon
            return petWindowRef
        }
        petWindowRef = petWindowComponent.createObject(null, {
            "petController": controller.petController,
            "agentController": controller.agentController,
            "petAssetSettings": settings,
            "selfAvatar": controller.currentUserIcon
        })
        return petWindowRef
    }

    function openPetWindow(petAssetSettings) {
        if (!petAssetSettings) {
            startupPetSettings.load()
        }
        const win = ensurePetWindow(petAssetSettings)
        if (!win) {
            console.warn("Failed to create pet window")
            return null
        }
        win.openPet()
        return win
    }

    function togglePetWindow() {
        const win = ensurePetWindow()
        if (!win) {
            console.warn("Failed to create pet window")
            return
        }
        if (win.visible) {
            win.hide()
            return
        }
        win.openPet()
    }

    function syncWindowsByPage() {
        if (controller.page === AppController.ChatPage) {
            showChatWindow()
        } else {
            ensureLoginWindowVisible("sync")
        }
    }

    Component.onCompleted: {
        startupPetSettings.load()
        syncWindowsByPage()
    }
    Component.onDestruction: {
        destroyLoginWindow()
        destroyChatWindow()
        if (petWindowRef) {
            petWindowRef.destroy()
            petWindowRef = null
        }
    }

    Connections {
        target: controller
        function onPageChanged() {
            syncWindowsByPage()
        }
        function onCurrentUserChanged() {
            if (petWindowRef) {
                petWindowRef.selfAvatar = controller.currentUserIcon
            }
        }
    }

    Timer {
        id: startupPetTimer
        interval: 160
        repeat: false
        onTriggered: root.openPetWindow()
    }

    Component {
        id: loginPageComponent
        LoginPage {
            credentialProvider: controller
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
            onPetPreviewRequested: function(petAssetSettings) {
                root.openPetWindow(petAssetSettings)
            }
        }
    }

    Component {
        id: loginWindowComponent
        ApplicationWindow {
            id: loginWindow
            visible: false
            title: root.appTitle
            flags: Qt.Window | Qt.FramelessWindowHint
            color: "transparent"
            width: root.loginWindowSize.width
            height: root.loginWindowSize.height
            minimumWidth: root.loginWindowSize.width
            minimumHeight: root.loginWindowSize.height
            maximumWidth: root.loginWindowSize.width
            maximumHeight: root.loginWindowSize.height
            readonly property bool isMaximized: visibility === Window.Maximized
            readonly property int windowRadius: isMaximized ? 0 : 20
            onClosing: Qt.quit()

            Rectangle {
                id: loginShell
                anchors.fill: parent
                radius: loginWindow.windowRadius
                antialiasing: !loginWindow.isMaximized
                clip: !loginWindow.isMaximized
                color: "transparent"

                Loader {
                    anchors.fill: parent
                    visible: controller.page === AppController.LoginPage
                    active: visible
                    asynchronous: true
                    sourceComponent: loginPageComponent
                }

                Loader {
                    anchors.fill: parent
                    visible: controller.page === AppController.RegisterPage
                    active: visible
                    asynchronous: true
                    sourceComponent: registerPageComponent
                }

                Loader {
                    anchors.fill: parent
                    visible: controller.page === AppController.ResetPage
                    active: visible
                    asynchronous: true
                    sourceComponent: resetPageComponent
                }

            }

            Item {
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
                        onClicked: loginWindow.showMinimized()
                    }

                    LoginIconButton {
                        iconSource: "qrc:/icons/maximize.png"
                        onClicked: {
                            if (loginWindow.visibility === Window.Maximized) {
                                loginWindow.showNormal()
                            } else {
                                loginWindow.showMaximized()
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

    Component {
        id: chatWindowComponent
        ApplicationWindow {
            id: chatWindow
            visible: false
            title: root.appTitle
            flags: Qt.Window | Qt.FramelessWindowHint
            color: "transparent"
            width: root.chatWindowSize.width
            height: root.chatWindowSize.height
            minimumWidth: 800
            minimumHeight: 600
            maximumWidth: 100000
            maximumHeight: 100000
            readonly property bool isMaximized: visibility === Window.Maximized
            readonly property int windowRadius: isMaximized ? 0 : 20
            onClosing: Qt.quit()

            Rectangle {
                anchors.fill: parent
                radius: chatWindow.windowRadius
                antialiasing: !chatWindow.isMaximized
                clip: !chatWindow.isMaximized
                color: "transparent"

                Loader {
                    anchors.fill: parent
                    active: true
                    asynchronous: true
                    sourceComponent: chatShellPageComponent
                }
            }

            Item {
                z: 200
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 20
                anchors.rightMargin: 14
                width: chatControlsRow.implicitWidth
                height: chatControlsRow.implicitHeight

                Row {
                    id: chatControlsRow
                    spacing: 20

                    LoginIconButton {
                        iconSource: "qrc:/icons/ai.png"
                        onClicked: root.togglePetWindow()
                    }

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

    Component {
        id: petWindowComponent
        PetWindow { }
    }
}
