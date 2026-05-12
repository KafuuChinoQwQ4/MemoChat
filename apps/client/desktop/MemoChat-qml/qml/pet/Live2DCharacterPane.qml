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
    property string characterName: "Memo Pet"
    property string roleIdentity: "聊天陪伴角色"
    property string modelRoot: ""
    property string modelJson: ""
    property string motionDirectory: ""
    property string expressionDirectory: ""
    property string voiceDirectory: ""
    property string defaultVoice: "normal"
    property string idleMotion: "Idle"
    property string speakingMotion: "TapBody"
    property string fallbackExpression: "neutral"
    property string personalityTags: "认真, 轻声, 可靠, 适度吐槽"
    property string relationshipStyle: "熟悉但不过界的同伴"
    property string worldSetting: "住在 MemoChat 旁边的小小工作台，会在用户聊天、学习和整理资料时陪伴。"
    property string speechRules: "用自然中文回复。少说套话，先回应情绪，再给明确建议。"
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

    readonly property color textPrimaryColor: "#253247"
    readonly property color textSecondaryColor: "#4e5d74"
    readonly property color textMutedColor: "#6e7f95"
    readonly property color accentBlue: Qt.rgba(0.32, 0.56, 0.86, 1.0)
    readonly property color accentGreen: Qt.rgba(0.32, 0.60, 0.44, 1.0)
    readonly property color accentRose: Qt.rgba(0.78, 0.36, 0.45, 1.0)

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
        assetValidationTimer.start()
    }

    function choosePlaceholder(kind) {
        draftStatus = "准备选择" + kind
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
        toneCombo.currentIndex = petAssetSettings.toneIndex
        responseLengthCombo.currentIndex = petAssetSettings.responseLengthIndex
        languageCombo.currentIndex = petAssetSettings.languageIndex
        draftStatus = petAssetSettings.statusText
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
    }

    function resetDraft() {
        petAssetSettings.resetToDefaults()
        root.applySettingsToDraft()
        assetValidationTimer.restart()
        draftStatus = petAssetSettings.statusText
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

                    Image {
                        anchors.centerIn: parent
                        width: 44
                        height: 44
                        source: "qrc:/icons/modelive2d.png"
                        fillMode: Image.PreserveAspectFit
                        mipmap: true
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

                            Image {
                                anchors.centerIn: parent
                                width: Math.min(parent.width - 30, 118)
                                height: width
                                source: "qrc:/icons/modelive2d.png"
                                fillMode: Image.PreserveAspectFit
                                mipmap: true
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
                                placeholderText: "/data/memochat/pet-assets/example-model"
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
                                    onClicked: root.choosePlaceholder("模型文件")
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
                                    onClicked: root.choosePlaceholder("资源目录")
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

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        FieldBlock {
                            title: "动作目录"
                            text: root.motionDirectory
                            placeholderText: "motions / motion3.json"
                            onTextChanged: root.motionDirectory = text
                        }

                        FieldBlock {
                            title: "表情目录"
                            text: root.expressionDirectory
                            placeholderText: "expressions / exp3.json"
                            onTextChanged: root.expressionDirectory = text
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        FieldBlock {
                            title: "语音目录"
                            text: root.voiceDirectory
                            placeholderText: "voice / wav / json"
                            onTextChanged: root.voiceDirectory = text
                        }

                        FieldBlock {
                            title: "默认音色"
                            text: root.defaultVoice
                            placeholderText: "normal / sweet / calm"
                            onTextChanged: root.defaultVoice = text
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
        model: ["中文优先", "中日混合", "中英双语"]
        currentIndex: 0
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
            anchors.centerIn: parent
            text: parent.text
            color: root.textSecondaryColor
            font.pixelSize: 11
            font.bold: true
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
