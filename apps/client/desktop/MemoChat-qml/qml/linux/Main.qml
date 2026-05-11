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
    property bool isMaximized: visibility === Window.Maximized
    property int windowRadius: isMaximized ? 0 : 24
    readonly property int glassInset: isMaximized ? 0 : 4
    readonly property int shellContentInset: isMaximized ? 0 : 8
    property string appTitle: "MemoChat QML"
    property size loginWindowSize: Qt.size(300, 500)
    property size chatWindowSize: Qt.size(900, 640)
    property var chatWindowRef: null
    property var petWindowRef: null

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

    function ensurePetWindow() {
        if (petWindowRef) {
            return petWindowRef
        }
        petWindowRef = petWindowComponent.createObject(null, {
            "petController": controller.petController
        })
        return petWindowRef
    }

    function togglePetWindow() {
        const win = ensurePetWindow()
        if (!win) {
            console.warn("Failed to create pet window")
            return
        }
        if (win.visible) {
            win.hide()
        } else {
            win.openPet()
        }
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
        }
    }

    Component.onCompleted: syncWindowsByPage()
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
    }

    Item {
        id: shell
        anchors.fill: parent
        anchors.margins: root.glassInset

        LinuxComponents.WindowGlassShell {
            anchors.fill: parent
            cornerRadius: root.windowRadius
            fillTopColor: Qt.rgba(0.90, 0.96, 1.0, 0.34)
            fillBottomColor: Qt.rgba(0.83, 0.91, 1.0, 0.32)
            glowTopColor: Qt.rgba(1, 1, 1, 0.18)
            glowMiddleColor: Qt.rgba(0.92, 0.97, 1.0, 0.06)
            glowBottomColor: Qt.rgba(0.74, 0.84, 0.96, 0.08)
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
        anchors.topMargin: 20
        anchors.rightMargin: 14
        width: controlsRow.implicitWidth
        height: controlsRow.implicitHeight

        Row {
            id: controlsRow
            spacing: 20

            SharedComponents.LoginIconButton {
                iconSource: "qrc:/icons/minimize.png"
                onClicked: root.showMinimized()
            }

            SharedComponents.LoginIconButton {
                iconSource: "qrc:/icons/maximize.png"
                onClicked: {
                    if (root.visibility === Window.Maximized) {
                        root.showNormal()
                    } else {
                        root.showMaximized()
                    }
                }
            }

            SharedComponents.LoginIconButton {
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
            property real acrylicPinkProgress: 0.0
            minimumWidth: 800
            minimumHeight: 600
            maximumWidth: 100000
            maximumHeight: 100000
            width: root.chatWindowSize.width
            height: root.chatWindowSize.height
            property bool isMaximized: visibility === Window.Maximized
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
                    fillTopColor: Qt.rgba(0.90, 0.96, 1.0, 0.38)
                    fillBottomColor: Qt.rgba(0.83, 0.91, 1.0, 0.34)
                    glowTopColor: Qt.rgba(1, 1, 1, 0.16)
                    glowMiddleColor: Qt.rgba(0.92, 0.97, 1.0, 0.06)
                    glowBottomColor: Qt.rgba(0.74, 0.84, 0.96, 0.08)
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
                        onClicked: {
                            if (chatWindow.visibility === Window.Maximized) {
                                chatWindow.showNormal()
                            } else {
                                chatWindow.showMaximized()
                            }
                        }
                    }

                    SharedComponents.LoginIconButton {
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
