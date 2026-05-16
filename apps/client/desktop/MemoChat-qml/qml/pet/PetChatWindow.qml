pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import "../chat/conversation"

Window {
    id: root
    width: 320
    height: 540
    minimumWidth: 286
    minimumHeight: 420
    title: "Live2D 聊天"
    flags: chatWindowFlags()
    color: "transparent"
    visible: false

    property var petController: null
    property var agentController: null
    property var petAssetSettings: null
    property bool alwaysOnTop: true
    property bool clickThrough: false
    property string selfAvatar: "qrc:/res/head_1.jpg"
    property string live2dAvatarSource: "qrc:/icons/modelive2d.png"
    property string draftText: ""
    property string chatStatusText: "准备聊天"
    property bool voiceCallActive: false
    property bool videoCallActive: false
    property string pendingSendText: ""
    property bool pendingSendAlreadyAppended: false
    property string pendingAssistantTurnId: ""
    property int pendingAssistantIndex: -1
    property var completedAssistantTurnKeys: ({})
    property var completedAssistantTurnOrder: []
    property string lastSyncedModel: ""
    property string lastSyncedLanguage: ""
    property string lastSyncedSpeechRules: ""

    signal voiceChatRequested(bool active)
    signal videoChatRequested()

    ListModel {
        id: messageModel
    }

    readonly property color accentColor: "#74b2ba"
    readonly property color borderColor: Qt.rgba(1.0, 1.0, 1.0, 0.58)
    readonly property string displayName: root.characterName()
    readonly property string selfName: "我"
    readonly property string live2dAvatarFallback: "qrc:/icons/modelive2d.png"

    function chatWindowFlags() {
        var nextFlags = (Qt.platform.os === "linux" ? Qt.Window : Qt.Tool)
                | Qt.FramelessWindowHint
        if (root.alwaysOnTop) {
            nextFlags |= Qt.WindowStaysOnTopHint
        }
        return nextFlags
    }

    function applyWindowFlags() {
        flags = chatWindowFlags()
    }

    function characterName() {
        if (!root.petAssetSettings) {
            return "Live2D"
        }
        const name = root.petAssetSettings.characterName
        return name && name.length > 0 ? name : "Live2D"
    }

    function isOutgoingMessage(value) {
        return value === true || value === "true" || value === 1
    }

    function stringValue(value) {
        if (value === undefined || value === null) {
            return ""
        }
        return String(value)
    }

    function hasText(value) {
        return root.stringValue(value).trim().length > 0
    }

    function effectiveSelfAvatar() {
        return root.selfAvatar && root.selfAvatar.length > 0 ? root.selfAvatar : "qrc:/res/head_1.jpg"
    }

    function effectiveLive2DAvatar() {
        return root.live2dAvatarSource && root.live2dAvatarSource.length > 0
                ? root.live2dAvatarSource
                : root.live2dAvatarFallback
    }

    function avatarForMessage(outgoing) {
        return outgoing ? root.effectiveSelfAvatar() : root.effectiveLive2DAvatar()
    }

    function defaultAvatarForMessage(outgoing) {
        return outgoing ? "qrc:/res/head_1.jpg" : root.live2dAvatarFallback
    }

    function messageSenderName(isOutgoing) {
        return isOutgoing ? root.selfName : root.displayName
    }

    function newMessage(outgoing, text, turnId, state, audioUrl, audioState, translation) {
        const isOutgoing = root.isOutgoingMessage(outgoing)
        return {
            "outgoing": isOutgoing,
            "turnId": turnId || "",
            "msgId": (isOutgoing ? "pet-user-" : "pet-ai-")
                     + Date.now() + "-" + Math.random().toString(16).slice(2),
            "msgType": "text",
            "content": text || "",
            "rawContent": text || "",
            "translationText": translation || "",
            "fileName": "",
            "senderName": root.messageSenderName(isOutgoing),
            "senderUid": 0,
            "senderIcon": "",
            "showAvatar": true,
            "showTimeDivider": false,
            "timeDividerText": "",
            "messageState": state || "sent",
            "isReply": false,
            "replyToMsgId": "",
            "replySender": "",
            "replyPreview": "",
            "audioUrl": audioUrl || "",
            "audioState": audioState || "idle"
        }
    }

    function assistantEventKey(event, turnId) {
        if (turnId.length > 0) {
            return turnId
        }
        if (root.pendingAssistantTurnId.length > 0) {
            return root.pendingAssistantTurnId
        }
        const eventId = event && event.event_id ? String(event.event_id).trim() : ""
        if (eventId.length > 0) {
            return eventId
        }
        if (event && event.seq !== undefined && event.seq !== null) {
            const seq = String(event.seq).trim()
            if (seq.length > 0) {
                return seq
            }
        }
        return ""
    }

    function assistantTurnCompleted(turnKey) {
        return turnKey.length > 0 && root.completedAssistantTurnKeys[turnKey] === true
    }

    function rememberCompletedAssistantTurn(turnKey) {
        if (turnKey.length === 0 || root.completedAssistantTurnKeys[turnKey] === true) {
            return
        }
        const nextKeys = root.completedAssistantTurnKeys
        nextKeys[turnKey] = true
        root.completedAssistantTurnKeys = nextKeys
        const nextOrder = root.completedAssistantTurnOrder.slice()
        nextOrder.push(turnKey)
        while (nextOrder.length > 128) {
            const oldKey = nextOrder.shift()
            delete nextKeys[oldKey]
        }
        root.completedAssistantTurnOrder = nextOrder
    }

    function refreshLive2DAvatar() {
        var nextAvatar = ""
        if (root.petAssetSettings && root.petAssetSettings.resolveLive2DAvatarUrl) {
            nextAvatar = root.petAssetSettings.resolveLive2DAvatarUrl(root.petAssetSettings.modelJson,
                                                                      root.petAssetSettings.modelRoot)
        }
        root.live2dAvatarSource = nextAvatar && nextAvatar.length > 0 ? nextAvatar : root.live2dAvatarFallback
    }

    function languageCode() {
        if (!root.petAssetSettings) {
            return "zh-CN"
        }
        const index = root.petAssetSettings.languageIndex !== undefined ? root.petAssetSettings.languageIndex : 0
        switch (index) {
        case 1:
            return "ja-JP"
        case 2:
            return "en-US"
        case 3:
            return "ko-KR"
        case 4:
            return "fr-FR"
        case 5:
            return "es-ES"
        default:
            return "zh-CN"
        }
    }

    function speechRulesText() {
        if (!root.petAssetSettings || root.petAssetSettings.speechRules === undefined) {
            return ""
        }
        return root.stringValue(root.petAssetSettings.speechRules).trim()
    }

    function currentModelParts() {
        if (!root.agentController || !root.agentController.currentModel) {
            return { "model_type": "", "model_name": "" }
        }
        const raw = root.agentController.currentModel
        const index = raw.indexOf(":")
        if (index < 0) {
            return { "model_type": raw, "model_name": "" }
        }
        return {
            "model_type": raw.slice(0, index),
            "model_name": raw.slice(index + 1)
        }
    }

    function syncModelSelection() {
        if (!root.petController) {
            return
        }
        const model = currentModelParts()
        const nextModel = (model.model_type || "") + ":" + (model.model_name || "")
        if (root.lastSyncedModel !== nextModel) {
            root.lastSyncedModel = nextModel
            if (root.petController.setModelSelection) {
                root.petController.setModelSelection(model.model_type || "", model.model_name || "")
            }
        }
    }

    function syncReplyLanguage() {
        if (!root.petController) {
            return
        }
        const nextLanguage = languageCode()
        if (root.lastSyncedLanguage !== nextLanguage) {
            root.lastSyncedLanguage = nextLanguage
            if (root.petController.setReplyLanguage) {
                root.petController.setReplyLanguage(nextLanguage)
            }
        }
    }

    function syncSpeechRules() {
        if (!root.petController) {
            return
        }
        const nextRules = speechRulesText()
        if (root.lastSyncedSpeechRules !== nextRules) {
            root.lastSyncedSpeechRules = nextRules
            if (root.petController.setSpeechRules) {
                root.petController.setSpeechRules(nextRules)
            }
        }
    }

    function syncContext() {
        syncModelSelection()
        syncReplyLanguage()
        syncSpeechRules()
    }

    function openChat() {
        applyWindowFlags()
        refreshLive2DAvatar()
        show()
        raise()
        requestActivate()
        syncContext()
        if (messageInput) {
            messageInput.forceActiveFocus()
        }
    }

    function appendUserMessage(text) {
        if (!text || text.length === 0) {
            return
        }
        messageModel.append(root.newMessage(true, text, "", "sent", "", "idle", ""))
        messageList.positionViewAtEnd()
    }

    function appendOrUpdateAssistantMessage(event) {
        if (!event || event.type !== "pet.control") {
            return
        }
        const text = event.text || {}
        const speech = event.speech || {}
        const audio = event.audio || {}
        const controllerText = root.petController ? root.stringValue(root.petController.speechDisplayText) : ""
        const controllerTranslation = root.petController ? root.stringValue(root.petController.speechTranslation) : ""
        const controllerAudioUrl = root.petController ? root.stringValue(root.petController.audioUrl) : ""
        const controllerAudioState = root.petController ? root.stringValue(root.petController.audioState) : ""
        const controllerTurnId = root.petController ? root.stringValue(root.petController.turnId).trim() : ""
        const delta = root.stringValue(text.delta || speech.text_delta || event.speech_text)
        const eventTranslation = root.stringValue(text.translation || event.speech_translation || speech.translation)
        const eventDisplay = root.stringValue(text.display || event.speech_display_text || event.speech_text || delta)
        const errorText = root.stringValue(text.error || speech.error || event.error || event.message)
        const phase = root.stringValue(event.phase).trim()
        var display = root.hasText(controllerText) ? controllerText : eventDisplay
        var translation = root.hasText(controllerTranslation) ? controllerTranslation : eventTranslation
        if (!root.hasText(display) && phase === "error") {
            display = root.hasText(errorText) ? errorText : "回复异常"
        }
        const turnId = root.stringValue(event.turn_id).trim()
        const eventKey = root.assistantEventKey(event, turnId)
        const isFinal = !!text.final || (!!root.petController && root.petController.speechFinal)
        const audioUrl = root.hasText(controllerAudioUrl) ? controllerAudioUrl : root.stringValue(audio.url)
        const audioState = root.hasText(controllerAudioState) ? controllerAudioState : root.stringValue(audio.state || "idle")
        const hasContent = root.hasText(display)
                           || root.hasText(translation)
                           || phase === "error"
                           || (isFinal && root.pendingAssistantIndex >= 0)
        if (!hasContent) {
            return
        }

        const resolvedEventKey = eventKey.length > 0 ? eventKey : controllerTurnId
        if (resolvedEventKey.length === 0) {
            return
        }
        if (root.assistantTurnCompleted(resolvedEventKey)) {
            return
        }
        if (root.pendingAssistantIndex < 0 || root.pendingAssistantTurnId !== resolvedEventKey) {
            messageModel.append(root.newMessage(false,
                                                display,
                                                resolvedEventKey,
                                                isFinal ? "sent" : "sending",
                                                audioUrl,
                                                audioState,
                                                translation))
            root.pendingAssistantIndex = messageModel.count - 1
            root.pendingAssistantTurnId = resolvedEventKey
        } else if (root.pendingAssistantIndex >= 0) {
            const current = messageModel.get(root.pendingAssistantIndex) || {}
            const nextDisplay = root.hasText(display) ? display : root.stringValue(current.content)
            messageModel.setProperty(root.pendingAssistantIndex, "content", nextDisplay)
            messageModel.setProperty(root.pendingAssistantIndex, "rawContent", nextDisplay)
            messageModel.setProperty(root.pendingAssistantIndex, "translationText", translation)
            messageModel.setProperty(root.pendingAssistantIndex, "audioUrl", audioUrl)
            messageModel.setProperty(root.pendingAssistantIndex, "audioState", audioState)
            messageModel.setProperty(root.pendingAssistantIndex, "messageState", isFinal ? "sent" : "sending")
        }

        if (root.pendingAssistantIndex >= 0) {
            messageList.positionViewAtEnd()
        }
        if (isFinal) {
            root.rememberCompletedAssistantTurn(resolvedEventKey)
            root.pendingAssistantIndex = -1
            root.pendingAssistantTurnId = ""
            root.chatStatusText = audioUrl.length > 0
                    ? "语音回复已生成"
                    : (root.voiceCallActive ? "语音未生成，已显示文字" : "文字回复已生成")
        } else if (phase === "speaking") {
            root.chatStatusText = "正在回复"
        } else if (phase === "error") {
            root.chatStatusText = "回复异常"
        }
    }

    function sendPendingText() {
        if (!root.petController || root.pendingSendText.length === 0 || root.petController.busy) {
            return
        }
        const text = root.pendingSendText
        root.pendingSendText = ""
        if (!root.pendingSendAlreadyAppended) {
            root.appendUserMessage(text)
        }
        root.pendingSendAlreadyAppended = false
        root.chatStatusText = "正在发送"
        root.syncContext()
        root.petController.sendText(text)
        messageInput.clear()
        messageInput.forceActiveFocus()
    }

    function sendMessage(text) {
        const trimmed = (text || "").trim()
        if (trimmed.length === 0) {
            return
        }
        if (!root.petController) {
            return
        }
        if (root.petController.sessionId.length === 0) {
            root.pendingSendText = trimmed
            root.pendingSendAlreadyAppended = true
            root.chatStatusText = "正在连接桌宠"
            root.appendUserMessage(trimmed)
            root.petController.startSession()
            messageInput.clear()
            messageInput.forceActiveFocus()
            return
        }
        root.pendingSendAlreadyAppended = false
        root.appendUserMessage(trimmed)
        root.chatStatusText = "正在发送"
        root.syncContext()
        root.petController.sendText(trimmed)
        messageInput.clear()
        messageInput.forceActiveFocus()
    }

    function toggleVoiceCall() {
        root.voiceCallActive = !root.voiceCallActive
        root.chatStatusText = root.voiceCallActive ? "语音回复已开启" : "语音回复已关闭"
        root.voiceChatRequested(root.voiceCallActive)
    }

    function toggleVideoCall() {
        root.videoCallActive = !root.videoCallActive
        root.chatStatusText = root.videoCallActive ? "视频聊天已开启" : "视频聊天已关闭"
        root.videoChatRequested()
    }

    Item {
        anchors.fill: parent
        anchors.margins: 2

        Rectangle {
            anchors.fill: parent
            anchors.topMargin: 5
            anchors.leftMargin: 3
            anchors.rightMargin: 3
            anchors.bottomMargin: 1
            radius: 20
            antialiasing: true
            color: Qt.rgba(0.30, 0.37, 0.48, 0.18)
        }

        Rectangle {
        id: chatPanel
        anchors.fill: parent
            anchors.margins: 6
            radius: 18
        antialiasing: true
            color: Qt.rgba(0.95, 0.975, 1.0, 0.78)
        border.color: root.borderColor

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            antialiasing: true
                color: "transparent"
                border.color: Qt.rgba(0.52, 0.67, 0.82, 0.16)
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 112
                radius: parent.radius
                antialiasing: true
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.72) }
                    GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0.05) }
                }
            }

            MouseArea {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 54
                acceptedButtons: Qt.LeftButton
                onPressed: root.startSystemMove()
        }

        ColumnLayout {
            anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    radius: 12
                    antialiasing: true
                    color: Qt.rgba(1, 1, 1, 0.30)
                    border.color: Qt.rgba(1, 1, 1, 0.50)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 8
                        spacing: 8

                        Rectangle {
                            Layout.preferredWidth: 26
                            Layout.preferredHeight: 26
                            radius: 13
                            antialiasing: true
                            clip: true
                            color: Qt.rgba(0.74, 0.86, 0.95, 0.72)
                            border.color: Qt.rgba(1, 1, 1, 0.66)

                            Image {
                                anchors.fill: parent
                                anchors.margins: 2
                                source: root.live2dAvatarSource
                                fillMode: Image.PreserveAspectCrop
                                mipmap: true
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 0

                            Label {
                                Layout.fillWidth: true
                                text: root.displayName
                                color: "#26364d"
                                font.pixelSize: 14
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.chatStatusText
                                color: "#6a7b92"
                                font.pixelSize: 10
                                elide: Text.ElideRight
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 8
                            Layout.preferredHeight: 8
                            Layout.alignment: Qt.AlignVCenter
                            radius: 4
                            antialiasing: true
                            color: root.voiceCallActive || root.videoCallActive ? root.accentColor : "#8fc7d1"
                        }

                        ToolButton {
                            id: closeButton
                            Layout.preferredWidth: 28
                            Layout.preferredHeight: 28
                            hoverEnabled: true
                            padding: 0
                            onClicked: root.hide()

                            contentItem: Text {
                                text: "x"
                                color: "#536174"
                                font.pixelSize: 13
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: 8
                                antialiasing: true
                                color: closeButton.down ? Qt.rgba(0.36, 0.52, 0.72, 0.22)
                                                        : closeButton.hovered ? Qt.rgba(0.36, 0.52, 0.72, 0.14)
                                                                              : "transparent"
                            }
                        }
                    }
                }

                Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 12
                antialiasing: true
                    color: Qt.rgba(1, 1, 1, 0.24)
                    border.color: Qt.rgba(1, 1, 1, 0.50)

                ListView {
                    id: messageList
                    anchors.fill: parent
                        anchors.margins: 10
                    clip: true
                        spacing: 0
                    model: messageModel
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: Item {
                        id: messageDelegateRoot
                        required property var outgoing
                        required property string msgId
                        required property string msgType
                        required property string content
                        required property string rawContent
                        required property string translationText
                        required property string fileName
                        required property string senderName
                        required property int senderUid
                        required property bool showAvatar
                        required property bool showTimeDivider
                        required property string timeDividerText
                        required property string messageState
                        required property bool isReply
                        required property string replyToMsgId
                        required property string replySender
                        required property string replyPreview

                        readonly property bool outgoingMessage: root.isOutgoingMessage(outgoing)

                        width: messageList.width
                        height: messageDelegate.height

                        ChatMessageDelegate {
                            id: messageDelegate
                            width: messageDelegateRoot.width
                            outgoing: messageDelegateRoot.outgoingMessage
                            msgId: messageDelegateRoot.msgId
                            senderUid: messageDelegateRoot.senderUid
                            msgType: messageDelegateRoot.msgType
                            content: messageDelegateRoot.content
                            rawContent: messageDelegateRoot.rawContent
                            translationText: messageDelegateRoot.translationText
                            fileName: messageDelegateRoot.fileName
                            senderName: messageDelegateRoot.senderName.length > 0
                                        ? messageDelegateRoot.senderName
                                        : root.messageSenderName(messageDelegateRoot.outgoingMessage)
                            showOutgoingSenderName: true
                            showAvatar: messageDelegateRoot.showAvatar
                            showTimeDivider: messageDelegateRoot.showTimeDivider
                            timeDividerText: messageDelegateRoot.timeDividerText
                            messageState: messageDelegateRoot.messageState
                            isReply: messageDelegateRoot.isReply
                            replyToMsgId: messageDelegateRoot.replyToMsgId
                            replySender: messageDelegateRoot.replySender
                            replyPreview: messageDelegateRoot.replyPreview
                            avatarSource: root.avatarForMessage(messageDelegateRoot.outgoingMessage)
                            defaultAvatarSource: root.defaultAvatarForMessage(messageDelegateRoot.outgoingMessage)
                        }
                    }

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }

                    onCountChanged: {
                        if (count > 0) {
                            positionViewAtEnd()
                        }
                    }

                        Rectangle {
                            anchors.centerIn: parent
                            width: Math.min(parent.width - 36, 190)
                            height: 74
                            radius: 12
                            antialiasing: true
                            visible: messageList.count === 0
                            color: Qt.rgba(1, 1, 1, 0.34)
                            border.color: Qt.rgba(1, 1, 1, 0.48)

                            Text {
                                anchors.centerIn: parent
                                width: parent.width - 20
                                text: "和 " + root.displayName + " 说点什么"
                                color: "#6a7b92"
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                            }
                        }
                }
            }

                Rectangle {
                Layout.fillWidth: true
                    Layout.preferredHeight: 94
                    radius: 12
                    antialiasing: true
                    color: Qt.rgba(1, 1, 1, 0.30)
                    border.color: Qt.rgba(1, 1, 1, 0.48)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            CompactIconButton {
                                iconSource: "qrc:/icons/emoji.png"
                                enabled: !!root.petController && !root.petController.busy
                                onClicked: {
                                    messageInput.insert(messageInput.cursorPosition, "😀")
                                    root.draftText = messageInput.text
                                }
                            }

                            CompactIconButton {
                                iconSource: "qrc:/icons/phone.png"
                                checked: root.voiceCallActive
                                enabled: !!root.petController && !root.petController.busy
                                onClicked: root.toggleVoiceCall()
                            }

                            CompactIconButton {
                                iconSource: "qrc:/icons/video.png"
                                checked: root.videoCallActive
                                enabled: !!root.petController && !root.petController.busy
                                onClicked: root.toggleVideoCall()
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.lastSyncedModel.length > 0 ? root.lastSyncedModel : "AI API"
                                color: "#7a8798"
                                font.pixelSize: 10
                                horizontalAlignment: Text.AlignRight
                                elide: Text.ElideLeft
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 8

                            TextArea {
                                id: messageInput
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                placeholderText: "输入消息..."
                                enabled: !!root.petController && !root.petController.busy
                                text: root.draftText
                                color: "#253247"
                                placeholderTextColor: "#7f8b9b"
                                selectionColor: "#7baee899"
                                selectedTextColor: "#ffffff"
                                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                                font.pixelSize: 13
                                inputMethodHints: Qt.ImhMultiLine
                                background: Item { }
                                Keys.onReturnPressed: function(event) {
                                    if (!(event.modifiers & Qt.ShiftModifier)) {
                                        root.sendMessage(messageInput.text)
                                        event.accepted = true
                                    }
                                }
                                Keys.onEnterPressed: function(event) {
                                    if (!(event.modifiers & Qt.ShiftModifier)) {
                                        root.sendMessage(messageInput.text)
                                        event.accepted = true
                                    }
                                }
                                onTextChanged: {
                                    if (root.draftText !== text) {
                                        root.draftText = text
                                    }
                                }
                            }

                            Button {
                                id: sendButton
                                Layout.alignment: Qt.AlignBottom
                                Layout.preferredWidth: 64
                                Layout.preferredHeight: 32
                                enabled: !!root.petController
                                         && !root.petController.busy
                                         && messageInput.text.trim().length > 0
                                text: "发送"
                                padding: 0
                                onClicked: root.sendMessage(messageInput.text)

                                background: Rectangle {
                                    radius: 9
                                    antialiasing: true
                                    color: !sendButton.enabled ? Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                                               : sendButton.down ? Qt.rgba(0.35, 0.61, 0.90, 0.40)
                                                                                 : sendButton.hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.30)
                                                                                                      : Qt.rgba(0.35, 0.61, 0.90, 0.22)
                                }

                                contentItem: Text {
                                    text: sendButton.text
                                    color: sendButton.enabled ? "#38547b" : "#98a1ae"
                                    font.pixelSize: 12
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: root.petController

        function onStateChanged() {
            if (!root.petController) {
                return
            }
            if (root.pendingSendText.length > 0 && root.petController.sessionId.length > 0 && !root.petController.busy) {
                root.sendPendingText()
            }
            root.syncContext()
        }

        function onControlEventReceived(event) {
            root.appendOrUpdateAssistantMessage(event)
        }

        function onWindowsImeBridgeChanged() {
            if (!root.petController.windowsImeBridgeBusy && messageInput) {
                messageInput.forceActiveFocus()
            }
        }
    }

    Connections {
        target: root.agentController
        function onModelChanged() {
            root.syncModelSelection()
        }
    }

    Connections {
        target: root.petAssetSettings
        ignoreUnknownSignals: true
        function onSettingsChanged() {
            root.refreshLive2DAvatar()
            root.syncContext()
        }
    }

    onVisibleChanged: {
        if (visible) {
            applyWindowFlags()
            refreshLive2DAvatar()
            syncContext()
            if (messageInput) {
                messageInput.forceActiveFocus()
            }
        }
    }

    onPetAssetSettingsChanged: refreshLive2DAvatar()

    onAlwaysOnTopChanged: applyWindowFlags()

    Component.onCompleted: {
        refreshLive2DAvatar()
        syncContext()
    }

    component CompactIconButton: ToolButton {
        id: iconButton
        property string iconSource: ""

        Layout.preferredWidth: 26
        Layout.preferredHeight: 24
        hoverEnabled: true
        padding: 0

        contentItem: Image {
            source: iconButton.iconSource
            fillMode: Image.PreserveAspectFit
            sourceSize.width: 15
            sourceSize.height: 15
            opacity: iconButton.enabled ? 0.90 : 0.36
            mipmap: true
        }

        background: Rectangle {
            radius: 7
            antialiasing: true
            color: !iconButton.enabled ? Qt.rgba(1, 1, 1, 0.08)
                                      : iconButton.down ? Qt.rgba(0.35, 0.61, 0.90, 0.30)
                                                        : iconButton.checked ? Qt.rgba(0.35, 0.61, 0.90, 0.24)
                                                                             : iconButton.hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.16)
                                                                                                  : "transparent"
        }
    }
}
