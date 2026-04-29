import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root

    property string msgId: ""
    property string content: ""
    property string role: "assistant"
    property bool isUser: false
    property bool isAssistant: false
    property bool isStreaming: false
    property string streamingContent: ""
    property string thinkingContent: ""
    property string errorMessage: ""
    property int createdAt: 0
    property string sourcesJson: ""
    property real avatarSize: 34
    property real avatarSlotWidth: 42
    property string selfAvatar: "qrc:/res/head_1.jpg"
    property string aiAvatar: "qrc:/res/ai_icon.png"
    property bool manualThinkingExpanded: false

    readonly property string displayText: {
        if (root.errorMessage.length > 0) {
            return root.errorMessage
        }
        return root.isStreaming ? root.streamingContent : root.content
    }
    readonly property bool hasThinking: root.thinkingContent.length > 0 && !root.isUser
    readonly property bool showThinkingBody: root.hasThinking && (root.isStreaming || root.manualThinkingExpanded)
    readonly property real bubbleMaxWidth: Math.max(120, width - avatarSlotWidth - 28)
    readonly property real bubbleContentMaxWidth: Math.max(80, Math.min(520, bubbleMaxWidth - 18))
    readonly property real bubbleInnerWidth: Math.max(40, bubbleWidth - 14)
    readonly property real bubbleMinWidth: root.isStreaming && displayText.length === 0 ? 90 : 36
    readonly property real thinkingPreferredWidth: root.hasThinking
        ? Math.min(root.bubbleContentMaxWidth,
                   Math.max(220, thinkingHeaderTitle.implicitWidth + thinkingStateLabel.implicitWidth + 58))
        : 0
    readonly property real answerPreferredWidth: contentText.visible ? contentText.implicitWidth + 18 : 0
    readonly property real bubbleWidth: Math.min(bubbleMaxWidth, Math.max(bubbleMinWidth, thinkingPreferredWidth, answerPreferredWidth))
    readonly property real bubbleHeight: bubbleColumn.implicitHeight + 14

    width: ListView.view ? ListView.view.width : 320
    height: Math.max(avatarSize, bubbleHeight) + 14

    onMsgIdChanged: {
        manualThinkingExpanded = false
    }

    onIsStreamingChanged: {
        if (!isStreaming) {
            manualThinkingExpanded = false
        }
    }

    Item {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 6
        height: parent.height - 12

        Rectangle {
            id: leftAvatar
            width: root.avatarSize
            height: root.avatarSize
            radius: width / 2
            clip: true
            anchors.left: parent.left
            anchors.top: parent.top
            visible: !root.isUser
            color: Qt.rgba(0.73, 0.82, 0.92, 0.44)
            border.color: Qt.rgba(1, 1, 1, 0.48)

            Image {
                anchors.fill: parent
                source: root.aiAvatar
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
            }
        }

        Rectangle {
            id: rightAvatar
            width: root.avatarSize
            height: root.avatarSize
            radius: width / 2
            clip: true
            anchors.right: parent.right
            anchors.top: parent.top
            visible: root.isUser
            color: Qt.rgba(0.73, 0.82, 0.92, 0.44)
            border.color: Qt.rgba(1, 1, 1, 0.48)

            Image {
                anchors.fill: parent
                source: root.selfAvatar
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
            }
        }

        Rectangle {
            id: bubble
            width: root.bubbleWidth
            height: root.bubbleHeight
            radius: 10
            anchors.top: parent.top
            anchors.left: root.isUser ? undefined : parent.left
            anchors.leftMargin: root.isUser ? 0 : root.avatarSlotWidth
            anchors.right: root.isUser ? parent.right : undefined
            anchors.rightMargin: root.isUser ? root.avatarSlotWidth : 0
            color: {
                if (root.errorMessage.length > 0) {
                    return Qt.rgba(0.89, 0.27, 0.27, 0.14)
                }
                return root.isUser ? Qt.rgba(0.62, 0.80, 1.0, 0.52) : Qt.rgba(1, 1, 1, 0.46)
            }
            border.width: 1
            border.color: {
                if (root.errorMessage.length > 0) {
                    return Qt.rgba(0.89, 0.27, 0.27, 0.28)
                }
                return root.isUser ? Qt.rgba(0.44, 0.67, 0.95, 0.76) : Qt.rgba(1, 1, 1, 0.58)
            }

            Column {
                id: bubbleColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 7
                spacing: 6

                Rectangle {
                    id: thinkingPanel
                    width: root.bubbleInnerWidth
                    height: thinkingHeader.height + (root.showThinkingBody ? thinkingText.implicitHeight + 16 : 0)
                    radius: 8
                    color: Qt.rgba(0.54, 0.60, 0.68, 0.12)
                    border.width: 1
                    border.color: Qt.rgba(0.54, 0.60, 0.68, 0.16)
                    visible: root.hasThinking
                    clip: true

                    Column {
                        anchors.fill: parent
                        spacing: 0

                        Item {
                            id: thinkingHeader
                            width: parent.width
                            height: 30

                            Label {
                                id: thinkingHeaderTitle
                                anchors.left: parent.left
                                anchors.leftMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                text: "思考过程"
                                color: "#738095"
                                font.pixelSize: 12
                                font.bold: true
                            }

                            Label {
                                id: thinkingStateLabel
                                anchors.left: thinkingHeaderTitle.right
                                anchors.leftMargin: 8
                                anchors.right: thinkingToggle.left
                                anchors.rightMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.isStreaming ? "思考中" : (root.manualThinkingExpanded ? "已展开" : "已折叠")
                                color: "#8d98aa"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }

                            Label {
                                id: thinkingToggle
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.isStreaming ? "自动展开" : (root.showThinkingBody ? "收起" : "展开")
                                color: root.isStreaming ? "#8d98aa" : "#5f7fae"
                                font.pixelSize: 11
                                font.bold: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: !root.isStreaming
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.manualThinkingExpanded = !root.manualThinkingExpanded
                            }
                        }

                        TextEdit {
                            id: thinkingText
                            width: parent.width - 20
                            x: 10
                            visible: root.showThinkingBody
                            text: root.thinkingContent
                            wrapMode: TextEdit.Wrap
                            color: "#8893a5"
                            font.pixelSize: 12
                            textFormat: Text.PlainText
                            readOnly: true
                            selectByMouse: true
                            cursorVisible: false
                            leftPadding: 0
                            rightPadding: 0
                            topPadding: 0
                            bottomPadding: 8
                        }
                    }
                }

                TextEdit {
                    id: contentText
                    width: root.bubbleInnerWidth
                    visible: root.displayText.length > 0 || root.thinkingContent.length === 0
                    text: root.displayText.length > 0 ? root.displayText : (root.isStreaming ? "正在生成" : "")
                    wrapMode: TextEdit.Wrap
                    color: root.errorMessage.length > 0 ? "#c14d4d" : (root.isUser ? "#20334f" : "#253247")
                    font.pixelSize: 14
                    textFormat: Text.PlainText
                    readOnly: true
                    selectByMouse: true
                    cursorVisible: false
                    leftPadding: 0
                    rightPadding: 0
                    topPadding: 0
                    bottomPadding: 0
                }

                Rectangle {
                    width: streamLabel.implicitWidth + 14
                    height: 22
                    radius: 11
                    color: Qt.rgba(1, 1, 1, root.isUser ? 0.16 : 0.48)
                    visible: root.isStreaming && root.errorMessage.length === 0

                    Label {
                        id: streamLabel
                        anchors.centerIn: parent
                        text: root.thinkingContent.length > 0 && root.displayText.length === 0 ? "正在思考" : "正在生成回复"
                        font.pixelSize: 11
                        font.bold: true
                        color: root.isUser ? "#36567e" : "#6a7b92"
                    }
                }

                Rectangle {
                    width: root.bubbleInnerWidth
                    height: sourceLabel.implicitHeight + 12
                    radius: 7
                    color: Qt.rgba(0.54, 0.70, 0.93, 0.10)
                    border.width: 1
                    border.color: Qt.rgba(0.54, 0.70, 0.93, 0.16)
                    visible: root.sourcesJson.length > 0 && !root.isUser

                    Label {
                        id: sourceLabel
                        anchors.fill: parent
                        anchors.margins: 6
                        text: "参考: " + root.sourcesJson
                        font.pixelSize: 11
                        color: "#5373a4"
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }
}
