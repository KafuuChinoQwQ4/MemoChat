import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "transparent"

    property string msgId: ""
    property string content: ""
    property string role: "assistant"
    property bool isUser: false
    property bool isAssistant: false
    property bool isStreaming: false
    property string streamingContent: ""
    property int createdAt: 0
    property string sourcesJson: ""

    property real avatarSize: 36
    property string selfAvatar: "qrc:/res/head_1.jpg"
    property string aiAvatar: "qrc:/res/ai_icon.png"

    Rectangle {
        id: bubble
        anchors.left: root.isUser ? parent.right : parent.left
        anchors.leftMargin: root.isUser ? 0 : 12
        anchors.right: root.isUser ? undefined : parent.right
        anchors.rightMargin: root.isUser ? 0 : 60
        anchors.top: parent.top
        anchors.topMargin: 4
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4
        radius: 12
        color: {
            if (root.isUser) return Qt.rgba(0.35, 0.61, 0.90, 0.85)
            return Qt.rgba(0.94, 0.96, 0.98, 0.95)
        }

        border.color: Qt.rgba(1, 1, 1, 0.4)
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 4

            TextEdit {
                Layout.fillWidth: true
                text: root.isStreaming ? root.streamingContent : root.content
                wrapMode: Text.Wrap
                color: root.isUser ? "#ffffff" : "#253247"
                font.pixelSize: 14
                textFormat: Text.PlainText
                readOnly: true
                selectByMouse: true
                cursorVisible: false
                onTextChanged: {
                    if (root.isStreaming) {
                        Qt.callLater(function() {
                            root.forceActiveFocus()
                        })
                    }
                }

                Label {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: -4
                    anchors.bottomMargin: -4
                    text: root.isStreaming ? "..." : ""
                    font.pixelSize: 10
                    color: root.isUser ? Qt.rgba(255,255,255,0.6) : "#6a7b92"
                    visible: root.isStreaming
                }
            }

            // RAG 来源标注
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: sourceLabel.height + 8
                visible: root.sourcesJson.length > 0 && !root.isUser
                radius: 4
                color: Qt.rgba(0.54, 0.70, 0.93, 0.15)

                Label {
                    id: sourceLabel
                    anchors.fill: parent
                    anchors.margins: 4
                    text: "📚 " + root.sourcesJson
                    font.pixelSize: 11
                    color: "#4a6fa5"
                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                    maximumLineCount: 2
                }
            }
        }

        // 气泡尖角
        Rectangle {
            anchors.left: root.isUser ? parent.right : undefined
            anchors.leftMargin: root.isUser ? -6 : undefined
            anchors.right: root.isUser ? undefined : parent.left
            anchors.rightMargin: root.isUser ? undefined : -6
            anchors.verticalCenter: parent.verticalCenter
            width: 8
            height: 14
            rotation: root.isUser ? -45 : 45
            visible: false
        }
    }

    // 头像
    Rectangle {
        anchors.right: root.isUser ? parent.right : undefined
        anchors.rightMargin: root.isUser ? 8 : undefined
        anchors.left: root.isUser ? undefined : parent.left
        anchors.leftMargin: root.isUser ? undefined : 4
        anchors.top: parent.top
        anchors.topMargin: 4
        width: root.avatarSize
        height: root.avatarSize
        radius: root.avatarSize / 2
        clip: true

        Image {
            anchors.fill: parent
            source: root.isUser ? root.selfAvatar : root.aiAvatar
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
        }
    }

    // 时间戳
    Label {
        anchors.top: bubble.bottom
        anchors.topMargin: 2
        anchors.right: root.isUser ? parent.right : undefined
        anchors.rightMargin: root.isUser ? 8 : undefined
        anchors.left: root.isUser ? undefined : bubble.right
        anchors.leftMargin: root.isUser ? undefined : 8
        text: root.createdAt > 0 ? formatTime(root.createdAt) : ""
        font.pixelSize: 10
        color: "#6a7b92"
        visible: root.createdAt > 0
    }

    function formatTime(timestamp) {
        var date = new Date(timestamp * 1000)
        var h = date.getHours()
        var m = date.getMinutes()
        return (h < 10 ? "0" : "") + h + ":" + (m < 10 ? "0" : "") + m
    }
}
