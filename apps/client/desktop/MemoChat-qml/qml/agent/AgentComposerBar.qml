import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root
    color: "transparent"

    property bool enabledComposer: true

    signal sendComposer(string text)
    signal newSession()

    function submitComposer() {
        const text = textEdit.text.trim()
        if (!root.enabledComposer || text.length === 0) {
            return
        }
        root.sendComposer(text)
        textEdit.text = ""
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.32)
            border.color: Qt.rgba(1, 1, 1, 0.44)
            border.width: 1

            ScrollView {
                anchors.fill: parent
                anchors.margins: 10
                clip: true

                TextEdit {
                    id: textEdit
                    width: parent.width
                    height: Math.max(parent.height, paintedHeight + 8)
                    wrapMode: TextEdit.Wrap
                    color: root.enabledComposer ? "#253247" : "#8b97aa"
                    selectionColor: "#7baee899"
                    selectedTextColor: "#ffffff"
                    font.pixelSize: 14
                    readOnly: !root.enabledComposer
                    property bool hasText: length > 0

                    Text {
                        anchors.fill: parent
                        text: "输入消息，Enter 发送，Shift+Enter 换行"
                        color: Qt.rgba(106, 123, 146, 0.64)
                        font: parent.font
                        visible: !parent.hasText && !parent.activeFocus
                        wrapMode: Text.Wrap
                    }

                    Keys.onReturnPressed: function(event) {
                        if (event.modifiers & Qt.ShiftModifier) {
                            event.accepted = false
                            return
                        }
                        root.submitComposer()
                        event.accepted = true
                    }

                    Keys.onEnterPressed: function(event) {
                        if (event.modifiers & Qt.ShiftModifier) {
                            event.accepted = false
                            return
                        }
                        root.submitComposer()
                        event.accepted = true
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label {
                Layout.fillWidth: true
                text: root.enabledComposer ? "保持输入简洁，必要时可分多轮提问。" : "AI 正在处理，请稍候。"
                color: "#6a7b92"
                font.pixelSize: 12
                elide: Text.ElideRight
            }

            GlassButton {
                Layout.preferredWidth: 88
                Layout.preferredHeight: 34
                text: "发送"
                textPixelSize: 13
                cornerRadius: 9
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.28)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.38)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.48)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                enabled: root.enabledComposer && textEdit.length > 0
                onClicked: root.submitComposer()
            }
        }
    }
}
