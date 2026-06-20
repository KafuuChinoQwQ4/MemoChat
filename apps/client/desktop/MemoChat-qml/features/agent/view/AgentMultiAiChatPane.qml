pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"
import "../runtime/AgentMultiAiRuntime.js" as AgentMultiAiRuntime

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
    readonly property var displayEventRows: root.displayEvents()
    readonly property int chatMessageCount: {
        var rows = root.displayEventRows || []
        var count = 0
        for (var i = 0; i < rows.length; ++i) {
            if (root.isChatEvent(rows[i] || {})) {
                ++count
            }
        }
        return count
    }

    function roomObject() {
        return gameState && gameState.room ? gameState.room : ({})
    }

    function openParticipantEditor(participant) {
        participantEditor.openForParticipant(participant)
    }

    function roomId() {
        var room = roomObject()
        return room.room_id || currentGameRoomId || ""
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
            if ((participant.participant_id || "") === participantId) {
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
        return participant.display_name || participantId || "系统"
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
                return participant.participant_id || ""
            }
        }
        for (var j = 0; j < ps.length; ++j) {
            var item = ps[j] || {}
            if ((item.kind || "") === "human") {
                return item.participant_id || ""
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
        return AgentMultiAiRuntime.isChatEvent(event)
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
        return AgentMultiAiRuntime.normalizeEscapedMarkdown(value)
    }

    function textFromParsedPayload(payload) {
        return AgentMultiAiRuntime.textFromParsedPayload(payload)
    }

    function readableEventText(value) {
        return AgentMultiAiRuntime.readableEventText(value)
    }

    function eventBody(event) {
        var body = root.readableEventText(event.content || event.message || "")
        var type = event.event_type || event.type || "event"
        if (type === "participant_joined" && root.selfName.length > 0 && root.isSelfEvent(event)) {
            var participant = participantById(event.actor_participant_id || "")
            var storedName = participant.display_name || ""
            if (storedName.length > 0 && body.indexOf(storedName) === 0) {
                return root.selfName + body.slice(storedName.length)
            }
        }
        return body
    }

    function sendDraft(textValue) {
        var text = (textValue || "").trim()
        var room = roomId()
        var actor = humanParticipantId()
        if (text.length === 0 || room.length === 0 || actor.length === 0 || !agentController) {
            return
        }
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

    AgentMultiAiSessionPanel {
        participants: root.participants()
        selfName: root.selfName
        expanded: root.detailsExpanded
        onParticipantEditRequested: function(participant) { root.openParticipantEditor(participant) }
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
            model: root.displayEventRows
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
            visible: root.chatMessageCount === 0
        }
    }

    AgentMultiAiControls {
        gameBusy: root.gameBusy
        roomReady: root.roomId().length > 0 && root.humanParticipantId().length > 0
        onSendRequested: function(text) { root.sendDraft(text) }
    }

    AgentMultiAiParticipantEditor {
        id: participantEditor
        gameBusy: root.gameBusy
        onSaveRequested: function(participantId, displayName, persona, strategy, skillName) {
            if (!root.agentController || root.roomId().length === 0 || participantId.length === 0) {
                return
            }
            root.agentController.updateGameParticipant(root.roomId(),
                                                       participantId,
                                                       displayName,
                                                       persona,
                                                       strategy,
                                                       skillName)
        }
    }
}
