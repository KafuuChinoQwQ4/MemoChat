pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Window 2.15
import MemoChat 1.0

Window {
    id: root
    readonly property int baseWindowWidth: 320
    readonly property int baseWindowHeight: 460
    readonly property int speechBubbleSafeHeight: 84
    readonly property int panelGap: 12

    width: scaledWindowWidth()
    height: scaledWindowHeight()
    minimumWidth: Math.round(baseWindowWidth * 0.65)
    minimumHeight: speechBubbleSafeHeight + Math.round(baseWindowHeight * 0.65)
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
    property string cameraCaptureStatus: cameraEnabled ? "摄像头本地捕捉" : "摄像头关闭"
    property bool cloudVisionEnabled: false
    property bool localOnlyMode: true
    property bool debugRetentionEnabled: false
    property bool voiceReplyEnabled: true
    property bool providerAvailable: false
    property var petAssetSettings: null
    readonly property var petSettingsSignalTarget: root.petAssetSettings
                                                   && root.petAssetSettings.settingsChanged !== undefined
                                                   ? root.petAssetSettings : null
    property string selfAvatar: "qrc:/res/head_1.jpg"
    property bool positionInitialized: false
    property bool scaleInteractionActive: false
    property bool panelPositionPending: false
    property bool chatPositionPending: false
    property var petChatWindowRef: null
    property var petControlWindowRef: null
    property var petSceneItem: null

    function scaledWindowWidth(value) {
        var factor = value === undefined ? root.scaleFactor : value
        return Math.round(root.baseWindowWidth * factor)
    }

    function scaledWindowHeight(value) {
        var factor = value === undefined ? root.scaleFactor : value
        return root.speechBubbleSafeHeight + Math.round(root.baseWindowHeight * factor)
    }

    function openPet() {
        syncPetRuntimeSettings()
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
        petChatWindowRef.voiceCallActive = root.voiceReplyEnabled
    }

    function settingsLanguageCode() {
        if (!root.petAssetSettings) {
            return "zh-CN"
        }
        const index = root.petAssetSettings.languageIndex !== undefined ? root.petAssetSettings.languageIndex : 0
        switch (index) {
        case 1:
            return "ja-JP"
        case 2:
            return "en-US"
        case 3:
            return "ko-KR"
        case 4:
            return "fr-FR"
        case 5:
            return "es-ES"
        default:
            return "zh-CN"
        }
    }

    function settingsSpeechRulesText() {
        if (!root.petAssetSettings || root.petAssetSettings.speechRules === undefined) {
            return ""
        }
        return String(root.petAssetSettings.speechRules || "").trim()
    }

    function petSettingText(name) {
        if (!root.petAssetSettings || root.petAssetSettings[name] === undefined || root.petAssetSettings[name] === null) {
            return ""
        }
        return String(root.petAssetSettings[name] || "").trim()
    }

    function settingsVoicePath() {
        const voiceDirectory = petSettingText("voiceDirectory")
        const defaultVoice = petSettingText("defaultVoice")
        if (voiceDirectory.length === 0 || defaultVoice.length === 0) {
            return ""
        }
        if (defaultVoice.indexOf("/") === 0 || defaultVoice.indexOf("\\") === 0
                || defaultVoice.indexOf(":/") > 0 || /^[A-Za-z]:[\\/]/.test(defaultVoice)) {
            return defaultVoice
        }
        const separator = voiceDirectory.endsWith("/") || voiceDirectory.endsWith("\\") ? "" : "/"
        return voiceDirectory + separator + defaultVoice
    }

    function settingsVoiceName() {
        const defaultVoice = petSettingText("defaultVoice")
        if (defaultVoice.length > 0) {
            const normalized = defaultVoice.replace(/\\/g, "/")
            const baseName = normalized.split("/").pop()
            return baseName.replace(/\.[^.]+$/, "")
        }
        return petSettingText("characterName")
    }

    function syncPetRuntimeSettings() {
        if (!root.petController) {
            return
        }
        if (root.petController.setReplyLanguage) {
            root.petController.setReplyLanguage(settingsLanguageCode())
        }
        if (root.petController.setSpeechRules) {
            root.petController.setSpeechRules(settingsSpeechRulesText())
        }
        if (root.petController.setVoiceRuntimeSettings) {
            const voicePath = settingsVoicePath()
            root.petController.setVoiceRuntimeSettings({
                "voiceProvider": voicePath.length > 0 ? "gpt-sovits" : "",
                "voiceName": settingsVoiceName(),
                "voiceLanguage": settingsLanguageCode(),
                "textLanguage": settingsLanguageCode().split("-")[0].toLowerCase(),
                "referenceAudioPath": voicePath,
                "voiceDirectory": petSettingText("voiceDirectory"),
                "defaultVoice": petSettingText("defaultVoice"),
                "voiceTrainingArtifactPath": petSettingText("voiceTrainingArtifactPath"),
                "referenceAudioSource": voicePath.length > 0 ? "pet_asset_settings" : ""
            })
        }
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
                "agentController": root.agentController,
                "petAssetSettings": root.petAssetSettings
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
        petControlWindowRef.petAssetSettings = root.petAssetSettings
        petControlWindowRef.alwaysOnTop = root.alwaysOnTop
        petControlWindowRef.clickThrough = root.clickThrough
        petControlWindowRef.debugPanelVisible = root.debugPanelVisible
        petControlWindowRef.scaleFactor = root.scaleFactor
        petControlWindowRef.micMuted = root.micMuted
        petControlWindowRef.cameraEnabled = root.cameraEnabled
        petControlWindowRef.cameraCaptureStatus = root.cameraCaptureStatus
        petControlWindowRef.cloudVisionEnabled = root.cloudVisionEnabled
        petControlWindowRef.localOnlyMode = root.localOnlyMode
        petControlWindowRef.debugRetentionEnabled = root.debugRetentionEnabled
        petControlWindowRef.voiceReplyEnabled = root.voiceReplyEnabled
        petControlWindowRef.providerAvailable = root.providerAvailable
    }

    function applyLive2DAction(action) {
        if (petSceneItem && petSceneItem.applyLive2DAction) {
            petSceneItem.applyLive2DAction(action)
        }
    }

    function clearManualLive2DAction() {
        if (petSceneItem && petSceneItem.clearManualLive2DAction) {
            petSceneItem.clearManualLive2DAction()
        }
    }

    function providerRuntimeAvailable() {
        if (!root.agentController) {
            return false
        }
        var current = root.agentController.currentModel || ""
        if (current.length > 0) {
            return true
        }
        var models = root.agentController.availableModels || []
        if (models.length > 0) {
            return true
        }
        var providerStatus = root.agentController.apiProviderStatus || ""
        return providerStatus.indexOf("已接入") >= 0
    }

    function refreshProviderAvailability() {
        root.providerAvailable = root.providerRuntimeAvailable()
    }

    function enforceVisionPrivacy() {
        if ((root.localOnlyMode || !root.providerAvailable) && root.cloudVisionEnabled) {
            root.cloudVisionEnabled = false
        }
    }

    function setCloudVisionEnabled(value) {
        root.cloudVisionEnabled = value && !root.localOnlyMode && root.providerAvailable
        root.syncControlWindowState()
    }

    function setLocalOnlyMode(value) {
        root.localOnlyMode = value
        root.enforceVisionPrivacy()
        root.syncControlWindowState()
    }

    function applyAssetPrivacySettings() {
        if (!root.petAssetSettings) {
            return
        }
        root.cameraEnabled = root.petAssetSettings.cameraEnabled
        root.cloudVisionEnabled = root.petAssetSettings.cloudVisionEnabled
        root.enforceVisionPrivacy()
        root.syncPetRuntimeSettings()
        root.syncControlWindowState()
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
        syncPetRuntimeSettings()
        syncControlWindowState()
        syncChatWindowState()
    }
    onAgentControllerChanged: {
        refreshProviderAvailability()
        enforceVisionPrivacy()
        syncControlWindowState()
        syncChatWindowState()
    }
    onPetAssetSettingsChanged: {
        applyAssetPrivacySettings()
        syncPetRuntimeSettings()
        syncChatWindowState()
    }
    onSelfAvatarChanged: syncChatWindowState()
    onVoiceReplyEnabledChanged: {
        syncControlWindowState()
        syncChatWindowState()
    }

    PetScene {
        id: petScene
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
        onCameraCaptureStatusChanged: {
            root.cameraCaptureStatus = cameraCaptureStatus
            root.syncControlWindowState()
        }
        onDragRequested: root.beginWindowDrag()
        onControlsRequested: root.openControlWindow()
        onLocalOnlyModeToggled: function(value) { root.setLocalOnlyMode(value) }
        onDebugRetentionToggled: function(value) { root.debugRetentionEnabled = value; root.syncControlWindowState() }
        Component.onCompleted: root.petSceneItem = petScene
    }

    Component {
        id: petChatWindowComponent
        PetChatWindow {
            onVoiceChatRequested: function(active) { root.voiceReplyEnabled = active }
        }
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
            onCloudVisionToggled: function(value) { root.setCloudVisionEnabled(value) }
            onLocalOnlyModeToggled: function(value) { root.setLocalOnlyMode(value) }
            onDebugRetentionToggled: function(value) { root.debugRetentionEnabled = value; root.syncControlWindowState() }
            onVoiceReplyToggled: function(value) { root.voiceReplyEnabled = value; root.syncControlWindowState() }
            onLive2DActionRequested: function(action) { root.applyLive2DAction(action) }
            onLive2DAutoRequested: root.clearManualLive2DAction()
        }
    }

    Connections {
        target: root.agentController
        function onModelChanged() {
            root.refreshProviderAvailability()
            root.enforceVisionPrivacy()
            root.syncControlWindowState()
        }
        function onModelsChanged() {
            root.refreshProviderAvailability()
            root.enforceVisionPrivacy()
            root.syncControlWindowState()
        }
        function onModelStateChanged() {
            root.refreshProviderAvailability()
            root.enforceVisionPrivacy()
            root.syncControlWindowState()
        }
    }

    Connections {
        target: root.petSettingsSignalTarget
        ignoreUnknownSignals: true
        function onSettingsChanged() {
            root.applyAssetPrivacySettings()
            root.syncPetRuntimeSettings()
            root.syncChatWindowState()
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

    Component.onCompleted: {
        refreshProviderAvailability()
        applyAssetPrivacySettings()
        syncPetRuntimeSettings()
        applyWindowFlags()
    }
}
