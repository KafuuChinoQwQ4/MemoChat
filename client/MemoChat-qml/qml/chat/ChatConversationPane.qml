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
    property string selfAvatar: "qrc:/res/head_1.jpg"
    property string peerAvatar: "qrc:/res/head_1.jpg"
    property bool hasCurrentChat: false
    property bool isGroupChat: false
    property int currentGroupRole: 1
    property var messageModel
    property string currentDraftText: ""
    property bool currentDialogPinned: false
    property bool currentDialogMuted: false
    property bool hasPendingReply: false
    property string replyTargetName: ""
    property string replyPreviewText: ""
    property bool privateHistoryLoading: false
    property bool canLoadMorePrivateHistory: false
    property bool mediaUploadInProgress: false
    property string mediaUploadProgressText: ""
    property bool _followTail: true
    property bool _topLoadArmed: true
    property real _wheelStepPx: 44
    property bool _stickToBottom: true
    signal sendText(string text)
    signal sendImage()
    signal sendFile()
    signal sendVoiceCall()
    signal sendVideoCall()
    signal draftEdited(string text)
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
            Layout.preferredHeight: 64
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.24)
            border.color: Qt.rgba(1, 1, 1, 0.46)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 18
                spacing: 0

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        text: root.peerName.length > 0 ? root.peerName : "聊天"
                        color: "#2a3649"
                        font.pixelSize: 18
                        font.bold: true
                        elide: Text.ElideRight
                    }
                }

                GlassButton {
                    Layout.preferredWidth: 88
                    Layout.preferredHeight: 32
                    visible: root.hasCurrentChat
                    text: root.currentDialogPinned ? "取消置顶" : "置顶"
                    textPixelSize: 13
                    cornerRadius: 9
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.22)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.32)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.40)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    onClicked: root.toggleDialogPinned()
                }

                GlassButton {
                    Layout.preferredWidth: 88
                    Layout.preferredHeight: 32
                    Layout.leftMargin: 8
                    visible: root.hasCurrentChat
                    text: root.currentDialogMuted ? "取消静音" : "静音"
                    textPixelSize: 13
                    cornerRadius: 9
                    normalColor: Qt.rgba(0.42, 0.56, 0.74, 0.22)
                    hoverColor: Qt.rgba(0.42, 0.56, 0.74, 0.32)
                    pressedColor: Qt.rgba(0.42, 0.56, 0.74, 0.40)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    onClicked: root.toggleDialogMuted()
                }

                GlassButton {
                    Layout.preferredWidth: 84
                    Layout.preferredHeight: 32
                    Layout.leftMargin: 8
                    visible: root.hasCurrentChat && root.isGroupChat
                    text: "群管理"
                    textPixelSize: 13
                    cornerRadius: 9
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    onClicked: root.openGroupManageRequested()
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
                onContentHeightChanged: {
                    if (count > 0
                            && root.hasCurrentChat
                            && root._followTail
                            && !root.privateHistoryLoading) {
                        root._requestScrollToEnd()
                    }
                }
                onHeightChanged: {
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
                    outgoing: model.outgoing
                    msgType: model.msgType
                    content: model.content
                    rawContent: model.rawContent
                    fileName: model.fileName
                    senderName: model.senderName
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
                    onForwardRequested: function(msgId) { root.forwardMessage(msgId) }
                    onRevokeRequested: function(msgId) { root.revokeMessage(msgId) }
                    onReplyRequested: function(msgId, senderName, previewText) { root.replyMessage(msgId, senderName, previewText) }
                    onMentionRequested: function(mentionText) { composer.insertMention(mentionText) }
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
                    text: root.hasCurrentChat ? "还没有消息，开始聊吧" : "请选择一个会话"
                    color: "#6a7b92"
                    font.pixelSize: 13
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 206
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
                hasReplyContext: root.hasPendingReply
                replyTargetName: root.replyTargetName
                replyPreviewText: root.replyPreviewText
                onSendText: function(text) {
                    root._followTail = true
                    root._stickToBottom = true
                    root.sendText(text)
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
                onSendVoiceCall: root.sendVoiceCall()
                onSendVideoCall: root.sendVideoCall()
                onDraftEdited: function(text) { root.draftEdited(text) }
                onClearReplyRequested: root.cancelReplyMessage()
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
