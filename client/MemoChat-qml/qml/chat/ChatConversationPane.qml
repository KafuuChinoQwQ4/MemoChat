import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "conversation"

Rectangle {
    id: root
    color: "#f5f7fa"

    property string peerName: ""
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
            color: "#ffffff"
            border.color: "#e3e8ef"
            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 20
                text: root.peerName.length > 0 ? root.peerName : "聊天"
                color: "#2f3a4a"
                font.pixelSize: 18
                font.bold: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#f8fafc"
            border.color: "#e3e8ef"

            ListView {
                id: messageList
                anchors.fill: parent
                anchors.margins: 14
                spacing: 8
                clip: true
                model: root.messageModel
                onCountChanged: {
                    if (count > 0) {
                        positionViewAtEnd()
                    }
                }

                delegate: Item {
                    width: ListView.view.width
                    height: delegateRoot.height

                    ChatMessageDelegate {
                        id: delegateRoot
                        width: parent.width
                        outgoing: model.outgoing
                        msgType: model.msgType
                        content: model.content
                        fileName: model.fileName
                        onOpenUrlRequested: root.openAttachment(url)
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 140
            color: "#ffffff"
            border.color: "#e3e8ef"

            ChatComposerBar {
                anchors.fill: parent
                enabledComposer: root.hasCurrentChat
                onSendText: root.sendText(text)
                onSendImage: root.sendImage()
                onSendFile: root.sendFile()
            }
        }
    }
}
