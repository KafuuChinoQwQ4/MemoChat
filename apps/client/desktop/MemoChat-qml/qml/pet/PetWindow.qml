pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Window 2.15
import MemoChat 1.0

Window {
    id: root
    readonly property int baseWindowWidth: 320
    readonly property int baseWindowHeight: 460
    readonly property int panelGap: 12

    width: scaledWindowWidth()
    height: scaledWindowHeight()
    minimumWidth: Math.round(baseWindowWidth * 0.65)
    minimumHeight: Math.round(baseWindowHeight * 0.65)
    title: "MemoChat Pet"
    flags: (Qt.platform.os === "linux" ? Qt.Window : Qt.Tool)
           | Qt.FramelessWindowHint
           | Qt.WindowStaysOnTopHint
    color: "transparent"
    visible: false

    property var petController: null
    property var agentController: null
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
    property bool voiceReplyEnabled: true
    property bool providerAvailable: false
    property var petAssetSettings: null
    property string selfAvatar: "qrc:/res/head_1.jpg"
    property bool positionInitialized: false
    property bool scaleInteractionActive: false
    property bool panelPositionPending: false
    property bool chatPositionPending: false
    property var petChatWindowRef: null
    property var petControlWindowRef: null

    function scaledWindowWidth(value) {
        var factor = value === undefined ? root.scaleFactor : value
        return Math.round(root.baseWindowWidth * factor)
    }

    function scaledWindowHeight(value) {
        var factor = value === undefined ? root.scaleFactor : value
        return Math.round(root.baseWindowHeight * factor)
    }

    function openPet() {
        if (petController && petController.sessionId.length === 0) {
            petController.startSession()
        }
        if (!positionInitialized) {
            resetPosition()
        }
        clickThrough = false
        decorativeMode = false
        root.applyWindowFlags()
        show()
        raise()
    }

    function beginWindowDrag() {
        root.startSystemMove()
    }

    function openPetChat() {
        if (!petChatWindowRef) {
            petChatWindowRef = petChatWindowComponent.createObject(null, {
                "petController": root.petController,
                "agentController": root.agentController,
                "petAssetSettings": root.petAssetSettings,
                "selfAvatar": root.selfAvatar
            })
        } else {
            petChatWindowRef.petController = root.petController
            petChatWindowRef.agentController = root.agentController
            petChatWindowRef.petAssetSettings = root.petAssetSettings
            petChatWindowRef.selfAvatar = root.selfAvatar
        }
        if (!petChatWindowRef) {
            console.warn("Failed to create pet chat window")
            return
        }
        syncChatWindowState()
        scheduleChatWindowPosition()
        petChatWindowRef.openChat()
    }

    function ensureChatWindow() {
        if (!petChatWindowRef) {
            petChatWindowRef = petChatWindowComponent.createObject(null, {
                "petController": root.petController,
                "agentController": root.agentController,
                "petAssetSettings": root.petAssetSettings,
                "selfAvatar": root.selfAvatar
            })
        }
        if (petChatWindowRef) {
            syncChatWindowState()
        }
        return petChatWindowRef
    }

    function syncChatWindowState() {
        if (!petChatWindowRef) {
            return
        }
        petChatWindowRef.petController = root.petController
        petChatWindowRef.agentController = root.agentController
        petChatWindowRef.petAssetSettings = root.petAssetSettings
        petChatWindowRef.selfAvatar = root.selfAvatar
        petChatWindowRef.alwaysOnTop = root.alwaysOnTop
        petChatWindowRef.clickThrough = root.clickThrough
    }

    function positionChatWindow() {
        var panel = petChatWindowRef
        if (!panel) {
            return
        }
        syncChatWindowState()
        var areaX = 0
        var areaY = 0
        var areaWidth = Screen.desktopAvailableWidth > 0 ? Screen.desktopAvailableWidth : Screen.width
        var areaHeight = Screen.desktopAvailableHeight > 0 ? Screen.desktopAvailableHeight : Screen.height
        var rightX = root.x + root.width + root.panelGap
        var leftX = root.x - panel.width - root.panelGap
        var rightSpace = areaX + areaWidth - rightX
        var leftSpace = leftX - areaX
        var rightFits = rightSpace >= panel.width
        var leftFits = leftSpace >= 0
        var nextX = rightFits || (!leftFits && rightSpace >= leftSpace) ? rightX : leftX
        var desiredY = root.y + Math.round((root.height - panel.height) / 2)
        panel.x = Math.max(areaX + 8, Math.min(nextX, areaX + areaWidth - panel.width - 8))
        panel.y = Math.max(areaY + 8, Math.min(desiredY, areaY + areaHeight - panel.height - 8))
    }

    function scheduleChatWindowPosition() {
        if (!petChatWindowRef) {
            return
        }
        if (scaleInteractionActive) {
            chatPositionPending = true
            return
        }
        chatPositionPending = false
        positionChatWindow()
    }

    function ensureControlWindow() {
        if (!petControlWindowRef) {
            petControlWindowRef = petControlWindowComponent.createObject(null, {
                "petController": root.petController,
                "agentController": root.agentController
            })
        }
        if (petControlWindowRef) {
            syncControlWindowState()
        }
        return petControlWindowRef
    }

    function openControlWindow() {
        var panel = ensureControlWindow()
        if (!panel) {
            console.warn("Failed to create pet control window")
            return
        }
        scheduleControlWindowPosition()
        panel.openPanel()
    }

    function scheduleControlWindowPosition() {
        if (!petControlWindowRef) {
            return
        }
        if (scaleInteractionActive) {
            panelPositionPending = true
            return
        }
        panelPositionPending = false
        positionControlWindow()
    }

    function beginScaleInteraction() {
        scaleInteractionActive = true
        panelPositionPending = false
    }

    function endScaleInteraction() {
        scaleInteractionActive = false
        scheduleControlWindowPosition()
        scheduleChatWindowPosition()
    }

    function positionControlWindow() {
        var panel = petControlWindowRef
        if (!panel) {
            return
        }
        syncControlWindowState()
        var areaX = 0
        var areaY = 0
        var areaWidth = Screen.desktopAvailableWidth > 0 ? Screen.desktopAvailableWidth : Screen.width
        var areaHeight = Screen.desktopAvailableHeight > 0 ? Screen.desktopAvailableHeight : Screen.height
        var rightX = root.x + root.width + root.panelGap
        var leftX = root.x - panel.width - root.panelGap
        var rightSpace = areaX + areaWidth - rightX
        var leftSpace = leftX - areaX
        var rightFits = rightSpace >= panel.width
        var leftFits = leftSpace >= 0
        var nextX = rightFits || (!leftFits && rightSpace >= leftSpace) ? rightX : leftX
        var desiredY = root.y + Math.round((root.height - panel.height) / 2)
        panel.x = Math.max(areaX + 8, Math.min(nextX, areaX + areaWidth - panel.width - 8))
        panel.y = Math.max(areaY + 8, Math.min(desiredY, areaY + areaHeight - panel.height - 8))
    }

    function syncControlWindowState() {
        if (!petControlWindowRef) {
            return
        }
        petControlWindowRef.petController = root.petController
        petControlWindowRef.agentController = root.agentController
        petControlWindowRef.alwaysOnTop = root.alwaysOnTop
        petControlWindowRef.clickThrough = root.clickThrough
        petControlWindowRef.debugPanelVisible = root.debugPanelVisible
        petControlWindowRef.scaleFactor = root.scaleFactor
        petControlWindowRef.micMuted = root.micMuted
        petControlWindowRef.cameraEnabled = root.cameraEnabled
        petControlWindowRef.cloudVisionEnabled = root.cloudVisionEnabled
        petControlWindowRef.localOnlyMode = root.localOnlyMode
        petControlWindowRef.debugRetentionEnabled = root.debugRetentionEnabled
        petControlWindowRef.voiceReplyEnabled = root.voiceReplyEnabled
        petControlWindowRef.providerAvailable = root.providerAvailable
    }

    function resetPosition() {
        scaleFactor = 1.0
        width = scaledWindowWidth()
        height = scaledWindowHeight()
        var availableWidth = Screen.desktopAvailableWidth > 0 ? Screen.desktopAvailableWidth : Screen.width
        var availableHeight = Screen.desktopAvailableHeight > 0 ? Screen.desktopAvailableHeight : Screen.height
        x = Math.max(24, Math.round(availableWidth - width - 48))
        y = Math.max(24, Math.round((availableHeight - height) * 0.62))
        positionInitialized = true
        positionControlWindow()
        positionChatWindow()
    }

    function applyWindowFlags() {
        var nextFlags = (Qt.platform.os === "linux" ? Qt.Window : Qt.Tool) | Qt.FramelessWindowHint
        if (alwaysOnTop) {
            nextFlags |= Qt.WindowStaysOnTopHint
        }
        if (clickThrough) {
            nextFlags |= Qt.WindowTransparentForInput
        }
        flags = nextFlags
    }

    function applyScale(value) {
        var nextScale = Math.max(0.65, Math.min(2.2, value))
        if (Math.abs(nextScale - root.scaleFactor) < 0.001) {
            return
        }
        root.scaleFactor = nextScale
        root.width = scaledWindowWidth(nextScale)
        root.height = scaledWindowHeight(nextScale)
        positionInitialized = true
        scheduleControlWindowPosition()
        syncControlWindowState()
    }

    onClickThroughChanged: {
        decorativeMode = clickThrough
        applyWindowFlags()
        syncControlWindowState()
    }
    onXChanged: {
        scheduleControlWindowPosition()
        scheduleChatWindowPosition()
    }
    onYChanged: {
        scheduleControlWindowPosition()
        scheduleChatWindowPosition()
    }
    onWidthChanged: {
        scheduleControlWindowPosition()
        scheduleChatWindowPosition()
    }
    onHeightChanged: {
        scheduleControlWindowPosition()
        scheduleChatWindowPosition()
    }
    onPetControllerChanged: {
        syncControlWindowState()
        syncChatWindowState()
    }
    onAgentControllerChanged: {
        syncControlWindowState()
        syncChatWindowState()
    }
    onPetAssetSettingsChanged: {
        syncControlWindowState()
        syncChatWindowState()
    }
    onSelfAvatarChanged: syncChatWindowState()

    PetScene {
        anchors.fill: parent
        petController: root.petController
        clickThrough: root.clickThrough
        decorativeMode: root.decorativeMode
        scaleFactor: root.scaleFactor
        cameraEnabled: root.cameraEnabled
        cloudVisionEnabled: root.cloudVisionEnabled
        voiceReplyEnabled: root.voiceReplyEnabled
        petAssetSettings: root.petAssetSettings
        localOnlyMode: root.localOnlyMode
        debugRetentionEnabled: root.debugRetentionEnabled
        debugPanelVisible: root.debugPanelVisible
        providerAvailable: root.providerAvailable
        onDragRequested: root.beginWindowDrag()
        onControlsRequested: root.openControlWindow()
        onLocalOnlyModeToggled: function(value) { root.localOnlyMode = value; root.syncControlWindowState() }
        onDebugRetentionToggled: function(value) { root.debugRetentionEnabled = value; root.syncControlWindowState() }
    }

    Component {
        id: petChatWindowComponent
        PetChatWindow { }
    }

    Component {
        id: petControlWindowComponent
        PetControlWindow {
            onClosePetRequested: {
                hide()
                root.hide()
            }
            onChatRequested: root.openPetChat()
            onResetPositionRequested: root.resetPosition()
            onAlwaysOnTopToggled: function(value) { root.alwaysOnTop = value }
            onClickThroughToggled: function(value) { root.clickThrough = value }
            onDebugToggled: function(value) { root.debugPanelVisible = value; root.syncControlWindowState() }
            onScaleInteractionStarted: root.beginScaleInteraction()
            onScaleRequested: function(value) { root.applyScale(value) }
            onScaleInteractionFinished: root.endScaleInteraction()
            onMicMuteToggled: function(value) { root.micMuted = value; root.syncControlWindowState() }
            onCameraToggled: function(value) { root.cameraEnabled = value; root.syncControlWindowState() }
            onCloudVisionToggled: function(value) { root.cloudVisionEnabled = value; root.syncControlWindowState() }
            onLocalOnlyModeToggled: function(value) { root.localOnlyMode = value; root.syncControlWindowState() }
            onDebugRetentionToggled: function(value) { root.debugRetentionEnabled = value; root.syncControlWindowState() }
            onVoiceReplyToggled: function(value) { root.voiceReplyEnabled = value; root.syncControlWindowState() }
        }
    }

    onAlwaysOnTopChanged: {
        applyWindowFlags()
        if (petControlWindowRef) {
            var panelFlags = (Qt.platform.os === "linux" ? Qt.Window : Qt.Tool) | Qt.FramelessWindowHint
            if (alwaysOnTop) {
                panelFlags |= Qt.WindowStaysOnTopHint
            }
            petControlWindowRef.flags = panelFlags
        }
        if (petChatWindowRef) {
            petChatWindowRef.alwaysOnTop = alwaysOnTop
        }
    }

    Component.onDestruction: {
        if (petChatWindowRef) {
            petChatWindowRef.destroy()
            petChatWindowRef = null
        }
        if (petControlWindowRef) {
            petControlWindowRef.destroy()
            petControlWindowRef = null
        }
    }

    Component.onCompleted: applyWindowFlags()
}
