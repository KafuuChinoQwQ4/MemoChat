pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

ColumnLayout {
    id: root

    property var agentController: null
    property var gameState: ({})
    property string currentGameRoomId: ""
    property bool gameBusy: false
    property string selfName: ""
    property string selfAvatar: "qrc:/res/head_1.jpg"
    property bool stickToBottom: true
    property bool detailsExpanded: false
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

    function roomObject() {
        return gameState && gameState.room ? gameState.room : ({})
    }

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

    function openParticipantEditor(participant) {
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
        if (!root.agentController || root.roomId().length === 0 || participantId.length === 0) {
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
        root.agentController.updateGameParticipant(root.roomId(),
                                                   participantId,
                                                   editorNameField.text,
                                                   editorPersonaField.text,
                                                   strategy,
                                                   skillName)
        agentEditorPopup.close()
    }

    function comboBackground(combo) {
        return combo.pressed ? Qt.rgba(0.88, 0.93, 1.0, 0.72)
                             : combo.hovered ? Qt.rgba(1, 1, 1, 0.62)
                                             : Qt.rgba(1, 1, 1, 0.46)
    }

    function roomId() {
        var room = roomObject()
        return room.room_id || room.id || currentGameRoomId || ""
    }

    function participants() {
        return gameState && gameState.participants ? gameState.participants : []
    }

    function events() {
        return gameState && gameState.events ? gameState.events : []
    }

    function displayEvents() {
        var rows = []
        var source = events()
        for (var i = 0; i < source.length; ++i) {
            var event = source[i] || {}
            if ((event.content || "").length === 0 && (event.event_type || "") !== "participant_joined") {
                continue
            }
            rows.push(event)
        }
        return rows
    }

    function participantById(participantId) {
        var ps = participants()
        for (var i = 0; i < ps.length; ++i) {
            var participant = ps[i] || {}
            if ((participant.participant_id || participant.id || "") === participantId) {
                return participant
            }
        }
        return ({})
    }

    function participantName(participantId) {
        var participant = participantById(participantId)
        if ((participant.kind || "") === "self" && root.selfName.length > 0) {
            return root.selfName
        }
        return participant.display_name || participant.name || participantId || "系统"
    }

    function participantKind(participantId) {
        var participant = participantById(participantId)
        return participant.kind || participant.type || ""
    }

    function humanParticipantId() {
        var ps = participants()
        for (var i = 0; i < ps.length; ++i) {
            var participant = ps[i] || {}
            if ((participant.kind || "") === "self") {
                return participant.participant_id || participant.id || ""
            }
        }
        for (var j = 0; j < ps.length; ++j) {
            var item = ps[j] || {}
            if ((item.kind || "") === "human") {
                return item.participant_id || item.id || ""
            }
        }
        return ""
    }

    function isSelfEvent(event) {
        var actorId = event.actor_participant_id || ""
        var kind = participantKind(actorId)
        return kind === "self" || actorId === humanParticipantId()
    }

    function isChatEvent(event) {
        return (event.event_type || event.type || "") === "speak"
    }

    function eventLabel(event) {
        var type = event.event_type || event.type || "event"
        if (type === "room_created") return "房间已创建"
        if (type === "participant_joined") return "成员加入"
        if (type === "agent_error") return "AI 回复异常"
        if (type === "skip") return root.participantName(event.actor_participant_id || "")
        if (type === "host") return "提示"
        return root.participantName(event.actor_participant_id || "")
    }

    function normalizeEscapedMarkdown(value) {
        var text = value || ""
        if (text.indexOf("\\n```") >= 0 || text.indexOf("\\n~~~") >= 0) {
            return text.replace(/\\r\\n/g, "\n").replace(/\\n/g, "\n").replace(/\\t/g, "\t")
        }
        return text
    }

    function textFromParsedPayload(payload) {
        if (!payload || typeof payload !== "object") {
            return ""
        }
        var keys = ["content", "message", "text", "reply"]
        for (var i = 0; i < keys.length; ++i) {
            var value = payload[keys[i]]
            if (typeof value === "string" && value.length > 0) {
                return root.normalizeEscapedMarkdown(value)
            }
        }
        if (payload.payload && typeof payload.payload === "object") {
            return root.textFromParsedPayload(payload.payload)
        }
        return ""
    }

    function readableEventText(value) {
        if (value === undefined || value === null) {
            return ""
        }
        if (typeof value !== "string") {
            var parsedText = root.textFromParsedPayload(value)
            return parsedText.length > 0 ? parsedText : JSON.stringify(value)
        }
        var text = value
        var trimmed = text.trim()
        if (trimmed.length > 0 && (trimmed.charAt(0) === "{" || trimmed.charAt(0) === "[")) {
            try {
                var parsed = JSON.parse(trimmed)
                var extracted = root.textFromParsedPayload(parsed)
                if (extracted.length > 0) {
                    return extracted
                }
            } catch (error) {
                // Keep the original text when it is not a complete JSON payload.
            }
        }
        return root.normalizeEscapedMarkdown(text)
    }

    function eventBody(event) {
        var body = root.readableEventText(event.content || event.message || "")
        var type = event.event_type || event.type || "event"
        if (type === "participant_joined" && root.selfName.length > 0 && root.isSelfEvent(event)) {
            var participant = participantById(event.actor_participant_id || "")
            var storedName = participant.display_name || participant.name || ""
            if (storedName.length > 0 && body.indexOf(storedName) === 0) {
                return root.selfName + body.slice(storedName.length)
            }
        }
        return body
    }

    function sendDraft() {
        var text = composerField.text.trim()
        var room = roomId()
        var actor = humanParticipantId()
        if (text.length === 0 || room.length === 0 || actor.length === 0 || !agentController) {
            return
        }
        composerField.text = ""
        agentController.submitGameAction(room, actor, "speak", "", text)
    }

    function isNearBottom() {
        return messageListView.contentY + messageListView.height >= messageListView.contentHeight - 36
    }

    function scrollToEndSoon() {
        Qt.callLater(function() {
            if (messageListView.count > 0) {
                messageListView.forceLayout()
                messageListView.positionViewAtEnd()
            }
        })
    }

    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 10

    onGameStateChanged: {
        root.stickToBottom = true
        root.scrollToEndSoon()
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: root.detailsExpanded ? 78 : 0
        visible: root.detailsExpanded
        radius: 12
        color: Qt.rgba(1, 1, 1, 0.16)
        border.color: Qt.rgba(1, 1, 1, 0.36)
        clip: true

        ListView {
            anchors.fill: parent
            anchors.margins: 10
            orientation: ListView.Horizontal
            spacing: 8
            model: root.participants()
            clip: true

            delegate: Rectangle {
                id: participantDelegate
                required property var modelData
                readonly property bool editableAgent: (modelData.kind || "") === "agent"

                width: 148
                height: ListView.view.height
                radius: 8
                color: participantMouse.pressed && editableAgent
                       ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                       : participantMouse.containsMouse && editableAgent
                         ? Qt.rgba(1, 1, 1, 0.48)
                         : Qt.rgba(1, 1, 1, 0.34)
                border.color: participantMouse.containsMouse && editableAgent
                              ? Qt.rgba(0.35, 0.61, 0.90, 0.42)
                              : Qt.rgba(1, 1, 1, 0.34)

                MouseArea {
                    id: participantMouse
                    anchors.fill: parent
                    hoverEnabled: participantDelegate.editableAgent
                    enabled: participantDelegate.editableAgent
                    cursorShape: participantDelegate.editableAgent ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: root.openParticipantEditor(participantDelegate.modelData)
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: (participantDelegate.modelData.kind || "") === "self" && root.selfName.length > 0
                              ? root.selfName
                              : (participantDelegate.modelData.display_name || participantDelegate.modelData.name || "成员")
                        color: "#263448"
                        font.pixelSize: 12
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: (participantDelegate.modelData.kind || "agent") === "self" ? "你" : ((participantDelegate.modelData.kind || "agent") === "agent" ? "AI" : "成员")
                        color: "#6a7b92"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        radius: 12
        color: Qt.rgba(1, 1, 1, 0.16)
        border.color: Qt.rgba(1, 1, 1, 0.42)
        clip: true

        ListView {
            id: messageListView
            anchors.fill: parent
            anchors.margins: 12
            clip: true
            spacing: 6
            model: root.displayEvents()
            ScrollBar.vertical: GlassScrollBar { }

            delegate: Item {
                id: eventDelegate
                required property var modelData

                readonly property bool chatEvent: root.isChatEvent(modelData)
                readonly property bool selfEvent: root.isSelfEvent(modelData)
                readonly property bool systemEvent: !chatEvent
                readonly property string actorName: root.eventLabel(modelData)
                readonly property string bodyText: root.eventBody(modelData)
                readonly property real bubbleLimit: Math.max(160, Math.min(560, width * 0.72))
                readonly property bool bodyHasCodeBlock: bodyText.indexOf("```") >= 0 || bodyText.indexOf("~~~") >= 0
                readonly property real bodyContentWidth: Math.max(
                    40,
                    Math.min(bubbleLimit - 20,
                             bodyHasCodeBlock ? bubbleLimit - 20 : bodyMeasure.implicitWidth))

                width: ListView.view.width
                height: systemEvent ? 34 : Math.max(52, bubbleColumn.implicitHeight + 18)

                TextEdit {
                    id: bodyMeasure
                    visible: false
                    text: eventDelegate.bodyText
                    font.pixelSize: 14
                    textFormat: Text.PlainText
                    wrapMode: TextEdit.NoWrap
                    readOnly: true
                }

                Rectangle {
                    id: systemChip
                    visible: eventDelegate.systemEvent
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(parent.width - 24, systemText.implicitWidth + 22)
                    height: 24
                    radius: 12
                    color: Qt.rgba(1, 1, 1, 0.34)
                    border.color: Qt.rgba(1, 1, 1, 0.34)

                    Label {
                        id: systemText
                        anchors.centerIn: parent
                        width: parent.width - 16
                        text: eventDelegate.actorName + (eventDelegate.bodyText.length > 0 ? (" · " + eventDelegate.bodyText) : "")
                        color: "#6a7b92"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                }

                Rectangle {
                    id: avatar
                    visible: eventDelegate.chatEvent
                    width: 34
                    height: 34
                    radius: 17
                    anchors.top: parent.top
                    anchors.left: eventDelegate.selfEvent ? undefined : parent.left
                    anchors.right: eventDelegate.selfEvent ? parent.right : undefined
                    color: eventDelegate.selfEvent ? Qt.rgba(0.62, 0.80, 1.0, 0.54) : Qt.rgba(0.73, 0.82, 0.92, 0.44)
                    border.color: Qt.rgba(1, 1, 1, 0.50)
                    clip: true

                    Image {
                        anchors.fill: parent
                        source: eventDelegate.selfEvent ? root.selfAvatar : "qrc:/res/ai_icon.png"
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                    }
                }

                Rectangle {
                    id: bubble
                    visible: eventDelegate.chatEvent
                    width: Math.min(eventDelegate.bubbleLimit, Math.max(86, bubbleColumn.implicitWidth + 20))
                    height: bubbleColumn.implicitHeight + 14
                    radius: 10
                    anchors.top: parent.top
                    anchors.left: eventDelegate.selfEvent ? undefined : parent.left
                    anchors.leftMargin: eventDelegate.selfEvent ? 0 : 44
                    anchors.right: eventDelegate.selfEvent ? parent.right : undefined
                    anchors.rightMargin: eventDelegate.selfEvent ? 44 : 0
                    color: eventDelegate.selfEvent ? Qt.rgba(0.62, 0.80, 1.0, 0.52) : Qt.rgba(1, 1, 1, 0.46)
                    border.color: eventDelegate.selfEvent ? Qt.rgba(0.44, 0.67, 0.95, 0.72) : Qt.rgba(1, 1, 1, 0.58)

                    ColumnLayout {
                        id: bubbleColumn
                        anchors.fill: parent
                        anchors.margins: 7
                        spacing: 4

                        Label {
                            Layout.fillWidth: true
                            text: eventDelegate.actorName
                            color: eventDelegate.selfEvent ? "#36567e" : "#6a7b92"
                            font.pixelSize: 11
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        AgentMarkdownText {
                            Layout.preferredWidth: eventDelegate.bodyContentWidth
                            text: eventDelegate.bodyText
                            textColor: eventDelegate.selfEvent ? "#20334f" : "#253247"
                            textPixelSize: 14
                            codePixelSize: 12
                            maxCodeBlockHeight: 360
                        }
                    }
                }
            }

            onCountChanged: {
                root.stickToBottom = true
                root.scrollToEndSoon()
            }
            onContentHeightChanged: {
                if (root.stickToBottom || root.gameBusy) {
                    root.scrollToEndSoon()
                }
            }
            onMovementEnded: root.stickToBottom = root.isNearBottom()
            onFlickEnded: root.stickToBottom = root.isNearBottom()
        }

        Label {
            anchors.centerIn: parent
            width: parent.width - 36
            text: "发送一条消息后，AI 会在这里依次回复。"
            color: "#6a7b92"
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            visible: messageListView.count === 0
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 112
        radius: 12
        color: Qt.rgba(1, 1, 1, 0.20)
        border.color: Qt.rgba(1, 1, 1, 0.44)

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            TextArea {
                id: composerField
                Layout.fillWidth: true
                Layout.fillHeight: true
                placeholderText: root.gameBusy ? "AI 正在回复..." : "输入要发给多个 AI 的消息"
                wrapMode: TextArea.Wrap
                selectByMouse: true
                enabled: !root.gameBusy && root.roomId().length > 0 && root.humanParticipantId().length > 0
                Keys.onPressed: function(event) {
                    if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && (event.modifiers & Qt.ControlModifier)) {
                        root.sendDraft()
                        event.accepted = true
                    }
                }
            }

            GlassButton {
                Layout.preferredWidth: 88
                Layout.fillHeight: true
                text: root.gameBusy ? "等待" : "发送"
                textPixelSize: 13
                cornerRadius: 10
                enabled: !root.gameBusy && composerField.text.trim().length > 0 && root.roomId().length > 0 && root.humanParticipantId().length > 0
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                onClicked: root.sendDraft()
            }
        }
    }

    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: 0
        Layout.minimumHeight: 0

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
}
