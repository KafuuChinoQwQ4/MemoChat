import QtQuick 2.15
import QtQuick.Window 2.15
import MemoChat 1.0

Window {
    id: root
    width: 280
    height: 420
    minimumWidth: 220
    minimumHeight: 320
    title: "MemoChat Pet"
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    visible: false

    property var petController: null
    property bool alwaysOnTop: true
    property bool clickThrough: false
    property bool decorativeMode: false
    property bool debugPanelVisible: false
    property real scaleFactor: 1.0
    property bool micMuted: true
    property bool cameraEnabled: false
    property bool cloudVisionEnabled: false
    property bool localOnlyMode: true
    property bool debugRetentionEnabled: false
    property bool providerAvailable: false

    function openPet() {
        if (petController && petController.sessionId.length === 0) {
            petController.startSession()
        }
        clickThrough = false
        decorativeMode = false
        root.applyWindowFlags()
        show()
        raise()
    }

    function resetPosition() {
        scaleFactor = 1.0
        width = 280
        height = 420
        var availableWidth = Screen.desktopAvailableWidth > 0 ? Screen.desktopAvailableWidth : Screen.width
        var availableHeight = Screen.desktopAvailableHeight > 0 ? Screen.desktopAvailableHeight : Screen.height
        x = Math.max(24, Math.round(availableWidth - width - 48))
        y = Math.max(24, Math.round((availableHeight - height) * 0.62))
    }

    function applyWindowFlags() {
        var nextFlags = Qt.Tool | Qt.FramelessWindowHint
        if (alwaysOnTop) {
            nextFlags |= Qt.WindowStaysOnTopHint
        }
        if (clickThrough) {
            nextFlags |= Qt.WindowTransparentForInput
        }
        flags = nextFlags
    }

    onAlwaysOnTopChanged: applyWindowFlags()
    onClickThroughChanged: {
        decorativeMode = clickThrough
        applyWindowFlags()
    }

    Rectangle {
        anchors.fill: parent
        radius: 18
        color: Qt.rgba(0.13, 0.17, 0.22, 0.22)
        border.color: Qt.rgba(1, 1, 1, 0.28)
    }

    PetScene {
        anchors.fill: parent
        petController: root.petController
        alwaysOnTop: root.alwaysOnTop
        clickThrough: root.clickThrough
        decorativeMode: root.decorativeMode
        debugPanelVisible: root.debugPanelVisible
        scaleFactor: root.scaleFactor
        micMuted: root.micMuted
        cameraEnabled: root.cameraEnabled
        cloudVisionEnabled: root.cloudVisionEnabled
        localOnlyMode: root.localOnlyMode
        debugRetentionEnabled: root.debugRetentionEnabled
        providerAvailable: root.providerAvailable
        onCloseRequested: root.hide()
        onResetPositionRequested: root.resetPosition()
        onAlwaysOnTopToggled: function(value) { root.alwaysOnTop = value }
        onClickThroughToggled: function(value) { root.clickThrough = value }
        onDebugToggled: function(value) { root.debugPanelVisible = value }
        onScaleRequested: function(value) {
            root.scaleFactor = Math.max(0.65, Math.min(1.45, value))
            root.width = Math.round(280 * root.scaleFactor)
            root.height = Math.round(420 * root.scaleFactor)
        }
        onMicMuteToggled: function(value) { root.micMuted = value }
        onCameraToggled: function(value) { root.cameraEnabled = value }
        onCloudVisionToggled: function(value) { root.cloudVisionEnabled = value }
        onLocalOnlyModeToggled: function(value) { root.localOnlyMode = value }
        onDebugRetentionToggled: function(value) { root.debugRetentionEnabled = value }
    }

    DragHandler {
        target: null
        enabled: !root.clickThrough
        onActiveChanged: {
            if (active) {
                root.startSystemMove()
            }
        }
    }

    Component.onCompleted: applyWindowFlags()
}
