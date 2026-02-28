import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    property bool enabledComposer: false
    signal sendText(string text)
    signal sendImage()
    signal sendFile()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "图片"
                enabled: root.enabledComposer
                onClicked: root.sendImage()
            }

            Button {
                text: "文件"
                enabled: root.enabledComposer
                onClicked: root.sendFile()
            }

            Item { Layout.fillWidth: true }
        }

        TextArea {
            id: messageInput
            Layout.fillWidth: true
            Layout.fillHeight: true
            placeholderText: "输入消息..."
            enabled: root.enabledComposer
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: "发送"
            enabled: root.enabledComposer && messageInput.text.trim().length > 0
            onClicked: {
                root.sendText(messageInput.text)
                messageInput.clear()
            }
        }
    }
}
