pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "AgentGameRuntime.js" as AgentGameRuntime

ColumnLayout {
    id: root

    property bool gameSetupMode: false
    property var gameRulesets: []
    property var gameRolePresets: []
    property string customHumanRoleValue: "__custom_human_role__"
    property string testRulesetId: "multi_ai_chat.test"

    readonly property bool customHumanRoleSelected: root.gameSetupMode
        && roleCombo.currentIndex >= 0
        && root.gameRoleOptions()[roleCombo.currentIndex]
        && root.gameRoleOptions()[roleCombo.currentIndex].value === root.customHumanRoleValue

    signal rulesetActivated(string rulesetId)

    function formalRulesets() {
        return AgentGameRuntime.formalRulesets(root.gameRulesets, root.testRulesetId)
    }

    function selectedRulesetId() {
        if (!root.gameSetupMode) {
            return root.testRulesetId
        }
        var rows = root.formalRulesets()
        if (rulesetCombo.currentIndex >= 0 && rulesetCombo.currentIndex < rows.length) {
            return rows[rulesetCombo.currentIndex].ruleset_id || rows[rulesetCombo.currentIndex].id || "werewolf.basic"
        }
        return "werewolf.basic"
    }

    function gameRoleOptions() {
        return AgentGameRuntime.roleOptions(
                    root.gameRolePresets, root.selectedRulesetId(),
                    root.customHumanRoleValue,
                    "开始游戏时随机分配身份",
                    "未指定身份时，系统会在开局时随机分配角色。",
                    "使用你输入的 role_key 加入房间；开局时后端会保留该身份。")
    }

    function optionText(options, index) {
        return AgentGameRuntime.optionText(options, index, [root.customHumanRoleValue])
    }

    function selectedRoleKey() {
        if (!root.gameSetupMode || roleCombo.currentIndex < 0) {
            return ""
        }
        var rows = root.gameRoleOptions()
        if (roleCombo.currentIndex >= rows.length) {
            return ""
        }
        var value = rows[roleCombo.currentIndex].value || ""
        if (value === root.customHumanRoleValue) {
            return customHumanRoleField.text.trim()
        }
        return value
    }

    function selectedRoleRule() {
        if (!root.gameSetupMode || roleCombo.currentIndex < 0) {
            return ""
        }
        return AgentGameRuntime.optionRule(root.gameRoleOptions(), roleCombo.currentIndex)
    }

    function comboBackground(combo) {
        return combo.pressed ? Qt.rgba(0.88, 0.93, 1.0, 0.72)
                             : combo.hovered ? Qt.rgba(1, 1, 1, 0.62)
                                             : Qt.rgba(1, 1, 1, 0.46)
    }

    function roomTitle() {
        return roomTitleField.text
    }

    function gameContent() {
        return gameContentField.text.trim()
    }

    function resetDefaults() {
        roomTitleField.text = root.gameSetupMode ? "自定义 Game 房间" : "多 AI 测试房间"
        gameContentField.text = root.gameSetupMode ? "背景：\n目标：\n限制：" : ""
        customHumanRoleField.text = ""
        roleCombo.currentIndex = 0
        rulesetCombo.currentIndex = 0
        hostEnabledCheck.checked = true
        hostNameField.text = "主持人"
        hostPersonaField.text = ""
    }

    function disabledHostConfig() {
        return AgentGameRuntime.disabledHostConfig(root.gameSetupMode ? selectedRoleKey() : "")
    }

    function hostConfig(model) {
        if (!root.gameSetupMode || !hostEnabledCheck.checked) {
            return disabledHostConfig()
        }
        return AgentGameRuntime.hostConfig(
                    root.gameSetupMode, hostEnabledCheck.checked,
                    hostNameField.text.trim(), hostPersonaField.text.trim(),
                    gameContentField.text.trim(), model, selectedRoleKey())
    }

    Layout.fillWidth: true
    spacing: 8

    TextField {
        id: roomTitleField
        Layout.fillWidth: true
        Layout.preferredHeight: 36
        text: root.gameSetupMode ? "自定义 Game 房间" : "多 AI 测试房间"
        placeholderText: "房间标题"
        selectByMouse: true
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        visible: root.gameSetupMode

        ComboBox {
            id: rulesetCombo
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            textRole: "label"
            valueRole: "value"
            model: {
                var rows = []
                var source = root.formalRulesets()
                for (var i = 0; i < source.length; ++i) {
                    var item = source[i] || {}
                    rows.push({
                        "label": item.display_name || item.name || item.ruleset_id || "规则集",
                        "value": item.ruleset_id || item.id || ""
                    })
                }
                return rows
            }
            onActivated: {
                roleCombo.currentIndex = 0
                customHumanRoleField.text = ""
                root.rulesetActivated(root.selectedRulesetId())
            }
        }

        ComboBox {
            id: roleCombo
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            textRole: "label"
            valueRole: "value"
            model: root.gameRoleOptions()
            displayText: root.optionText(root.gameRoleOptions(), currentIndex)
            onActivated: {
                if (root.gameRoleOptions()[currentIndex].value !== root.customHumanRoleValue) {
                    customHumanRoleField.text = ""
                } else {
                    customHumanRoleField.forceActiveFocus()
                }
            }
            delegate: ItemDelegate {
                id: humanRoleDelegate
                width: ListView.view ? ListView.view.width : 260
                height: 44
                highlighted: roleCombo.highlightedIndex === index

                required property int index
                required property var modelData

                contentItem: Column {
                    spacing: 1
                    Text {
                        width: parent.width
                        text: root.optionText(root.gameRoleOptions(), humanRoleDelegate.index)
                        color: "#243145"
                        font.pixelSize: 12
                        font.bold: true
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: humanRoleDelegate.modelData.hint
                        color: "#6c7a8e"
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                }
                background: Rectangle {
                    color: humanRoleDelegate.highlighted ? Qt.rgba(0.35, 0.61, 0.90, 0.16) : "transparent"
                    radius: 6
                }
            }
            background: Rectangle {
                radius: 8
                color: root.comboBackground(roleCombo)
                border.color: roleCombo.hovered || roleCombo.pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.42) : Qt.rgba(1, 1, 1, 0.38)
            }
            contentItem: Text {
                leftPadding: 10
                rightPadding: 30
                text: "真人身份：" + roleCombo.displayText
                color: "#243145"
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            indicator: Image {
                x: roleCombo.width - width - 10
                y: (roleCombo.height - height) / 2
                width: 12
                height: 12
                source: "qrc:/icons/dropdown.png"
                fillMode: Image.PreserveAspectFit
                rotation: roleCombo.popup.visible ? 180 : 0
                opacity: roleCombo.enabled ? 0.78 : 0.38
            }
        }
    }

    TextField {
        id: customHumanRoleField
        Layout.fillWidth: true
        Layout.preferredHeight: root.customHumanRoleSelected ? 32 : 0
        visible: root.gameSetupMode && root.customHumanRoleSelected
        placeholderText: "自定义真人 role_key"
        selectByMouse: true
    }

    Label {
        Layout.fillWidth: true
        Layout.preferredHeight: root.gameSetupMode ? implicitHeight : 0
        visible: root.gameSetupMode
        text: root.selectedRoleRule()
        color: "#65758b"
        font.pixelSize: 11
        wrapMode: Text.Wrap
        maximumLineCount: 2
        elide: Text.ElideRight
    }

    TextArea {
        id: gameContentField
        Layout.fillWidth: true
        Layout.preferredHeight: root.gameSetupMode ? 86 : 0
        visible: root.gameSetupMode
        text: "背景：\n目标：\n限制："
        placeholderText: "自定义游戏内容、背景、胜负目标或主持约束"
        wrapMode: TextArea.Wrap
        selectByMouse: true
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: hostColumn.implicitHeight + 18
        visible: root.gameSetupMode
        radius: 10
        color: Qt.rgba(1, 1, 1, 0.28)
        border.color: Qt.rgba(1, 1, 1, 0.36)

        ColumnLayout {
            id: hostColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 9
            spacing: 7

            CheckBox {
                id: hostEnabledCheck
                text: "启用主持人"
                checked: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                enabled: hostEnabledCheck.checked

                TextField {
                    id: hostNameField
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 30
                    text: "主持人"
                    placeholderText: "主持名"
                    selectByMouse: true
                }

                TextField {
                    id: hostPersonaField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    placeholderText: "主持风格 / 开场约束"
                    selectByMouse: true
                }
            }
        }
    }
}
