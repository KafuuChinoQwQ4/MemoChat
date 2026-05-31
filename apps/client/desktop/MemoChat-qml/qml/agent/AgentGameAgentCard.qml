pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"
import "AgentGameRuntime.js" as AgentGameRuntime

Rectangle {
    id: root

    property var agentData: ({})
    property int agentIndex: -1
    property var availableModels: []
    property var rolePresets: []
    property string rulesetId: "werewolf.basic"
    property bool showRoleControls: true
    readonly property string customRoleValue: "__custom_role__"
    readonly property string customSkillValue: "__custom_skill__"
    readonly property string customStrategyValue: "__custom_strategy__"

    signal fieldChanged(int agentIndex, string key, var value)
    signal removeRequested(int agentIndex)

    readonly property var skillOptions: AgentGameRuntime.skillOptions(root.customSkillValue)
    readonly property var strategyOptions: AgentGameRuntime.strategyOptions(root.customStrategyValue)
    readonly property var roleOptions: buildRoleOptions()
    readonly property bool customRoleSelected: roleCombo.currentIndex >= 0
        && roleCombo.currentIndex < roleOptions.length
        && roleOptions[roleCombo.currentIndex].value === customRoleValue
    readonly property bool customSkillSelected: skillCombo.currentIndex >= 0
        && skillOptions[skillCombo.currentIndex].value === customSkillValue
    readonly property bool customStrategySelected: strategyCombo.currentIndex >= 0
        && strategyOptions[strategyCombo.currentIndex].value === customStrategyValue

    function modelLabel(model) {
        return AgentGameRuntime.modelLabel(model)
    }

    function optionIndex(options, value, fallbackValue) {
        return AgentGameRuntime.optionIndex(options, value, fallbackValue)
    }

    function isCustomStoredValue(options, value) {
        return AgentGameRuntime.isCustomStoredValue(options, value)
    }

    function fallbackWerewolfRoles() {
        return AgentGameRuntime.fallbackWerewolfRoles()
    }

    function buildRoleOptions() {
        return AgentGameRuntime.roleOptions(
                    root.rolePresets, root.rulesetId, root.customRoleValue,
                    "开局随机分配身份",
                    "未指定身份时，系统会在开始游戏时随机分配该 AI 的角色。",
                    "使用你输入的 role_key 加入房间；后端会按该身份保存和传给 Agent。")
    }

    function selectedRoleRule() {
        if (!root.showRoleControls) {
            return ""
        }
        var value = root.agentData.role_key || ""
        var index = root.optionIndex(root.roleOptions, value, "")
        return AgentGameRuntime.optionRule(root.roleOptions, index)
    }

    function commitCustomRole() {
        if (customRoleSelected) {
            root.fieldChanged(root.agentIndex, "role_key", customRoleField.text.trim())
        }
    }

    function commitCustomSkill() {
        if (customSkillSelected) {
            var value = customSkillField.text.trim()
            if (value.length > 0) {
                root.fieldChanged(root.agentIndex, "skill_name", value)
            }
        }
    }

    function commitCustomStrategy() {
        if (customStrategySelected) {
            var value = customStrategyField.text.trim()
            if (value.length > 0) {
                root.fieldChanged(root.agentIndex, "strategy", value)
            }
        }
    }

    Layout.fillWidth: true
    Layout.preferredHeight: root.showRoleControls
        ? (346
           + (root.customRoleSelected ? 38 : 0)
           + (root.customSkillSelected || root.customStrategySelected ? 38 : 0))
        : (root.customSkillSelected || root.customStrategySelected ? 264 : 226)
    radius: 8
    color: Qt.rgba(1, 1, 1, 0.38)
    border.color: Qt.rgba(1, 1, 1, 0.38)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 7

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                text: root.agentData.display_name || ""
                placeholderText: "Agent 名称"
                selectByMouse: true
                onEditingFinished: root.fieldChanged(root.agentIndex, "display_name", text)
            }

            GlassButton {
                Layout.preferredWidth: 54
                Layout.preferredHeight: 30
                text: "移除"
                textPixelSize: 11
                cornerRadius: 7
                normalColor: Qt.rgba(0.80, 0.34, 0.34, 0.16)
                hoverColor: Qt.rgba(0.80, 0.34, 0.34, 0.26)
                onClicked: root.removeRequested(root.agentIndex)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            AgentGameOptionCombo {
                id: roleCombo
                Layout.preferredWidth: 168
                Layout.preferredHeight: 30
                visible: root.showRoleControls
                optionModel: root.roleOptions
                selectedValue: root.agentData.role_key || ""
                fallbackValue: ""
                customValues: [root.customRoleValue, root.customSkillValue, root.customStrategyValue]
                placeholderWidth: 280
                onOptionActivated: function(option) {
                    var value = option.value || ""
                    if (value === root.customRoleValue) {
                        customRoleField.forceActiveFocus()
                    } else {
                        root.fieldChanged(root.agentIndex, "role_key", value)
                    }
                }
            }

            ComboBox {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                textRole: "label"
                valueRole: "value"
                model: {
                    var rows = []
                    for (var i = 0; i < root.availableModels.length; ++i) {
                        rows.push({ "label": root.modelLabel(root.availableModels[i]), "value": i })
                    }
                    if (rows.length === 0) rows.push({ "label": root.modelLabel(null), "value": 0 })
                    return rows
                }
                currentIndex: {
                    for (var i = 0; i < root.availableModels.length; ++i) {
                        var m = root.availableModels[i]
                        if (m.model_type === root.agentData.model_type && m.model_name === root.agentData.model_name) return i
                    }
                    return 0
                }
                onActivated: {
                    var chosen = root.availableModels[currentIndex] || { "model_type": "ollama", "model_name": "qwen2.5:7b" }
                    root.fieldChanged(root.agentIndex, "model_type", chosen.model_type || "ollama")
                    root.fieldChanged(root.agentIndex, "model_name", chosen.model_name || "qwen2.5:7b")
                }
            }
        }

        TextField {
            id: customRoleField
            Layout.fillWidth: true
            Layout.preferredHeight: root.customRoleSelected ? 30 : 0
            visible: root.showRoleControls && root.customRoleSelected
            text: root.isCustomStoredValue(root.roleOptions, root.agentData.role_key || "") ? root.agentData.role_key : ""
            placeholderText: "自定义 role_key"
            selectByMouse: true
            onEditingFinished: root.commitCustomRole()
            onAccepted: root.commitCustomRole()
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.showRoleControls ? 46 : 0
            visible: root.showRoleControls
            radius: 8
            color: Qt.rgba(0.35, 0.61, 0.90, 0.10)
            border.color: Qt.rgba(0.35, 0.61, 0.90, 0.16)

            Label {
                anchors.fill: parent
                anchors.margins: 8
                text: root.selectedRoleRule()
                color: "#50647b"
                font.pixelSize: 11
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                maximumLineCount: 2
                verticalAlignment: Text.AlignVCenter
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 8
            rowSpacing: 7

            AgentGameOptionCombo {
                id: skillCombo
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                optionModel: root.skillOptions
                selectedValue: root.agentData.skill_name || ""
                fallbackValue: "writer"
                customValues: [root.customRoleValue, root.customSkillValue, root.customStrategyValue]
                placeholderWidth: 260
                onOptionActivated: function(option) {
                    var value = option.value || ""
                    if (value === root.customSkillValue) {
                        customSkillField.forceActiveFocus()
                    } else {
                        root.fieldChanged(root.agentIndex, "skill_name", value)
                    }
                }
            }

            AgentGameOptionCombo {
                id: strategyCombo
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                optionModel: root.strategyOptions
                selectedValue: root.agentData.strategy || ""
                fallbackValue: root.showRoleControls ? "roleplay" : "group_chat"
                customValues: [root.customRoleValue, root.customSkillValue, root.customStrategyValue]
                placeholderWidth: 260
                onOptionActivated: function(option) {
                    var value = option.value || ""
                    if (value === root.customStrategyValue) {
                        customStrategyField.forceActiveFocus()
                    } else {
                        root.fieldChanged(root.agentIndex, "strategy", value)
                    }
                }
            }

            TextField {
                id: customSkillField
                Layout.fillWidth: true
                Layout.preferredHeight: root.customSkillSelected ? 30 : 0
                visible: root.customSkillSelected
                text: root.isCustomStoredValue(root.skillOptions, root.agentData.skill_name || "") ? root.agentData.skill_name : ""
                placeholderText: "自定义 skill_name"
                selectByMouse: true
                onEditingFinished: root.commitCustomSkill()
                onAccepted: root.commitCustomSkill()
            }

            TextField {
                id: customStrategyField
                Layout.fillWidth: true
                Layout.preferredHeight: root.customStrategySelected ? 30 : 0
                visible: root.customStrategySelected
                text: root.isCustomStoredValue(root.strategyOptions, root.agentData.strategy || "") ? root.agentData.strategy : ""
                placeholderText: "自定义 strategy"
                selectByMouse: true
                onEditingFinished: root.commitCustomStrategy()
                onAccepted: root.commitCustomStrategy()
            }
        }

        TextArea {
            Layout.fillWidth: true
            Layout.preferredHeight: 54
            text: root.agentData.environment || ""
            placeholderText: "环境"
            wrapMode: TextArea.Wrap
            selectByMouse: true
            onActiveFocusChanged: if (!activeFocus) root.fieldChanged(root.agentIndex, "environment", text)
        }

        TextArea {
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: root.agentData.persona || ""
            placeholderText: "角色模板 / 行为约束"
            wrapMode: TextArea.Wrap
            selectByMouse: true
            onActiveFocusChanged: if (!activeFocus) root.fieldChanged(root.agentIndex, "persona", text)
        }
    }
}
