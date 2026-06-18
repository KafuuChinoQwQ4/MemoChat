import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Live2DSectionPanel {
    id: root

    property Item backdrop: null
    property var petController: null
    property string motionDirectory: ""
    property string expressionDirectory: ""
    property string voiceDirectory: ""
    property string defaultVoice: ""
    property string idleMotion: ""
    property string speakingMotion: ""
    property string fallbackExpression: ""
    property real voiceSpeed: 1.0
    property real lipSyncSensitivity: 0.55
    property bool voiceLipSyncEnabled: true
    property bool emotionSoundEnabled: true
    property bool voiceTrainingConsent: false
    property string voiceTrainingStatus: "idle"
    property string voiceTrainingStage: ""
    property int voiceTrainingProgress: 0
    property string voiceTrainingArtifactPath: ""
    property string voiceTrainingMessage: ""
    property string voiceTrainingJobId: ""
    property string voiceTrainingStatusLabel: ""
    property string voiceTrainingSummary: ""
    property string assetStatusLabel: ""
    property color assetStatusColor: Qt.rgba(0.32, 0.56, 0.86, 1.0)
    property string assetStatusText: ""
    property int motionCount: 0
    property int expressionCount: 0
    property int textureCount: 0
    property int voiceCount: 0
    property int referencedFileCount: 0
    property string issueText: ""
    property string physicsLabel: ""
    property string poseLabel: ""
    property string userDataLabel: ""
    property string mappingLabel: ""
    property string checksumText: ""
    property var actionItems: []
    property color textPrimaryColor: "#253247"
    property color textSecondaryColor: "#4e5d74"
    property color textMutedColor: "#6e7f95"
    property color accentBlue: Qt.rgba(0.32, 0.56, 0.86, 1.0)
    property color accentGreen: Qt.rgba(0.32, 0.60, 0.44, 1.0)
    property color accentRose: Qt.rgba(0.78, 0.36, 0.45, 1.0)

    signal validateRequested()
    signal motionDirectoryEdited(string text)
    signal motionDirectoryPickRequested()
    signal expressionDirectoryEdited(string text)
    signal expressionDirectoryPickRequested()
    signal voiceDirectoryEdited(string text)
    signal voiceDirectoryPickRequested()
    signal defaultVoiceEdited(string text)
    signal defaultVoicePickRequested()
    signal idleMotionEdited(string text)
    signal speakingMotionEdited(string text)
    signal fallbackExpressionEdited(string text)
    signal voiceSpeedEdited(real value)
    signal lipSyncSensitivityEdited(real value)
    signal voiceLipSyncEnabledEdited(bool checked)
    signal emotionSoundEnabledEdited(bool checked)
    signal voiceTrainingConsentEdited(bool checked)
    signal voiceTrainingStartRequested()
    signal voiceTrainingRefreshRequested()

    title: "资源与语音"
    subtitle: "拆分模型、动作、表情和音色资源，方便后续替换运行时"

    Live2DAssetSummaryPanel {
        Layout.fillWidth: true
        statusLabel: root.assetStatusLabel
        statusColor: root.assetStatusColor
        statusText: root.assetStatusText
        motionCount: root.motionCount
        expressionCount: root.expressionCount
        textureCount: root.textureCount
        voiceCount: root.voiceCount
        referencedFileCount: root.referencedFileCount
        issueText: root.issueText
        physicsLabel: root.physicsLabel
        poseLabel: root.poseLabel
        userDataLabel: root.userDataLabel
        mappingLabel: root.mappingLabel
        checksumText: root.checksumText
        actionItems: root.actionItems
        textPrimaryColor: root.textPrimaryColor
        textSecondaryColor: root.textSecondaryColor
        textMutedColor: root.textMutedColor
        accentBlue: root.accentBlue
        accentGreen: root.accentGreen
        accentRose: root.accentRose
        onValidateRequested: root.validateRequested()
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Live2DFormFieldBlock {
            backdrop: root.backdrop
            title: "动作目录"
            text: root.motionDirectory
            placeholderText: "motions / motion3.json"
            onTextChanged: root.motionDirectoryEdited(text)
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
            onClicked: root.motionDirectoryPickRequested()
        }

        Live2DFormFieldBlock {
            backdrop: root.backdrop
            title: "表情目录"
            text: root.expressionDirectory
            placeholderText: "expressions / exp3.json"
            onTextChanged: root.expressionDirectoryEdited(text)
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
            onClicked: root.expressionDirectoryPickRequested()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Live2DFormFieldBlock {
            backdrop: root.backdrop
            title: "语音目录"
            text: root.voiceDirectory
            placeholderText: "选择用户导入的语音目录"
            onTextChanged: root.voiceDirectoryEdited(text)
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
            onClicked: root.voiceDirectoryPickRequested()
        }

        Live2DFormFieldBlock {
            backdrop: root.backdrop
            title: "默认音色"
            text: root.defaultVoice
            placeholderText: "选择默认语音文件"
            onTextChanged: root.defaultVoiceEdited(text)
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
            onClicked: root.defaultVoicePickRequested()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Live2DFormFieldBlock {
            backdrop: root.backdrop
            title: "待机动作"
            text: root.idleMotion
            placeholderText: "Idle"
            onTextChanged: root.idleMotionEdited(text)
        }

        Live2DFormFieldBlock {
            backdrop: root.backdrop
            title: "说话动作"
            text: root.speakingMotion
            placeholderText: "TapBody / Talk"
            onTextChanged: root.speakingMotionEdited(text)
        }

        Live2DFormFieldBlock {
            backdrop: root.backdrop
            title: "兜底表情"
            text: root.fallbackExpression
            placeholderText: "neutral / smile"
            onTextChanged: root.fallbackExpressionEdited(text)
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Live2DSliderBlock {
            title: "语速"
            value: root.voiceSpeed
            fromValue: 0.70
            toValue: 1.35
            valueText: root.voiceSpeed.toFixed(2) + "x"
            onValueChanged: root.voiceSpeedEdited(value)
        }

        Live2DSliderBlock {
            title: "口型灵敏度"
            value: root.lipSyncSensitivity
            fromValue: 0.0
            toValue: 1.0
            valueText: Math.round(root.lipSyncSensitivity * 100) + "%"
            onValueChanged: root.lipSyncSensitivityEdited(value)
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Live2DToggleRow {
            Layout.fillWidth: true
            title: "语音驱动口型"
            subtitle: "播放语音时同步 lip sync 值"
            checked: root.voiceLipSyncEnabled
            onCheckedChanged: root.voiceLipSyncEnabledEdited(checked)
        }

        Live2DToggleRow {
            Layout.fillWidth: true
            title: "情绪音效"
            subtitle: "按情绪选择问候、惊讶、确认等短音频"
            checked: root.emotionSoundEnabled
            onCheckedChanged: root.emotionSoundEnabledEdited(checked)
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Live2DToggleRow {
            Layout.fillWidth: true
            title: "允许使用参考音频"
            subtitle: "确认你有权用当前默认音频准备本地声音训练"
            checked: root.voiceTrainingConsent
            onCheckedChanged: root.voiceTrainingConsentEdited(checked)
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
                    text: root.voiceTrainingStatusLabel
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
            onClicked: root.voiceTrainingStartRequested()
        }

        Label {
            Layout.fillWidth: true
            text: root.voiceTrainingSummary
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
            onClicked: root.voiceTrainingRefreshRequested()
        }
    }
}
