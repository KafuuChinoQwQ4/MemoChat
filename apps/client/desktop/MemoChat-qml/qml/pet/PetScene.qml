import QtQuick 2.15
import QtQuick.Controls 2.15
import MemoChat 1.0

Item {
    id: root
    property var petController: null
    property bool clickThrough: false
    property bool decorativeMode: false
    property real scaleFactor: 1.0
    property bool cameraEnabled: false
    property bool cloudVisionEnabled: false
    property bool voiceReplyEnabled: true
    property var petAssetSettings: null
    property bool localOnlyMode: true
    property bool debugRetentionEnabled: false
    property bool debugPanelVisible: false
    property bool providerAvailable: false
    property string cameraCaptureStatus: cameraEnabled ? "摄像头本地捕捉" : "摄像头关闭"
    property bool manualActionActive: false
    property string manualExpression: "neutral"
    property string manualMotion: "idle"
    property string manualEmotion: "neutral"
    property int manualActionSerial: 0

    signal dragRequested()
    signal controlsRequested(real sceneX, real sceneY)
    signal localOnlyModeToggled(bool checked)
    signal debugRetentionToggled(bool checked)

    readonly property color sceneTintColor: Qt.rgba(1.0, 0.96, 0.98, 0.96)

    function speechPlaybackText() {
        if (!root.petController) {
            return ""
        }
        if (!root.petController.speechFinal) {
            return ""
        }
        return root.petController.speechDisplayText.length > 0
                ? root.petController.speechDisplayText
                : root.petController.speechText
    }

    function micPrivacyText() {
        return root.voiceReplyEnabled ? "麦克风 开启" : "麦克风 关闭"
    }

    function cameraPrivacyText() {
        return root.cameraEnabled ? "摄像头 开启" : "摄像头 关闭"
    }

    function cloudPrivacyText() {
        if (root.localOnlyMode) {
            return "云视觉 本地锁定"
        }
        if (!root.cloudVisionEnabled) {
            return "云视觉 关闭"
        }
        return root.providerAvailable ? "云视觉 已授权" : "云视觉 等待提供方"
    }

    function localModeText() {
        return root.localOnlyMode ? "本地优先" : "允许云能力"
    }

    function debugRetentionText() {
        return root.debugRetentionEnabled ? "调试保留 开启" : "调试保留 关闭"
    }

    function visionDiagnosticText() {
        if (!root.cameraEnabled) {
            return "摄像头关闭"
        }
        return root.cameraCaptureStatus.length > 0 ? root.cameraCaptureStatus : "等待摄像头状态"
    }

    function applyLive2DAction(action) {
        if (!action) {
            return
        }
        var kind = action.kind || ""
        var trigger = action.trigger || action.name || ""
        if (trigger.length === 0) {
            return
        }
        root.manualActionActive = true
        root.manualActionSerial += 1
        if (kind === "expression") {
            root.manualExpression = trigger
            root.manualEmotion = trigger
        } else if (kind === "motion") {
            root.manualMotion = trigger
        } else {
            root.manualExpression = trigger
            root.manualMotion = trigger
            root.manualEmotion = trigger
        }
    }

    function clearManualLive2DAction() {
        root.manualActionActive = false
        root.manualExpression = "neutral"
        root.manualMotion = "idle"
        root.manualEmotion = "neutral"
        root.manualActionSerial += 1
    }

    function privacyColor() {
        if (root.localOnlyMode) {
            return "#b7a6b0"
        }
        if (!root.cloudVisionEnabled) {
            return "#b7a6b0"
        }
        return root.providerAvailable ? "#74b2ba" : "#d28ca6"
    }

    Live2DRenderItem {
        id: live2d
        anchors.fill: parent
        modelRoot: root.petAssetSettings ? root.petAssetSettings.modelRoot : ""
        modelJson: root.petAssetSettings ? root.petAssetSettings.modelJson : ""
        motionDirectory: root.petAssetSettings ? root.petAssetSettings.motionDirectory : ""
        expressionDirectory: root.petAssetSettings ? root.petAssetSettings.expressionDirectory : ""
        expression: root.manualActionActive ? root.manualExpression
                                            : (root.petController ? root.petController.expression : "neutral")
        motion: root.manualActionActive ? root.manualMotion
                                        : (root.petController ? root.petController.motion : "idle")
        emotion: root.manualActionActive ? root.manualEmotion
                                         : (root.petController ? root.petController.emotion : "neutral")
        intensity: root.petController ? root.petController.intensity : 0.35
        gazeX: root.petController ? root.petController.gazeX : 0.5
        gazeY: root.petController ? root.petController.gazeY : 0.5
        lipSyncValue: root.petController ? root.petController.lipSyncValue : 0
        actionSerial: root.manualActionSerial
    }

    Loader {
        id: petAudioLoader
        active: root.voiceReplyEnabled && root.petController
        source: "PetAudioPlayer.qml"
        onLoaded: {
            item.textToSpeechFallbackEnabled = false
            item.speechKey = root.petController ? root.petController.turnId : ""
            item.sourceUrl = root.petController ? root.petController.audioUrl : ""
            item.playbackState = root.petController ? root.petController.audioState : "idle"
            item.speechText = root.speechPlaybackText()
            item.speechFinal = root.petController ? root.petController.speechFinal : false
        }
    }

    Loader {
        id: petCameraLoader
        active: root.cameraEnabled && root.petController
        source: "PetCameraCapture.qml"
        onLoaded: {
            item.petController = root.petController
            item.cameraEnabled = root.cameraEnabled
            item.cloudVisionEnabled = root.cloudVisionEnabled
            root.cameraCaptureStatus = item.statusText
            item.statusChanged.connect(function(text) {
                root.cameraCaptureStatus = text
            })
        }
    }

    Connections {
        target: root.petController
        function onPetStateChanged() {
            if (root.voiceReplyEnabled && petAudioLoader.item) {
                petAudioLoader.item.textToSpeechFallbackEnabled = false
                petAudioLoader.item.speechKey = root.petController ? root.petController.turnId : ""
                petAudioLoader.item.sourceUrl = root.petController ? root.petController.audioUrl : ""
                petAudioLoader.item.playbackState = root.petController ? root.petController.audioState : "idle"
                petAudioLoader.item.speechText = root.speechPlaybackText()
                petAudioLoader.item.speechFinal = root.petController ? root.petController.speechFinal : false
            }
            if (petCameraLoader.item) {
                petCameraLoader.item.petController = root.petController
            }
        }

        function onControlEventReceived(event) {
            live2d.applyControlEvent(event)
        }
    }

    Rectangle {
        visible: root.debugPanelVisible
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 10
        anchors.bottomMargin: 10
        width: 182
        height: 118
        radius: 10
        antialiasing: true
        color: root.sceneTintColor
        border.color: root.privacyColor()

        Column {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            Label {
                width: parent.width
                text: root.cloudPrivacyText()
                color: "#4b3042"
                font.pixelSize: 11
                font.bold: true
                elide: Text.ElideRight
            }

            Label {
                width: parent.width
                text: root.localModeText()
                color: "#4b3042"
                font.pixelSize: 11
                elide: Text.ElideRight
            }

            Label {
                width: parent.width
                text: root.visionDiagnosticText()
                color: "#7a6b78"
                font.pixelSize: 10
                elide: Text.ElideRight
            }

            Row {
                width: parent.width
                spacing: 6

                Label {
                    text: "调试保留"
                    color: "#4b3042"
                    font.pixelSize: 11
                }

                Switch {
                    checked: root.debugRetentionEnabled
                    onToggled: root.debugRetentionToggled(checked)
                }
            }

            Label {
                width: parent.width
                text: root.debugRetentionText()
                color: "#7a6b78"
                font.pixelSize: 10
                elide: Text.ElideRight
            }
        }
    }

    onVoiceReplyEnabledChanged: {
        if (!root.voiceReplyEnabled && petAudioLoader.item) {
            petAudioLoader.item.textToSpeechFallbackEnabled = false
            petAudioLoader.item.sourceUrl = ""
            petAudioLoader.item.playbackState = "stopped"
            petAudioLoader.item.speechFinal = false
        }
    }

    onCameraEnabledChanged: {
        if (petCameraLoader.item) {
            petCameraLoader.item.cameraEnabled = root.cameraEnabled
        }
        cameraCaptureStatus = root.cameraEnabled ? "摄像头本地捕捉" : "摄像头关闭"
    }

    onCloudVisionEnabledChanged: {
        if (petCameraLoader.item) {
            petCameraLoader.item.cloudVisionEnabled = root.cloudVisionEnabled
        }
    }

    DragHandler {
        id: petDragHandler
        target: null
        enabled: !root.clickThrough
        acceptedButtons: Qt.LeftButton
        onActiveChanged: {
            if (active) {
                root.dragRequested()
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: !root.clickThrough
        acceptedButtons: Qt.RightButton
        hoverEnabled: true
        cursorShape: petDragHandler.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        propagateComposedEvents: true
        onPressed: function(mouse) {
            if (mouse.button !== Qt.RightButton) {
                mouse.accepted = false
            }
        }
        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                root.controlsRequested(mouse.x, mouse.y)
                mouse.accepted = true
            } else {
                mouse.accepted = false
            }
        }
    }

}
