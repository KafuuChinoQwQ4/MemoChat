import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Item {
    id: root
    width: ListView.view ? ListView.view.width : 300

    property bool outgoing: false
    property string msgId: ""
    property string msgType: "text"
    property string content: ""
    property string rawContent: ""
    property string fileName: ""
    property string senderName: ""
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
    signal openUrlRequested(string url)
    signal replyRequested(string msgId, string senderName, string previewText)
    signal mentionRequested(string mentionText)
    signal forwardRequested(string msgId)
    signal editRequested(string msgId, string text)
    signal revokeRequested(string msgId)

    property int avatarSize: 34
    property int avatarSlotWidth: 42
    readonly property bool showSenderName: (!outgoing && senderName.length > 0 && showAvatar)
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
    readonly property bool compactTextBubble: msgType === "text" && !isReply
    readonly property int bubbleHorizontalPadding: compactTextBubble ? 6 : 8
    readonly property int bubbleVerticalPadding: compactTextBubble ? 6 : 8
    readonly property real bubbleMaxWidth: Math.max(120, width - avatarSlotWidth - 20)
    readonly property real bubbleMinWidth: compactTextBubble ? 24 : 56
    readonly property real bubbleContentMaxWidth: Math.max(72, bubbleMaxWidth - (bubbleHorizontalPadding * 2 + 2))
    readonly property real messageHeight: Math.max(bubble.implicitHeight, showAvatar ? avatarSize : 0)
    readonly property bool showStateLabel: (root.outgoing && root.messageState !== "sent")
                                           || (!root.outgoing && (root.messageState === "edited" || root.messageState === "deleted"))
    readonly property int timeDividerHeight: root.showTimeDivider ? 32 : 0
    readonly property int stateLabelHeight: showStateLabel ? 16 : 0

    height: timeDividerHeight + topSpacing + messageHeight + stateLabelHeight + bottomSpacing

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

        Item {
            id: leftSlot
            anchors.left: parent.left
            width: root.avatarSlotWidth
            height: parent.height

            Rectangle {
                width: root.avatarSize
                height: root.avatarSize
                radius: width / 2
                clip: true
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                visible: !root.outgoing && root.showAvatar
                color: Qt.rgba(0.73, 0.82, 0.92, 0.50)
                border.color: Qt.rgba(1, 1, 1, 0.56)

                Image {
                    anchors.fill: parent
                    property string baseSource: root.avatarSource.length > 0 ? root.avatarSource : root.defaultAvatarSource
                    property bool loadFailed: false
                    source: loadFailed ? root.defaultAvatarSource : baseSource
                    fillMode: Image.PreserveAspectCrop
                    onBaseSourceChanged: loadFailed = false
                    onStatusChanged: {
                        if (status === Image.Error) {
                            loadFailed = true
                        }
                    }
                }
            }
        }

        Item {
            id: rightSlot
            anchors.right: parent.right
            width: root.avatarSlotWidth
            height: parent.height

            Rectangle {
                width: root.avatarSize
                height: root.avatarSize
                radius: width / 2
                clip: true
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                visible: root.outgoing && root.showAvatar
                color: Qt.rgba(0.73, 0.82, 0.92, 0.50)
                border.color: Qt.rgba(1, 1, 1, 0.56)

                Image {
                    anchors.fill: parent
                    property string baseSource: root.avatarSource.length > 0 ? root.avatarSource : root.defaultAvatarSource
                    property bool loadFailed: false
                    source: loadFailed ? root.defaultAvatarSource : baseSource
                    fillMode: Image.PreserveAspectCrop
                    onBaseSourceChanged: loadFailed = false
                    onStatusChanged: {
                        if (status === Image.Error) {
                            loadFailed = true
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: bubble
        width: Math.min(root.bubbleMaxWidth, Math.max(root.bubbleMinWidth, bubbleColumn.implicitWidth + root.bubbleHorizontalPadding * 2))
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
                menu.x = Math.max(0, Math.min(root.width - menu.width, eventPoint.position.x))
                menu.y = Math.max(0, Math.min(root.height - menu.height, eventPoint.position.y))
                menu.open()
            }
        }
    }

    Text {
        visible: root.showSenderName
        anchors.left: bubble.left
        anchors.bottom: bubble.top
        anchors.bottomMargin: 2
        text: root.senderName
        color: "#4f6078"
        font.pixelSize: 11
    }

    Text {
        visible: root.showStateLabel
        anchors.right: bubble.right
        anchors.top: bubble.bottom
        anchors.topMargin: 3
        text: {
            if (root.messageState === "sending") {
                return "发送中..."
            }
            if (root.messageState === "failed") {
                return "发送失败"
            }
            if (root.messageState === "read") {
                return "已读"
            }
            if (root.messageState === "edited") {
                return "已编辑"
            }
            if (root.messageState === "deleted") {
                return "已撤回"
            }
            return ""
        }
        color: {
            if (root.messageState === "sending") {
                return "#6c7d92"
            }
            if (root.messageState === "failed") {
                return "#c74747"
            }
            return "#6c7d92"
        }
        font.pixelSize: 11
    }

    Component {
        id: textComp
        Text {
            text: root.content
            width: Math.min(root.bubbleContentMaxWidth, implicitWidth)
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            color: "#213045"
            font.pixelSize: 14
        }
    }

    Component {
        id: imageComp
        Item {
            implicitWidth: img.status === Image.Ready ? Math.min(root.bubbleContentMaxWidth, img.implicitWidth) : 200
            implicitHeight: img.status === Image.Ready ? Math.min(160, img.implicitHeight) : 120

            Image {
                id: img
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                source: root.content
            }
        }
    }

    Component {
        id: fileComp
        Rectangle {
            id: fileItem
            property bool hovering: fileArea.containsMouse
            property bool pressed: fileArea.pressed
            color: fileItem.pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                                    : fileItem.hovering ? Qt.rgba(0.35, 0.61, 0.90, 0.12)
                                                        : "transparent"
            implicitWidth: Math.min(root.bubbleContentMaxWidth, fileText.implicitWidth + 34)
            implicitHeight: fileText.implicitHeight + 8
            radius: 6
            scale: fileItem.pressed ? 0.96 : (fileItem.hovering ? 1.02 : 1.0)

            Behavior on color {
                ColorAnimation {
                    duration: 110
                    easing.type: Easing.OutQuad
                }
            }
            Behavior on scale {
                NumberAnimation {
                    duration: 110
                    easing.type: Easing.OutQuad
                }
            }

            Text {
                id: fileText
                anchors.verticalCenter: parent.verticalCenter
                text: "[FILE] " + (root.fileName.length > 0 ? root.fileName : "文件")
                color: fileItem.pressed ? "#2d5f96" : (fileItem.hovering ? "#366ea9" : "#233247")
                font.pixelSize: 14
                font.underline: fileItem.hovering
                wrapMode: Text.Wrap

                Behavior on color {
                    ColorAnimation {
                        duration: 110
                        easing.type: Easing.OutQuad
                    }
                }
            }

            MouseArea {
                id: fileArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.openUrlRequested(root.content)
            }
        }
    }

    Component {
        id: callComp
        Rectangle {
            color: "transparent"
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

    Menu {
        id: menu
        parent: bubble
        transformOrigin: Item.TopLeft

        MenuItem {
            visible: root.canReply
            text: "回复"
            onTriggered: root.replyRequested(root.msgId, root.senderName, root.previewForReply)
        }
        MenuItem {
            visible: root.canMention
            text: "@Ta"
            onTriggered: root.mentionRequested("@" + root.senderName + " ")
        }
        MenuItem {
            visible: root.canForward
            text: "转发"
            onTriggered: root.forwardRequested(root.msgId)
        }
        MenuItem {
            visible: root.canEdit
            text: "编辑"
            onTriggered: root.editRequested(root.msgId, root.content)
        }
        MenuItem {
            visible: root.canRevoke
            text: "撤回"
            onTriggered: root.revokeRequested(root.msgId)
        }
    }
}
