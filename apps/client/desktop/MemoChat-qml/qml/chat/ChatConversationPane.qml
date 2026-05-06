import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"
import "conversation"

Rectangle {
    id: root
    color: "transparent"

    property Item backdrop: null
    property string peerName: ""
    property string selfName: ""
    property string selfAvatar: "qrc:/res/head_1.jpg"
    property string peerAvatar: "qrc:/res/head_1.jpg"
    property bool hasCurrentChat: false
    property bool isGroupChat: false
    property int currentGroupRole: 1
    property var messageModel
    property var agentController: null
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
    signal avatarProfileRequested(int uid, string name, string icon)

    property string _editMsgId: ""
    property string _editText: ""

    function minContentY() {
        return messageList.originY
    }

    function maxContentY() {
        return messageList.originY + Math.max(0, messageList.contentHeight - messageList.height)
    }

    function clampY(y) {
        return Math.max(minContentY(), Math.min(maxContentY(), y))
    }

    function isAtBottom(epsilon) {
        return maxContentY() - messageList.contentY <= (epsilon || 1.0)
    }

    function _requestScrollToEnd() {
        if (!root.hasCurrentChat) {
            return
        }
        scrollToEndTimer.restart()
    }

    function _resetScrollState() {
        _followTail = true
        _topLoadArmed = true
        _stickToBottom = true
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
        let text = result || ""
        const options = []
        try {
            const parsed = JSON.parse(text)
            if (Array.isArray(parsed)) {
                for (let i = 0; i < parsed.length && options.length < 3; ++i) {
                    const item = String(parsed[i]).trim()
                    if (item.length > 0) {
                        options.push(item)
                    }
                }
            }
        } catch (e) {
        }
        if (options.length === 0) {
            const lines = text.split(/\r?\n/)
            for (let j = 0; j < lines.length && options.length < 3; ++j) {
                let line = lines[j].replace(/^[\s\-*•\d.、)）]+/, "").trim()
                if (line.length > 0) {
                    options.push(line)
                }
            }
        }
        while (options.length < 3) {
            options.push(options.length === 0 ? "好的，我看一下。" : (options.length === 1 ? "收到，我稍后回复。" : "可以，我们继续聊。"))
        }
        return options.slice(0, 3)
    }

    function openTranslateDialog(msgId, text) {
        if (!msgId || !text || text.length === 0) {
            root.smartStatusText = "没有找到可翻译的文本。"
            return
        }
        root.pendingTranslateMsgId = msgId
        root.pendingTranslateText = text
        translateSourceBox.currentIndex = 0
        translateTargetBox.currentIndex = 1
        translateDialog.open()
    }

    function confirmTranslate() {
        if (!root.agentController || root.smartBusy || root.pendingTranslateText.length === 0) {
            return
        }
        root.activeSmartAction = "translate"
        root.activeTranslateMsgId = root.pendingTranslateMsgId
        root.smartBusy = true
        root.smartStatusText = "正在翻译消息..."
        root.agentController.translateMessageWithSource(
                    root.pendingTranslateText,
                    translateSourceBox.currentText,
                    translateTargetBox.currentText)
        translateDialog.close()
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
            suggestionDialog.open()
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

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.24)
            border.color: Qt.rgba(1, 1, 1, 0.46)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 10
                spacing: 10

                Label {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    text: root.peerName.length > 0 ? root.peerName : "聊天"
                    color: "#2a3649"
                    font.pixelSize: 17
                    font.bold: true
                    elide: Text.ElideRight
                }

                Item { Layout.fillWidth: true }

                GlassButton {
                    id: groupSettingsButton
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 28
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                    visible: root.hasCurrentChat && root.isGroupChat
                    text: "..."
                    textPixelSize: 22
                    cornerRadius: 9
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.22)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.32)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.40)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    onClicked: root.openGroupManageRequested()

                    ToolTip.visible: hovering
                    ToolTip.delay: 180
                    ToolTip.text: "群设置"
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.16)
            border.color: Qt.rgba(1, 1, 1, 0.42)

            ListView {
                id: messageList
                anchors.fill: parent
                anchors.margins: 14
                spacing: 0
                clip: true
                interactive: false
                model: root.messageModel
                ScrollBar.vertical: GlassScrollBar { }
                WheelHandler {
                    target: null
                    onWheel: function(event) {
                        let deltaY = 0
                        if (event.pixelDelta.y !== 0) {
                            deltaY = event.pixelDelta.y
                        } else if (event.angleDelta.y !== 0) {
                            deltaY = (event.angleDelta.y / 120) * root._wheelStepPx
                        }
                        if (deltaY === 0) {
                            return
                        }
                        const nextY = root.clampY(messageList.contentY - deltaY)
                        if (Math.abs(nextY - messageList.contentY) > 0.1) {
                            messageList.contentY = nextY
                            if (deltaY > 0) {
                                root._followTail = false
                            } else if (root.isAtBottom(1.0)) {
                                root._followTail = true
                            }
                            root._stickToBottom = root.isAtBottom(1.0)
                            event.accepted = true
                        }
                    }
                }
                onCountChanged: {
                    if (count > 0
                            && root.hasCurrentChat
                            && root._followTail
                            && !root.privateHistoryLoading) {
                        root._requestScrollToEnd()
                    }
                }
                onContentYChanged: {
                    root._stickToBottom = root.isAtBottom(1.0)
                    if (root._stickToBottom) {
                        root._followTail = true
                    }
                    if (contentY > 24) {
                        root._topLoadArmed = true
                    }
                    if (contentY <= root.minContentY() + 1
                            && count > 0
                            && root.canLoadMorePrivateHistory
                            && !root.privateHistoryLoading
                            && root._topLoadArmed) {
                        root._topLoadArmed = false
                        root._followTail = false
                        root.requestLoadMoreHistory()
                    }
                }

                delegate: ChatMessageDelegate {
                    width: messageList.width
                    msgId: model.msgId
                    senderUid: model.outgoing ? 0 : model.fromUid
                    outgoing: model.outgoing
                    msgType: model.msgType
                    content: model.content
                    rawContent: model.rawContent
                    translationText: root.translationForMessage(model.msgId)
                    fileName: model.fileName
                    senderName: (model.outgoing && root.isGroupChat) ? root.selfName : model.senderName
                    showOutgoingSenderName: root.isGroupChat
                    showAvatar: model.showAvatar
                    showTimeDivider: model.showTimeDivider
                    timeDividerText: model.timeDividerText
                    messageState: model.messageState
                    isReply: model.isReply
                    replyToMsgId: model.replyToMsgId
                    replySender: model.replySender
                    replyPreview: model.replyPreview
                    enableContextMenu: root.hasCurrentChat
                    canReply: root.isGroupChat
                    canMention: root.isGroupChat && !model.outgoing && model.senderName && model.senderName.length > 0
                    canForward: root.hasCurrentChat
                    canEdit: model.outgoing && model.msgType === "text"
                    canRevoke: root.isGroupChat ? (model.outgoing || root.currentGroupRole >= 2) : model.outgoing
                    avatarSource: model.outgoing
                                  ? root.selfAvatar
                                  : ((model.senderIcon && model.senderIcon.length > 0) ? model.senderIcon : root.peerAvatar)
                    onOpenUrlRequested: function(url) { root.openAttachment(url) }
                    onTranslateRequested: function(msgId, text) { root.openTranslateDialog(msgId, text) }
                    onForwardRequested: function(msgId) { root.forwardMessage(msgId) }
                    onRevokeRequested: function(msgId) { root.revokeMessage(msgId) }
                    onReplyRequested: function(msgId, senderName, previewText) { root.replyMessage(msgId, senderName, previewText) }
                    onMentionRequested: function(mentionText) { composer.insertMention(mentionText) }
                    onAvatarClicked: function(uid, name, icon) { root.avatarProfileRequested(uid, name, icon) }
                    onEditRequested: function(msgId, text) {
                        root._editMsgId = msgId
                        root._editText = text
                        editDialog.open()
                    }
                }
            }

            GlassSurface {
                anchors.centerIn: parent
                width: 210
                height: 86
                visible: messageList.count === 0
                backdrop: root.backdrop !== null ? root.backdrop : root
                cornerRadius: 10
                blurRadius: 16
                fillColor: Qt.rgba(1, 1, 1, 0.20)
                strokeColor: Qt.rgba(1, 1, 1, 0.42)

                Label {
                    anchors.centerIn: parent
                    text: root.hasCurrentChat
                          ? "还没有消息，开始聊吧"
                          : (root.dialogsReady ? "请选择一个会话" : "正在准备最近会话")
                    color: "#6a7b92"
                    font.pixelSize: 13
                }
            }

            Item {
                id: summaryOverlay
                anchors.fill: parent
                anchors.margins: 12
                z: 4
                visible: summaryCards.count > 0

                Repeater {
                    model: summaryCards

                    delegate: GlassSurface {
                        width: Math.min(summaryOverlay.width - 24, 420)
                        height: Math.min(220, Math.max(124, summaryColumn.implicitHeight + 22))
                        x: Math.max(0, summaryOverlay.width - width - (index % 2) * 26)
                        y: 12 + index * 18
                        backdrop: root.backdrop !== null ? root.backdrop : root
                        cornerRadius: 12
                        blurRadius: 18
                        fillColor: Qt.rgba(1, 1, 1, 0.88)
                        strokeColor: Qt.rgba(1, 1, 1, 0.62)

                        ColumnLayout {
                            id: summaryColumn
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 7

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    Layout.fillWidth: true
                                    text: summaryTitle
                                    color: "#26364d"
                                    font.pixelSize: 14
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                ToolButton {
                                    Layout.preferredWidth: 26
                                    Layout.preferredHeight: 26
                                    text: "x"
                                    onClicked: summaryCards.remove(index)
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                visible: summaryBusy || summaryContent.length === 0
                                text: summaryStatus
                                color: summaryBusy ? "#4f6788" : "#9b4b4b"
                                font.pixelSize: 12
                                wrapMode: Text.Wrap
                            }

                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                visible: summaryContent.length > 0
                                clip: true

                                TextEdit {
                                    width: parent.width
                                    text: summaryContent
                                    readOnly: true
                                    selectByMouse: true
                                    cursorVisible: false
                                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                                    textFormat: Text.PlainText
                                    color: "#223047"
                                    font.pixelSize: 13
                                }
                            }
                        }
                    }
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

    Timer {
        id: scrollToEndTimer
        interval: 0
        repeat: false
        onTriggered: {
            if (messageList.count > 0) {
                messageList.positionViewAtEnd()
                root._stickToBottom = true
                root._followTail = true
            }
        }
    }

    Popup {
        id: suggestionDialog
        modal: false
        focus: true
        width: Math.min(root.width - 48, 460)
        height: 260
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: GlassSurface {
            anchors.fill: parent
            backdrop: root.backdrop !== null ? root.backdrop : root
            cornerRadius: 12
            blurRadius: 18
            fillColor: Qt.rgba(1, 1, 1, 0.88)
            strokeColor: Qt.rgba(1, 1, 1, 0.62)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    text: "建议回复"
                    color: "#26364d"
                    font.pixelSize: 15
                    font.bold: true
                }

                Label {
                    text: root.smartBusy && root.activeSmartAction === "suggest" ? "生成中" : ""
                    color: "#60718a"
                    font.pixelSize: 12
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                Repeater {
                    model: suggestionModel

                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        radius: 8
                        color: root.selectedSuggestionText === optionText
                               ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                               : optionMouse.containsMouse ? Qt.rgba(0.35, 0.61, 0.90, 0.12)
                                                           : Qt.rgba(1, 1, 1, 0.30)
                        border.color: root.selectedSuggestionText === optionText
                                      ? Qt.rgba(0.35, 0.61, 0.90, 0.52)
                                      : Qt.rgba(1, 1, 1, 0.42)

                        Text {
                            anchors.fill: parent
                            anchors.margins: 10
                            text: optionText
                            color: "#223047"
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        MouseArea {
                            id: optionMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.selectedSuggestionText = optionText
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: suggestionModel.count === 0
                    text: root.smartBusy ? "正在生成 3 条候选回复..." : "暂无建议"
                    color: "#60718a"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Item { Layout.fillWidth: true }

                GlassButton {
                    text: "取消"
                    implicitWidth: 72
                    implicitHeight: 32
                    textPixelSize: 12
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.22)
                    onClicked: suggestionDialog.close()
                }

                GlassButton {
                    text: "确定"
                    implicitWidth: 72
                    implicitHeight: 32
                    textPixelSize: 12
                    cornerRadius: 8
                    enabled: root.selectedSuggestionText.length > 0
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    onClicked: {
                        composer.applySuggestedText(root.selectedSuggestionText)
                        suggestionDialog.close()
                    }
                }
            }
        }
    }

    Popup {
        id: translateDialog
        modal: true
        focus: true
        width: Math.min(root.width - 48, 420)
        height: 248
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: GlassSurface {
            anchors.fill: parent
            backdrop: root.backdrop !== null ? root.backdrop : root
            cornerRadius: 12
            blurRadius: 18
            fillColor: Qt.rgba(1, 1, 1, 0.88)
            strokeColor: Qt.rgba(1, 1, 1, 0.62)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            Label {
                Layout.fillWidth: true
                text: "翻译消息"
                color: "#26364d"
                font.pixelSize: 15
                font.bold: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                ComboBox {
                    id: translateSourceBox
                    Layout.fillWidth: true
                    editable: true
                    model: ["自动检测", "中文", "英语", "日语", "韩语", "法语", "德语"]
                    currentIndex: 0
                }

                Label {
                    text: "到"
                    color: "#60718a"
                    font.pixelSize: 13
                }

                ComboBox {
                    id: translateTargetBox
                    Layout.fillWidth: true
                    editable: true
                    model: ["英语", "中文", "日语", "韩语", "法语", "德语"]
                    currentIndex: 1
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: Qt.rgba(1, 1, 1, 0.34)
                border.color: Qt.rgba(1, 1, 1, 0.42)
                clip: true

                Text {
                    anchors.fill: parent
                    anchors.margins: 10
                    text: root.pendingTranslateText
                    color: "#34445c"
                    font.pixelSize: 12
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    elide: Text.ElideRight
                    maximumLineCount: 3
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Item { Layout.fillWidth: true }

                GlassButton {
                    text: "取消"
                    implicitWidth: 72
                    implicitHeight: 32
                    textPixelSize: 12
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.22)
                    onClicked: translateDialog.close()
                }

                GlassButton {
                    text: "翻译"
                    implicitWidth: 72
                    implicitHeight: 32
                    textPixelSize: 12
                    cornerRadius: 8
                    enabled: !root.smartBusy && root.pendingTranslateText.length > 0
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    onClicked: root.confirmTranslate()
                }
            }
        }
    }

    Popup {
        id: editDialog
        modal: true
        focus: true
        width: Math.min(root.width - 40, 420)
        height: 220
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onOpened: editInput.text = root._editText

        background: GlassSurface {
            anchors.fill: parent
            backdrop: root.backdrop !== null ? root.backdrop : root
            cornerRadius: 12
            blurRadius: 18
            fillColor: Qt.rgba(1, 1, 1, 0.24)
            strokeColor: Qt.rgba(1, 1, 1, 0.46)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Label {
                text: "编辑消息"
                color: "#2a3649"
                font.pixelSize: 15
                font.bold: true
            }

            TextArea {
                id: editInput
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: root._editText
                wrapMode: Text.Wrap
                color: "#253247"
                selectionColor: "#7baee899"
                selectedTextColor: "#ffffff"
                background: Rectangle {
                    radius: 8
                    color: Qt.rgba(1, 1, 1, 0.30)
                    border.color: Qt.rgba(1, 1, 1, 0.44)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Item { Layout.fillWidth: true }
                GlassButton {
                    text: "取消"
                    implicitWidth: 72
                    implicitHeight: 32
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.24)
                    hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.34)
                    pressedColor: Qt.rgba(0.54, 0.60, 0.68, 0.42)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    onClicked: editDialog.close()
                }
                GlassButton {
                    text: "保存"
                    implicitWidth: 72
                    implicitHeight: 32
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    enabled: editInput.text.trim().length > 0
                    onClicked: {
                        root.editMessage(root._editMsgId, editInput.text)
                        editDialog.close()
                    }
                }
            }
        }
    }
}
