pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

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
        { "label": "欺骗", "value": "deceptive", "hint": "适合隐藏身份角色" },
        { "label": "直觉", "value": "intuitive", "hint": "更依赖感觉和判断" },
        { "label": "自定义风格", "value": customStrategyValue, "hint": "输入自定义 strategy" }
    ]
    readonly property var roleOptions: buildRoleOptions()
    readonly property bool customRoleSelected: roleCombo.currentIndex >= 0
        && roleCombo.currentIndex < roleOptions.length
        && roleOptions[roleCombo.currentIndex].value === customRoleValue
    readonly property bool customSkillSelected: skillCombo.currentIndex >= 0
        && skillOptions[skillCombo.currentIndex].value === customSkillValue
    readonly property bool customStrategySelected: strategyCombo.currentIndex >= 0
        && strategyOptions[strategyCombo.currentIndex].value === customStrategyValue

    function modelLabel(model) {
        if (!model) {
            return "ollama:qwen2.5:7b"
        }
        return (model.model_type || "") + ":" + (model.model_name || "")
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
        if (item.showValue === false || item.value === "" || item.value === root.customRoleValue
                || item.value === root.customSkillValue || item.value === root.customStrategyValue) {
            return item.label
        }
        return item.label + " · " + item.value
    }

    function isCustomStoredValue(options, value) {
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

    function fallbackWerewolfRoles() {
        return [
            {
                "role_key": "werewolf",
                "display_name": "狼人",
                "description": "夜晚可选择击杀目标；白天隐藏身份、误导投票，狼人阵营胜利条件是人数压制好人阵营。",
                "persona": "你是狼人。发言克制，保护自己，制造合理怀疑。"
            },
            {
                "role_key": "villager",
                "display_name": "村民",
                "description": "没有夜晚技能；白天通过发言、票型和逻辑找出狼人。",
                "persona": "你是村民。基于时间线和发言矛盾做判断。"
            },
            {
                "role_key": "seer",
                "display_name": "预言家",
                "description": "每晚可查验一名玩家的阵营；白天需要在保护身份和公布信息之间取舍。",
                "persona": "你是预言家。谨慎利用查验信息带队。"
            },
            {
                "role_key": "witch",
                "display_name": "女巫",
                "description": "拥有解药和毒药；夜晚可救人或毒杀一名玩家，通常每种药只能使用一次。",
                "persona": "你是女巫。谨慎管理药水并观察狼人刀口。"
            },
            {
                "role_key": "hunter",
                "display_name": "猎人",
                "description": "出局时可开枪带走一名玩家；需要用发言压制狼人并保护好人信息。",
                "persona": "你是猎人。保持威慑，避免被狼人轻易带偏。"
            },
            {
                "role_key": "guard",
                "display_name": "守卫",
                "description": "每晚可守护一名玩家免于出局；通常不能连续守护同一目标。",
                "persona": "你是守卫。根据局势选择保护目标。"
            },
            {
                "role_key": "idiot",
                "display_name": "白痴",
                "description": "被投票出局时可翻牌免死，但之后通常失去投票权，只能继续发言。",
                "persona": "你是白痴。用稳定发言吸收压力并提供判断。"
            }
        ]
    }

    function buildRoleOptions() {
        var rows = [{
            "label": "系统随机",
            "value": "",
            "hint": "开局随机分配身份",
            "rule": "未指定身份时，系统会在开始游戏时随机分配该 AI 的角色。",
            "showValue": false
        }]
        var seen = { "": true }
        var source = root.rolePresets && root.rolePresets.length > 0
            ? root.rolePresets
            : (root.rulesetId === "werewolf.basic" ? root.fallbackWerewolfRoles() : [])
        for (var i = 0; i < source.length; ++i) {
            var item = source[i] || {}
            var key = item.role_key || item.key || ""
            if (key.length === 0 || seen[key]) {
                continue
            }
            rows.push({
                "label": item.display_name || item.name || key,
                "value": key,
                "hint": item.description || item.rule || item.persona || "",
                "rule": item.rule || item.rules || item.description || item.persona || "",
                "showValue": false
            })
            seen[key] = true
        }
        rows.push({
            "label": "自定义身份",
            "value": root.customRoleValue,
            "hint": "输入自定义 role_key",
            "rule": "使用你输入的 role_key 加入房间；后端会按该身份保存和传给 Agent。",
            "showValue": false
        })
        return rows
    }

    function selectedRoleRule() {
        if (!root.showRoleControls) {
            return ""
        }
        var value = root.agentData.role_key || ""
        var index = root.optionIndex(root.roleOptions, value, "")
        if (index < 0 || index >= root.roleOptions.length) {
            return ""
        }
        return root.roleOptions[index].rule || root.roleOptions[index].hint || ""
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
