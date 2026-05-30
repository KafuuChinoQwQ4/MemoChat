import QtQuick 2.15
import QtQuick.Layouts 1.15

Live2DSectionPanel {
    id: root

    property var toneCombo: null
    property var responseLengthCombo: null
    property var languageCombo: null
    property real emotionLevel: 0.62
    property real creativityLevel: 0.48
    property string speechRules: ""
    property string catchphrases: ""

    signal emotionLevelEdited(real value)
    signal creativityLevelEdited(real value)
    signal speechRulesEdited(string text)
    signal catchphrasesEdited(string text)

    title: "说话风格"
    subtitle: "控制语气、语言、长度和情绪表现"

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Live2DComboBlock {
            title: "语气"
            comboId: root.toneCombo
        }

        Live2DComboBlock {
            title: "回复长度"
            comboId: root.responseLengthCombo
        }

        Live2DComboBlock {
            title: "语言"
            comboId: root.languageCombo
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Live2DSliderBlock {
            title: "情绪表现"
            value: root.emotionLevel
            fromValue: 0.0
            toValue: 1.0
            valueText: Math.round(root.emotionLevel * 100) + "%"
            onValueChanged: root.emotionLevelEdited(value)
        }

        Live2DSliderBlock {
            title: "创造性"
            value: root.creativityLevel
            fromValue: 0.0
            toValue: 1.0
            valueText: Math.round(root.creativityLevel * 100) + "%"
            onValueChanged: root.creativityLevelEdited(value)
        }
    }

    Live2DTextAreaBlock {
        title: "说话规则"
        text: root.speechRules
        preferredHeight: 86
        placeholderText: "例如 先安抚情绪、不要长篇解释、少用反问..."
        onTextChanged: root.speechRulesEdited(text)
    }

    Live2DTextAreaBlock {
        title: "常用口头禅"
        text: root.catchphrases
        preferredHeight: 74
        placeholderText: "每行一句，可以按情绪映射到动作或语音。"
        onTextChanged: root.catchphrasesEdited(text)
    }
}
