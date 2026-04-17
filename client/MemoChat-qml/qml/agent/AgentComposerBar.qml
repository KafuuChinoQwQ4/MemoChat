import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root
    color: "transparent"

    property bool enabledComposer: true
    property string _text: ""
    property bool _atBottom: true

    signal sendComposer(string text)
    signal newSession()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 6

        // 功能按钮行
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 28

            GlassButton {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 28
                text: "+"
                textPixelSize: 14
                cornerRadius: 6
                normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.18)
                hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.28)
                pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.38)
                onClicked: filePicker.open()
            }

            Item { Layout.fillWidth: true }

            GlassButton {
                Layout.preferredWidth: 80
                Layout.preferredHeight: 28
                text: "新会话"
                textPixelSize: 12
                cornerRadius: 6
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.18)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.28)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.38)
                onClicked: root.newSession()
            }

            GlassButton {
                Layout.preferredWidth: 64
                Layout.preferredHeight: 28
                text: "📎"
                textPixelSize: 12
                cornerRadius: 6
                normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.18)
                hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.28)
                pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.38)
                onClicked: kbPicker.open()
            }
        }

        // 输入框
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: Qt.rgba(1, 1, 1, 0.30)
            border.color: Qt.rgba(1, 1, 1, 0.44)
            border.width: 1

            ScrollView {
                anchors.fill: parent
                anchors.margins: 8
                clip: true

                TextEdit {
                    id: textEdit
                    width: parent.width
                    height: Math.max(parent.height, 60)
                    wrapMode: TextEdit.Wrap
                    color: "#253247"
                    selectionColor: "#7baee899"
                    selectedTextColor: "#ffffff"
                    font.pixelSize: 14
                    property string placeholder: "输入消息... (Enter 发送, Shift+Enter 换行)"
                    property bool _hasText: length > 0

                    Text {
                        anchors.fill: parent
                        text: parent.placeholder
                        color: Qt.rgba(106, 123, 146, 0.6)
                        font: parent.font
                        visible: !parent._hasText && !parent.activeFocus
                        wrapMode: TextEdit.Wrap
                    }

                    onTextChanged: root._text = text
                }
            }
        }

        // 发送按钮
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            spacing: 8

            Item { Layout.fillWidth: true }

            GlassButton {
                Layout.preferredWidth: 88
                Layout.preferredHeight: 32
                text: "发送"
                textPixelSize: 13
                cornerRadius: 8
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.28)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.38)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.48)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                enabled: root.enabledComposer && textEdit.length > 0
                onClicked: {
                    var text = textEdit.text.trim()
                    if (text.length > 0) {
                        root.sendComposer(text)
                        textEdit.text = ""
                    }
                }
            }
        }
    }

    MouseArea {
        id: filePicker
        visible: false
        onClicked: {
            if (root.agentController) {
                var filePath = "/path/to/file"  // 在实际实现中调用 QML 文件选择器
            }
        }
    }

    MouseArea {
        id: kbPicker
        visible: false
    }

    Keys.onReturnPressed: {
        if (event.modifiers & Qt.ShiftModifier) {
            event.accepted = false
        } else {
            var text = textEdit.text.trim()
            if (text.length > 0) {
                root.sendComposer(text)
                textEdit.text = ""
            }
            event.accepted = true
        }
    }

    Keys.onEnterPressed: Keys.onReturnPressed
}
