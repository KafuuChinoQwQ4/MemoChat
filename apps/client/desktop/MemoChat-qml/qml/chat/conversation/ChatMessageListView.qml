import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../components"

Item {
    id: root

    property Item backdrop: null
    property var messageModel
    property bool hasCurrentChat: false
    property bool isGroupChat: false
    property int currentGroupRole: 1
    property string selfName: ""
    property string selfAvatar: "qrc:/res/head_1.jpg"
    property string peerAvatar: "qrc:/res/head_1.jpg"
    property bool dialogsReady: false
    property bool privateHistoryLoading: false
    property bool canLoadMorePrivateHistory: false
    property bool followTail: true
    property bool topLoadArmed: true
    property bool stickToBottom: true
    property var translationByMsgId: ({})
    property alias count: messageList.count
    property alias contentY: messageList.contentY

    signal scrollStateChanged(bool followTail, bool topLoadArmed, bool stickToBottom)
    signal loadMoreHistoryRequested()
    signal openAttachment(string url)
    signal translateRequested(string msgId, string text)
    signal forwardMessage(string msgId)
    signal revokeMessage(string msgId)
    signal replyMessage(string msgId, string senderName, string previewText)
    signal mentionRequested(string mentionText)
    signal editRequested(string msgId, string text)
    signal avatarProfileRequested(int uid, string name, string icon)

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

    function scrollToEnd() {
        if (!root.hasCurrentChat) {
            return
        }
        scrollToEndTimer.restart()
    }

    function resetScrollState() {
        root.scrollStateChanged(true, true, true)
    }

    function translationForMessage(msgId) {
        if (!msgId || !root.translationByMsgId) {
            return ""
        }
        return root.translationByMsgId[msgId] || ""
    }

    ListView {
        id: messageList
        anchors.fill: parent
        spacing: 0
        clip: true
        interactive: contentHeight > height
        reuseItems: true
        cacheBuffer: Math.max(height, 720)
        model: root.messageModel
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: GlassScrollBar { }
        onCountChanged: {
            if (count > 0
                    && root.hasCurrentChat
                    && root.followTail
                    && !root.privateHistoryLoading) {
                root.scrollToEnd()
            }
        }
        onContentYChanged: {
            let nextStickToBottom = root.isAtBottom(1.0)
            let nextFollowTail = root.followTail
            let nextTopLoadArmed = root.topLoadArmed
            if (nextStickToBottom) {
                nextFollowTail = true
            } else if (contentY > root.minContentY() + 1) {
                nextFollowTail = false
            }
            if (contentY > 24) {
                nextTopLoadArmed = true
            }
            if (contentY <= root.minContentY() + 1
                    && count > 0
                    && root.canLoadMorePrivateHistory
                    && !root.privateHistoryLoading
                    && root.topLoadArmed) {
                nextTopLoadArmed = false
                nextFollowTail = false
                root.loadMoreHistoryRequested()
            }
            root.scrollStateChanged(nextFollowTail, nextTopLoadArmed, nextStickToBottom)
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
            onTranslateRequested: function(msgId, text) { root.translateRequested(msgId, text) }
            onForwardRequested: function(msgId) { root.forwardMessage(msgId) }
            onRevokeRequested: function(msgId) { root.revokeMessage(msgId) }
            onReplyRequested: function(msgId, senderName, previewText) { root.replyMessage(msgId, senderName, previewText) }
            onMentionRequested: function(mentionText) { root.mentionRequested(mentionText) }
            onAvatarClicked: function(uid, name, icon) { root.avatarProfileRequested(uid, name, icon) }
            onEditRequested: function(msgId, text) { root.editRequested(msgId, text) }
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

    Timer {
        id: scrollToEndTimer
        interval: 0
        repeat: false
        onTriggered: {
            if (messageList.count > 0) {
                messageList.positionViewAtEnd()
                root.scrollStateChanged(true, root.topLoadArmed, true)
            }
        }
    }
}
