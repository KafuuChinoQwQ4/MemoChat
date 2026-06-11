pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/features/chat/view/conversation"

Rectangle {
    id: root

    property var messageModel: null
    property string displayName: "Live2D"
    property string selfName: "我"
    property string selfAvatar: "qrc:/res/head_1.jpg"
    property string live2dAvatarSource: "qrc:/icons/modelive2d.png"
    property string live2dAvatarFallback: "qrc:/icons/modelive2d.png"
    readonly property int modelMessageCount: root.messageModel && root.messageModel.count !== undefined
                                             ? root.messageModel.count
                                             : 0

    Layout.fillWidth: true
    Layout.fillHeight: true
    radius: 12
    antialiasing: true
    color: Qt.rgba(1, 1, 1, 0.24)
    border.color: Qt.rgba(1, 1, 1, 0.50)

    function isOutgoingMessage(value) {
        return value === true || value === "true" || value === 1
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

    function messageSenderName(outgoing) {
        return outgoing ? root.selfName : root.displayName
    }

    function positionViewAtEnd() {
        messageList.positionViewAtEnd()
    }

    function scrollToEnd() {
        positionViewAtEnd()
    }

    ListView {
        id: messageList
        anchors.fill: parent
        anchors.margins: 10
        clip: true
        spacing: 0
        reuseItems: true
        cacheBuffer: Math.max(height, 720)
        model: root.messageModel
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
            visible: root.modelMessageCount === 0
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
