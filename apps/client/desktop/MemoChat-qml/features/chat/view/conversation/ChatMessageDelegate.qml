pragma ComponentBehavior: Bound

import QtQuick 2.15
import "."

Item {
    id: root
    width: ListView.view ? ListView.view.width : 300

    property bool outgoing: false
    property string msgId: ""
    property string msgType: "text"
    property string content: ""
    property string rawContent: ""
    property string translationText: ""
    property string fileName: ""
    property string senderName: ""
    property int senderUid: 0
    property string senderUserId: ""
    property bool showOutgoingSenderName: false
    property bool showAvatar: true
    property bool showTimeDivider: false
    property string timeDividerText: ""
    property string avatarSource: "qrc:/res/head_1.jpg"
    property string defaultAvatarSource: "qrc:/res/head_1.jpg"
    property string messageState: "sent"
    property bool isReply: false
    property string replyToMsgId: ""
    property string replySender: ""
    property string replyPreview: ""
    property bool enableContextMenu: false
    property bool canReply: false
    property bool canMention: false
    property bool canForward: false
    property bool canEdit: false
    property bool canRevoke: false
    property bool showRevokeExpiredHint: false

    signal openUrlRequested(string url)
    signal replyRequested(string msgId, string senderName, string previewText)
    signal mentionRequested(string mentionText)
    signal forwardRequested(string msgId)
    signal editRequested(string msgId, string text)
    signal revokeRequested(string msgId)
    signal translateRequested(string msgId, string text)
    signal avatarClicked(int uid, string name, string icon, string userId)

    property int avatarSize: 34
    property int avatarSlotWidth: 42
    readonly property bool showSenderName: (((!outgoing) || showOutgoingSenderName) && senderName.length > 0 && showAvatar)
    readonly property string previewForReply: {
        if (msgType === "image") {
            return "[图片]"
        }
        if (msgType === "file") {
            return fileName && fileName.length > 0 ? ("[文件] " + fileName) : "[文件]"
        }
        if (msgType === "call") {
            return fileName && fileName.length > 0 ? ("[" + fileName + "]") : "[通话邀请]"
        }
        return content
    }
    readonly property string rowIdentity: msgId + "|" + msgType + "|" + content + "|" + fileName
    property int topSpacing: (showAvatar ? 8 : 2) + (showSenderName ? 16 : 0)
    property int bottomSpacing: 2
    readonly property bool imageBubble: msgType === "image"
    readonly property bool compactTextBubble: msgType === "text" && !isReply
    readonly property int bubbleHorizontalPadding: imageBubble ? 4 : (compactTextBubble ? 6 : 8)
    readonly property int bubbleVerticalPadding: imageBubble ? 4 : (compactTextBubble ? 6 : 8)
    readonly property real bubbleMaxWidth: Math.max(120, width - avatarSlotWidth - 20)
    readonly property real bubbleMinWidth: imageBubble ? 92 : (compactTextBubble ? 24 : 56)
    readonly property real bubbleContentMaxWidth: Math.max(72, bubbleMaxWidth - (bubbleHorizontalPadding * 2 + 2))
    readonly property real imageContentMaxWidth: Math.min(bubbleContentMaxWidth, 280)
    readonly property real imageContentMaxHeight: 240
    // Single source of truth for how wide an image body may render. The bubble
    // layout reserves this width (see bodyPreferredWidth), so the image body
    // must use the same cap.
    readonly property real imageContentRenderWidth: Math.min(imageContentMaxWidth, 180)
    readonly property real replyPreferredWidth: root.isReply
        ? Math.min(root.bubbleContentMaxWidth,
                   Math.max(replySenderMeasure.advanceWidth, replyPreviewMeasure.advanceWidth) + 10)
        : 0
    readonly property real bodyPreferredWidth: {
        if (root.msgType === "image") {
            return root.imageContentRenderWidth
        }
        if (root.msgType === "file") {
            return Math.min(root.bubbleContentMaxWidth, fileMeasure.advanceWidth + 34)
        }
        if (root.msgType === "call") {
            return Math.min(root.bubbleContentMaxWidth, 220)
        }
        return Math.min(root.bubbleContentMaxWidth, textMeasure.advanceWidth)
    }
    readonly property real bubblePreferredWidth: Math.min(
        root.bubbleMaxWidth,
        Math.max(root.bubbleMinWidth,
                 Math.max(root.replyPreferredWidth, root.bodyPreferredWidth) + root.bubbleHorizontalPadding * 2))
    readonly property real bubbleX: root.outgoing
        ? Math.max(0, root.width - root.avatarSlotWidth - root.bubblePreferredWidth)
        : root.avatarSlotWidth
    readonly property real bubbleY: root.timeDividerHeight + root.topSpacing
    readonly property real translationPreferredWidth: Math.min(
        root.bubbleMaxWidth,
        Math.max(120, translationTextMeasure.advanceWidth + 22))
    readonly property real messageHeight: Math.max(bubble.implicitHeight, showAvatar ? avatarSize : 0)
    readonly property int translationHeight: meta.translationBlockHeight
    readonly property int timeDividerHeight: root.showTimeDivider ? 32 : 0
    readonly property int stateLabelHeight: meta.stateBlockHeight

    height: timeDividerHeight + topSpacing + messageHeight + translationHeight + stateLabelHeight + bottomSpacing

    onRowIdentityChanged: resetTransientState()

    function resetTransientState() {
        if (actionMenu.opened) {
            actionMenu.close()
        }
    }

    function openActionMenuAtBubblePoint(pointX, pointY) {
        const boundaryItem = ListView.view ? ListView.view : root
        const listPoint = bubble.mapToItem(boundaryItem, pointX, pointY)
        actionMenu.parent = boundaryItem
        actionMenu.openAt(listPoint.x, listPoint.y, boundaryItem.width, boundaryItem.height)
    }

    // Text width measurers keep bubble sizing independent from rendered Text
    // implicitWidth, which avoids feedback from a reused row's visual item.
    TextMetrics {
        id: textMeasure
        text: root.content
        font.pixelSize: 14
    }

    TextMetrics {
        id: fileMeasure
        text: "[FILE] " + (root.fileName.length > 0 ? root.fileName : "文件")
        font.pixelSize: 14
    }

    TextMetrics {
        id: replySenderMeasure
        text: root.replySender.length > 0 ? ("回复 " + root.replySender) : "回复"
        font.pixelSize: 11
        font.bold: true
    }

    TextMetrics {
        id: replyPreviewMeasure
        text: root.replyPreview
        font.pixelSize: 11
    }

    TextMetrics {
        id: translationTextMeasure
        text: root.translationText
        font.pixelSize: 13
    }

    Rectangle {
        visible: root.showTimeDivider
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: timeDividerLabel.implicitWidth + 20
        height: 22
        radius: height / 2
        color: Qt.rgba(0.32, 0.41, 0.53, 0.18)
        border.color: Qt.rgba(1, 1, 1, 0.40)

        Text {
            id: timeDividerLabel
            anchors.centerIn: parent
            text: root.timeDividerText
            color: "#5f7087"
            font.pixelSize: 11
        }
    }

    Item {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: root.timeDividerHeight + root.topSpacing
        height: root.messageHeight

        MessageAvatar {
            anchors.left: parent.left
            width: root.avatarSlotWidth
            height: parent.height
            visible: !root.outgoing && root.showAvatar
            alignRight: false
            avatarSize: root.avatarSize
            avatarSource: root.avatarSource
            defaultAvatarSource: root.defaultAvatarSource
            senderUid: root.senderUid
            senderName: root.senderName
            senderUserId: root.senderUserId
            onAvatarClicked: function(uid, name, icon, userId) { root.avatarClicked(uid, name, icon, userId) }
        }

        MessageAvatar {
            anchors.right: parent.right
            width: root.avatarSlotWidth
            height: parent.height
            visible: root.outgoing && root.showAvatar
            alignRight: true
            avatarSize: root.avatarSize
            avatarSource: root.avatarSource
            defaultAvatarSource: root.defaultAvatarSource
            senderUid: root.senderUid
            senderName: root.senderName
            senderUserId: root.senderUserId
            onAvatarClicked: function(uid, name, icon, userId) { root.avatarClicked(uid, name, icon, userId) }
        }
    }

    ChatMessageBubble {
        id: bubble
        bubbleWidth: root.bubblePreferredWidth
        contentMaxWidth: root.bubbleContentMaxWidth
        imageMaxWidth: root.imageContentRenderWidth
        imageMaxHeight: root.imageContentMaxHeight
        horizontalPadding: root.bubbleHorizontalPadding
        verticalPadding: root.bubbleVerticalPadding
        outgoing: root.outgoing
        isReply: root.isReply
        msgType: root.msgType
        content: root.content
        fileName: root.fileName
        replySender: root.replySender
        replyPreview: root.replyPreview
        enableContextMenu: root.enableContextMenu
        x: root.bubbleX
        y: root.bubbleY
        onOpenUrlRequested: function(url) { root.openUrlRequested(url) }
        onContextMenuRequested: function(pointX, pointY) { root.openActionMenuAtBubblePoint(pointX, pointY) }
    }

    ChatMessageMeta {
        id: meta
        anchors.fill: parent
        outgoing: root.outgoing
        showSenderName: root.showSenderName
        senderName: root.senderName
        translationText: root.translationText
        messageState: root.messageState
        bubbleX: bubble.x
        bubbleY: bubble.y
        bubbleWidth: bubble.width
        bubbleHeight: bubble.height
        contentMaxWidth: root.bubbleContentMaxWidth
        translationPreferredWidth: root.translationPreferredWidth
    }

    ChatMessageActionMenu {
        id: actionMenu
        parent: root
        msgId: root.msgId
        msgType: root.msgType
        content: root.content
        senderName: root.senderName
        previewText: root.previewForReply
        canReply: root.canReply
        canMention: root.canMention
        canForward: root.canForward
        canEdit: root.canEdit
        canRevoke: root.canRevoke
        showRevokeExpiredHint: root.showRevokeExpiredHint
        onReplyRequested: function(msgId, senderName, previewText) { root.replyRequested(msgId, senderName, previewText) }
        onMentionRequested: function(mentionText) { root.mentionRequested(mentionText) }
        onForwardRequested: function(msgId) { root.forwardRequested(msgId) }
        onEditRequested: function(msgId, text) { root.editRequested(msgId, text) }
        onRevokeRequested: function(msgId) { root.revokeRequested(msgId) }
        onTranslateRequested: function(msgId, text) { root.translateRequested(msgId, text) }
    }
}
