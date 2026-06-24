import QtQuick 2.15
import QtQuick.Layouts 1.15
import "conversation"
import "../runtime/ChatConversationRuntime.js" as ChatConversationRuntime

Rectangle {
    id: root
    color: "transparent"

    property Item backdrop: null
    property string peerName: ""
    property int selfUid: 0
    property string selfUserId: ""
    property string selfName: ""
    property string selfAvatar: "qrc:/res/head_1.jpg"
    property string peerAvatar: "qrc:/res/head_1.jpg"
    property bool hasCurrentChat: false
    property bool isGroupChat: false
    property int currentGroupRole: 1
    property var messageModel
    property var agentController: null
    property var imeBridgeController: null
    property string currentDraftText: ""
    property var currentPendingAttachments: []
    property bool currentDialogPinned: false
    property bool currentDialogMuted: false
    property bool hasPendingReply: false
    property string replyTargetName: ""
    property string replyPreviewText: ""
    property bool privateHistoryLoading: false
    property bool canLoadMorePrivateHistory: false
    property bool mediaUploadInProgress: false
    property string mediaUploadProgressText: ""
    property bool dialogsReady: false
    property bool _followTail: true
    property bool _topLoadArmed: true
    property real _wheelStepPx: 44
    property bool _stickToBottom: true
    property string smartResult: ""
    property string smartStatusText: ""
    property string smartResultTitle: "智能结果"
    property bool smartBusy: false
    property string activeSmartAction: ""
    property int activeSummaryIndex: -1
    property string activeTranslateMsgId: ""
    property string pendingTranslateMsgId: ""
    property string pendingTranslateText: ""
    property string selectedSuggestionText: ""
    property var translationByMsgId: ({})
    signal sendComposer(string text)
    signal sendImage()
    signal sendFile()
    signal removePendingAttachment(string attachmentId)
    signal sendVoiceCall()
    signal sendVideoCall()
    signal draftEdited(string text)
    signal refreshGroupRequested()
    signal toggleDialogPinned()
    signal toggleDialogMuted()
    signal openGroupManageRequested()
    signal openAttachment(string url)
    signal requestLoadMoreHistory()
    signal forwardMessage(string msgId)
    signal revokeMessage(string msgId)
    signal editMessage(string msgId, string text)
    signal replyMessage(string msgId, string senderName, string previewText)
    signal cancelReplyMessage()
    signal avatarProfileRequested(int uid, string name, string icon, string userId)

    function minContentY() {
        return messageList.minContentY()
    }

    function maxContentY() {
        return messageList.maxContentY()
    }

    function clampY(y) {
        return ChatConversationRuntime.clampValue(y, minContentY(), maxContentY())
    }

    function isAtBottom(epsilon) {
        return messageList.isAtBottom(epsilon)
    }

    function _requestScrollToEnd() {
        if (!root.hasCurrentChat) {
            return
        }
        messageList.scrollToEnd()
    }

    function _resetScrollState() {
        _followTail = true
        _topLoadArmed = true
        _stickToBottom = true
        if (messageList && messageList.resetScrollState) {
            messageList.resetScrollState()
        }
    }

    function recentTranscript(maxMessages) {
        if (!root.messageModel || !root.messageModel.exportRecentText) {
            return ""
        }
        return root.messageModel.exportRecentText(maxMessages || 100)
    }

    function latestIncomingTextInfo() {
        if (!root.messageModel || !root.messageModel.latestTextMessageInfo) {
            return {}
        }
        return root.messageModel.latestTextMessageInfo(true)
    }

    function translationForMessage(msgId) {
        if (!msgId || !root.translationByMsgId) {
            return ""
        }
        return root.translationByMsgId[msgId] || ""
    }

    function setTranslationForMessage(msgId, text) {
        if (!msgId || !text) {
            return
        }
        const next = {}
        for (const key in root.translationByMsgId) {
            next[key] = root.translationByMsgId[key]
        }
        next[msgId] = text
        root.translationByMsgId = next
    }

    function parseSuggestionOptions(result) {
        return ChatConversationRuntime.parseSuggestionOptions(result)
    }

    function openTranslateDialog(msgId, text) {
        if (!msgId || !text || text.length === 0) {
            root.smartStatusText = "没有找到可翻译的文本。"
            return
        }
        root.pendingTranslateMsgId = msgId
        root.pendingTranslateText = text
        smartActionPopups.openTranslateDialog()
    }

    function confirmTranslate(sourceLanguage, targetLanguage) {
        if (!root.agentController || root.smartBusy || root.pendingTranslateText.length === 0) {
            return
        }
        root.activeSmartAction = "translate"
        root.activeTranslateMsgId = root.pendingTranslateMsgId
        root.smartBusy = true
        root.smartStatusText = "正在翻译消息..."
        root.agentController.translateMessageWithSource(
                    root.pendingTranslateText,
                    sourceLanguage,
                    targetLanguage)
    }

    function runSmartAction(action) {
        if (!root.agentController || root.smartBusy) {
            return
        }
        if (action === "summary") {
            const transcript = root.recentTranscript(100)
            if (transcript.length === 0) {
                root.smartStatusText = "当前会话没有可总结的文本。"
                return
            }
            root.smartResultTitle = root.isGroupChat ? "群聊摘要" : "会话摘要"
            root.smartResult = ""
            summaryCards.append({
                                    "summaryTitle": root.smartResultTitle,
                                    "summaryContent": "",
                                    "summaryStatus": "正在总结最近 100 条内的文本消息...",
                                    "summaryBusy": true
                                })
            root.activeSummaryIndex = summaryCards.count - 1
            root.activeSmartAction = "summary"
            root.smartBusy = true
            root.smartStatusText = "正在总结最近 100 条内的文本消息..."
            root.agentController.summarizeChat(root.peerName, transcript)
            return
        }
        if (action === "suggest") {
            const context = root.recentTranscript(100)
            if (context.length === 0) {
                root.smartStatusText = "当前会话没有可参考的文本。"
                return
            }
            root.smartResultTitle = "建议回复"
            root.smartResult = ""
            root.activeSmartAction = "suggest"
            suggestionModel.clear()
            root.selectedSuggestionText = ""
            smartActionPopups.openSuggestionDialog()
            root.smartBusy = true
            root.smartStatusText = "正在生成回复建议..."
            root.agentController.suggestReply(root.peerName, context)
            return
        }
        if (action === "translate") {
            const info = root.latestIncomingTextInfo()
            const text = info && info.content ? info.content : ""
            const msgId = info && info.msgId ? info.msgId : ""
            if (text.length === 0 || msgId.length === 0) {
                root.smartStatusText = "没有找到可翻译的最近文本。"
                return
            }
            root.openTranslateDialog(msgId, text)
        }
    }

    Connections {
        target: root.agentController
        enabled: root.agentController !== null
        function onSmartResultReady(featureType, result) {
            root.smartBusy = false
            if (featureType === "summary") {
                root.smartResultTitle = root.isGroupChat ? "群聊摘要" : "会话摘要"
                if (root.activeSummaryIndex >= 0 && root.activeSummaryIndex < summaryCards.count) {
                    summaryCards.setProperty(root.activeSummaryIndex, "summaryContent", result || "")
                    summaryCards.setProperty(root.activeSummaryIndex, "summaryStatus", result && result.length > 0 ? "已生成" : "AI 没有返回内容。")
                    summaryCards.setProperty(root.activeSummaryIndex, "summaryBusy", false)
                }
            } else if (featureType === "suggest") {
                root.smartResultTitle = "建议回复"
                const options = root.parseSuggestionOptions(result || "")
                suggestionModel.clear()
                for (let i = 0; i < options.length; ++i) {
                    suggestionModel.append({ "optionText": options[i] })
                }
                root.selectedSuggestionText = options.length > 0 ? options[0] : ""
            } else if (featureType === "translate") {
                root.smartResultTitle = "最近消息翻译"
                root.setTranslationForMessage(root.activeTranslateMsgId, result || "")
                root.activeTranslateMsgId = ""
            }
            root.activeSmartAction = ""
            root.smartStatusText = result && result.length > 0 ? "已生成" : "AI 没有返回内容。"
        }
        function onErrorOccurred(error) {
            if (root.smartBusy && error && error.length > 0) {
                root.smartBusy = false
                root.smartStatusText = error
                if (root.activeSmartAction === "summary"
                        && root.activeSummaryIndex >= 0
                        && root.activeSummaryIndex < summaryCards.count) {
                    summaryCards.setProperty(root.activeSummaryIndex, "summaryStatus", error)
                    summaryCards.setProperty(root.activeSummaryIndex, "summaryBusy", false)
                }
                root.activeSmartAction = ""
            }
        }
    }

    ListModel {
        id: summaryCards
    }

    ListModel {
        id: suggestionModel
    }

    onPrivateHistoryLoadingChanged: {
        if (!privateHistoryLoading && messageList.contentY > 24) {
            _topLoadArmed = true
        }
    }

    onHasCurrentChatChanged: {
        _resetScrollState()
        if (hasCurrentChat) {
            _requestScrollToEnd()
        }
    }

    onPeerNameChanged: {
        if (!hasCurrentChat) {
            return
        }
        _resetScrollState()
        _requestScrollToEnd()
    }

    onIsGroupChatChanged: {
        if (!hasCurrentChat) {
            return
        }
        _resetScrollState()
        _requestScrollToEnd()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ChatConversationHeader {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            peerName: root.peerName
            hasCurrentChat: root.hasCurrentChat
            isGroupChat: root.isGroupChat
            onOpenGroupManageRequested: root.openGroupManageRequested()
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.16)
            border.color: Qt.rgba(1, 1, 1, 0.42)

            ChatMessageListView {
                id: messageList
                anchors.fill: parent
                anchors.margins: 14
                backdrop: root.backdrop !== null ? root.backdrop : root
                messageModel: root.messageModel
                hasCurrentChat: root.hasCurrentChat
                isGroupChat: root.isGroupChat
                currentGroupRole: root.currentGroupRole
                selfUid: root.selfUid
                selfUserId: root.selfUserId
                selfName: root.selfName
                selfAvatar: root.selfAvatar
                peerAvatar: root.peerAvatar
                dialogsReady: root.dialogsReady
                privateHistoryLoading: root.privateHistoryLoading
                canLoadMorePrivateHistory: root.canLoadMorePrivateHistory
                followTail: root._followTail
                topLoadArmed: root._topLoadArmed
                stickToBottom: root._stickToBottom
                translationByMsgId: root.translationByMsgId
                onScrollStateChanged: function(followTail, topLoadArmed, stickToBottom) {
                    root._followTail = followTail
                    root._topLoadArmed = topLoadArmed
                    root._stickToBottom = stickToBottom
                }
                onLoadMoreHistoryRequested: root.requestLoadMoreHistory()
                onOpenAttachment: function(url) { root.openAttachment(url) }
                onTranslateRequested: function(msgId, text) { root.openTranslateDialog(msgId, text) }
                onForwardMessage: function(msgId) { root.forwardMessage(msgId) }
                onRevokeMessage: function(msgId) { root.revokeMessage(msgId) }
                onReplyMessage: function(msgId, senderName, previewText) { root.replyMessage(msgId, senderName, previewText) }
                onMentionRequested: function(mentionText) { composer.insertMention(mentionText) }
                onEditRequested: function(msgId, text) { smartActionPopups.openEditDialog(msgId, text) }
                onAvatarProfileRequested: function(uid, name, icon, userId) {
                    root.avatarProfileRequested(uid, name, icon, userId)
                }
            }

            ChatSmartActionPopups {
                id: smartActionPopups
                anchors.fill: parent
                anchors.margins: 12
                z: 4
                backdrop: root.backdrop !== null ? root.backdrop : root
                summaryModel: summaryCards
                suggestionModel: suggestionModel
                smartBusy: root.smartBusy
                activeSmartAction: root.activeSmartAction
                selectedSuggestionText: root.selectedSuggestionText
                pendingTranslateText: root.pendingTranslateText
                onSuggestionSelected: function(text) { root.selectedSuggestionText = text }
                onSuggestedTextAccepted: function(text) { composer.applySuggestedText(text) }
                onTranslateConfirmed: function(sourceLanguage, targetLanguage) {
                    root.confirmTranslate(sourceLanguage, targetLanguage)
                }
                onEditConfirmed: function(msgId, text) {
                    root.editMessage(msgId, text)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.hasPendingReply
                                    || (root.currentPendingAttachments && root.currentPendingAttachments.length > 0)
                                    ? 204
                                    : 156
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.22)
            border.color: Qt.rgba(1, 1, 1, 0.46)

            ChatComposerBar {
                id: composer
                anchors.fill: parent
                backdrop: root.backdrop
                enabledComposer: root.hasCurrentChat
                isGroupChat: root.isGroupChat
                mediaUploadInProgress: root.mediaUploadInProgress
                mediaUploadProgressText: root.mediaUploadProgressText
                draftText: root.currentDraftText
                pendingAttachments: root.currentPendingAttachments
                hasReplyContext: root.hasPendingReply
                replyTargetName: root.replyTargetName
                replyPreviewText: root.replyPreviewText
                smartResult: root.smartResult
                smartBusy: root.smartBusy
                smartStatusText: root.smartStatusText
                smartResultTitle: root.smartResultTitle
                imeBridgeController: root.imeBridgeController
                onSendComposer: function(text) {
                    root._followTail = true
                    root._stickToBottom = true
                    root.sendComposer(text)
                    root._requestScrollToEnd()
                }
                onSendImage: {
                    root._followTail = true
                    root._stickToBottom = true
                    root.sendImage()
                    root._requestScrollToEnd()
                }
                onSendFile: {
                    root._followTail = true
                    root._stickToBottom = true
                    root.sendFile()
                    root._requestScrollToEnd()
                }
                onRemovePendingAttachment: function(attachmentId) { root.removePendingAttachment(attachmentId) }
                onSendVoiceCall: root.sendVoiceCall()
                onSendVideoCall: root.sendVideoCall()
                onDraftEdited: function(text) { root.draftEdited(text) }
                onClearReplyRequested: root.cancelReplyMessage()
                onSummaryRequested: root.runSmartAction("summary")
                onSuggestRequested: root.runSmartAction("suggest")
                onTranslateRequested: root.runSmartAction("translate")
                onClearSmartResultRequested: {
                    root.smartResult = ""
                    root.smartStatusText = ""
                }
            }
        }
    }

}
