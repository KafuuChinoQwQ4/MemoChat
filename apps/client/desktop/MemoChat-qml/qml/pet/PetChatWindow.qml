pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import "PetChatRuntime.js" as PetChatRuntime

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
    readonly property var petSettingsSignalTarget: root.petAssetSettings
                                                   && root.petAssetSettings.settingsChanged !== undefined
                                                   ? root.petAssetSettings : null
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
    property string lastControllerError: ""

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
        return PetChatRuntime.isOutgoingMessage(value)
    }

    function stringValue(value) {
        return PetChatRuntime.stringValue(value)
    }

    function hasText(value) {
        return PetChatRuntime.hasText(value)
    }

    function newMessage(outgoing, text, turnId, state, audioUrl, audioState, translation) {
        return PetChatRuntime.newMessage({
            "outgoing": outgoing,
            "text": text,
            "turnId": turnId,
            "state": state,
            "audioUrl": audioUrl,
            "audioState": audioState,
            "translation": translation,
            "selfName": root.selfName,
            "displayName": root.displayName
        })
    }

    function assistantEventKey(event, turnId) {
        return PetChatRuntime.assistantEventKey(event, turnId, root.pendingAssistantTurnId)
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

    function assistantMessageIndex(turnKey) {
        if (turnKey.length === 0) {
            return -1
        }
        for (var index = messageModel.count - 1; index >= 0; --index) {
            var message = messageModel.get(index) || {}
            if (!message.outgoing && root.stringValue(message.turnId) === turnKey) {
                return index
            }
        }
        return -1
    }

    function updateCompletedAssistantAudio(turnKey, audioUrl, audioState) {
        var index = root.assistantMessageIndex(turnKey)
        if (index < 0) {
            return false
        }
        if (root.hasText(audioUrl)) {
            messageModel.setProperty(index, "audioUrl", audioUrl)
        }
        if (root.hasText(audioState)) {
            messageModel.setProperty(index, "audioState", audioState)
        }
        messageModel.setProperty(index, "messageState", "sent")
        root.chatStatusText = root.hasText(audioUrl) ? "语音回复已生成" : "语音生成失败，已显示文字"
        return true
    }

    function handleControllerError() {
        const errorText = root.petController ? root.stringValue(root.petController.error).trim() : ""
        if (errorText.length === 0) {
            root.lastControllerError = ""
            return
        }
        if (errorText === root.lastControllerError) {
            return
        }
        root.lastControllerError = errorText
        if (root.pendingAssistantIndex >= 0) {
            messageModel.setProperty(root.pendingAssistantIndex, "messageState", "failed")
            root.pendingAssistantIndex = -1
            root.pendingAssistantTurnId = ""
        }
        root.pendingSendText = ""
        root.pendingSendAlreadyAppended = false
        root.chatStatusText = "发送失败，可重试"
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
        return PetChatRuntime.languageCodeFromSettings(root.petAssetSettings)
    }

    function speechRulesText() {
        return PetChatRuntime.speechRulesTextFromSettings(root.petAssetSettings)
    }

    function petSettingText(name) {
        return PetChatRuntime.petSettingText(root.petAssetSettings, name)
    }

    function currentModelParts() {
        return PetChatRuntime.currentModelParts(root.agentController ? root.agentController.currentModel : "")
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

    function syncVoiceRuntimeSettings() {
        if (!root.petController || !root.petController.setVoiceRuntimeSettings) {
            return
        }
        const nextLanguage = languageCode()
        root.petController.setVoiceRuntimeSettings(
                    PetChatRuntime.voiceRuntimeSettings(root.petAssetSettings,
                                                        root.characterName(),
                                                        nextLanguage))
    }

    function syncContext() {
        syncModelSelection()
        syncReplyLanguage()
        syncSpeechRules()
        syncVoiceRuntimeSettings()
    }

    function openChat() {
        applyWindowFlags()
        refreshLive2DAvatar()
        show()
        raise()
        requestActivate()
        syncContext()
        if (composerBar) {
            composerBar.forceInputFocus()
        }
    }

    function appendUserMessage(text) {
        if (!text || text.length === 0) {
            return
        }
        messageModel.append(root.newMessage(true, text, "", "sent", "", "idle", ""))
        messageListPane.positionViewAtEnd()
    }

    function appendOrUpdateAssistantMessage(event) {
        const assistantEvent = PetChatRuntime.assistantEventSnapshot(
                    event,
                    PetChatRuntime.assistantControllerSnapshot(root.petController),
                    root.pendingAssistantTurnId,
                    root.pendingAssistantIndex)
        if (!assistantEvent.valid || !assistantEvent.hasContent
                || assistantEvent.resolvedEventKey.length === 0) {
            return
        }
        if (root.assistantTurnCompleted(assistantEvent.resolvedEventKey)) {
            if (assistantEvent.audioReadyPayload
                    && root.updateCompletedAssistantAudio(assistantEvent.resolvedEventKey,
                                                          assistantEvent.audioUrl,
                                                          assistantEvent.audioState)) {
                return
            }
            return
        }
        if (root.pendingAssistantIndex < 0
                || root.pendingAssistantTurnId !== assistantEvent.resolvedEventKey) {
            messageModel.append(root.newMessage(false,
                                                assistantEvent.display,
                                                assistantEvent.resolvedEventKey,
                                                assistantEvent.isFinal ? "sent" : "sending",
                                                assistantEvent.audioUrl,
                                                assistantEvent.audioState,
                                                assistantEvent.translation))
            root.pendingAssistantIndex = messageModel.count - 1
            root.pendingAssistantTurnId = assistantEvent.resolvedEventKey
        } else if (root.pendingAssistantIndex >= 0) {
            const current = messageModel.get(root.pendingAssistantIndex) || {}
            const nextDisplay = root.hasText(assistantEvent.display)
                    ? assistantEvent.display
                    : root.stringValue(current.content)
            messageModel.setProperty(root.pendingAssistantIndex, "content", nextDisplay)
            messageModel.setProperty(root.pendingAssistantIndex, "rawContent", nextDisplay)
            messageModel.setProperty(root.pendingAssistantIndex, "translationText", assistantEvent.translation)
            messageModel.setProperty(root.pendingAssistantIndex, "audioUrl", assistantEvent.audioUrl)
            messageModel.setProperty(root.pendingAssistantIndex, "audioState", assistantEvent.audioState)
            messageModel.setProperty(root.pendingAssistantIndex, "messageState",
                                     assistantEvent.isFinal ? "sent" : "sending")
        }

        if (root.pendingAssistantIndex >= 0) {
            messageListPane.positionViewAtEnd()
        }
        if (assistantEvent.isFinal) {
            root.rememberCompletedAssistantTurn(assistantEvent.resolvedEventKey)
            root.pendingAssistantIndex = -1
            root.pendingAssistantTurnId = ""
            root.chatStatusText = PetChatRuntime.assistantFinalStatus(assistantEvent.audioUrl,
                                                                      root.voiceCallActive)
        } else {
            const nextStatus = PetChatRuntime.assistantProgressStatus(assistantEvent.phase)
            if (nextStatus.length > 0) {
                root.chatStatusText = nextStatus
            }
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
        composerBar.clear()
        composerBar.forceInputFocus()
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
            root.syncContext()
            root.petController.startSession()
            composerBar.clear()
            composerBar.forceInputFocus()
            return
        }
        root.pendingSendAlreadyAppended = false
        root.appendUserMessage(trimmed)
        root.chatStatusText = "正在发送"
        root.syncContext()
        root.petController.sendText(trimmed)
        composerBar.clear()
        composerBar.forceInputFocus()
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

                PetChatHeader {
                    displayName: root.displayName
                    chatStatusText: root.chatStatusText
                    avatarSource: root.live2dAvatarSource
                    voiceCallActive: root.voiceCallActive
                    videoCallActive: root.videoCallActive
                    accentColor: root.accentColor
                    onCloseRequested: root.hide()
                }

                PetChatMessageList {
                    id: messageListPane
                    messageModel: messageModel
                    displayName: root.displayName
                    selfName: root.selfName
                    selfAvatar: root.selfAvatar
                    live2dAvatarSource: root.live2dAvatarSource
                    live2dAvatarFallback: root.live2dAvatarFallback
                }

                PetChatComposer {
                    id: composerBar
                    draftText: root.draftText
                    controllerAvailable: !!root.petController
                    controllerBusy: !!root.petController && root.petController.busy
                    voiceCallActive: root.voiceCallActive
                    videoCallActive: root.videoCallActive
                    modelStatusText: root.lastSyncedModel.length > 0 ? root.lastSyncedModel : "AI API"
                    onDraftEdited: function(text) { root.draftText = text }
                    onSendRequested: function(text) { root.sendMessage(text) }
                    onVoiceCallToggled: root.toggleVoiceCall()
                    onVideoCallToggled: root.toggleVideoCall()
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
            root.handleControllerError()
            root.syncContext()
        }

        function onControlEventReceived(event) {
            root.appendOrUpdateAssistantMessage(event)
        }

        function onWindowsImeBridgeChanged() {
            if (!root.petController.windowsImeBridgeBusy && composerBar) {
                composerBar.forceInputFocus()
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
        target: root.petSettingsSignalTarget
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
            if (composerBar) {
                composerBar.forceInputFocus()
            }
        }
    }

    onPetAssetSettingsChanged: refreshLive2DAvatar()

    onAlwaysOnTopChanged: applyWindowFlags()

    Component.onCompleted: {
        refreshLive2DAvatar()
        syncContext()
    }
}
