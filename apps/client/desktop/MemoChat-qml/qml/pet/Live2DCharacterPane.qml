pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "../components"

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
        var slash = Math.max(path.lastIndexOf("/"), path.lastIndexOf("\\"))
        return slash >= 0 ? path.slice(0, slash) : ""
    }

    function pathFileName(path) {
        var slash = Math.max(path.lastIndexOf("/"), path.lastIndexOf("\\"))
        return slash >= 0 ? path.slice(slash + 1) : path
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
        if (assetValidator.status === "ready") {
            return "资源可用"
        }
        if (assetValidator.status === "missing") {
            return "资源缺失"
        }
        if (assetValidator.status === "invalid") {
            return "格式异常"
        }
        return "待补充资源"
    }

    function assetStatusColor() {
        if (assetValidator.status === "ready") {
            return root.accentGreen
        }
        if (assetValidator.status === "empty") {
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
        return label + " " + (value.length > 0 ? "已识别" : "未声明")
    }

    function actionKindLabel(kind) {
        if (kind === "motion") {
            return "动作"
        }
        if (kind === "expression") {
            return "表情"
        }
        return "控制"
    }

    function runAssetValidation() {
        assetValidator.validate()
        return assetValidator.statusText
    }

    function applySettingsToDraft() {
        characterName = petAssetSettings.characterName
        roleIdentity = petAssetSettings.roleIdentity
        modelRoot = petAssetSettings.modelRoot
        modelJson = petAssetSettings.modelJson
        motionDirectory = petAssetSettings.motionDirectory
        expressionDirectory = petAssetSettings.expressionDirectory
        voiceDirectory = petAssetSettings.voiceDirectory
        defaultVoice = petAssetSettings.defaultVoice
        idleMotion = petAssetSettings.idleMotion
        speakingMotion = petAssetSettings.speakingMotion
        fallbackExpression = petAssetSettings.fallbackExpression
        personalityTags = petAssetSettings.personalityTags
        relationshipStyle = petAssetSettings.relationshipStyle
        worldSetting = petAssetSettings.worldSetting
        speechRules = petAssetSettings.speechRules
        catchphrases = petAssetSettings.catchphrases
        forbiddenRules = petAssetSettings.forbiddenRules
        emotionLevel = petAssetSettings.emotionLevel
        creativityLevel = petAssetSettings.creativityLevel
        voiceSpeed = petAssetSettings.voiceSpeed
        lipSyncSensitivity = petAssetSettings.lipSyncSensitivity
        voiceLipSyncEnabled = petAssetSettings.voiceLipSyncEnabled
        emotionSoundEnabled = petAssetSettings.emotionSoundEnabled
        idleMotionEnabled = petAssetSettings.idleMotionEnabled
        gazeFollowEnabled = petAssetSettings.gazeFollowEnabled
        memoryEnabled = petAssetSettings.memoryEnabled
        interruptEnabled = petAssetSettings.interruptEnabled
        cameraEnabled = petAssetSettings.cameraEnabled
        cloudVisionEnabled = petAssetSettings.cloudVisionEnabled
        autoStartPetOnClientStart = petAssetSettings.autoStartPetOnClientStart
        voiceTrainingConsent = petAssetSettings.voiceTrainingConsent
        voiceTrainingConsentScope = petAssetSettings.voiceTrainingConsentScope
        voiceTrainingJobId = petAssetSettings.voiceTrainingJobId
        voiceTrainingStatus = petAssetSettings.voiceTrainingStatus
        voiceTrainingStage = petAssetSettings.voiceTrainingStage
        voiceTrainingProgress = petAssetSettings.voiceTrainingProgress
        voiceTrainingArtifactPath = petAssetSettings.voiceTrainingArtifactPath
        voiceTrainingMessage = petAssetSettings.voiceTrainingMessage
        toneCombo.currentIndex = petAssetSettings.toneIndex
        responseLengthCombo.currentIndex = petAssetSettings.responseLengthIndex
        languageCombo.currentIndex = petAssetSettings.languageIndex
        draftStatus = petAssetSettings.statusText
        root.refreshCharacterAvatar()
    }

    function storeDraftToSettings() {
        petAssetSettings.characterName = characterName
        petAssetSettings.roleIdentity = roleIdentity
        petAssetSettings.modelRoot = modelRoot
        petAssetSettings.modelJson = modelJson
        petAssetSettings.motionDirectory = motionDirectory
        petAssetSettings.expressionDirectory = expressionDirectory
        petAssetSettings.voiceDirectory = voiceDirectory
        petAssetSettings.defaultVoice = defaultVoice
        petAssetSettings.idleMotion = idleMotion
        petAssetSettings.speakingMotion = speakingMotion
        petAssetSettings.fallbackExpression = fallbackExpression
        petAssetSettings.personalityTags = personalityTags
        petAssetSettings.relationshipStyle = relationshipStyle
        petAssetSettings.worldSetting = worldSetting
        petAssetSettings.speechRules = speechRules
        petAssetSettings.catchphrases = catchphrases
        petAssetSettings.forbiddenRules = forbiddenRules
        petAssetSettings.emotionLevel = emotionLevel
        petAssetSettings.creativityLevel = creativityLevel
        petAssetSettings.voiceSpeed = voiceSpeed
        petAssetSettings.lipSyncSensitivity = lipSyncSensitivity
        petAssetSettings.voiceLipSyncEnabled = voiceLipSyncEnabled
        petAssetSettings.emotionSoundEnabled = emotionSoundEnabled
        petAssetSettings.idleMotionEnabled = idleMotionEnabled
        petAssetSettings.gazeFollowEnabled = gazeFollowEnabled
        petAssetSettings.memoryEnabled = memoryEnabled
        petAssetSettings.interruptEnabled = interruptEnabled
        petAssetSettings.cameraEnabled = cameraEnabled
        petAssetSettings.cloudVisionEnabled = cloudVisionEnabled
        petAssetSettings.autoStartPetOnClientStart = autoStartPetOnClientStart
        petAssetSettings.voiceTrainingConsent = voiceTrainingConsent
        petAssetSettings.voiceTrainingConsentScope = voiceTrainingConsentScope
        petAssetSettings.voiceTrainingJobId = voiceTrainingJobId
        petAssetSettings.voiceTrainingStatus = voiceTrainingStatus
        petAssetSettings.voiceTrainingStage = voiceTrainingStage
        petAssetSettings.voiceTrainingProgress = voiceTrainingProgress
        petAssetSettings.voiceTrainingArtifactPath = voiceTrainingArtifactPath
        petAssetSettings.voiceTrainingMessage = voiceTrainingMessage
        petAssetSettings.toneIndex = toneCombo.currentIndex
        petAssetSettings.responseLengthIndex = responseLengthCombo.currentIndex
        petAssetSettings.languageIndex = languageCombo.currentIndex
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
        if (voiceDirectory.length === 0 || defaultVoice.length === 0) {
            return ""
        }
        var separator = voiceDirectory.endsWith("/") || voiceDirectory.endsWith("\\") ? "" : "/"
        return voiceDirectory + separator + defaultVoice
    }

    function languageCode() {
        var current = languageCombo.currentText
        if (current === "日语") {
            return "ja-JP"
        }
        if (current === "英语") {
            return "en-US"
        }
        if (current === "韩语") {
            return "ko-KR"
        }
        if (current === "法语") {
            return "fr-FR"
        }
        if (current === "西班牙语") {
            return "es-ES"
        }
        return "zh-CN"
    }

    onModelRootChanged: root.refreshCharacterAvatar()
    onModelJsonChanged: root.refreshCharacterAvatar()

    function voiceTrainingStatusLabel() {
        if (voiceTrainingStatus === "submitting") {
            return "正在提交"
        }
        if (voiceTrainingStatus === "queued") {
            return "训练队列已创建"
        }
        if (voiceTrainingStatus === "ready"
                || voiceTrainingStage === "ready_for_gpt_sovits"
                || voiceTrainingStage === "needs_reference_clip") {
            return "GPT-SoVITS 可用"
        }
        if (voiceTrainingStatus === "prepared") {
            return "训练准备包已生成"
        }
        if (voiceTrainingStatus === "blocked") {
            return "等待授权"
        }
        return "未开始"
    }

    function normalizeVoiceTrainingState() {
        if (voiceTrainingStage === "ready_for_worker") {
            voiceTrainingStage = "ready_for_gpt_sovits"
            if (voiceTrainingStatus === "prepared"
                    || voiceTrainingStatus === "idle"
                    || voiceTrainingStatus === "blocked") {
                voiceTrainingStatus = "ready"
            }
            if (voiceTrainingProgress < 70) {
                voiceTrainingProgress = 70
            }
            if (voiceTrainingMessage.length === 0
                    || voiceTrainingMessage.toLowerCase().indexOf("worker") >= 0) {
                voiceTrainingMessage = "声音参考已就绪，可直接用于 GPT-SoVITS 零样本合成。"
            }
        }
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
        return "角色名：" + characterName
                + "\n定位：" + roleIdentity
                + "\n关系：" + relationshipStyle
                + "\n性格标签：" + personalityTags
                + "\n世界观：" + worldSetting
                + "\n待机动作：" + idleMotion
                + "\n说话动作：" + speakingMotion
                + "\n兜底表情：" + fallbackExpression
                + "\n说话规则：" + speechRules
                + "\n常用口头禅：" + catchphrases
                + "\n禁忌：" + forbiddenRules
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

                    AvatarPreviewImage {
                        anchors.fill: parent
                        anchors.margins: 4
                        imageSource: root.characterAvatarSource
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

                        StatusChip {
                            text: root.assetStatusLabel()
                            colorBase: root.assetStatusColor()
                        }
                        StatusChip { text: "语音" ; colorBase: root.accentGreen }
                        StatusChip { text: "人设" ; colorBase: root.accentRose }

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

                SectionPanel {
                    title: "角色预览"
                    subtitle: "选择当前角色基础信息和资源目录"
                    accentColor: root.accentBlue

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Rectangle {
                            Layout.preferredWidth: 170
                            Layout.preferredHeight: 220
                            Layout.minimumWidth: 140
                            radius: 8
                            color: Qt.rgba(0.85, 0.91, 0.98, 0.45)
                            border.color: Qt.rgba(1, 1, 1, 0.62)
                            clip: true

                            AvatarPreviewImage {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.top: parent.top
                                anchors.topMargin: 9
                                width: Math.min(parent.width - 18, parent.height - 72)
                                height: width
                                imageSource: root.characterAvatarSource
                                fallbackInset: 18
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 56
                                color: Qt.rgba(1, 1, 1, 0.50)

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 2

                                    Label {
                                        Layout.fillWidth: true
                                        text: root.characterName.length > 0 ? root.characterName : "未命名角色"
                                        color: root.textPrimaryColor
                                        font.pixelSize: 14
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: root.roleIdentity
                                        color: root.textMutedColor
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                FieldBlock {
                                    title: "角色名"
                                    text: root.characterName
                                    placeholderText: "例如 Kafuu Chino"
                                    onTextChanged: root.characterName = text
                                }

                                FieldBlock {
                                    title: "角色定位"
                                    text: root.roleIdentity
                                    placeholderText: "例如 桌宠助手"
                                    onTextChanged: root.roleIdentity = text
                                }
                            }

                            FieldBlock {
                                title: "模型根目录"
                                text: root.modelRoot
                                placeholderText: "src/KafuuChino/香风智乃live2D"
                                onTextChanged: root.modelRoot = text
                            }

                            FieldBlock {
                                title: "model3.json"
                                text: root.modelJson
                                placeholderText: "选择 Live2D model3.json"
                                onTextChanged: root.modelJson = text
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                GlassButton {
                                    Layout.preferredWidth: 92
                                    Layout.preferredHeight: 32
                                    text: "选模型"
                                    textPixelSize: 12
                                    cornerRadius: 8
                                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.18)
                                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.28)
                                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.36)
                                    onClicked: root.pickModelJson()
                                }

                                GlassButton {
                                    Layout.preferredWidth: 92
                                    Layout.preferredHeight: 32
                                    text: "选目录"
                                    textPixelSize: 12
                                    cornerRadius: 8
                                    normalColor: Qt.rgba(0.32, 0.60, 0.44, 0.18)
                                    hoverColor: Qt.rgba(0.32, 0.60, 0.44, 0.28)
                                    pressedColor: Qt.rgba(0.32, 0.60, 0.44, 0.36)
                                    onClicked: root.pickModelRootDirectory()
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: "选择模型文件、动作目录或整套角色资源。"
                                    color: root.textMutedColor
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }

                SectionPanel {
                    title: "资源与语音"
                    subtitle: "拆分模型、动作、表情和音色资源，方便后续替换运行时"
                    accentColor: root.accentGreen

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            StatusChip {
                                text: root.assetStatusLabel()
                                colorBase: root.assetStatusColor()
                            }

                            Label {
                                Layout.fillWidth: true
                                text: assetValidator.statusText
                                color: root.textSecondaryColor
                                font.pixelSize: 12
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }

                            GlassButton {
                                Layout.preferredWidth: 74
                                Layout.preferredHeight: 30
                                text: "校验"
                                textPixelSize: 12
                                cornerRadius: 8
                                normalColor: Qt.rgba(0.32, 0.60, 0.44, 0.18)
                                hoverColor: Qt.rgba(0.32, 0.60, 0.44, 0.28)
                                pressedColor: Qt.rgba(0.32, 0.60, 0.44, 0.36)
                                onClicked: root.runAssetValidation()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            StatusChip { text: "动作 " + assetValidator.motionCount }
                            StatusChip { text: "表情 " + assetValidator.expressionCount; colorBase: root.accentRose }
                            StatusChip { text: "贴图 " + assetValidator.textureCount; colorBase: root.accentGreen }
                            StatusChip { text: "语音 " + assetValidator.voiceCount }
                            StatusChip { text: "文件 " + assetValidator.referencedFileCount; colorBase: root.accentGreen }

                            Label {
                                Layout.fillWidth: true
                                text: root.firstAssetIssue()
                                visible: text.length > 0
                                color: root.textMutedColor
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            StatusChip { text: root.optionalFileLabel("物理", assetValidator.physicsFile) }
                            StatusChip { text: root.optionalFileLabel("姿势", assetValidator.poseFile); colorBase: root.accentRose }
                            StatusChip { text: root.optionalFileLabel("标注", assetValidator.userDataFile); colorBase: root.accentGreen }
                            StatusChip { text: root.optionalFileLabel("映射", assetValidator.vtubeMappingFile) }

                            Label {
                                Layout.fillWidth: true
                                text: "校验码 " + root.shortChecksum()
                                color: root.textMutedColor
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: actionOverviewLayout.implicitHeight + 18
                        radius: 8
                        color: Qt.rgba(1, 1, 1, 0.18)
                        border.color: Qt.rgba(1, 1, 1, 0.32)

                        ColumnLayout {
                            id: actionOverviewLayout
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 9
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    Layout.fillWidth: true
                                    text: "动作总览"
                                    color: root.textPrimaryColor
                                    font.pixelSize: 13
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                StatusChip {
                                    text: assetValidator.actionItems.length + " 个"
                                    colorBase: root.accentBlue
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                visible: assetValidator.actionItems.length === 0
                                text: "暂无可用动作"
                                color: root.textMutedColor
                                font.pixelSize: 11
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: 4
                                visible: assetValidator.actionItems.length > 0
                                rowSpacing: 6
                                columnSpacing: 6

                                Repeater {
                                    model: assetValidator.actionItems

                                    delegate: StatusChip {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        text: (modelData.name || modelData.trigger || "")
                                              + " · " + root.actionKindLabel(modelData.kind || "")
                                        colorBase: modelData.kind === "expression" ? root.accentRose
                                                                                   : root.accentBlue
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        FieldBlock {
                            title: "动作目录"
                            text: root.motionDirectory
                            placeholderText: "motions / motion3.json"
                            onTextChanged: root.motionDirectory = text
                        }

                        GlassButton {
                            Layout.preferredWidth: 72
                            Layout.preferredHeight: 36
                            text: "选择"
                            textPixelSize: 12
                            cornerRadius: 8
                            normalColor: Qt.rgba(0.32, 0.60, 0.44, 0.18)
                            hoverColor: Qt.rgba(0.32, 0.60, 0.44, 0.28)
                            pressedColor: Qt.rgba(0.32, 0.60, 0.44, 0.36)
                            onClicked: root.pickMotionDirectory()
                        }

                        FieldBlock {
                            title: "表情目录"
                            text: root.expressionDirectory
                            placeholderText: "expressions / exp3.json"
                            onTextChanged: root.expressionDirectory = text
                        }

                        GlassButton {
                            Layout.preferredWidth: 72
                            Layout.preferredHeight: 36
                            text: "选择"
                            textPixelSize: 12
                            cornerRadius: 8
                            normalColor: Qt.rgba(0.32, 0.60, 0.44, 0.18)
                            hoverColor: Qt.rgba(0.32, 0.60, 0.44, 0.28)
                            pressedColor: Qt.rgba(0.32, 0.60, 0.44, 0.36)
                            onClicked: root.pickExpressionDirectory()
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        FieldBlock {
                            title: "语音目录"
                            text: root.voiceDirectory
                            placeholderText: "src/KafuuChino/香风智乃voice"
                            onTextChanged: root.voiceDirectory = text
                        }

                        GlassButton {
                            Layout.preferredWidth: 72
                            Layout.preferredHeight: 36
                            text: "选择"
                            textPixelSize: 12
                            cornerRadius: 8
                            normalColor: Qt.rgba(0.32, 0.60, 0.44, 0.18)
                            hoverColor: Qt.rgba(0.32, 0.60, 0.44, 0.28)
                            pressedColor: Qt.rgba(0.32, 0.60, 0.44, 0.36)
                            onClicked: root.pickVoiceDirectory()
                        }

                        FieldBlock {
                            title: "默认音色"
                            text: root.defaultVoice
                            placeholderText: "Kafuuchino-voice.mp3"
                            onTextChanged: root.defaultVoice = text
                        }

                        GlassButton {
                            Layout.preferredWidth: 72
                            Layout.preferredHeight: 36
                            text: "选择"
                            textPixelSize: 12
                            cornerRadius: 8
                            normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.18)
                            hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.28)
                            pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.36)
                            onClicked: root.pickDefaultVoice()
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        FieldBlock {
                            title: "待机动作"
                            text: root.idleMotion
                            placeholderText: "Idle"
                            onTextChanged: root.idleMotion = text
                        }

                        FieldBlock {
                            title: "说话动作"
                            text: root.speakingMotion
                            placeholderText: "TapBody / Talk"
                            onTextChanged: root.speakingMotion = text
                        }

                        FieldBlock {
                            title: "兜底表情"
                            text: root.fallbackExpression
                            placeholderText: "neutral / smile"
                            onTextChanged: root.fallbackExpression = text
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        SliderBlock {
                            title: "语速"
                            value: root.voiceSpeed
                            fromValue: 0.70
                            toValue: 1.35
                            valueText: root.voiceSpeed.toFixed(2) + "x"
                            onValueChanged: root.voiceSpeed = value
                        }

                        SliderBlock {
                            title: "口型灵敏度"
                            value: root.lipSyncSensitivity
                            fromValue: 0.0
                            toValue: 1.0
                            valueText: Math.round(root.lipSyncSensitivity * 100) + "%"
                            onValueChanged: root.lipSyncSensitivity = value
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ToggleRow {
                            Layout.fillWidth: true
                            title: "语音驱动口型"
                            subtitle: "播放语音时同步 lip sync 值"
                            checked: root.voiceLipSyncEnabled
                            onCheckedChanged: root.voiceLipSyncEnabled = checked
                        }

                        ToggleRow {
                            Layout.fillWidth: true
                            title: "情绪音效"
                            subtitle: "按情绪选择问候、惊讶、确认等短音频"
                            checked: root.emotionSoundEnabled
                            onCheckedChanged: root.emotionSoundEnabled = checked
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ToggleRow {
                            Layout.fillWidth: true
                            title: "允许使用参考音频"
                            subtitle: "确认你有权用当前默认音频准备本地声音训练"
                            checked: root.voiceTrainingConsent
                            onCheckedChanged: root.voiceTrainingConsent = checked
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: Math.max(60, voiceTrainingStatusLayout.implicitHeight + 14)
                            radius: 8
                            color: Qt.rgba(1, 1, 1, 0.22)
                            border.color: Qt.rgba(1, 1, 1, 0.34)

                            ColumnLayout {
                                id: voiceTrainingStatusLayout
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 3

                                Label {
                                    Layout.fillWidth: true
                                    text: root.voiceTrainingStatusLabel()
                                    color: root.textPrimaryColor
                                    font.pixelSize: 13
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: root.voiceTrainingMessage
                                    color: root.textMutedColor
                                    font.pixelSize: 11
                                    wrapMode: Text.Wrap
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        GlassButton {
                            Layout.preferredWidth: 118
                            Layout.preferredHeight: 32
                            enabled: root.voiceTrainingConsent
                                     && (!root.petController || !root.petController.voiceTrainingBusy)
                            text: "开始声音训练"
                            textPixelSize: 12
                            textColor: "#285986"
                            cornerRadius: 8
                            normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.22)
                            hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.32)
                            pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.40)
                            onClicked: root.startVoiceTraining()
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.voiceTrainingJobId.length > 0
                                  ? "任务 " + root.voiceTrainingJobId
                                    + " · " + root.voiceTrainingProgress + "%"
                                    + (root.voiceTrainingStage.length > 0 ? " · " + root.voiceTrainingStage : "")
                                  : "默认使用当前 src 音频；真实 GPT-SoVITS 训练稍后接入。"
                            color: root.textMutedColor
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        visible: root.voiceTrainingArtifactPath.length > 0
                        implicitHeight: Math.max(42, voiceTrainingArtifactLabel.implicitHeight + 18)
                        radius: 8
                        color: Qt.rgba(1, 1, 1, 0.18)
                        border.color: Qt.rgba(1, 1, 1, 0.30)

                        Label {
                            id: voiceTrainingArtifactLabel
                            anchors.fill: parent
                            anchors.margins: 9
                            text: "准备包 " + root.voiceTrainingArtifactPath
                            color: root.textMutedColor
                            font.pixelSize: 11
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        ProgressBar {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 6
                            from: 0
                            to: 100
                            value: root.voiceTrainingProgress
                            visible: root.voiceTrainingStatus !== "idle"
                        }

                        GlassButton {
                            Layout.preferredWidth: 84
                            Layout.preferredHeight: 30
                            enabled: root.voiceTrainingJobId.length > 0
                                     && root.petController
                                     && !root.petController.voiceTrainingBusy
                            text: "刷新"
                            textPixelSize: 12
                            cornerRadius: 8
                            normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.16)
                            hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.24)
                            pressedColor: Qt.rgba(0.54, 0.60, 0.68, 0.32)
                            onClicked: root.refreshVoiceTraining()
                        }
                    }
                }

                SectionPanel {
                    title: "人设"
                    subtitle: "把人物设定和边界整理成后续可持久化的角色档案"
                    accentColor: root.accentRose

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        FieldBlock {
                            title: "关系定位"
                            text: root.relationshipStyle
                            placeholderText: "例如 同伴 / 助手 / 管家"
                            onTextChanged: root.relationshipStyle = text
                        }

                        FieldBlock {
                            title: "性格标签"
                            text: root.personalityTags
                            placeholderText: "用逗号分隔"
                            onTextChanged: root.personalityTags = text
                        }
                    }

                    TextAreaBlock {
                        title: "背景与世界观"
                        text: root.worldSetting
                        preferredHeight: 86
                        placeholderText: "角色来自哪里、和用户是什么关系、关注什么事情..."
                        onTextChanged: root.worldSetting = text
                    }

                    TextAreaBlock {
                        title: "禁忌与边界"
                        text: root.forbiddenRules
                        preferredHeight: 74
                        placeholderText: "不该说什么、不该做什么、隐私和安全边界..."
                        onTextChanged: root.forbiddenRules = text
                    }
                }

                SectionPanel {
                    title: "说话风格"
                    subtitle: "控制语气、语言、长度和情绪表现"
                    accentColor: root.accentBlue

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ComboBlock {
                            id: toneComboBlock
                            title: "语气"
                            comboId: toneCombo
                        }

                        ComboBlock {
                            id: responseLengthComboBlock
                            title: "回复长度"
                            comboId: responseLengthCombo
                        }

                        ComboBlock {
                            id: languageComboBlock
                            title: "语言"
                            comboId: languageCombo
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        SliderBlock {
                            title: "情绪表现"
                            value: root.emotionLevel
                            fromValue: 0.0
                            toValue: 1.0
                            valueText: Math.round(root.emotionLevel * 100) + "%"
                            onValueChanged: root.emotionLevel = value
                        }

                        SliderBlock {
                            title: "创造性"
                            value: root.creativityLevel
                            fromValue: 0.0
                            toValue: 1.0
                            valueText: Math.round(root.creativityLevel * 100) + "%"
                            onValueChanged: root.creativityLevel = value
                        }
                    }

                    TextAreaBlock {
                        title: "说话规则"
                        text: root.speechRules
                        preferredHeight: 86
                        placeholderText: "例如 先安抚情绪、不要长篇解释、少用反问..."
                        onTextChanged: root.speechRules = text
                    }

                    TextAreaBlock {
                        title: "常用口头禅"
                        text: root.catchphrases
                        preferredHeight: 74
                        placeholderText: "每行一句，可以按情绪映射到动作或语音。"
                        onTextChanged: root.catchphrases = text
                    }
                }

                SectionPanel {
                    title: "行为与记忆"
                    subtitle: "定义待机动作、上下文记忆和隐私权限"
                    accentColor: root.accentGreen

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ToggleRow {
                            Layout.fillWidth: true
                            title: "打开客户端自启"
                            subtitle: "默认关闭；保存后下次打开客户端自动启动桌宠"
                            checked: root.autoStartPetOnClientStart
                            onCheckedChanged: root.autoStartPetOnClientStart = checked
                        }

                        ToggleRow {
                            Layout.fillWidth: true
                            title: "待机动作"
                            subtitle: "无对话时循环 idle motion"
                            checked: root.idleMotionEnabled
                            onCheckedChanged: root.idleMotionEnabled = checked
                        }

                        ToggleRow {
                            Layout.fillWidth: true
                            title: "视线跟随"
                            subtitle: "根据鼠标位置更新 gaze"
                            checked: root.gazeFollowEnabled
                            onCheckedChanged: root.gazeFollowEnabled = checked
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ToggleRow {
                            Layout.fillWidth: true
                            title: "角色记忆"
                            subtitle: "允许保存偏好、称呼和长期约束"
                            checked: root.memoryEnabled
                            onCheckedChanged: root.memoryEnabled = checked
                        }

                        ToggleRow {
                            Layout.fillWidth: true
                            title: "打断响应"
                            subtitle: "用户输入时停止当前语音并切换动作"
                            checked: root.interruptEnabled
                            onCheckedChanged: root.interruptEnabled = checked
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ToggleRow {
                            Layout.fillWidth: true
                            title: "摄像头权限"
                            subtitle: "默认关闭，后续需用户主动开启"
                            checked: root.cameraEnabled
                            onCheckedChanged: root.cameraEnabled = checked
                        }

                        ToggleRow {
                            Layout.fillWidth: true
                            title: "云端多模态"
                            subtitle: "默认仅发送文本，图像另行确认"
                            checked: root.cloudVisionEnabled
                            onCheckedChanged: root.cloudVisionEnabled = checked
                        }
                    }
                }

                SectionPanel {
                    title: "角色提示词预览"
                    subtitle: "把当前界面内容整理成之后可发给 agent 的角色配置"
                    accentColor: root.accentRose

                    TextArea {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 186
                        text: root.promptPreview()
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextArea.Wrap
                        color: root.textPrimaryColor
                        font.pixelSize: 13
                        background: Rectangle {
                            radius: 8
                            color: Qt.rgba(1, 1, 1, 0.38)
                            border.width: 1
                            border.color: Qt.rgba(1, 1, 1, 0.46)
                        }
                    }
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

    component AvatarPreviewImage: Item {
        id: avatarPreview
        property string imageSource: ""
        property int fallbackInset: 8

        Image {
            id: avatarImage
            anchors.fill: parent
            source: avatarPreview.imageSource
            fillMode: Image.PreserveAspectCrop
            sourceSize.width: Math.max(1, width * 2)
            sourceSize.height: Math.max(1, height * 2)
            cache: false
            mipmap: true
            visible: status === Image.Ready
        }

        Image {
            anchors.fill: parent
            anchors.margins: avatarPreview.fallbackInset
            source: root.characterAvatarFallback
            fillMode: Image.PreserveAspectFit
            sourceSize.width: Math.max(1, width * 2)
            sourceSize.height: Math.max(1, height * 2)
            mipmap: true
            visible: avatarImage.status !== Image.Ready
        }
    }

    component StatusChip: Rectangle {
        property string text: ""
        property color colorBase: root.accentBlue
        implicitWidth: chipText.implicitWidth + 18
        implicitHeight: 24
        radius: 8
        color: Qt.rgba(colorBase.r, colorBase.g, colorBase.b, 0.15)
        border.color: Qt.rgba(colorBase.r, colorBase.g, colorBase.b, 0.26)

        Label {
            id: chipText
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            text: parent.text
            color: root.textSecondaryColor
            font.pixelSize: 11
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    component SectionPanel: Rectangle {
        id: panel
        property string title: ""
        property string subtitle: ""
        property color accentColor: root.accentBlue
        default property alias content: body.data

        Layout.fillWidth: true
        implicitHeight: panelLayout.implicitHeight + 24
        radius: 8
        color: Qt.rgba(1, 1, 1, 0.24)
        border.color: Qt.rgba(1, 1, 1, 0.42)

        ColumnLayout {
            id: panelLayout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 12
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    Layout.preferredWidth: 4
                    Layout.preferredHeight: 28
                    radius: 2
                    color: panel.accentColor
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: panel.title
                        color: root.textPrimaryColor
                        font.pixelSize: 15
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: panel.subtitle
                        visible: text.length > 0
                        color: root.textMutedColor
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                }
            }

            ColumnLayout {
                id: body
                Layout.fillWidth: true
                spacing: 10
            }
        }
    }

    component FieldBlock: ColumnLayout {
        id: field
        property string title: ""
        property alias text: input.text
        property string placeholderText: ""

        Layout.fillWidth: true
        spacing: 5

        Label {
            Layout.fillWidth: true
            text: field.title
            color: root.textSecondaryColor
            font.pixelSize: 12
            font.bold: true
            elide: Text.ElideRight
        }

        GlassTextField {
            id: input
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            backdrop: root.backdrop
            textHorizontalAlignment: TextInput.AlignLeft
            textPixelSize: 13
            leftInset: 10
            rightInset: 10
            cornerRadius: 8
            placeholderText: field.placeholderText
            fillColor: Qt.rgba(1, 1, 1, 0.28)
            strokeColor: Qt.rgba(1, 1, 1, 0.46)
        }
    }

    component TextAreaBlock: ColumnLayout {
        id: areaBlock
        property string title: ""
        property alias text: area.text
        property string placeholderText: ""
        property int preferredHeight: 80

        Layout.fillWidth: true
        spacing: 5

        Label {
            Layout.fillWidth: true
            text: areaBlock.title
            color: root.textSecondaryColor
            font.pixelSize: 12
            font.bold: true
            elide: Text.ElideRight
        }

        TextArea {
            id: area
            Layout.fillWidth: true
            Layout.preferredHeight: areaBlock.preferredHeight
            placeholderText: areaBlock.placeholderText
            placeholderTextColor: root.textMutedColor
            color: root.textPrimaryColor
            font.pixelSize: 13
            wrapMode: TextArea.Wrap
            selectByMouse: true
            background: Rectangle {
                radius: 8
                color: Qt.rgba(1, 1, 1, 0.34)
                border.width: 1
                border.color: area.activeFocus ? Qt.rgba(0.35, 0.61, 0.90, 0.46)
                                               : Qt.rgba(1, 1, 1, 0.42)
            }
        }
    }

    component ComboBlock: ColumnLayout {
        id: comboBlock
        property string title: ""
        property ComboBox comboId: null

        Layout.fillWidth: true
        spacing: 5

        Label {
            Layout.fillWidth: true
            text: comboBlock.title
            color: root.textSecondaryColor
            font.pixelSize: 12
            font.bold: true
            elide: Text.ElideRight
        }

        ComboBox {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            model: comboBlock.comboId ? comboBlock.comboId.model : []
            currentIndex: comboBlock.comboId ? comboBlock.comboId.currentIndex : 0
            font.pixelSize: 13
            onActivated: function(index) {
                if (comboBlock.comboId) {
                    comboBlock.comboId.currentIndex = index
                }
            }
            background: Rectangle {
                radius: 8
                color: Qt.rgba(1, 1, 1, 0.34)
                border.width: 1
                border.color: Qt.rgba(1, 1, 1, 0.46)
            }
        }
    }

    component SliderBlock: ColumnLayout {
        id: sliderBlock
        property string title: ""
        property real value: 0.5
        property real fromValue: 0
        property real toValue: 1
        property string valueText: ""

        Layout.fillWidth: true
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: sliderBlock.title
                color: root.textSecondaryColor
                font.pixelSize: 12
                font.bold: true
                elide: Text.ElideRight
            }

            Label {
                text: sliderBlock.valueText
                color: root.textMutedColor
                font.pixelSize: 11
            }
        }

        Slider {
            Layout.fillWidth: true
            from: sliderBlock.fromValue
            to: sliderBlock.toValue
            value: sliderBlock.value
            onMoved: sliderBlock.value = value
        }
    }

    component ToggleRow: Rectangle {
        id: toggleRow
        property string title: ""
        property string subtitle: ""
        property alias checked: optionSwitch.checked

        implicitHeight: Math.max(60, row.implicitHeight + 14)
        radius: 8
        color: Qt.rgba(1, 1, 1, 0.22)
        border.color: Qt.rgba(1, 1, 1, 0.34)

        RowLayout {
            id: row
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: toggleRow.title
                    color: root.textPrimaryColor
                    font.pixelSize: 13
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: toggleRow.subtitle
                    color: root.textMutedColor
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
            }

            Switch {
                id: optionSwitch
                Layout.preferredWidth: 56
            }
        }
    }
}
