pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: root
    spacing: 12

    property var backdrop: null
    property var petController: null
    property var actionItems: []
    property color accentBlue: Qt.rgba(0.32, 0.56, 0.86, 1.0)
    property color accentGreen: Qt.rgba(0.32, 0.60, 0.44, 1.0)
    property color accentRose: Qt.rgba(0.78, 0.36, 0.45, 1.0)
    property color textPrimaryColor: "#253247"
    property color textSecondaryColor: "#4e5d74"
    property color textMutedColor: "#6e7f95"
    property string characterName: ""
    property string roleIdentity: ""
    property string modelRoot: ""
    property string modelJson: ""
    property string characterAvatarSource: ""
    property string characterAvatarFallback: ""
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
    property color assetStatusColor: accentBlue
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

    signal characterNameEdited(string text)
    signal roleIdentityEdited(string text)
    signal modelRootEdited(string text)
    signal modelJsonEdited(string text)
    signal modelJsonPickRequested()
    signal modelRootPickRequested()
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

    Live2DCharacterPreviewPanel {
        backdrop: root.backdrop
        accentColor: root.accentBlue
        characterName: root.characterName
        roleIdentity: root.roleIdentity
        modelRoot: root.modelRoot
        modelJson: root.modelJson
        characterAvatarSource: root.characterAvatarSource
        characterAvatarFallback: root.characterAvatarFallback
        textPrimaryColor: root.textPrimaryColor
        textMutedColor: root.textMutedColor
        onCharacterNameEdited: function(text) { root.characterNameEdited(text) }
        onRoleIdentityEdited: function(text) { root.roleIdentityEdited(text) }
        onModelRootEdited: function(text) { root.modelRootEdited(text) }
        onModelJsonEdited: function(text) { root.modelJsonEdited(text) }
        onModelJsonPickRequested: root.modelJsonPickRequested()
        onModelRootPickRequested: root.modelRootPickRequested()
    }

    Live2DResourceVoicePanel {
        backdrop: root.backdrop
        petController: root.petController
        accentColor: root.accentGreen
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
        voiceTrainingStatusLabel: root.voiceTrainingStatusLabel
        voiceTrainingSummary: root.voiceTrainingSummary
        assetStatusLabel: root.assetStatusLabel
        assetStatusColor: root.assetStatusColor
        assetStatusText: root.assetStatusText
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
        onMotionDirectoryEdited: function(text) { root.motionDirectoryEdited(text) }
        onMotionDirectoryPickRequested: root.motionDirectoryPickRequested()
        onExpressionDirectoryEdited: function(text) { root.expressionDirectoryEdited(text) }
        onExpressionDirectoryPickRequested: root.expressionDirectoryPickRequested()
        onVoiceDirectoryEdited: function(text) { root.voiceDirectoryEdited(text) }
        onVoiceDirectoryPickRequested: root.voiceDirectoryPickRequested()
        onDefaultVoiceEdited: function(text) { root.defaultVoiceEdited(text) }
        onDefaultVoicePickRequested: root.defaultVoicePickRequested()
        onIdleMotionEdited: function(text) { root.idleMotionEdited(text) }
        onSpeakingMotionEdited: function(text) { root.speakingMotionEdited(text) }
        onFallbackExpressionEdited: function(text) { root.fallbackExpressionEdited(text) }
        onVoiceSpeedEdited: function(value) { root.voiceSpeedEdited(value) }
        onLipSyncSensitivityEdited: function(value) { root.lipSyncSensitivityEdited(value) }
        onVoiceLipSyncEnabledEdited: function(checked) { root.voiceLipSyncEnabledEdited(checked) }
        onEmotionSoundEnabledEdited: function(checked) { root.emotionSoundEnabledEdited(checked) }
        onVoiceTrainingConsentEdited: function(checked) { root.voiceTrainingConsentEdited(checked) }
        onVoiceTrainingStartRequested: root.voiceTrainingStartRequested()
        onVoiceTrainingRefreshRequested: root.voiceTrainingRefreshRequested()
    }
}
