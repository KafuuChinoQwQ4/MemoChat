import QtQuick 2.15
import QtQuick.Layouts 1.15

Live2DSectionPanel {
    id: root

    property Item backdrop: null
    property string relationshipStyle: ""
    property string personalityTags: ""
    property string worldSetting: ""
    property string forbiddenRules: ""

    signal relationshipStyleEdited(string text)
    signal personalityTagsEdited(string text)
    signal worldSettingEdited(string text)
    signal forbiddenRulesEdited(string text)

    title: "人设"
    subtitle: "把人物设定和边界整理成后续可持久化的角色档案"

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Live2DFormFieldBlock {
            backdrop: root.backdrop
            title: "关系定位"
            text: root.relationshipStyle
            placeholderText: "例如 同伴 / 助手 / 管家"
            onTextChanged: root.relationshipStyleEdited(text)
        }

        Live2DFormFieldBlock {
            backdrop: root.backdrop
            title: "性格标签"
            text: root.personalityTags
            placeholderText: "用逗号分隔"
            onTextChanged: root.personalityTagsEdited(text)
        }
    }

    Live2DTextAreaBlock {
        title: "背景与世界观"
        text: root.worldSetting
        preferredHeight: 86
        placeholderText: "角色来自哪里、和用户是什么关系、关注什么事情..."
        onTextChanged: root.worldSettingEdited(text)
    }

    Live2DTextAreaBlock {
        title: "禁忌与边界"
        text: root.forbiddenRules
        preferredHeight: 74
        placeholderText: "不该说什么、不该做什么、隐私和安全边界..."
        onTextChanged: root.forbiddenRulesEdited(text)
    }
}
