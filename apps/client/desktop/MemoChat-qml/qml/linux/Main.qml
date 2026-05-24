pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import MemoChat 1.0
import "../" as SharedPages
import "../components" as SharedComponents
import "components" as LinuxComponents
import "../pet"

Item {
    id: root
    visible: false
    property string appTitle: "MemoChat QML"
    property size loginWindowSize: Qt.size(360, 560)
    property size chatWindowSize: Qt.size(1100, 760)
    property var loginWindowRef: null
    property var chatWindowRef: null
    property var petWindowRef: null
    property bool memochatStartupCenter: true
    property int windowSwitchToken: 0
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

    function centerWindow(win) {
        if (!win || win.visibility === Window.Maximized
                || win.visibility === Window.FullScreen
                || win.visibility === Window.Minimized) {
            return false
        }
        const screenObj = win.screen || (Qt.application.screens && Qt.application.screens.length > 0
                                         ? Qt.application.screens[0]
                                         : null)
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
        const maxX = area.x + Math.max(0, area.width - win.width)
        const maxY = area.y + Math.max(0, area.height - win.height)
        win.x = Math.max(area.x, Math.min(centeredX, maxX))
        win.y = Math.max(area.y, Math.min(centeredY, maxY))
        return true
    }

    function showWindowCentered(win) {
        win.opacity = 1
        centerWindow(win)
        win.showNormal()
        centerWindow(win)
        Qt.callLater(function() {
            if (win && win.visible) {
                centerWindow(win)
            }
        })
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

    function destroyLoginWindow() {
        if (!loginWindowRef) {
            return
        }
        const win = loginWindowRef
        loginWindowRef = null
        win.opacity = 0
        win.visible = false
        win.hide()
        win.destroy()
    }

    function destroyChatWindow() {
        if (!chatWindowRef) {
            return
        }
        const win = chatWindowRef
        chatWindowRef = null
        win.opacity = 0
        win.visible = false
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
        if (win.windowMaximized || win.visibility === Window.Maximized) {
            win.windowMaximized = false
            win.showNormal()
        }
        win.width = loginWindowSize.width
        win.height = loginWindowSize.height
        win.normalWindowWidth = win.width
        win.normalWindowHeight = win.height
        showWindowCentered(win)
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
        if (win.windowMaximized || win.visibility === Window.Maximized) {
            win.windowMaximized = false
            win.showNormal()
        }
        win.width = chatWindowSize.width
        win.height = chatWindowSize.height
        win.normalWindowWidth = win.width
        win.normalWindowHeight = win.height
        showWindowCentered(win)
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
        const token = ++windowSwitchToken
        const targetChatPage = controller.page === AppController.ChatPage
        if (targetChatPage) {
            destroyLoginWindow()
        } else {
            destroyChatWindow()
        }
        Qt.callLater(function() {
            if (token !== windowSwitchToken) {
                return
            }
            if (targetChatPage && controller.page === AppController.ChatPage) {
                showChatWindow()
            } else if (!targetChatPage && controller.page !== AppController.ChatPage) {
                ensureLoginWindowVisible("sync")
            }
        })
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
        SharedPages.RegisterPage { }
    }

    Component {
        id: resetPageComponent
        SharedPages.ResetPage { }
    }

    Component {
        id: chatShellPageComponent
        SharedPages.ChatShellPage {
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
            flags: Qt.Window | Qt.FramelessWindowHint
            color: "transparent"
            width: root.loginWindowSize.width
            height: root.loginWindowSize.height
            minimumWidth: 340
            minimumHeight: 520
            maximumWidth: 100000
            maximumHeight: 100000
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
                    visible: controller.page === AppController.LoginPage
                    active: visible
                    asynchronous: true
                    sourceComponent: loginPageComponent
                }

                Loader {
                    anchors.fill: parent
                    anchors.margins: loginWindow.shellContentInset
                    visible: controller.page === AppController.RegisterPage
                    active: visible
                    asynchronous: true
                    sourceComponent: registerPageComponent
                }

                Loader {
                    anchors.fill: parent
                    anchors.margins: loginWindow.shellContentInset
                    visible: controller.page === AppController.ResetPage
                    active: visible
                    asynchronous: true
                    sourceComponent: resetPageComponent
                }

            }

            Item {
                id: loginWindowControls
                z: 200
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: loginWindow.glassInset + loginWindow.shellContentInset + 10
                anchors.rightMargin: loginWindow.glassInset + loginWindow.shellContentInset + 10
                width: controlsRow.implicitWidth + root.controlHoverPadding * 2
                height: controlsRow.implicitHeight + root.controlHoverPadding * 2

                Row {
                    id: controlsRow
                    anchors.centerIn: parent
                    spacing: 20

                    SharedComponents.LoginIconButton {
                        iconSource: "qrc:/icons/minimize.png"
                        onClicked: loginWindow.showMinimized()
                    }

                    SharedComponents.LoginIconButton {
                        iconSource: "qrc:/icons/maximize.png"
                        onClicked: root.toggleWindowMaximized(loginWindow)
                    }

                    SharedComponents.LoginIconButton {
                        iconSource: "qrc:/icons/close.png"
                        onClicked: Qt.quit()
                    }
                }
            }

            Item {
                z: 180
                anchors.fill: parent
                visible: !loginWindow.isMaximized

                ResizeHandle {
                    x: root.borderResizeThickness
                    y: 0
                    width: Math.max(0, loginWindowControls.x - root.borderResizeThickness * 2)
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
                                loginWindowControls.y + loginWindowControls.height + 6)
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
        id: chatWindowComponent
        ApplicationWindow {
            id: chatWindow
            visible: false
            title: root.appTitle
            property bool memochatStartupCenter: true
            flags: Qt.Window | Qt.FramelessWindowHint
            color: "transparent"
            width: root.chatWindowSize.width
            height: root.chatWindowSize.height
            minimumWidth: 900
            minimumHeight: 640
            maximumWidth: 100000
            maximumHeight: 100000
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
                    asynchronous: true
                    sourceComponent: chatShellPageComponent
                }
            }

            Item {
                id: chatWindowControls
                z: 200
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: chatWindow.glassInset + chatWindow.shellContentInset + 10
                anchors.rightMargin: chatWindow.glassInset + chatWindow.shellContentInset + 10
                width: chatControlsRow.implicitWidth + root.controlHoverPadding * 2
                height: chatControlsRow.implicitHeight + root.controlHoverPadding * 2

                Row {
                    id: chatControlsRow
                    anchors.centerIn: parent
                    spacing: 20

                    SharedComponents.LoginIconButton {
                        iconSource: "qrc:/icons/ai.png"
                        onClicked: root.togglePetWindow()
                    }

                    SharedComponents.LoginIconButton {
                        iconSource: "qrc:/icons/minimize.png"
                        onClicked: chatWindow.showMinimized()
                    }

                    SharedComponents.LoginIconButton {
                        iconSource: "qrc:/icons/maximize.png"
                        onClicked: root.toggleWindowMaximized(chatWindow)
                    }

                    SharedComponents.LoginIconButton {
                        iconSource: "qrc:/icons/close.png"
                        onClicked: Qt.quit()
                    }
                }
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
