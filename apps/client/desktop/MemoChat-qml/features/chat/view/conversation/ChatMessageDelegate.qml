pragma ComponentBehavior: Bound

import QtQuick 2.15
import "qrc:/qml/components"
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
    signal avatarClicked(int uid, string name, string icon)

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
    // MUST use the same cap — otherwise a portrait image fits to the 240 height
    // and a wider 280 cap, stretching the bubble taller/narrower than its box.
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
    readonly property real translationPreferredWidth: Math.min(
        root.bubbleMaxWidth,
        Math.max(120, translationTextMeasure.advanceWidth + 22))
    readonly property real messageHeight: Math.max(bubble.implicitHeight, showAvatar ? avatarSize : 0)
    readonly property int translationHeight: translationText.length > 0 ? (translationBubble.implicitHeight + 6) : 0
    readonly property int timeDividerHeight: root.showTimeDivider ? 32 : 0
    readonly property int stateLabelHeight: stateBadge.active ? 16 : 0

    height: timeDividerHeight + topSpacing + messageHeight + translationHeight + stateLabelHeight + bottomSpacing

    function openActionMenuAtBubblePoint(pointX, pointY) {
        const boundaryItem = ListView.view ? ListView.view : root
        const listPoint = bubble.mapToItem(boundaryItem, pointX, pointY)
        actionMenu.parent = boundaryItem
        actionMenu.openAt(listPoint.x, listPoint.y, boundaryItem.width, boundaryItem.height)
    }

    // Text width measurers. TextMetrics computes width SYNCHRONOUSLY when text
    // changes, unlike a hidden Text whose implicitWidth updates on a deferred
    // scene-graph polish pass. With ListView reuseItems:true, fast scrolling
    // rebinds a recycled delegate faster than a Text would re-layout, so a short
    // message briefly sampled the previous (longer) message's stale implicitWidth
    // and the bubble rendered stretched. TextMetrics has no such transient.
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
            onAvatarClicked: function(uid, name, icon) { root.avatarClicked(uid, name, icon) }
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
            onAvatarClicked: function(uid, name, icon) { root.avatarClicked(uid, name, icon) }
        }
    }

    Rectangle {
        id: bubble
        width: root.bubblePreferredWidth
        height: implicitHeight
        implicitHeight: bubbleColumn.implicitHeight + root.bubbleVerticalPadding * 2
        radius: 10
        color: root.outgoing ? Qt.rgba(0.62, 0.80, 1.0, 0.52) : Qt.rgba(1, 1, 1, 0.50)
        border.color: root.outgoing ? Qt.rgba(0.44, 0.67, 0.95, 0.82) : Qt.rgba(1, 1, 1, 0.66)
        anchors.top: parent.top
        anchors.topMargin: root.timeDividerHeight + root.topSpacing
        anchors.right: root.outgoing ? parent.right : undefined
        anchors.rightMargin: root.outgoing ? root.avatarSlotWidth : 0
        anchors.left: root.outgoing ? undefined : parent.left
        anchors.leftMargin: root.outgoing ? 0 : root.avatarSlotWidth

        Column {
            id: bubbleColumn
            anchors.fill: parent
            anchors.leftMargin: root.bubbleHorizontalPadding
            anchors.rightMargin: root.bubbleHorizontalPadding
            anchors.topMargin: root.bubbleVerticalPadding
            anchors.bottomMargin: root.bubbleVerticalPadding
            spacing: root.isReply ? 6 : 0

            Rectangle {
                visible: root.isReply
                width: Math.min(root.bubbleContentMaxWidth, replyColumn.implicitWidth + 10)
                height: replyColumn.implicitHeight + 8
                radius: 6
                color: Qt.rgba(0.25, 0.33, 0.46, 0.14)
                border.color: Qt.rgba(0.44, 0.60, 0.82, 0.40)

                Column {
                    id: replyColumn
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 1

                    Text {
                        text: root.replySender.length > 0 ? ("回复 " + root.replySender) : "回复"
                        color: "#4f6788"
                        font.pixelSize: 11
                        font.bold: true
                    }
                    Text {
                        text: root.replyPreview
                        color: "#60718a"
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                        elide: Text.ElideRight
                        maximumLineCount: 2
                    }
                }
            }

            Loader {
                id: contentItem
                sourceComponent: {
                    if (root.msgType === "image") {
                        return imageComp
                    }
                    if (root.msgType === "file") {
                        return fileComp
                    }
                    if (root.msgType === "call") {
                        return callComp
                    }
                    return textComp
                }
            }
        }

        TapHandler {
            acceptedButtons: Qt.RightButton
            enabled: root.enableContextMenu
            onTapped: function(eventPoint, button) {
                if (button !== Qt.RightButton) {
                    return
                }
                if (!root.enableContextMenu) {
                    return
                }
                root.openActionMenuAtBubblePoint(eventPoint.position.x, eventPoint.position.y)
            }
        }
    }

    Text {
        visible: root.showSenderName
        anchors.left: root.outgoing ? undefined : bubble.left
        anchors.right: root.outgoing ? bubble.right : undefined
        anchors.bottom: bubble.top
        anchors.bottomMargin: 2
        text: root.senderName
        color: "#4f6078"
        font.pixelSize: 11
    }

    Rectangle {
        id: translationBubble
        visible: root.translationText.length > 0
        width: root.translationPreferredWidth
        height: implicitHeight
        implicitHeight: translationColumn.implicitHeight + 14
        radius: 9
        color: Qt.rgba(0.89, 0.94, 1.0, 0.54)
        border.color: Qt.rgba(0.42, 0.62, 0.86, 0.38)
        anchors.top: bubble.bottom
        anchors.topMargin: 6
        anchors.right: root.outgoing ? parent.right : undefined
        anchors.rightMargin: root.outgoing ? root.avatarSlotWidth : 0
        anchors.left: root.outgoing ? undefined : parent.left
        anchors.leftMargin: root.outgoing ? 0 : root.avatarSlotWidth

        Column {
            id: translationColumn
            anchors.fill: parent
            anchors.margins: 7
            spacing: 3

            Text {
                text: "翻译"
                color: "#4f6788"
                font.pixelSize: 10
                font.bold: true
            }

            Text {
                id: translationTextItem
                text: root.translationText
                width: Math.min(root.bubbleContentMaxWidth, implicitWidth)
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                color: "#253247"
                font.pixelSize: 13
            }
        }
    }

    ChatMessageStatusBadge {
        id: stateBadge
        outgoing: root.outgoing
        messageState: root.messageState
        anchors.right: bubble.right
        anchors.top: root.translationText.length > 0 ? translationBubble.bottom : bubble.bottom
        anchors.topMargin: 3
    }

    Component {
        id: textComp
        ChatMessageTextBody {
            messageText: root.content
            maxWidth: root.bubbleContentMaxWidth
        }
    }

    Component {
        id: imageComp
        ChatMessageImageBody {
            imageSource: root.content
            maxWidth: root.imageContentRenderWidth
            maxHeight: root.imageContentMaxHeight
        }
    }

    Component {
        id: fileComp
        ChatMessageFileBody {
            fileName: root.fileName
            maxWidth: root.bubbleContentMaxWidth
            width: implicitWidth
            height: implicitHeight
            onOpenRequested: root.openUrlRequested(root.content)
        }
    }

    Component {
        id: callComp
        Rectangle {
            color: "transparent"
            width: implicitWidth
            height: implicitHeight
            implicitWidth: Math.min(root.bubbleContentMaxWidth, 220)
            implicitHeight: 62

            Column {
                anchors.fill: parent
                spacing: 6

                Text {
                    text: root.fileName.length > 0 ? root.fileName : "通话邀请"
                    color: "#233247"
                    font.pixelSize: 14
                    font.bold: true
                }

                GlassButton {
                    text: "加入通话"
                    implicitWidth: 92
                    implicitHeight: 30
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    enabled: root.content.length > 0
                    onClicked: root.openUrlRequested(root.content)
                }
            }
        }
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
