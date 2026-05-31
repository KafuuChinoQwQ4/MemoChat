pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import MemoChat 1.0
import "AppWindowRuntime.js" as AppWindowRuntime
import "components"
import "pet"

Item {
    id: root
    visible: false
    property string appTitle: "MemoChat QML"
    property size loginWindowSize: Qt.size(300, 500)
    property size chatWindowSize: Qt.size(900, 640)
    readonly property size chatWindowMinimumSize: Qt.size(800, 600)
    readonly property size unboundedWindowSize: Qt.size(100000, 100000)
    property var appWindowRef: null
    property var petWindowRef: null
    property var pendingCenterWindowRef: null
    property int pendingCenterPasses: 0
    property int appWindowSwitchToken: 0
    property bool memochatStartupCenter: true
    readonly property bool chatPageActive: controller.page === AppController.ChatPage

    PetAssetSettings {
        id: startupPetSettings
    }

    function centerWindowForSize(win, size) {
        if (!win || win.visibility === Window.Maximized
                || win.visibility === Window.FullScreen
                || win.visibility === Window.Minimized) {
            return false
        }
        const targetWidth = size ? size.width : win.width
        const targetHeight = size ? size.height : win.height
        const area = AppWindowRuntime.availableScreenGeometry(win)
        const position = AppWindowRuntime.clampedWindowPosition(area, targetWidth, targetHeight)
        if (!position) {
            return false
        }
        win.x = position.x
        win.y = position.y
        return true
    }

    function centerWindow(win) {
        return centerWindowForSize(win, Qt.size(win.width, win.height))
    }

    function showWindowCentered(win) {
        centerWindow(win)
        win.opacity = 1
        if (win.visibility === Window.Maximized
                || win.visibility === Window.FullScreen
                || win.visibility === Window.Minimized) {
            win.showNormal()
        } else if (!win.visible) {
            win.visible = true
        }
        requestWindowCenter(win)
        Qt.callLater(function() {
            if (win && win.visible) {
                requestWindowCenter(win)
            }
        })
    }

    function requestWindowCenter(win) {
        if (!win) {
            return
        }
        centerWindow(win)
        pendingCenterWindowRef = win
        pendingCenterPasses = 4
        centerWindowTimer.restart()
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

    function targetWindowSize() {
        return AppWindowRuntime.targetWindowSize(chatPageActive, loginWindowSize, chatWindowSize)
    }

    function ensureAppWindow() {
        if (appWindowRef) {
            return appWindowRef
        }
        appWindowRef = appWindowComponent.createObject(null)
        if (!appWindowRef) {
            console.warn("Failed to create app window")
        }
        return appWindowRef
    }

    function configureAppWindowForPage(win) {
        if (!win) {
            return
        }
        const loginMode = !chatPageActive
        const size = targetWindowSize()
        const minimumSize = loginMode ? root.loginWindowSize : root.chatWindowMinimumSize
        const maximumSize = loginMode ? root.loginWindowSize : root.unboundedWindowSize
        win.maximumWidth = root.unboundedWindowSize.width
        win.maximumHeight = root.unboundedWindowSize.height
        win.minimumWidth = minimumSize.width
        win.minimumHeight = minimumSize.height
        centerWindowForSize(win, size)
        win.width = size.width
        win.height = size.height
        win.maximumWidth = maximumSize.width
        win.maximumHeight = maximumSize.height
        requestWindowCenter(win)
    }

    function finishAppWindowShown() {
        if (!chatPageActive) {
            return
        }
        Qt.callLater(function() {
            if (chatPageActive) {
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
        const win = ensureAppWindow()
        if (!win) {
            return
        }
        const token = ++appWindowSwitchToken
        const targetSize = targetWindowSize()
        const sizeChanged = win.visibility !== Window.Maximized
                && win.visibility !== Window.FullScreen
                && win.visibility !== Window.Minimized
                && AppWindowRuntime.shouldHideResize(win, targetSize.width, targetSize.height)
        if (sizeChanged) {
            win.opacity = 0
            win.visible = false
            configureAppWindowForPage(win)
            Qt.callLater(function() {
                if (token !== root.appWindowSwitchToken) {
                    return
                }
                showWindowCentered(win)
                win.raise()
                win.requestActivate()
                logWindowState("app-window hidden-resize sync", win)
                finishAppWindowShown()
            })
            return
        }
        configureAppWindowForPage(win)
        showWindowCentered(win)
        win.raise()
        win.requestActivate()
        logWindowState("app-window sync", win)
        finishAppWindowShown()
    }

    Component.onCompleted: {
        startupPetSettings.load()
        syncWindowsByPage()
    }
    Component.onDestruction: {
        if (appWindowRef) {
            appWindowRef.destroy()
            appWindowRef = null
        }
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

    Timer {
        id: centerWindowTimer
        interval: 70
        repeat: false
        onTriggered: {
            const win = root.pendingCenterWindowRef
            if (!win || !win.visible || root.pendingCenterPasses <= 0) {
                root.pendingCenterWindowRef = null
                root.pendingCenterPasses = 0
                return
            }
            root.centerWindow(win)
            root.pendingCenterPasses -= 1
            if (root.pendingCenterPasses > 0) {
                restart()
            } else {
                root.pendingCenterWindowRef = null
            }
        }
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
        id: appWindowComponent
        ApplicationWindow {
            id: appWindow
            visible: false
            title: root.appTitle
            property bool memochatStartupCenter: true
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

            MouseArea {
                id: headerDragArea
                z: 150
                x: 56
                y: root.chatPageActive ? 4 : 0
                width: Math.max(0, parent.width - x - windowControls.width - 70)
                height: 64
                acceptedButtons: Qt.LeftButton
                cursorShape: Qt.SizeAllCursor
                onPressed: function(mouse) {
                    appWindow.startSystemMove()
                    mouse.accepted = true
                }
            }

            Rectangle {
                id: appShell
                anchors.fill: parent
                radius: appWindow.windowRadius
                antialiasing: !appWindow.isMaximized
                clip: !appWindow.isMaximized
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

                Loader {
                    anchors.fill: parent
                    active: root.chatPageActive
                    visible: active
                    asynchronous: true
                    sourceComponent: chatShellPageComponent
                }
            }

            AppWindowControls {
                id: windowControls
                z: 200
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 20
                anchors.rightMargin: 14
                width: implicitWidth
                height: implicitHeight
                chatPageActive: root.chatPageActive
                onPetRequested: root.togglePetWindow()
                onMinimizeRequested: appWindow.showMinimized()
                onMaximizeRequested: {
                    if (appWindow.visibility === Window.Maximized) {
                        appWindow.showNormal()
                    } else {
                        appWindow.showMaximized()
                    }
                }
                onCloseRequested: Qt.quit()
            }
        }
    }

    Component {
        id: petWindowComponent
        PetWindow { }
    }
}
