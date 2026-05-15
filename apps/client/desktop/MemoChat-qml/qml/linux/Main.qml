import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import MemoChat 1.0
import "../" as SharedPages
import "../components" as SharedComponents
import "components" as LinuxComponents
import "../pet"

ApplicationWindow {
    id: root
    visible: controller.page !== AppController.ChatPage
    title: appTitle
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    property bool windowMaximized: false
    property bool isMaximized: windowMaximized
    property int windowRadius: isMaximized ? 0 : 24
    readonly property int glassInset: isMaximized ? 0 : 4
    readonly property int shellContentInset: isMaximized ? 0 : 8
    readonly property int controlHoverPadding: 7
    readonly property int borderResizeThickness: 12
    property string appTitle: "MemoChat QML"
    property size loginWindowSize: Qt.size(360, 560)
    property size chatWindowSize: Qt.size(1100, 760)
    property int normalWindowX: x
    property int normalWindowY: y
    property int normalWindowWidth: loginWindowSize.width
    property int normalWindowHeight: loginWindowSize.height
    property var chatWindowRef: null
    property var petWindowRef: null
    property bool memochatStartupCenter: true

    PetAssetSettings {
        id: startupPetSettings
    }

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

    function centerWindowWithRetry(win, attempts) {
        if (!win || attempts <= 0) {
            return
        }
        centerWindow(win)
        retryCenterTimer.targetWindow = win
        retryCenterTimer.remainingAttempts = attempts - 1
        retryCenterTimer.restart()
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

    function showLoginWindow() {
        if (root.windowMaximized || root.visibility === Window.Maximized) {
            root.windowMaximized = false
            root.showNormal()
        }
        root.width = loginWindowSize.width
        root.height = loginWindowSize.height
        root.normalWindowWidth = root.width
        root.normalWindowHeight = root.height
        root.show()
        centerWindowWithRetry(root, 6)
        root.raise()
        root.requestActivate()
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

    function ensureLoginWindowVisible(reason) {
        if (controller.page === AppController.ChatPage) {
            startupShowRetryTimer.stop()
            return
        }
        showLoginWindow()
        logWindowState("login-window " + reason, root)
        if (startupShowRetryTimer.remainingAttempts > 0) {
            startupShowRetryTimer.restart()
        }
    }

    function showChatWindow() {
        const win = ensureChatWindow()
        if (!win) {
            console.warn("Failed to create chat window")
            return false
        }
        if (win.windowMaximized || win.visibility === Window.Maximized) {
            win.windowMaximized = false
            win.showNormal()
        }
        win.width = chatWindowSize.width
        win.height = chatWindowSize.height
        win.normalWindowWidth = win.width
        win.normalWindowHeight = win.height
        win.show()
        centerWindowWithRetry(win, 6)
        win.raise()
        win.requestActivate()
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
            startupShowRetryTimer.stop()
            root.hide()
            if (!showChatWindow()) {
                showLoginWindow()
            }
        } else {
            hideChatWindow()
            startupShowRetryTimer.remainingAttempts = 5
            ensureLoginWindowVisible("sync")
        }
    }

    minimumWidth: 340
    minimumHeight: 520
    maximumWidth: 100000
    maximumHeight: 100000

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

    Component.onCompleted: {
        startupPetSettings.load()
        syncWindowsByPage()
        startupShowRetryTimer.remainingAttempts = 5
        startupShowRetryTimer.start()
        if (startupPetSettings.autoStartPetOnClientStart) {
            startupPetTimer.start()
        }
    }
    Component.onDestruction: {
        if (chatWindowRef) {
            chatWindowRef.destroy()
            chatWindowRef = null
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
        id: retryCenterTimer
        interval: 80
        repeat: false
        property var targetWindow: null
        property int remainingAttempts: 0
        onTriggered: {
            if (targetWindow && targetWindow.visible && remainingAttempts > 0) {
                centerWindowWithRetry(targetWindow, remainingAttempts)
            }
        }
    }

    Timer {
        id: startupShowRetryTimer
        interval: 120
        repeat: false
        property int remainingAttempts: 0
        onTriggered: {
            if (remainingAttempts <= 0 || controller.page === AppController.ChatPage) {
                remainingAttempts = 0
                return
            }
            remainingAttempts -= 1
            root.ensureLoginWindowVisible("retry")
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

    Item {
        id: shell
        anchors.fill: parent
        anchors.margins: root.glassInset

        LinuxComponents.WindowGlassShell {
            anchors.fill: parent
            cornerRadius: root.windowRadius
            fillTopColor: Qt.rgba(0.93, 0.97, 1.0, 0.56)
            fillBottomColor: Qt.rgba(0.86, 0.93, 1.0, 0.52)
            glowTopColor: Qt.rgba(1, 1, 1, 0.22)
            glowMiddleColor: Qt.rgba(0.92, 0.97, 1.0, 0.08)
            glowBottomColor: Qt.rgba(0.74, 0.84, 0.96, 0.10)
            strokeColor: root.isMaximized ? "transparent" : Qt.rgba(1, 1, 1, 0.42)
            strokeWidth: root.isMaximized ? 0 : 0.9
        }

        StackLayout {
            id: pageStack
            anchors.fill: parent
            anchors.margins: root.shellContentInset
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
        anchors.topMargin: root.glassInset + root.shellContentInset + 10
        anchors.rightMargin: root.glassInset + root.shellContentInset + 10
        width: controlsRow.implicitWidth + root.controlHoverPadding * 2
        height: controlsRow.implicitHeight + root.controlHoverPadding * 2

        Row {
            id: controlsRow
            anchors.centerIn: parent
            spacing: 20

            SharedComponents.LoginIconButton {
                iconSource: "qrc:/icons/minimize.png"
                onClicked: root.showMinimized()
            }

            SharedComponents.LoginIconButton {
                iconSource: "qrc:/icons/maximize.png"
                onClicked: root.toggleWindowMaximized(root)
            }

            SharedComponents.LoginIconButton {
                iconSource: "qrc:/icons/close.png"
                onClicked: Qt.quit()
            }
        }
    }

    Item {
        id: loginResizeFrame
        z: 180
        anchors.fill: parent
        visible: !root.isMaximized

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

    Component {
        id: chatWindowComponent

        Window {
            id: chatWindow
            visible: false
            title: root.appTitle
            flags: Qt.Window | Qt.FramelessWindowHint
            color: "transparent"
            property real acrylicPinkProgress: 0.0
            minimumWidth: 900
            minimumHeight: 640
            maximumWidth: 100000
            maximumHeight: 100000
            width: root.chatWindowSize.width
            height: root.chatWindowSize.height
            property bool memochatStartupCenter: true
            property bool windowMaximized: false
            property int normalWindowX: x
            property int normalWindowY: y
            property int normalWindowWidth: root.chatWindowSize.width
            property int normalWindowHeight: root.chatWindowSize.height
            property bool isMaximized: windowMaximized
            property int windowRadius: isMaximized ? 0 : 24
            readonly property int glassInset: isMaximized ? 0 : 4
            readonly property int shellContentInset: isMaximized ? 0 : 8

            onClosing: function(close) {
                close.accepted = false
                Qt.quit()
            }

            Item {
                id: chatShell
                anchors.fill: parent
                anchors.margins: chatWindow.glassInset

                LinuxComponents.WindowGlassShell {
                    anchors.fill: parent
                    cornerRadius: chatWindow.windowRadius
                    fillTopColor: Qt.rgba(0.93, 0.97, 1.0, 0.58)
                    fillBottomColor: Qt.rgba(0.86, 0.93, 1.0, 0.54)
                    glowTopColor: Qt.rgba(1, 1, 1, 0.20)
                    glowMiddleColor: Qt.rgba(0.92, 0.97, 1.0, 0.08)
                    glowBottomColor: Qt.rgba(0.74, 0.84, 0.96, 0.10)
                    strokeColor: chatWindow.isMaximized ? "transparent" : Qt.rgba(1, 1, 1, 0.40)
                    strokeWidth: chatWindow.isMaximized ? 0 : 0.9
                }

                Loader {
                    anchors.fill: parent
                    anchors.margins: chatWindow.shellContentInset
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
                anchors.topMargin: chatWindow.isMaximized
                                   ? 10
                                   : chatWindow.glassInset + chatWindow.shellContentInset + 4
                anchors.rightMargin: chatWindow.isMaximized
                                     ? 10
                                     : chatWindow.glassInset + chatWindow.shellContentInset + 18
                width: chatControlsRow.implicitWidth + root.controlHoverPadding * 4
                height: chatControlsRow.implicitHeight + root.controlHoverPadding * 2 + 2

                Rectangle {
                    anchors.fill: parent
                    radius: height / 2
                    antialiasing: true
                    color: Qt.rgba(1, 1, 1, 0.32)
                    border.color: Qt.rgba(1, 1, 1, 0.54)
                }

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
                id: chatResizeFrame
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
