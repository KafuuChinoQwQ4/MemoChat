import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Item {
    id: root

    property Item backdrop: null
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

            ToolButton {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                enabled: root.enabledComposer
                hoverEnabled: true
                onClicked: root.sendImage()

                contentItem: Image {
                    source: "qrc:/icons/emoji.png"
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: 16
                    sourceSize.height: 16
                    opacity: root.enabledComposer ? 0.95 : 0.45
                }

                background: Item { }
            }

            ToolButton {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                enabled: root.enabledComposer
                hoverEnabled: true
                onClicked: root.sendFile()

                contentItem: Image {
                    source: "qrc:/icons/file.png"
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: 16
                    sourceSize.height: 16
                    opacity: root.enabledComposer ? 0.95 : 0.45
                }

                background: Item { }
            }

            Item { Layout.fillWidth: true }
        }

        GlassSurface {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 106
            backdrop: root.backdrop !== null ? root.backdrop : root
            cornerRadius: 10
            blurRadius: 30
            fillColor: Qt.rgba(1, 1, 1, 0.24)
            strokeColor: Qt.rgba(1, 1, 1, 0.46)
            glowTopColor: Qt.rgba(1, 1, 1, 0.20)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.02)

            TextArea {
                id: messageInput
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.topMargin: 12
                anchors.rightMargin: 106
                anchors.bottomMargin: 12
                placeholderText: "输入消息..."
                enabled: root.enabledComposer
                color: "#253247"
                selectionColor: "#7baee899"
                selectedTextColor: "#ffffff"
                wrapMode: Text.Wrap
                font.pixelSize: 17
                background: Item { }
            }

            GlassButton {
                id: sendButton
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 10
                anchors.bottomMargin: 10
                implicitWidth: 88
                implicitHeight: 34
                textPixelSize: 14
                cornerRadius: 9
                text: "发送"
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.26)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.36)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.44)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                enabled: root.enabledComposer && messageInput.text.trim().length > 0
                onClicked: {
                    root.sendText(messageInput.text)
                    messageInput.clear()
                }
            }
        }
    }
}
