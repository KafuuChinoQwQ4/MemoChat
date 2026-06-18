pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import MemoChat 1.0
import "../app" as SharedApp
import "qrc:/qml/runtime/AppWindowRuntime.js" as AppWindowRuntime
import "qrc:/features/auth/view" as SharedAuth
import "../components" as SharedComponents
import "components" as LinuxComponents
import "qrc:/features/pet/view"

Item {
    id: root
    visible: false
    property string appTitle: "MemoChat QML"
    property size loginWindowSize: Qt.size(360, 560)
    property size chatWindowSize: Qt.size(1240, 900)
    readonly property size chatWindowMinimumSize: Qt.size(900, 640)
    readonly property size unboundedWindowSize: Qt.size(100000, 100000)
    property var loginWindowRef: null
    property var chatWindowRef: null
    property var petWindowRef: null
    property var pendingCenterWindowRef: null
    property int pendingCenterPasses: 0
    property int windowSwitchToken: 0
    property bool handoffRetiredWindowPending: false
    property int handoffPasses: 0
    property int handoffTargetPage: ShellViewModel.LoginPage
    property int handoffToken: 0
    property bool memochatStartupCenter: true
    readonly property int handoffMinimumPasses: 2
    readonly property int windowHandoffIntervalMs: 42
    readonly property bool chatPageActive: shell.page === ShellViewModel.ChatPage

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
        if (!win) {
            return
        }
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

    function clearPendingCenterForWindow(win) {
        if (pendingCenterWindowRef !== win) {
            return
        }
        pendingCenterWindowRef = null
        pendingCenterPasses = 0
        centerWindowTimer.stop()
    }

    function retireWindow(win) {
        if (!win) {
            return
        }
        clearPendingCenterForWindow(win)
        win.retiring = true
        win.opacity = 0
        win.visible = false
        win.hide()
        win.destroy()
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
                     + " page=" + shell.page)
    }

    function configureLoginWindow(win) {
        if (!win) {
            return
        }
        win.maximumWidth = root.unboundedWindowSize.width
        win.maximumHeight = root.unboundedWindowSize.height
        win.minimumWidth = root.loginWindowSize.width
        win.minimumHeight = root.loginWindowSize.height
        centerWindowForSize(win, root.loginWindowSize)
        win.width = root.loginWindowSize.width
        win.height = root.loginWindowSize.height
        win.maximumWidth = root.loginWindowSize.width
        win.maximumHeight = root.loginWindowSize.height
        win.normalWindowWidth = root.loginWindowSize.width
        win.normalWindowHeight = root.loginWindowSize.height
        requestWindowCenter(win)
    }

    function configureChatWindow(win) {
        if (!win) {
            return
        }
        win.maximumWidth = root.unboundedWindowSize.width
        win.maximumHeight = root.unboundedWindowSize.height
        win.minimumWidth = root.chatWindowMinimumSize.width
        win.minimumHeight = root.chatWindowMinimumSize.height
        centerWindowForSize(win, root.chatWindowSize)
        win.width = root.chatWindowSize.width
        win.height = root.chatWindowSize.height
        win.maximumWidth = root.unboundedWindowSize.width
        win.maximumHeight = root.unboundedWindowSize.height
        win.normalWindowWidth = root.chatWindowSize.width
        win.normalWindowHeight = root.chatWindowSize.height
        requestWindowCenter(win)
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

    function destroyLoginWindow() {
        const win = root.loginWindowRef
        if (!win) {
            return false
        }
        root.loginWindowRef = null
        retireWindow(win)
        return true
    }

    function destroyChatWindow() {
        const win = root.chatWindowRef
        if (!win) {
            return false
        }
        root.chatWindowRef = null
        retireWindow(win)
        return true
    }

    function petAccountReady() {
        return root.chatPageActive && shell.currentUserUid > 0
    }

    function bindStartupPetSettingsToCurrentUser() {
        if (shell.currentUserUid <= 0) {
            startupPetSettings.clearAccountBinding()
            return false
        }
        startupPetSettings.bindAccount(shell.currentUserUid, shell.currentUserId)
        startupPetSettings.load()
        return true
    }

    function destroyPetWindow() {
        startupPetTimer.stop()
        const win = root.petWindowRef
        if (!win) {
            return false
        }
        root.petWindowRef = null
        win.petAssetSettings = null
        win.petController = null
        win.agentController = null
        win.visible = false
        win.hide()
        win.destroy()
        return true
    }

    function finishChatWindowShown() {
        if (!chatPageActive) {
            return
        }
        root.bindStartupPetSettingsToCurrentUser()
        Qt.callLater(function() {
            if (chatPageActive) {
                shell.beginPostLoginBootstrap()
            }
        })
        if (startupPetSettings.autoStartPetOnClientStart) {
            startupPetTimer.start()
        }
    }

    function showLoginWindow() {
        const win = ensureLoginWindow()
        if (!win) {
            return
        }
        configureLoginWindow(win)
        showWindowCentered(win)
        win.raise()
        win.requestActivate()
        logWindowState("login-window sync", win)
    }

    function showChatWindow() {
        const win = ensureChatWindow()
        if (!win) {
            return
        }
        configureChatWindow(win)
        showWindowCentered(win)
        win.raise()
        win.requestActivate()
        logWindowState("chat-window sync", win)
        finishChatWindowShown()
    }

    function handoffTargetIsCurrent(targetPage) {
        if (targetPage === ShellViewModel.ChatPage) {
            return root.chatPageActive
        }
        return shell.page === targetPage && !root.chatPageActive
    }

    function showWindowForHandoffTarget(targetPage) {
        if (targetPage === ShellViewModel.ChatPage) {
            showChatWindow()
            return
        }
        showLoginWindow()
    }

    function scheduleWindowHandoff(retiredWindowPending, token, targetPage) {
        windowHandoffTimer.stop()
        root.handoffRetiredWindowPending = retiredWindowPending
        root.handoffToken = token
        root.handoffTargetPage = targetPage
        root.handoffPasses = 0
        windowHandoffTimer.interval = root.handoffRetiredWindowPending ? root.windowHandoffIntervalMs : 0
        windowHandoffTimer.restart()
    }

    function continueWindowHandoff() {
        if (root.handoffToken !== root.windowSwitchToken
                || !handoffTargetIsCurrent(root.handoffTargetPage)) {
            root.handoffRetiredWindowPending = false
            root.handoffPasses = 0
            return
        }

        root.handoffPasses += 1
        if (root.handoffRetiredWindowPending
                && root.handoffPasses < root.handoffMinimumPasses) {
            windowHandoffTimer.interval = root.windowHandoffIntervalMs
            windowHandoffTimer.restart()
            return
        }

        root.handoffRetiredWindowPending = false
        root.handoffPasses = 0
        showWindowForHandoffTarget(root.handoffTargetPage)
    }

    function ensurePetWindow(petAssetSettings) {
        const settings = petAssetSettings || startupPetSettings
        if (petWindowRef) {
            petWindowRef.petAssetSettings = settings
            petWindowRef.selfAvatar = shell.currentUserIcon
            return petWindowRef
        }
        petWindowRef = petWindowComponent.createObject(null, {
            "petController": pet,
            "agentController": agent,
            "petAssetSettings": settings,
            "selfAvatar": shell.currentUserIcon
        })
        return petWindowRef
    }

    function openPetWindow(petAssetSettings) {
        if (!root.petAccountReady()) {
            root.destroyPetWindow()
            return null
        }
        if (!petAssetSettings) {
            if (!root.bindStartupPetSettingsToCurrentUser()) {
                return null
            }
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
        if (!root.petAccountReady()) {
            root.destroyPetWindow()
            return
        }
        root.bindStartupPetSettingsToCurrentUser()
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
        const token = ++windowSwitchToken
        if (chatPageActive) {
            bindStartupPetSettingsToCurrentUser()
            const retiredWindowPending = destroyLoginWindow()
            scheduleWindowHandoff(retiredWindowPending, token, ShellViewModel.ChatPage)
            return
        }
        destroyPetWindow()
        const retiredWindowPending = destroyChatWindow()
        scheduleWindowHandoff(retiredWindowPending, token, shell.page)
    }

    Component.onCompleted: {
        bindStartupPetSettingsToCurrentUser()
        syncWindowsByPage()
    }

    Component.onDestruction: {
        destroyLoginWindow()
        destroyChatWindow()
        destroyPetWindow()
    }

    Connections {
        target: shell
        function onPageChanged() {
            syncWindowsByPage()
        }
        function onCurrentUserChanged() {
            if (!root.petAccountReady()) {
                root.destroyPetWindow()
                startupPetSettings.clearAccountBinding()
                return
            }
            const previousPetAccountUid = startupPetSettings.accountUid
            root.bindStartupPetSettingsToCurrentUser()
            if (petWindowRef && previousPetAccountUid > 0
                    && previousPetAccountUid !== shell.currentUserUid) {
                root.destroyPetWindow()
                return
            }
            if (petWindowRef) {
                petWindowRef.petAssetSettings = startupPetSettings
                petWindowRef.selfAvatar = shell.currentUserIcon
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
        id: windowHandoffTimer
        interval: root.windowHandoffIntervalMs
        repeat: false
        onTriggered: root.continueWindowHandoff()
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
            credentialProvider: auth
            tipText: auth.tipText
            tipError: auth.tipError
            busy: auth.busy

            onClearTipRequested: auth.clearTip()
            onSwitchToRegisterRequested: shell.switchToRegister()
            onSwitchToResetRequested: shell.switchToReset()
            onLoginRequested: function(email, password) {
                auth.login(email, password)
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
        id: loginWindowComponent
        ApplicationWindow {
            id: loginWindow
            visible: false
            title: root.appTitle
            property bool memochatStartupCenter: true
            property bool retiring: false
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
            readonly property int windowRadius: isMaximized ? 0 : root.loginWindowRadius
            readonly property int glassInset: isMaximized ? 0 : root.glassInset
            readonly property int shellContentInset: isMaximized ? 0 : root.shellContentInset
            property int normalWindowX: x
            property int normalWindowY: y
            property int normalWindowWidth: root.loginWindowSize.width
            property int normalWindowHeight: root.loginWindowSize.height
            onClosing: Qt.quit()

            MouseArea {
                id: loginHeaderDragArea
                z: 160
                x: root.borderResizeThickness + 44
                y: root.borderResizeThickness
                width: Math.max(0, loginWindowControls.x - x - root.borderResizeThickness)
                height: Math.max(32, loginWindowControls.y + loginWindowControls.height - y)
                acceptedButtons: Qt.LeftButton
                cursorShape: Qt.SizeAllCursor
                onPressed: function(mouse) {
                    if (!loginWindow.isMaximized) {
                        loginWindow.startSystemMove()
                        mouse.accepted = true
                    }
                }
            }

            Item {
                id: loginShell
                anchors.fill: parent
                anchors.margins: loginWindow.glassInset

                LinuxComponents.WindowGlassShell {
                    anchors.fill: parent
                    cornerRadius: loginWindow.windowRadius
                    fillTopColor: Qt.rgba(0.93, 0.97, 1.0, 0.56)
                    fillBottomColor: Qt.rgba(0.86, 0.93, 1.0, 0.52)
                    glowTopColor: Qt.rgba(1, 1, 1, 0.22)
                    glowMiddleColor: Qt.rgba(0.92, 0.97, 1.0, 0.08)
                    glowBottomColor: Qt.rgba(0.74, 0.84, 0.96, 0.10)
                    strokeColor: loginWindow.isMaximized ? "transparent" : Qt.rgba(1, 1, 1, 0.42)
                    strokeWidth: loginWindow.isMaximized ? 0 : 0.9
                }

                Loader {
                    anchors.fill: parent
                    anchors.margins: loginWindow.shellContentInset
                    visible: shell.page === ShellViewModel.LoginPage
                    active: visible
                    asynchronous: true
                    sourceComponent: loginPageComponent
                }

                Loader {
                    anchors.fill: parent
                    anchors.margins: loginWindow.shellContentInset
                    visible: shell.page === ShellViewModel.RegisterPage
                    active: visible
                    asynchronous: true
                    sourceComponent: registerPageComponent
                }

                Loader {
                    anchors.fill: parent
                    anchors.margins: loginWindow.shellContentInset
                    visible: shell.page === ShellViewModel.ResetPage
                    active: visible
                    asynchronous: true
                    sourceComponent: resetPageComponent
                }
            }

            SharedComponents.AppWindowControls {
                id: loginWindowControls
                z: 200
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: loginWindow.glassInset + loginWindow.shellContentInset + 10
                anchors.rightMargin: loginWindow.glassInset + loginWindow.shellContentInset + 10
                width: implicitWidth
                height: implicitHeight
                chatPageActive: false
                hoverPadding: root.controlHoverPadding
                onPetRequested: root.togglePetWindow()
                onMinimizeRequested: loginWindow.showMinimized()
                onMaximizeRequested: root.toggleWindowMaximized(loginWindow)
                onCloseRequested: Qt.quit()
            }
        }
    }

    Component {
        id: chatWindowComponent
        ApplicationWindow {
            id: chatWindow
            visible: false
            title: root.appTitle
            property bool memochatStartupCenter: true
            property bool retiring: false
            flags: Qt.Window | Qt.FramelessWindowHint
            color: "transparent"
            width: root.chatWindowSize.width
            height: root.chatWindowSize.height
            minimumWidth: root.chatWindowMinimumSize.width
            minimumHeight: root.chatWindowMinimumSize.height
            maximumWidth: root.unboundedWindowSize.width
            maximumHeight: root.unboundedWindowSize.height
            property bool windowMaximized: false
            readonly property bool isMaximized: windowMaximized
            readonly property int windowRadius: isMaximized ? 0 : root.chatWindowRadius
            readonly property int glassInset: isMaximized ? 0 : root.glassInset
            readonly property int shellContentInset: isMaximized ? 0 : root.shellContentInset
            property int normalWindowX: x
            property int normalWindowY: y
            property int normalWindowWidth: root.chatWindowSize.width
            property int normalWindowHeight: root.chatWindowSize.height
            onClosing: Qt.quit()

            MouseArea {
                id: chatHeaderDragArea
                z: 160
                x: root.borderResizeThickness + 44
                y: root.borderResizeThickness
                width: Math.max(0, chatWindowControls.x - x - root.borderResizeThickness)
                height: Math.max(32, chatWindowControls.y + chatWindowControls.height - y)
                acceptedButtons: Qt.LeftButton
                cursorShape: Qt.SizeAllCursor
                onPressed: function(mouse) {
                    if (!chatWindow.isMaximized) {
                        chatWindow.startSystemMove()
                        mouse.accepted = true
                    }
                }
            }

            Item {
                id: chatShell
                anchors.fill: parent
                anchors.margins: chatWindow.glassInset

                LinuxComponents.WindowGlassShell {
                    anchors.fill: parent
                    cornerRadius: chatWindow.windowRadius
                    fillTopColor: Qt.rgba(0.93, 0.97, 1.0, 0.56)
                    fillBottomColor: Qt.rgba(0.86, 0.93, 1.0, 0.52)
                    glowTopColor: Qt.rgba(1, 1, 1, 0.22)
                    glowMiddleColor: Qt.rgba(0.92, 0.97, 1.0, 0.08)
                    glowBottomColor: Qt.rgba(0.74, 0.84, 0.96, 0.10)
                    strokeColor: chatWindow.isMaximized ? "transparent" : Qt.rgba(1, 1, 1, 0.42)
                    strokeWidth: chatWindow.isMaximized ? 0 : 0.9
                }

                Loader {
                    anchors.fill: parent
                    anchors.margins: chatWindow.shellContentInset
                    active: true
                    visible: true
                    asynchronous: false
                    sourceComponent: chatShellPageComponent
                }
            }

            SharedComponents.AppWindowControls {
                id: chatWindowControls
                z: 200
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: chatWindow.glassInset + chatWindow.shellContentInset + 10
                anchors.rightMargin: chatWindow.glassInset + chatWindow.shellContentInset + 10
                width: implicitWidth
                height: implicitHeight
                chatPageActive: true
                hoverPadding: root.controlHoverPadding
                onPetRequested: root.togglePetWindow()
                onMinimizeRequested: chatWindow.showMinimized()
                onMaximizeRequested: root.toggleWindowMaximized(chatWindow)
                onCloseRequested: Qt.quit()
            }

            Item {
                z: 180
                anchors.fill: parent
                visible: !chatWindow.isMaximized

                ResizeHandle {
                    x: root.borderResizeThickness
                    y: 0
                    width: Math.max(0, chatWindowControls.x - root.borderResizeThickness * 2)
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
                                chatWindowControls.y + chatWindowControls.height + 6)
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
