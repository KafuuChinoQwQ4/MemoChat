pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import MemoChat 1.0
import "../app" as SharedApp
import "../app/AppWindowRuntime.js" as AppWindowRuntime
import "../auth" as SharedAuth
import "../components" as SharedComponents
import "components" as LinuxComponents
import "../pet"

Item {
    id: root
    visible: false
    property string appTitle: "MemoChat QML"
    property size loginWindowSize: Qt.size(360, 560)
    property size chatWindowSize: Qt.size(1240, 900)
    readonly property size chatWindowMinimumSize: Qt.size(900, 640)
    readonly property size unboundedWindowSize: Qt.size(100000, 100000)
    property var appWindowRef: null
    property var petWindowRef: null
    property var pendingCenterWindowRef: null
    property int pendingCenterPasses: 0
    property int appWindowSwitchToken: 0
    property bool memochatStartupCenter: true
    readonly property bool chatPageActive: controller.page === AppController.ChatPage

    readonly property int loginWindowRadius: 24
    readonly property int chatWindowRadius: 24
    readonly property int glassInset: 4
    readonly property int shellContentInset: 8
    readonly property int controlHoverPadding: 7
    readonly property int borderResizeThickness: 12

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
        if (win.windowMaximized
                || win.visibility === Window.Maximized
                || win.visibility === Window.FullScreen
                || win.visibility === Window.Minimized) {
            win.windowMaximized = false
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

    function toggleWindowMaximized(win) {
        if (!win) {
            return
        }
        if (win.windowMaximized) {
            var restoreX = win.normalWindowX
            var restoreY = win.normalWindowY
            var restoreWidth = win.normalWindowWidth
            var restoreHeight = win.normalWindowHeight
            win.windowMaximized = false
            win.showNormal()
            Qt.callLater(function() {
                win.x = restoreX
                win.y = restoreY
                win.width = Math.max(win.minimumWidth, restoreWidth)
                win.height = Math.max(win.minimumHeight, restoreHeight)
            })
            return
        }
        win.normalWindowX = win.x
        win.normalWindowY = win.y
        win.normalWindowWidth = win.width
        win.normalWindowHeight = win.height
        win.windowMaximized = true
        win.showMaximized()
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
        win.normalWindowWidth = win.width
        win.normalWindowHeight = win.height
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

    component ResizeHandle: MouseArea {
        property int resizeEdges: Qt.LeftEdge
        property int resizeCursor: Qt.ArrowCursor
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: resizeCursor
        onPressed: function(mouse) {
            if (Window.window) {
                Window.window.startSystemResize(resizeEdges)
                mouse.accepted = true
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
        SharedAuth.RegisterPage { }
    }

    Component {
        id: resetPageComponent
        SharedAuth.ResetPage { }
    }

    Component {
        id: chatShellPageComponent
        SharedApp.ChatShellPage {
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
            property bool windowMaximized: false
            readonly property bool isMaximized: windowMaximized
            readonly property int windowRadius: isMaximized ? 0 : (root.chatPageActive ? root.chatWindowRadius : root.loginWindowRadius)
            readonly property int glassInset: isMaximized ? 0 : root.glassInset
            readonly property int shellContentInset: isMaximized ? 0 : root.shellContentInset
            property int normalWindowX: x
            property int normalWindowY: y
            property int normalWindowWidth: root.targetWindowSize().width
            property int normalWindowHeight: root.targetWindowSize().height
            onClosing: Qt.quit()

            MouseArea {
                id: headerDragArea
                z: 160
                x: root.borderResizeThickness + 44
                y: root.borderResizeThickness
                width: Math.max(0, appWindowControls.x - x - root.borderResizeThickness)
                height: Math.max(32, appWindowControls.y + appWindowControls.height - y)
                acceptedButtons: Qt.LeftButton
                cursorShape: Qt.SizeAllCursor
                onPressed: function(mouse) {
                    if (!appWindow.isMaximized) {
                        appWindow.startSystemMove()
                        mouse.accepted = true
                    }
                }
            }

            Item {
                id: appShell
                anchors.fill: parent
                anchors.margins: appWindow.glassInset

                LinuxComponents.WindowGlassShell {
                    anchors.fill: parent
                    cornerRadius: appWindow.windowRadius
                    fillTopColor: Qt.rgba(0.93, 0.97, 1.0, 0.56)
                    fillBottomColor: Qt.rgba(0.86, 0.93, 1.0, 0.52)
                    glowTopColor: Qt.rgba(1, 1, 1, 0.22)
                    glowMiddleColor: Qt.rgba(0.92, 0.97, 1.0, 0.08)
                    glowBottomColor: Qt.rgba(0.74, 0.84, 0.96, 0.10)
                    strokeColor: appWindow.isMaximized ? "transparent" : Qt.rgba(1, 1, 1, 0.42)
                    strokeWidth: appWindow.isMaximized ? 0 : 0.9
                }

                Loader {
                    anchors.fill: parent
                    anchors.margins: appWindow.shellContentInset
                    visible: controller.page === AppController.LoginPage
                    active: visible
                    asynchronous: true
                    sourceComponent: loginPageComponent
                }

                Loader {
                    anchors.fill: parent
                    anchors.margins: appWindow.shellContentInset
                    visible: controller.page === AppController.RegisterPage
                    active: visible
                    asynchronous: true
                    sourceComponent: registerPageComponent
                }

                Loader {
                    anchors.fill: parent
                    anchors.margins: appWindow.shellContentInset
                    visible: controller.page === AppController.ResetPage
                    active: visible
                    asynchronous: true
                    sourceComponent: resetPageComponent
                }

                Loader {
                    anchors.fill: parent
                    anchors.margins: appWindow.shellContentInset
                    active: root.chatPageActive
                    visible: active
                    asynchronous: true
                    sourceComponent: chatShellPageComponent
                }
            }

            SharedComponents.AppWindowControls {
                id: appWindowControls
                z: 200
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: appWindow.glassInset + appWindow.shellContentInset + 10
                anchors.rightMargin: appWindow.glassInset + appWindow.shellContentInset + 10
                width: implicitWidth
                height: implicitHeight
                chatPageActive: root.chatPageActive
                hoverPadding: root.controlHoverPadding
                onPetRequested: root.togglePetWindow()
                onMinimizeRequested: appWindow.showMinimized()
                onMaximizeRequested: root.toggleWindowMaximized(appWindow)
                onCloseRequested: Qt.quit()
            }

            Item {
                z: 180
                anchors.fill: parent
                visible: !appWindow.isMaximized

                ResizeHandle {
                    x: root.borderResizeThickness
                    y: 0
                    width: Math.max(0, appWindowControls.x - root.borderResizeThickness * 2)
                    height: root.borderResizeThickness
                    resizeEdges: Qt.TopEdge
                    resizeCursor: Qt.SizeVerCursor
                }

                ResizeHandle {
                    x: 0
                    y: root.borderResizeThickness
                    width: root.borderResizeThickness
                    height: Math.max(0, parent.height - root.borderResizeThickness * 2)
                    resizeEdges: Qt.LeftEdge
                    resizeCursor: Qt.SizeHorCursor
                }

                ResizeHandle {
                    x: parent.width - root.borderResizeThickness
                    y: Math.min(parent.height - root.borderResizeThickness,
                                appWindowControls.y + appWindowControls.height + 6)
                    width: root.borderResizeThickness
                    height: Math.max(0, parent.height - y - root.borderResizeThickness)
                    resizeEdges: Qt.RightEdge
                    resizeCursor: Qt.SizeHorCursor
                }

                ResizeHandle {
                    x: root.borderResizeThickness
                    y: parent.height - root.borderResizeThickness
                    width: Math.max(0, parent.width - root.borderResizeThickness * 2)
                    height: root.borderResizeThickness
                    resizeEdges: Qt.BottomEdge
                    resizeCursor: Qt.SizeVerCursor
                }

                ResizeHandle {
                    x: 0
                    y: 0
                    width: root.borderResizeThickness
                    height: root.borderResizeThickness
                    resizeEdges: Qt.TopEdge | Qt.LeftEdge
                    resizeCursor: Qt.SizeFDiagCursor
                }

                ResizeHandle {
                    x: 0
                    y: parent.height - root.borderResizeThickness
                    width: root.borderResizeThickness
                    height: root.borderResizeThickness
                    resizeEdges: Qt.BottomEdge | Qt.LeftEdge
                    resizeCursor: Qt.SizeBDiagCursor
                }

                ResizeHandle {
                    x: parent.width - root.borderResizeThickness
                    y: parent.height - root.borderResizeThickness
                    width: root.borderResizeThickness
                    height: root.borderResizeThickness
                    resizeEdges: Qt.BottomEdge | Qt.RightEdge
                    resizeCursor: Qt.SizeFDiagCursor
                }
            }
        }
    }

    Component {
        id: petWindowComponent
        PetWindow { }
    }
}
