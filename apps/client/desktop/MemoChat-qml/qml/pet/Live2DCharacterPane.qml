pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "../components"
import "Live2DCharacterRuntime.js" as Live2DCharacterRuntime

Rectangle {
    id: root
    color: "transparent"
    clip: true

    property var backdrop: null
    property var petController: null
    property string characterName: "香风智乃"
    property string roleIdentity: "Live2D 桌宠助手"
    property string modelRoot: "src/KafuuChino/香风智乃live2D"
    property string modelJson: "src/KafuuChino/香风智乃live2D/香风智乃.model3.json"
    property string motionDirectory: "src/KafuuChino/香风智乃live2D"
    property string expressionDirectory: "src/KafuuChino/香风智乃live2D"
    property string voiceDirectory: "src/KafuuChino/香风智乃voice"
    property string defaultVoice: "Kafuuchino-voice.mp3"
    property string idleMotion: "Idle"
    property string speakingMotion: "Talk"
    property string fallbackExpression: "脸红"
    property string personalityTags: "认真, 安静, 可靠, 轻声提醒"
    property string relationshipStyle: "熟悉但不过界的桌面同伴"
    property string worldSetting: "以香风智乃素材作为本地默认角色，在 MemoChat 旁边陪用户聊天、学习和整理资料。"
    property string speechRules: "使用当前选择的单一语言回复，不混用中文、日语或英语。少说套话，先回应情绪，再给明确建议。"
    property string catchphrases: "收到，我会记住。\n先别急，我们一步一步来。"
    property string forbiddenRules: "不要伪装成真人；不要主动索要隐私；不替用户做高风险决定。"
    property string draftStatus: ""
    property real emotionLevel: 0.62
    property real creativityLevel: 0.48
    property real voiceSpeed: 1.0
    property real lipSyncSensitivity: 0.55
    property bool voiceLipSyncEnabled: true
    property bool emotionSoundEnabled: true
    property bool idleMotionEnabled: true
    property bool gazeFollowEnabled: true
    property bool memoryEnabled: true
    property bool interruptEnabled: true
    property bool cameraEnabled: false
    property bool cloudVisionEnabled: false
    property bool autoStartPetOnClientStart: false
    property bool voiceTrainingConsent: false
    property string voiceTrainingConsentScope: "local_default_reference"
    property string voiceTrainingJobId: ""
    property string voiceTrainingStatus: "idle"
    property string voiceTrainingStage: ""
    property int voiceTrainingProgress: 0
    property string voiceTrainingArtifactPath: ""
    property string voiceTrainingMessage: "等待确认参考音频权限"
    property string characterAvatarSource: "qrc:/icons/modelive2d.png"

    signal petPreviewRequested(var petAssetSettings)

    readonly property color textPrimaryColor: "#253247"
    readonly property color textSecondaryColor: "#4e5d74"
    readonly property color textMutedColor: "#6e7f95"
    readonly property color accentBlue: Qt.rgba(0.32, 0.56, 0.86, 1.0)
    readonly property color accentGreen: Qt.rgba(0.32, 0.60, 0.44, 1.0)
    readonly property color accentRose: Qt.rgba(0.78, 0.36, 0.45, 1.0)
    readonly property string characterAvatarFallback: "qrc:/icons/modelive2d.png"

    PetAssetSettings {
        id: petAssetSettings
    }

    Live2DAsset {
        id: assetValidator
        modelRoot: root.modelRoot
        modelJson: root.modelJson
        motionDirectory: root.motionDirectory
        expressionDirectory: root.expressionDirectory
        voiceDirectory: root.voiceDirectory
        onAssetInputsChanged: assetValidationTimer.restart()
    }

    Timer {
        id: assetValidationTimer
        interval: 180
        repeat: false
        onTriggered: assetValidator.validate()
    }

    Component.onCompleted: {
        petAssetSettings.load()
        root.applySettingsToDraft()
        root.refreshCharacterAvatar()
        assetValidationTimer.start()
    }

    function pathDirectory(path) {
        return Live2DCharacterRuntime.pathDirectory(path)
    }

    function pathFileName(path) {
        return Live2DCharacterRuntime.pathFileName(path)
    }

    function pickPath(kind, picked, applySelection) {
        if (picked.length === 0) {
            draftStatus = "已取消选择" + kind
            return
        }
        applySelection(picked)
        draftStatus = "已选择" + kind
        root.refreshCharacterAvatar()
        assetValidationTimer.restart()
    }

    function refreshCharacterAvatar() {
        var nextAvatar = ""
        if (petAssetSettings && petAssetSettings.resolveLive2DAvatarUrl) {
            nextAvatar = petAssetSettings.resolveLive2DAvatarUrl(root.modelJson, root.modelRoot)
        }
        root.characterAvatarSource = nextAvatar && nextAvatar.length > 0 ? nextAvatar : root.characterAvatarFallback
    }

    function pickModelJson() {
        root.pickPath("模型文件",
                      petAssetSettings.pickLocalFilePath("选择 Live2D model3.json",
                                                         root.modelJson.length > 0 ? root.modelJson : root.modelRoot,
                                                         "Live2D 模型 (*.model3.json);;JSON 文件 (*.json);;所有文件 (*.*)"),
                      function(path) {
                          root.modelJson = path
                          var directory = root.pathDirectory(path)
                          if (directory.length > 0) {
                              root.modelRoot = directory
                          }
                      })
    }

    function pickModelRootDirectory() {
        root.pickPath("资源目录",
                      petAssetSettings.pickLocalDirectoryPath("选择 Live2D 资源目录", root.modelRoot),
                      function(path) { root.modelRoot = path })
    }

    function pickMotionDirectory() {
        root.pickPath("动作目录",
                      petAssetSettings.pickLocalDirectoryPath("选择动作目录", root.motionDirectory.length > 0 ? root.motionDirectory : root.modelRoot),
                      function(path) { root.motionDirectory = path })
    }

    function pickExpressionDirectory() {
        root.pickPath("表情目录",
                      petAssetSettings.pickLocalDirectoryPath("选择表情目录", root.expressionDirectory.length > 0 ? root.expressionDirectory : root.modelRoot),
                      function(path) { root.expressionDirectory = path })
    }

    function pickVoiceDirectory() {
        root.pickPath("语音目录",
                      petAssetSettings.pickLocalDirectoryPath("选择语音目录", root.voiceDirectory),
                      function(path) { root.voiceDirectory = path })
    }

    function pickDefaultVoice() {
        root.pickPath("默认音色",
                      petAssetSettings.pickLocalFilePath("选择默认音频",
                                                         root.defaultVoicePath().length > 0 ? root.defaultVoicePath() : root.voiceDirectory,
                                                         "音频文件 (*.wav *.mp3 *.flac *.ogg *.m4a);;所有文件 (*.*)"),
                      function(path) {
                          var directory = root.pathDirectory(path)
                          if (directory.length > 0) {
                              root.voiceDirectory = directory
                          }
                          root.defaultVoice = root.pathFileName(path)
                      })
    }

    function choosePlaceholder(kind) {
        if (kind === "模型文件") {
            root.pickModelJson()
        } else if (kind === "资源目录") {
            root.pickModelRootDirectory()
        } else {
            draftStatus = "准备选择" + kind
        }
    }

    function assetStatusLabel() {
        return Live2DCharacterRuntime.assetStatusLabel(assetValidator.status)
    }

    function assetStatusColor() {
        var tone = Live2DCharacterRuntime.assetStatusTone(assetValidator.status)
        if (tone === "green") {
            return root.accentGreen
        }
        if (tone === "blue") {
            return root.accentBlue
        }
        return root.accentRose
    }

    function firstAssetIssue() {
        if (assetValidator.errors.length > 0) {
            return assetValidator.errors[0]
        }
        if (assetValidator.warnings.length > 0) {
            return assetValidator.warnings[0]
        }
        return ""
    }

    function shortChecksum() {
        return assetValidator.packageChecksum.length > 0
                ? assetValidator.packageChecksum.slice(0, 12)
                : "未生成"
    }

    function optionalFileLabel(label, value) {
        return Live2DCharacterRuntime.optionalFileLabel(label, value)
    }

    function actionKindLabel(kind) {
        return Live2DCharacterRuntime.actionKindLabel(kind)
    }

    function runAssetValidation() {
        assetValidator.validate()
        return assetValidator.statusText
    }

    function applySettingsToDraft() {
        Live2DCharacterRuntime.applySettingsToDraft(root, petAssetSettings, toneCombo, responseLengthCombo, languageCombo)
        root.refreshCharacterAvatar()
    }

    function storeDraftToSettings() {
        Live2DCharacterRuntime.storeDraftToSettings(
                    petAssetSettings, root, toneCombo.currentIndex,
                    responseLengthCombo.currentIndex, languageCombo.currentIndex)
    }

    function saveDraft() {
        var status = runAssetValidation()
        root.storeDraftToSettings()
        if (petAssetSettings.save()) {
            draftStatus = assetValidator.valid ? "本地草稿已保存，资源可用"
                                               : "本地草稿已保存，" + status
        } else {
            draftStatus = petAssetSettings.statusText
        }
        root.refreshCharacterAvatar()
    }

    function resetDraft() {
        petAssetSettings.resetToDefaults()
        root.applySettingsToDraft()
        assetValidationTimer.restart()
        draftStatus = petAssetSettings.statusText
        root.refreshCharacterAvatar()
    }

    function requestPetPreview() {
        draftStatus = "正在启动桌宠预览"
        root.storeDraftToSettings()
        root.petPreviewRequested(petAssetSettings.toVariantMap())
    }

    function defaultVoicePath() {
        return Live2DCharacterRuntime.defaultVoicePath(voiceDirectory, defaultVoice)
    }

    function languageCode() {
        return Live2DCharacterRuntime.languageCode(languageCombo.currentText)
    }

    onModelRootChanged: root.refreshCharacterAvatar()
    onModelJsonChanged: root.refreshCharacterAvatar()

    function voiceTrainingStatusLabel() {
        return Live2DCharacterRuntime.voiceTrainingStatusLabel(voiceTrainingStatus, voiceTrainingStage)
    }

    function normalizeVoiceTrainingState() {
        var normalized = Live2DCharacterRuntime.normalizedVoiceTrainingState({
                                                                                "status": voiceTrainingStatus,
                                                                                "stage": voiceTrainingStage,
                                                                                "progress": voiceTrainingProgress,
                                                                                "message": voiceTrainingMessage
                                                                            })
        voiceTrainingStatus = normalized.status
        voiceTrainingStage = normalized.stage
        voiceTrainingProgress = normalized.progress
        voiceTrainingMessage = normalized.message
    }

    function syncVoiceTrainingFromController() {
        if (!petController) {
            return
        }
        if (petController.voiceTrainingJobId.length > 0) {
            voiceTrainingJobId = petController.voiceTrainingJobId
        }
        if (petController.voiceTrainingStatus.length > 0) {
            voiceTrainingStatus = petController.voiceTrainingStatus
        }
        if (petController.voiceTrainingStage.length > 0) {
            voiceTrainingStage = petController.voiceTrainingStage
        }
        voiceTrainingProgress = petController.voiceTrainingProgress
        if (petController.voiceTrainingArtifactPath.length > 0) {
            voiceTrainingArtifactPath = petController.voiceTrainingArtifactPath
        }
        if (petController.voiceTrainingMessage.length > 0) {
            voiceTrainingMessage = petController.voiceTrainingMessage
        }
        normalizeVoiceTrainingState()
        root.storeDraftToSettings()
        petAssetSettings.save()
    }

    function startVoiceTraining() {
        if (!voiceTrainingConsent) {
            voiceTrainingStatus = "blocked"
            voiceTrainingMessage = "请先确认你有权使用这段参考音频"
            draftStatus = voiceTrainingMessage
            return
        }
        var referencePath = defaultVoicePath()
        if (referencePath.length === 0) {
            voiceTrainingStatus = "blocked"
            voiceTrainingMessage = "请先设置语音目录和默认音色"
            draftStatus = voiceTrainingMessage
            return
        }
        root.storeDraftToSettings()
        petAssetSettings.save()
        if (!petController) {
            voiceTrainingStatus = "prepared"
            voiceTrainingMessage = "训练配置已保存，等待桌宠控制器连接"
            draftStatus = voiceTrainingMessage
            return
        }
        voiceTrainingStatus = "submitting"
        voiceTrainingStage = "submitting"
        voiceTrainingProgress = 0
        voiceTrainingMessage = "正在提交声音训练准备任务"
        petController.startVoiceTraining({
            "profile_id": "default",
            "voice_name": defaultVoice.length > 0 ? defaultVoice.replace(/\.[^.]+$/, "") : characterName,
            "language": languageCode(),
            "reference_audio_path": referencePath,
            "reference_audio_directory": voiceDirectory,
            "reference_audio_file": defaultVoice,
            "consent_confirmed": voiceTrainingConsent,
            "consent_scope": voiceTrainingConsentScope,
            "source": "src-default",
            "provider": "gpt-sovits",
            "metadata": {
                "character": characterName,
                "asset_status": assetValidator.status,
                "package_checksum": assetValidator.packageChecksum
            }
        })
        draftStatus = voiceTrainingMessage
    }

    function refreshVoiceTraining() {
        if (!petController || voiceTrainingJobId.length === 0) {
            draftStatus = "没有可刷新的声音训练任务"
            return
        }
        voiceTrainingMessage = "正在刷新声音训练任务"
        petController.refreshVoiceTrainingJob(voiceTrainingJobId)
        draftStatus = voiceTrainingMessage
    }

    function promptPreview() {
        return Live2DCharacterRuntime.promptPreview({
                                                        "characterName": characterName,
                                                        "roleIdentity": roleIdentity,
                                                        "relationshipStyle": relationshipStyle,
                                                        "personalityTags": personalityTags,
                                                        "worldSetting": worldSetting,
                                                        "idleMotion": idleMotion,
                                                        "speakingMotion": speakingMotion,
                                                        "fallbackExpression": fallbackExpression,
                                                        "speechRules": speechRules,
                                                        "catchphrases": catchphrases,
                                                        "forbiddenRules": forbiddenRules
                                                    })
    }

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        cornerRadius: 16
        blurRadius: 20
        fillColor: Qt.rgba(0.96, 0.98, 1.0, 0.88)
        strokeColor: Qt.rgba(1, 1, 1, 0.58)
        glowTopColor: Qt.rgba(1, 1, 1, 0.26)
        glowBottomColor: Qt.rgba(1, 1, 1, 0.05)
    }

    Connections {
        target: root.petController
        ignoreUnknownSignals: true

        function onVoiceTrainingChanged() {
            root.syncVoiceTrainingFromController()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 92

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                anchors.topMargin: 14
                anchors.bottomMargin: 12
                spacing: 14

                Rectangle {
                    Layout.preferredWidth: 58
                    Layout.preferredHeight: 58
                    radius: 8
                    color: Qt.rgba(0.80, 0.88, 0.98, 0.46)
                    border.color: Qt.rgba(1, 1, 1, 0.64)
                    clip: true

                    Live2DAvatarPreviewImage {
                        anchors.fill: parent
                        anchors.margins: 4
                        imageSource: root.characterAvatarSource
                        fallbackSource: root.characterAvatarFallback
                        fallbackInset: 8
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Label {
                        Layout.fillWidth: true
                        text: "Live2D 角色"
                        color: root.textPrimaryColor
                        font.pixelSize: 22
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Live2DStatusChip {
                            text: root.assetStatusLabel()
                            colorBase: root.assetStatusColor()
                        }
                        Live2DStatusChip { text: "语音" ; colorBase: root.accentGreen }
                        Live2DStatusChip { text: "人设" ; colorBase: root.accentRose }

                        Label {
                            Layout.fillWidth: true
                            text: root.draftStatus
                            color: root.textMutedColor
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }
                }

                GlassButton {
                    Layout.preferredWidth: 96
                    Layout.preferredHeight: 36
                    text: "启动桌宠"
                    textPixelSize: 13
                    textColor: "#285986"
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                    onClicked: root.requestPetPreview()
                }

                GlassButton {
                    Layout.preferredWidth: 78
                    Layout.preferredHeight: 36
                    text: "重置"
                    textPixelSize: 13
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.16)
                    hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.24)
                    pressedColor: Qt.rgba(0.54, 0.60, 0.68, 0.32)
                    onClicked: root.resetDraft()
                }

                GlassButton {
                    Layout.preferredWidth: 86
                    Layout.preferredHeight: 36
                    text: "保存"
                    textPixelSize: 13
                    textColor: "#285986"
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                    onClicked: root.saveDraft()
                }
            }
        }

        Flickable {
            id: scrollArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: width
            contentHeight: contentColumn.implicitHeight + 28
            ScrollBar.vertical: GlassScrollBar { }

            ColumnLayout {
                id: contentColumn
                x: 14
                y: 8
                width: Math.max(0, scrollArea.width - 28)
                spacing: 12

                Live2DCharacterAssetColumn {
                    backdrop: root.backdrop
                    petController: root.petController
                    accentBlue: root.accentBlue
                    accentGreen: root.accentGreen
                    accentRose: root.accentRose
                    characterName: root.characterName
                    roleIdentity: root.roleIdentity
                    modelRoot: root.modelRoot
                    modelJson: root.modelJson
                    characterAvatarSource: root.characterAvatarSource
                    characterAvatarFallback: root.characterAvatarFallback
                    motionDirectory: root.motionDirectory
                    expressionDirectory: root.expressionDirectory
                    voiceDirectory: root.voiceDirectory
                    defaultVoice: root.defaultVoice
                    idleMotion: root.idleMotion
                    speakingMotion: root.speakingMotion
                    fallbackExpression: root.fallbackExpression
                    voiceSpeed: root.voiceSpeed
                    lipSyncSensitivity: root.lipSyncSensitivity
                    voiceLipSyncEnabled: root.voiceLipSyncEnabled
                    emotionSoundEnabled: root.emotionSoundEnabled
                    voiceTrainingConsent: root.voiceTrainingConsent
                    voiceTrainingStatus: root.voiceTrainingStatus
                    voiceTrainingStage: root.voiceTrainingStage
                    voiceTrainingProgress: root.voiceTrainingProgress
                    voiceTrainingArtifactPath: root.voiceTrainingArtifactPath
                    voiceTrainingMessage: root.voiceTrainingMessage
                    voiceTrainingJobId: root.voiceTrainingJobId
                    voiceTrainingStatusLabel: root.voiceTrainingStatusLabel()
                    voiceTrainingSummary: root.voiceTrainingJobId.length > 0
                                          ? "任务 " + root.voiceTrainingJobId
                                            + " · " + root.voiceTrainingProgress + "%"
                                            + (root.voiceTrainingStage.length > 0 ? " · " + root.voiceTrainingStage : "")
                                          : "默认使用当前 src 音频；真实 GPT-SoVITS 训练稍后接入。"
                    assetStatusLabel: root.assetStatusLabel()
                    assetStatusColor: root.assetStatusColor()
                    assetStatusText: assetValidator.statusText
                    motionCount: assetValidator.motionCount
                    expressionCount: assetValidator.expressionCount
                    textureCount: assetValidator.textureCount
                    voiceCount: assetValidator.voiceCount
                    referencedFileCount: assetValidator.referencedFileCount
                    issueText: root.firstAssetIssue()
                    physicsLabel: root.optionalFileLabel("物理", assetValidator.physicsFile)
                    poseLabel: root.optionalFileLabel("姿势", assetValidator.poseFile)
                    userDataLabel: root.optionalFileLabel("标注", assetValidator.userDataFile)
                    mappingLabel: root.optionalFileLabel("映射", assetValidator.vtubeMappingFile)
                    checksumText: root.shortChecksum()
                    actionItems: assetValidator.actionItems
                    textPrimaryColor: root.textPrimaryColor
                    textSecondaryColor: root.textSecondaryColor
                    textMutedColor: root.textMutedColor
                    onCharacterNameEdited: function(text) { root.characterName = text }
                    onRoleIdentityEdited: function(text) { root.roleIdentity = text }
                    onModelRootEdited: function(text) { root.modelRoot = text }
                    onModelJsonEdited: function(text) { root.modelJson = text }
                    onModelJsonPickRequested: root.pickModelJson()
                    onModelRootPickRequested: root.pickModelRootDirectory()
                    onValidateRequested: root.runAssetValidation()
                    onMotionDirectoryEdited: function(text) { root.motionDirectory = text }
                    onMotionDirectoryPickRequested: root.pickMotionDirectory()
                    onExpressionDirectoryEdited: function(text) { root.expressionDirectory = text }
                    onExpressionDirectoryPickRequested: root.pickExpressionDirectory()
                    onVoiceDirectoryEdited: function(text) { root.voiceDirectory = text }
                    onVoiceDirectoryPickRequested: root.pickVoiceDirectory()
                    onDefaultVoiceEdited: function(text) { root.defaultVoice = text }
                    onDefaultVoicePickRequested: root.pickDefaultVoice()
                    onIdleMotionEdited: function(text) { root.idleMotion = text }
                    onSpeakingMotionEdited: function(text) { root.speakingMotion = text }
                    onFallbackExpressionEdited: function(text) { root.fallbackExpression = text }
                    onVoiceSpeedEdited: function(value) { root.voiceSpeed = value }
                    onLipSyncSensitivityEdited: function(value) { root.lipSyncSensitivity = value }
                    onVoiceLipSyncEnabledEdited: function(checked) { root.voiceLipSyncEnabled = checked }
                    onEmotionSoundEnabledEdited: function(checked) { root.emotionSoundEnabled = checked }
                    onVoiceTrainingConsentEdited: function(checked) { root.voiceTrainingConsent = checked }
                    onVoiceTrainingStartRequested: root.startVoiceTraining()
                    onVoiceTrainingRefreshRequested: root.refreshVoiceTraining()
                }

                Live2DCharacterBehaviorColumn {
                    backdrop: root.backdrop
                    toneCombo: toneCombo
                    responseLengthCombo: responseLengthCombo
                    languageCombo: languageCombo
                    accentBlue: root.accentBlue
                    accentGreen: root.accentGreen
                    accentRose: root.accentRose
                    relationshipStyle: root.relationshipStyle
                    personalityTags: root.personalityTags
                    worldSetting: root.worldSetting
                    forbiddenRules: root.forbiddenRules
                    emotionLevel: root.emotionLevel
                    creativityLevel: root.creativityLevel
                    speechRules: root.speechRules
                    catchphrases: root.catchphrases
                    autoStartPetOnClientStart: root.autoStartPetOnClientStart
                    idleMotionEnabled: root.idleMotionEnabled
                    gazeFollowEnabled: root.gazeFollowEnabled
                    memoryEnabled: root.memoryEnabled
                    interruptEnabled: root.interruptEnabled
                    cameraEnabled: root.cameraEnabled
                    cloudVisionEnabled: root.cloudVisionEnabled
                    onRelationshipStyleEdited: function(text) { root.relationshipStyle = text }
                    onPersonalityTagsEdited: function(text) { root.personalityTags = text }
                    onWorldSettingEdited: function(text) { root.worldSetting = text }
                    onForbiddenRulesEdited: function(text) { root.forbiddenRules = text }
                    onEmotionLevelEdited: function(value) { root.emotionLevel = value }
                    onCreativityLevelEdited: function(value) { root.creativityLevel = value }
                    onSpeechRulesEdited: function(text) { root.speechRules = text }
                    onCatchphrasesEdited: function(text) { root.catchphrases = text }
                    onAutoStartPetOnClientStartEdited: function(checked) { root.autoStartPetOnClientStart = checked }
                    onIdleMotionEnabledEdited: function(checked) { root.idleMotionEnabled = checked }
                    onGazeFollowEnabledEdited: function(checked) { root.gazeFollowEnabled = checked }
                    onMemoryEnabledEdited: function(checked) { root.memoryEnabled = checked }
                    onInterruptEnabledEdited: function(checked) { root.interruptEnabled = checked }
                    onCameraEnabledEdited: function(checked) { root.cameraEnabled = checked }
                    onCloudVisionEnabledEdited: function(checked) { root.cloudVisionEnabled = checked }
                }

                Live2DCharacterPromptColumn {
                    accentRose: root.accentRose
                    promptText: root.promptPreview()
                    textPrimaryColor: root.textPrimaryColor
                }
            }
        }
    }

    ComboBox {
        id: toneCombo
        visible: false
        model: ["温柔稳定", "元气活泼", "冷静简洁", "轻微吐槽"]
        currentIndex: 0
    }

    ComboBox {
        id: responseLengthCombo
        visible: false
        model: ["短句", "适中", "详细"]
        currentIndex: 1
    }

    ComboBox {
        id: languageCombo
        visible: false
        model: ["中文", "日语", "英语", "韩语", "法语", "西班牙语"]
        currentIndex: 0
    }
}
