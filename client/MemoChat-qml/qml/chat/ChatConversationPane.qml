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
    property var messageModel
    signal sendText(string text)
    signal sendImage()
    signal sendFile()
    signal openAttachment(string url)

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
                model: root.messageModel
                ScrollBar.vertical: GlassScrollBar { }
                onCountChanged: {
                    if (count > 0) {
                        positionViewAtEnd()
                    }
                }

                delegate: ChatMessageDelegate {
                    width: messageList.width
                    outgoing: model.outgoing
                    msgType: model.msgType
                    content: model.content
                    fileName: model.fileName
                    senderName: model.senderName
                    showAvatar: model.showAvatar
                    avatarSource: model.outgoing ? root.selfAvatar : root.peerAvatar
                    onOpenUrlRequested: root.openAttachment(url)
                }
            }

            GlassSurface {
                anchors.centerIn: parent
                width: 210
                height: 86
                visible: messageList.count === 0
                backdrop: root.backdrop !== null ? root.backdrop : root
                cornerRadius: 10
                blurRadius: 28
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
                anchors.fill: parent
                backdrop: root.backdrop
                enabledComposer: root.hasCurrentChat
                onSendText: root.sendText(text)
                onSendImage: root.sendImage()
                onSendFile: root.sendFile()
            }
        }
    }
}
