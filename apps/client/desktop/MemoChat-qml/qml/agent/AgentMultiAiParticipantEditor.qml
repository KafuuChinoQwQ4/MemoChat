pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Item {
    id: root

    property bool gameBusy: false
    property var editingParticipant: ({})
    readonly property string customSkillValue: "__custom_skill__"
    readonly property string customStrategyValue: "__custom_strategy__"
    readonly property var skillOptions: [
        { "label": "写作", "value": "writer", "hint": "生成自然发言和可读文本" },
        { "label": "通用对话", "value": "general_chat", "hint": "轻量聊天和普通回复" },
        { "label": "研究", "value": "researcher", "hint": "证据收集和分析" },
        { "label": "审阅", "value": "reviewer", "hint": "检查风险和漏洞" },
        { "label": "支持", "value": "support", "hint": "排查问题和给步骤" },
        { "label": "知识整理", "value": "knowledge_curator", "hint": "整理知识和摘要" },
        { "label": "知识库问答", "value": "knowledge_copilot", "hint": "优先检索知识库" },
        { "label": "联网研究", "value": "research_assistant", "hint": "搜索后回答" },
        { "label": "图谱推荐", "value": "graph_recommender", "hint": "使用关系图谱" },
        { "label": "自定义 skill", "value": customSkillValue, "hint": "输入后端已支持的新 skill" }
    ]
    readonly property var strategyOptions: [
        { "label": "角色扮演", "value": "roleplay", "hint": "优先保持人设和口吻" },
        { "label": "均衡", "value": "balanced", "hint": "稳定中立的发言方式" },
        { "label": "群聊", "value": "group_chat", "hint": "适合多 AI 聊天" },
        { "label": "推理", "value": "deductive", "hint": "重视线索和逻辑" },
        { "label": "观察", "value": "observant", "hint": "关注细节和他人发言" },
        { "label": "调查", "value": "investigative", "hint": "主动追问和求证" },
        { "label": "强势", "value": "assertive", "hint": "表达明确，推动局面" },
        { "label": "进攻", "value": "aggressive", "hint": "更主动施压" },
        { "label": "欺骗", "value": "deceptive", "hint": "适合隐藏意图的发言" },
        { "label": "直觉", "value": "intuitive", "hint": "更依赖感觉和快速判断" },
        { "label": "自定义风格", "value": customStrategyValue, "hint": "输入自定义 strategy" }
    ]
    readonly property bool customSkillSelected: editorSkillCombo.currentIndex >= 0
        && editorSkillCombo.currentIndex < skillOptions.length
        && skillOptions[editorSkillCombo.currentIndex].value === customSkillValue
    readonly property bool customStrategySelected: editorStrategyCombo.currentIndex >= 0
        && editorStrategyCombo.currentIndex < strategyOptions.length
        && strategyOptions[editorStrategyCombo.currentIndex].value === customStrategyValue

    signal saveRequested(string participantId, string displayName, string persona, string strategy, string skillName)

    Layout.fillWidth: true
    Layout.preferredHeight: 0
    Layout.minimumHeight: 0

    function participantId(participant) {
        return participant.participant_id || participant.id || ""
    }

    function participantStrategy(participant) {
        var metadata = participant.metadata || {}
        return metadata.strategy || participant.strategy || "balanced"
    }

    function participantSkill(participant) {
        return participant.skill_name || "writer"
    }

    function optionIndex(options, value, fallbackValue) {
        var current = value && value.length > 0 ? value : fallbackValue
        for (var i = 0; i < options.length; ++i) {
            if (options[i].value === current) {
                return i
            }
        }
        return Math.max(0, options.length - 1)
    }

    function optionText(options, index) {
        if (index < 0 || index >= options.length) {
            return ""
        }
        var item = options[index]
        if (item.value === root.customStrategyValue) {
            return item.label
        }
        return item.label
    }

    function isCustomValue(options, value) {
        if (!value || value.length === 0) {
            return false
        }
        for (var i = 0; i < options.length; ++i) {
            if (options[i].value === value) {
                return false
            }
        }
        return true
    }

    function comboBackground(combo) {
        return combo.pressed ? Qt.rgba(0.88, 0.93, 1.0, 0.72)
                             : combo.hovered ? Qt.rgba(1, 1, 1, 0.62)
                                             : Qt.rgba(1, 1, 1, 0.46)
    }

    function openForParticipant(participant) {
        if (!participant || (participant.kind || "") !== "agent") {
            return
        }
        root.editingParticipant = participant
        editorNameField.text = participant.display_name || participant.name || "AI"
        editorPersonaField.text = participant.persona || ""
        var skill = root.participantSkill(participant)
        editorSkillCombo.currentIndex = root.optionIndex(root.skillOptions, skill, "writer")
        customSkillField.text = root.isCustomValue(root.skillOptions, skill) ? skill : ""
        var strategy = root.participantStrategy(participant)
        editorStrategyCombo.currentIndex = root.optionIndex(root.strategyOptions, strategy, "balanced")
        customStrategyField.text = root.isCustomValue(root.strategyOptions, strategy) ? strategy : ""
        agentEditorPopup.open()
    }

    function saveParticipantEditor() {
        var participantId = root.participantId(root.editingParticipant)
        if (participantId.length === 0) {
            return
        }
        var strategy = ""
        if (root.customStrategySelected) {
            strategy = customStrategyField.text.trim()
        } else if (editorStrategyCombo.currentIndex >= 0 && editorStrategyCombo.currentIndex < root.strategyOptions.length) {
            strategy = root.strategyOptions[editorStrategyCombo.currentIndex].value
        }
        var skillName = ""
        if (root.customSkillSelected) {
            skillName = customSkillField.text.trim()
        } else if (editorSkillCombo.currentIndex >= 0 && editorSkillCombo.currentIndex < root.skillOptions.length) {
            skillName = root.skillOptions[editorSkillCombo.currentIndex].value
        }
        root.saveRequested(participantId,
                           editorNameField.text,
                           editorPersonaField.text,
                           strategy,
                           skillName)
        agentEditorPopup.close()
    }

    Popup {
        id: agentEditorPopup
        parent: Overlay.overlay
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        width: Math.min(460, Math.max(320, parent ? parent.width - 48 : 420))
        height: Math.min(editorColumn.implicitHeight + 28, parent ? parent.height - 48 : editorColumn.implicitHeight + 28)
        anchors.centerIn: Overlay.overlay
        padding: 0

        background: Rectangle {
            radius: 12
            color: Qt.rgba(0.96, 0.98, 1.0, 0.96)
            border.color: Qt.rgba(1, 1, 1, 0.62)
        }

        contentItem: ColumnLayout {
            id: editorColumn
            width: agentEditorPopup.width
            spacing: 9

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 14
                Layout.rightMargin: 14
                Layout.topMargin: 14
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    text: "编辑 Agent"
                    color: "#243145"
                    font.pixelSize: 15
                    font.bold: true
                    elide: Text.ElideRight
                }

                GlassButton {
                    Layout.preferredWidth: 58
                    Layout.preferredHeight: 28
                    text: "关闭"
                    textPixelSize: 11
                    cornerRadius: 7
                    normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.18)
                    hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.28)
                    onClicked: agentEditorPopup.close()
                }
            }

            TextField {
                id: editorNameField
                Layout.fillWidth: true
                Layout.leftMargin: 14
                Layout.rightMargin: 14
                Layout.preferredHeight: 34
                placeholderText: "Agent 昵称"
                selectByMouse: true
            }

            ComboBox {
                id: editorSkillCombo
                Layout.fillWidth: true
                Layout.leftMargin: 14
                Layout.rightMargin: 14
                Layout.preferredHeight: 34
                textRole: "label"
                valueRole: "value"
                model: root.skillOptions
                displayText: root.optionText(root.skillOptions, currentIndex)
                onActivated: {
                    if (root.customSkillSelected) {
                        customSkillField.forceActiveFocus()
                    } else {
                        customSkillField.text = ""
                    }
                }

                delegate: ItemDelegate {
                    id: skillDelegate
                    width: ListView.view ? ListView.view.width : 300
                    height: 42
                    highlighted: editorSkillCombo.highlightedIndex === index

                    required property int index
                    required property var modelData

                    contentItem: Column {
                        spacing: 1
                        Text {
                            width: parent.width
                            text: root.optionText(root.skillOptions, skillDelegate.index)
                            color: "#243145"
                            font.pixelSize: 12
                            font.bold: true
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            text: skillDelegate.modelData.hint
                            color: "#6c7a8e"
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }
                    }
                    background: Rectangle {
                        color: skillDelegate.highlighted ? Qt.rgba(0.35, 0.61, 0.90, 0.16) : "transparent"
                        radius: 6
                    }
                }
                background: Rectangle {
                    radius: 8
                    color: root.comboBackground(editorSkillCombo)
                    border.color: editorSkillCombo.hovered || editorSkillCombo.pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.42) : Qt.rgba(1, 1, 1, 0.50)
                }
                contentItem: Text {
                    leftPadding: 10
                    rightPadding: 30
                    text: "Skill：" + editorSkillCombo.displayText
                    color: "#243145"
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                indicator: Image {
                    x: editorSkillCombo.width - width - 10
                    y: (editorSkillCombo.height - height) / 2
                    width: 12
                    height: 12
                    source: "qrc:/icons/dropdown.png"
                    fillMode: Image.PreserveAspectFit
                    rotation: editorSkillCombo.popup.visible ? 180 : 0
                    opacity: editorSkillCombo.enabled ? 0.78 : 0.38
                }
            }

            TextField {
                id: customSkillField
                Layout.fillWidth: true
                Layout.leftMargin: 14
                Layout.rightMargin: 14
                Layout.preferredHeight: root.customSkillSelected ? 32 : 0
                visible: root.customSkillSelected
                placeholderText: "自定义 skill_name"
                selectByMouse: true
            }

            ComboBox {
                id: editorStrategyCombo
                Layout.fillWidth: true
                Layout.leftMargin: 14
                Layout.rightMargin: 14
                Layout.preferredHeight: 34
                textRole: "label"
                valueRole: "value"
                model: root.strategyOptions
                displayText: root.optionText(root.strategyOptions, currentIndex)
                onActivated: {
                    if (root.customStrategySelected) {
                        customStrategyField.forceActiveFocus()
                    } else {
                        customStrategyField.text = ""
                    }
                }

                delegate: ItemDelegate {
                    id: strategyDelegate
                    width: ListView.view ? ListView.view.width : 300
                    height: 42
                    highlighted: editorStrategyCombo.highlightedIndex === index

                    required property int index
                    required property var modelData

                    contentItem: Column {
                        spacing: 1
                        Text {
                            width: parent.width
                            text: root.optionText(root.strategyOptions, strategyDelegate.index)
                            color: "#243145"
                            font.pixelSize: 12
                            font.bold: true
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            text: strategyDelegate.modelData.hint
                            color: "#6c7a8e"
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }
                    }
                    background: Rectangle {
                        color: strategyDelegate.highlighted ? Qt.rgba(0.35, 0.61, 0.90, 0.16) : "transparent"
                        radius: 6
                    }
                }
                background: Rectangle {
                    radius: 8
                    color: root.comboBackground(editorStrategyCombo)
                    border.color: editorStrategyCombo.hovered || editorStrategyCombo.pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.42) : Qt.rgba(1, 1, 1, 0.50)
                }
                contentItem: Text {
                    leftPadding: 10
                    rightPadding: 30
                    text: "说话方式：" + editorStrategyCombo.displayText
                    color: "#243145"
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                indicator: Image {
                    x: editorStrategyCombo.width - width - 10
                    y: (editorStrategyCombo.height - height) / 2
                    width: 12
                    height: 12
                    source: "qrc:/icons/dropdown.png"
                    fillMode: Image.PreserveAspectFit
                    rotation: editorStrategyCombo.popup.visible ? 180 : 0
                    opacity: editorStrategyCombo.enabled ? 0.78 : 0.38
                }
            }

            TextField {
                id: customStrategyField
                Layout.fillWidth: true
                Layout.leftMargin: 14
                Layout.rightMargin: 14
                Layout.preferredHeight: root.customStrategySelected ? 32 : 0
                visible: root.customStrategySelected
                placeholderText: "自定义说话方式"
                selectByMouse: true
            }

            TextArea {
                id: editorPersonaField
                Layout.fillWidth: true
                Layout.leftMargin: 14
                Layout.rightMargin: 14
                Layout.preferredHeight: 118
                placeholderText: "性格 / 人设 / 回复约束"
                wrapMode: TextArea.Wrap
                selectByMouse: true
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 14
                Layout.rightMargin: 14
                Layout.bottomMargin: 14
                spacing: 8

                Item { Layout.fillWidth: true }

                GlassButton {
                    Layout.preferredWidth: 78
                    Layout.preferredHeight: 32
                    text: "取消"
                    textPixelSize: 12
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.18)
                    hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.28)
                    onClicked: agentEditorPopup.close()
                }

                GlassButton {
                    Layout.preferredWidth: 86
                    Layout.preferredHeight: 32
                    text: "保存"
                    textPixelSize: 12
                    cornerRadius: 8
                    enabled: editorNameField.text.trim().length > 0
                             && (!root.customSkillSelected || customSkillField.text.trim().length > 0)
                             && (!root.customStrategySelected || customStrategyField.text.trim().length > 0)
                             && !root.gameBusy
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                    onClicked: root.saveParticipantEditor()
                }
            }
        }
    }
}
