pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import MemoChat 1.0
import "qrc:/qml/runtime/AppWindowRuntime.js" as AppWindowRuntime
import "qrc:/features/auth/view"
import "../components"
import "qrc:/features/pet/view"

Item {
    id: root
    visible: false
    property string appTitle: "MemoChat QML"
    property size loginWindowSize: Qt.size(300, 500)
    property size chatWindowSize: Qt.size(900, 640)
    readonly property size chatWindowMinimumSize: Qt.size(800, 600)
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

    function finishChatWindowShown() {
        if (!chatPageActive) {
            return
        }
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
        const token = ++windowSwitchToken
        if (chatPageActive) {
            const retiredWindowPending = destroyLoginWindow()
            scheduleWindowHandoff(retiredWindowPending, token, ShellViewModel.ChatPage)
            return
        }
        const retiredWindowPending = destroyChatWindow()
        scheduleWindowHandoff(retiredWindowPending, token, shell.page)
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
        target: shell
        function onPageChanged() {
            syncWindowsByPage()
        }
        function onCurrentUserChanged() {
            if (petWindowRef) {
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
            readonly property bool isMaximized: visibility === Window.Maximized
            readonly property int windowRadius: isMaximized ? 0 : 20
            onClosing: Qt.quit()

            MouseArea {
                id: loginHeaderDragArea
                z: 150
                x: 56
                y: 0
                width: Math.max(0, parent.width - x - loginWindowControls.width - 70)
                height: 64
                acceptedButtons: Qt.LeftButton
                cursorShape: Qt.SizeAllCursor
                onPressed: function(mouse) {
                    loginWindow.startSystemMove()
                    mouse.accepted = true
                }
            }

            Rectangle {
                id: loginShell
                anchors.fill: parent
                radius: loginWindow.windowRadius
                antialiasing: !loginWindow.isMaximized
                clip: !loginWindow.isMaximized
                color: "transparent"

                Loader {
                    anchors.fill: parent
                    visible: shell.page === ShellViewModel.LoginPage
                    active: visible
                    asynchronous: true
                    sourceComponent: loginPageComponent
                }

                Loader {
                    anchors.fill: parent
                    visible: shell.page === ShellViewModel.RegisterPage
                    active: visible
                    asynchronous: true
                    sourceComponent: registerPageComponent
                }

                Loader {
                    anchors.fill: parent
                    visible: shell.page === ShellViewModel.ResetPage
                    active: visible
                    asynchronous: true
                    sourceComponent: resetPageComponent
                }
            }

            AppWindowControls {
                id: loginWindowControls
                z: 200
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 20
                anchors.rightMargin: 14
                width: implicitWidth
                height: implicitHeight
                chatPageActive: false
                onPetRequested: root.togglePetWindow()
                onMinimizeRequested: loginWindow.showMinimized()
                onMaximizeRequested: {
                    if (loginWindow.visibility === Window.Maximized) {
                        loginWindow.showNormal()
                    } else {
                        loginWindow.showMaximized()
                    }
                }
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
            readonly property bool isMaximized: visibility === Window.Maximized
            readonly property int windowRadius: isMaximized ? 0 : 20
            onClosing: Qt.quit()

            MouseArea {
                id: chatHeaderDragArea
                z: 150
                x: 56
                y: 4
                width: Math.max(0, parent.width - x - chatWindowControls.width - 70)
                height: 64
                acceptedButtons: Qt.LeftButton
                cursorShape: Qt.SizeAllCursor
                onPressed: function(mouse) {
                    chatWindow.startSystemMove()
                    mouse.accepted = true
                }
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
                    active: true
                    visible: true
                    asynchronous: false
                    sourceComponent: chatShellPageComponent
                }
            }

            AppWindowControls {
                id: chatWindowControls
                z: 200
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 20
                anchors.rightMargin: 14
                width: implicitWidth
                height: implicitHeight
                chatPageActive: true
                onPetRequested: root.togglePetWindow()
                onMinimizeRequested: chatWindow.showMinimized()
                onMaximizeRequested: {
                    if (chatWindow.visibility === Window.Maximized) {
                        chatWindow.showNormal()
                    } else {
                        chatWindow.showMaximized()
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
