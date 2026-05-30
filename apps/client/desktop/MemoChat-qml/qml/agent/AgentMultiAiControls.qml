import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root

    property bool gameBusy: false
    property bool roomReady: false

    signal sendRequested(string text)

    Layout.fillWidth: true
    Layout.preferredHeight: 112
    radius: 12
    color: Qt.rgba(1, 1, 1, 0.20)
    border.color: Qt.rgba(1, 1, 1, 0.44)

    function sendDraft() {
        const text = composerField.text.trim()
        if (text.length === 0 || !root.roomReady || root.gameBusy) {
            return
        }
        composerField.text = ""
        root.sendRequested(text)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        TextArea {
            id: composerField
            Layout.fillWidth: true
            Layout.fillHeight: true
            placeholderText: root.gameBusy ? "AI 正在回复..." : "输入要发给多个 AI 的消息"
            wrapMode: TextArea.Wrap
            selectByMouse: true
            enabled: !root.gameBusy && root.roomReady
            Keys.onPressed: function(event) {
                if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && (event.modifiers & Qt.ControlModifier)) {
                    root.sendDraft()
                    event.accepted = true
                }
            }
        }

        GlassButton {
            Layout.preferredWidth: 88
            Layout.fillHeight: true
            text: root.gameBusy ? "等待" : "发送"
            textPixelSize: 13
            cornerRadius: 10
            enabled: !root.gameBusy && composerField.text.trim().length > 0 && root.roomReady
            normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
            hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
            pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
            onClicked: root.sendDraft()
        }
    }
}
