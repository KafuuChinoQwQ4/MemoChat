import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    property string msgId: ""
    property string content: ""
    property string role: "assistant"
    property bool isUser: false
    property bool isAssistant: false
    property bool isStreaming: false
    property string streamingContent: ""
    property int createdAt: 0
    property string sourcesJson: ""

    property real avatarSize: 34
    property string selfAvatar: "qrc:/res/head_1.jpg"
    property string aiAvatar: "qrc:/res/ai_icon.png"
    property real bubbleMaxWidth: Math.min(560, width * 0.72)

    implicitHeight: messageRow.implicitHeight + 10

    RowLayout {
        id: messageRow
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.topMargin: 5
        anchors.bottomMargin: 5
        spacing: 10
        layoutDirection: root.isUser ? Qt.RightToLeft : Qt.LeftToRight

        Rectangle {
            Layout.preferredWidth: root.avatarSize
            Layout.preferredHeight: root.avatarSize
            Layout.alignment: Qt.AlignTop
            radius: root.avatarSize / 2
            clip: true
            color: root.isUser ? Qt.rgba(0.43, 0.64, 0.90, 0.18) : Qt.rgba(0.91, 0.94, 0.98, 0.88)
            border.width: 1
            border.color: root.isUser ? Qt.rgba(0.35, 0.61, 0.90, 0.22) : Qt.rgba(1, 1, 1, 0.56)

            Image {
                anchors.fill: parent
                source: root.isUser ? root.selfAvatar : root.aiAvatar
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
            }
        }

        ColumnLayout {
            Layout.preferredWidth: root.bubbleMaxWidth
            Layout.maximumWidth: root.bubbleMaxWidth
            Layout.alignment: Qt.AlignTop
            spacing: 4

            Label {
                Layout.alignment: root.isUser ? Qt.AlignRight : Qt.AlignLeft
                text: root.isUser ? "你" : "AI 助手"
                color: "#718098"
                font.pixelSize: 11
                font.bold: true
            }

            Rectangle {
                Layout.fillWidth: true
                radius: 14
                color: root.isUser ? Qt.rgba(0.35, 0.61, 0.90, 0.92) : Qt.rgba(0.97, 0.98, 1.0, 0.92)
                border.width: 1
                border.color: root.isUser ? Qt.rgba(0.22, 0.48, 0.78, 0.36) : Qt.rgba(1, 1, 1, 0.54)

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

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
                        leftPadding: 0
                        rightPadding: 0
                        topPadding: 0
                        bottomPadding: 0
                    }

                    Rectangle {
                        Layout.alignment: Qt.AlignLeft
                        Layout.preferredHeight: 24
                        Layout.preferredWidth: streamLabel.implicitWidth + 14
                        radius: 12
                        color: Qt.rgba(1, 1, 1, root.isUser ? 0.14 : 0.55)
                        visible: root.isStreaming

                        Label {
                            id: streamLabel
                            anchors.centerIn: parent
                            text: "正在生成"
                            font.pixelSize: 11
                            font.bold: true
                            color: root.isUser ? Qt.rgba(255, 255, 255, 0.86) : "#6a7b92"
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 8
                        color: Qt.rgba(0.54, 0.70, 0.93, 0.10)
                        border.width: 1
                        border.color: Qt.rgba(0.54, 0.70, 0.93, 0.16)
                        visible: root.sourcesJson.length > 0 && !root.isUser

                        Label {
                            anchors.fill: parent
                            anchors.margins: 8
                            text: "参考: " + root.sourcesJson
                            font.pixelSize: 11
                            color: "#5373a4"
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }

            Label {
                Layout.alignment: root.isUser ? Qt.AlignRight : Qt.AlignLeft
                text: root.createdAt > 0 ? formatTime(root.createdAt) : ""
                color: "#93a0b2"
                font.pixelSize: 10
                visible: root.createdAt > 0
            }
        }

        Item {
            Layout.fillWidth: true
        }
    }

    function formatTime(timestamp) {
        var date = new Date(timestamp * 1000)
        var h = date.getHours()
        var m = date.getMinutes()
        return (h < 10 ? "0" : "") + h + ":" + (m < 10 ? "0" : "") + m
    }
}
