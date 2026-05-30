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

    function optionText(options, index) {
        return AgentGameRuntime.optionText(
                    options, index,
                    [root.customRoleValue, root.customSkillValue, root.customStrategyValue])
    }

    function isCustomStoredValue(options, value) {
        return AgentGameRuntime.isCustomStoredValue(options, value)
    }

    function comboBackground(combo) {
        return combo.pressed ? Qt.rgba(0.88, 0.93, 1.0, 0.72)
                             : combo.hovered ? Qt.rgba(1, 1, 1, 0.62)
                                             : Qt.rgba(1, 1, 1, 0.46)
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

            ComboBox {
                id: roleCombo
                Layout.preferredWidth: 168
                Layout.preferredHeight: 30
                visible: root.showRoleControls
                textRole: "label"
                valueRole: "value"
                model: root.roleOptions
                currentIndex: root.optionIndex(root.roleOptions, root.agentData.role_key || "", "")
                displayText: root.optionText(root.roleOptions, currentIndex)
                onActivated: {
                    var value = root.roleOptions[currentIndex].value
                    if (value === root.customRoleValue) {
                        customRoleField.forceActiveFocus()
                    } else {
                        root.fieldChanged(root.agentIndex, "role_key", value)
                    }
                }

                delegate: ItemDelegate {
                    id: roleOptionDelegate
                    width: ListView.view ? ListView.view.width : 280
                    height: 46
                    highlighted: roleCombo.highlightedIndex === index

                    required property int index
                    required property var modelData

                    contentItem: Column {
                        spacing: 1
                        Text {
                            width: parent.width
                            text: root.optionText(root.roleOptions, roleOptionDelegate.index)
                            color: "#243145"
                            font.pixelSize: 12
                            font.bold: true
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            text: roleOptionDelegate.modelData.hint
                            color: "#6c7a8e"
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }
                    }
                    background: Rectangle {
                        color: roleOptionDelegate.highlighted ? Qt.rgba(0.35, 0.61, 0.90, 0.16) : "transparent"
                        radius: 6
                    }
                }
                background: Rectangle {
                    radius: 7
                    color: root.comboBackground(roleCombo)
                    border.color: roleCombo.hovered || roleCombo.pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.42) : Qt.rgba(1, 1, 1, 0.38)
                }
                contentItem: Text {
                    leftPadding: 10
                    rightPadding: 28
                    text: roleCombo.displayText
                    color: "#243145"
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                indicator: Image {
                    x: roleCombo.width - width - 9
                    y: (roleCombo.height - height) / 2
                    width: 12
                    height: 12
                    source: "qrc:/icons/dropdown.png"
                    fillMode: Image.PreserveAspectFit
                    rotation: roleCombo.popup.visible ? 180 : 0
                    opacity: roleCombo.enabled ? 0.78 : 0.38
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

            ComboBox {
                id: skillCombo
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                textRole: "label"
                valueRole: "value"
                model: root.skillOptions
                currentIndex: root.optionIndex(root.skillOptions, root.agentData.skill_name || "", "writer")
                displayText: root.optionText(root.skillOptions, currentIndex)
                onActivated: {
                    var value = root.skillOptions[currentIndex].value
                    if (value === root.customSkillValue) {
                        customSkillField.forceActiveFocus()
                    } else {
                        root.fieldChanged(root.agentIndex, "skill_name", value)
                    }
                }

                delegate: ItemDelegate {
                    id: skillOptionDelegate
                    width: ListView.view ? ListView.view.width : 260
                    height: 42
                    highlighted: skillCombo.highlightedIndex === index

                    required property int index
                    required property var modelData

                    contentItem: Column {
                        spacing: 1
                        Text {
                            width: parent.width
                            text: root.optionText(root.skillOptions, skillOptionDelegate.index)
                            color: "#243145"
                            font.pixelSize: 12
                            font.bold: true
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            text: skillOptionDelegate.modelData.hint
                            color: "#6c7a8e"
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }
                    }
                    background: Rectangle {
                        color: skillOptionDelegate.highlighted ? Qt.rgba(0.35, 0.61, 0.90, 0.16) : "transparent"
                        radius: 6
                    }
                }
                background: Rectangle {
                    radius: 7
                    color: root.comboBackground(skillCombo)
                    border.color: skillCombo.hovered || skillCombo.pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.42) : Qt.rgba(1, 1, 1, 0.38)
                }
                contentItem: Text {
                    leftPadding: 10
                    rightPadding: 28
                    text: skillCombo.displayText
                    color: "#243145"
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                indicator: Image {
                    x: skillCombo.width - width - 9
                    y: (skillCombo.height - height) / 2
                    width: 12
                    height: 12
                    source: "qrc:/icons/dropdown.png"
                    fillMode: Image.PreserveAspectFit
                    rotation: skillCombo.popup.visible ? 180 : 0
                    opacity: skillCombo.enabled ? 0.78 : 0.38
                }
            }

            ComboBox {
                id: strategyCombo
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                textRole: "label"
                valueRole: "value"
                model: root.strategyOptions
                currentIndex: root.optionIndex(root.strategyOptions, root.agentData.strategy || "", root.showRoleControls ? "roleplay" : "group_chat")
                displayText: root.optionText(root.strategyOptions, currentIndex)
                onActivated: {
                    var value = root.strategyOptions[currentIndex].value
                    if (value === root.customStrategyValue) {
                        customStrategyField.forceActiveFocus()
                    } else {
                        root.fieldChanged(root.agentIndex, "strategy", value)
                    }
                }

                delegate: ItemDelegate {
                    id: strategyOptionDelegate
                    width: ListView.view ? ListView.view.width : 260
                    height: 42
                    highlighted: strategyCombo.highlightedIndex === index

                    required property int index
                    required property var modelData

                    contentItem: Column {
                        spacing: 1
                        Text {
                            width: parent.width
                            text: root.optionText(root.strategyOptions, strategyOptionDelegate.index)
                            color: "#243145"
                            font.pixelSize: 12
                            font.bold: true
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            text: strategyOptionDelegate.modelData.hint
                            color: "#6c7a8e"
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }
                    }
                    background: Rectangle {
                        color: strategyOptionDelegate.highlighted ? Qt.rgba(0.35, 0.61, 0.90, 0.16) : "transparent"
                        radius: 6
                    }
                }
                background: Rectangle {
                    radius: 7
                    color: root.comboBackground(strategyCombo)
                    border.color: strategyCombo.hovered || strategyCombo.pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.42) : Qt.rgba(1, 1, 1, 0.38)
                }
                contentItem: Text {
                    leftPadding: 10
                    rightPadding: 28
                    text: strategyCombo.displayText
                    color: "#243145"
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                indicator: Image {
                    x: strategyCombo.width - width - 9
                    y: (strategyCombo.height - height) / 2
                    width: 12
                    height: 12
                    source: "qrc:/icons/dropdown.png"
                    fillMode: Image.PreserveAspectFit
                    rotation: strategyCombo.popup.visible ? 180 : 0
                    opacity: strategyCombo.enabled ? 0.78 : 0.38
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
