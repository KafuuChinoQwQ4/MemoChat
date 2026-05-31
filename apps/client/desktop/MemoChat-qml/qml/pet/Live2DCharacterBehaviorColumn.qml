pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: root
    spacing: 12

    property var backdrop: null
    property var toneCombo: null
    property var responseLengthCombo: null
    property var languageCombo: null
    property color accentBlue: Qt.rgba(0.32, 0.56, 0.86, 1.0)
    property color accentGreen: Qt.rgba(0.32, 0.60, 0.44, 1.0)
    property color accentRose: Qt.rgba(0.78, 0.36, 0.45, 1.0)
    property string relationshipStyle: ""
    property string personalityTags: ""
    property string worldSetting: ""
    property string forbiddenRules: ""
    property real emotionLevel: 0.62
    property real creativityLevel: 0.48
    property string speechRules: ""
    property string catchphrases: ""
    property bool autoStartPetOnClientStart: false
    property bool idleMotionEnabled: true
    property bool gazeFollowEnabled: true
    property bool memoryEnabled: true
    property bool interruptEnabled: true
    property bool cameraEnabled: false
    property bool cloudVisionEnabled: false

    signal relationshipStyleEdited(string text)
    signal personalityTagsEdited(string text)
    signal worldSettingEdited(string text)
    signal forbiddenRulesEdited(string text)
    signal emotionLevelEdited(real value)
    signal creativityLevelEdited(real value)
    signal speechRulesEdited(string text)
    signal catchphrasesEdited(string text)
    signal autoStartPetOnClientStartEdited(bool checked)
    signal idleMotionEnabledEdited(bool checked)
    signal gazeFollowEnabledEdited(bool checked)
    signal memoryEnabledEdited(bool checked)
    signal interruptEnabledEdited(bool checked)
    signal cameraEnabledEdited(bool checked)
    signal cloudVisionEnabledEdited(bool checked)

    Live2DPersonaPanel {
        backdrop: root.backdrop
        accentColor: root.accentRose
        relationshipStyle: root.relationshipStyle
        personalityTags: root.personalityTags
        worldSetting: root.worldSetting
        forbiddenRules: root.forbiddenRules
        onRelationshipStyleEdited: function(text) { root.relationshipStyleEdited(text) }
        onPersonalityTagsEdited: function(text) { root.personalityTagsEdited(text) }
        onWorldSettingEdited: function(text) { root.worldSettingEdited(text) }
        onForbiddenRulesEdited: function(text) { root.forbiddenRulesEdited(text) }
    }

    Live2DSpeechStylePanel {
        accentColor: root.accentBlue
        toneCombo: root.toneCombo
        responseLengthCombo: root.responseLengthCombo
        languageCombo: root.languageCombo
        emotionLevel: root.emotionLevel
        creativityLevel: root.creativityLevel
        speechRules: root.speechRules
        catchphrases: root.catchphrases
        onEmotionLevelEdited: function(value) { root.emotionLevelEdited(value) }
        onCreativityLevelEdited: function(value) { root.creativityLevelEdited(value) }
        onSpeechRulesEdited: function(text) { root.speechRulesEdited(text) }
        onCatchphrasesEdited: function(text) { root.catchphrasesEdited(text) }
    }

    Live2DBehaviorMemoryPanel {
        accentColor: root.accentGreen
        autoStartPetOnClientStart: root.autoStartPetOnClientStart
        idleMotionEnabled: root.idleMotionEnabled
        gazeFollowEnabled: root.gazeFollowEnabled
        memoryEnabled: root.memoryEnabled
        interruptEnabled: root.interruptEnabled
        cameraEnabled: root.cameraEnabled
        cloudVisionEnabled: root.cloudVisionEnabled
        onAutoStartPetOnClientStartEdited: function(checked) { root.autoStartPetOnClientStartEdited(checked) }
        onIdleMotionEnabledEdited: function(checked) { root.idleMotionEnabledEdited(checked) }
        onGazeFollowEnabledEdited: function(checked) { root.gazeFollowEnabledEdited(checked) }
        onMemoryEnabledEdited: function(checked) { root.memoryEnabledEdited(checked) }
        onInterruptEnabledEdited: function(checked) { root.interruptEnabledEdited(checked) }
        onCameraEnabledEdited: function(checked) { root.cameraEnabledEdited(checked) }
        onCloudVisionEnabledEdited: function(checked) { root.cloudVisionEnabledEdited(checked) }
    }
}
