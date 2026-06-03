import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root

    property string draftText: ""
    property bool controllerAvailable: false
    property bool controllerBusy: false
    property bool voiceCallActive: false
    property bool videoCallActive: false
    property string modelStatusText: "AI API"

    signal draftEdited(string text)
    signal sendRequested(string text)
    signal voiceCallToggled()
    signal videoCallToggled()

    Layout.fillWidth: true
    Layout.preferredHeight: 94
    radius: 12
    antialiasing: true
    color: Qt.rgba(1, 1, 1, 0.30)
    border.color: Qt.rgba(1, 1, 1, 0.48)

    function clear() {
        messageInput.clear()
    }

    function forceInputFocus() {
        messageInput.forceActiveFocus()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            CompactIconButton {
                iconSource: "qrc:/icons/emoji.png"
                enabled: root.controllerAvailable && !root.controllerBusy
                onClicked: {
                    messageInput.insert(messageInput.cursorPosition, "😀")
                    root.draftEdited(messageInput.text)
                }
            }

            CompactIconButton {
                iconSource: "qrc:/icons/phone.png"
                checked: root.voiceCallActive
                enabled: root.controllerAvailable && !root.controllerBusy
                onClicked: root.voiceCallToggled()
            }

            CompactIconButton {
                iconSource: "qrc:/icons/video.png"
                checked: root.videoCallActive
                enabled: root.controllerAvailable && !root.controllerBusy
                onClicked: root.videoCallToggled()
            }

            Label {
                Layout.fillWidth: true
                text: root.modelStatusText.length > 0 ? root.modelStatusText : "AI API"
                color: "#7a8798"
                font.pixelSize: 10
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideLeft
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            TextArea {
                id: messageInput
                Layout.fillWidth: true
                Layout.fillHeight: true
                placeholderText: "输入消息..."
                enabled: root.controllerAvailable && !root.controllerBusy
                text: root.draftText
                color: "#253247"
                placeholderTextColor: "#7f8b9b"
                selectionColor: "#7baee899"
                selectedTextColor: "#ffffff"
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                font.pixelSize: 13
                inputMethodHints: Qt.ImhMultiLine
                background: Item { }
                Keys.onReturnPressed: function(event) {
                    if (!(event.modifiers & Qt.ShiftModifier)) {
                        root.sendRequested(messageInput.text)
                        event.accepted = true
                    }
                }
                Keys.onEnterPressed: function(event) {
                    if (!(event.modifiers & Qt.ShiftModifier)) {
                        root.sendRequested(messageInput.text)
                        event.accepted = true
                    }
                }
                onTextChanged: {
                    if (root.draftText !== text) {
                        root.draftEdited(text)
                    }
                }
            }

            Button {
                id: sendButton
                Layout.alignment: Qt.AlignBottom
                Layout.preferredWidth: 64
                Layout.preferredHeight: 32
                enabled: root.controllerAvailable
                         && !root.controllerBusy
                         && messageInput.text.trim().length > 0
                text: "发送"
                padding: 0
                onClicked: root.sendRequested(messageInput.text)

                background: Rectangle {
                    radius: 9
                    antialiasing: true
                    color: !sendButton.enabled ? Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                               : sendButton.down ? Qt.rgba(0.35, 0.61, 0.90, 0.40)
                                                                 : sendButton.hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.30)
                                                                                      : Qt.rgba(0.35, 0.61, 0.90, 0.22)
                }

                contentItem: Text {
                    text: sendButton.text
                    color: sendButton.enabled ? "#38547b" : "#98a1ae"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
        }
    }

    component CompactIconButton: ToolButton {
        id: iconButton
        property string iconSource: ""

        Layout.preferredWidth: 26
        Layout.preferredHeight: 24
        hoverEnabled: true
        padding: 0

        contentItem: Image {
            source: iconButton.iconSource
            fillMode: Image.PreserveAspectFit
            sourceSize.width: 15
            sourceSize.height: 15
            opacity: iconButton.enabled ? 0.90 : 0.36
            mipmap: true
        }

        background: Rectangle {
            radius: 7
            antialiasing: true
            color: !iconButton.enabled ? Qt.rgba(1, 1, 1, 0.08)
                                      : iconButton.down ? Qt.rgba(0.35, 0.61, 0.90, 0.30)
                                                        : iconButton.checked ? Qt.rgba(0.35, 0.61, 0.90, 0.24)
                                                                             : iconButton.hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.16)
                                                                                                  : "transparent"
        }
    }
}
