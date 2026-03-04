import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Item {
    id: root

    property Item backdrop: null
    property bool enabledComposer: false
    property bool isGroupChat: false
    property string draftText: ""
    property bool syncingDraftText: false
    property string transientTipText: ""
    property bool hasReplyContext: false
    property string replyTargetName: ""
    property string replyPreviewText: ""
    signal sendText(string text)
    signal sendImage()
    signal sendFile()
    signal sendVoiceCall()
    signal sendVideoCall()
    signal draftEdited(string text)
    signal clearReplyRequested()

    function insertMention(text) {
        if (!root.enabledComposer || !text || text.length === 0) {
            return
        }
        const pos = messageInput.cursorPosition
        const source = messageInput.text
        const before = source.substring(0, pos)
        const after = source.substring(pos)
        messageInput.text = before + text + after
        messageInput.cursorPosition = pos + text.length
        root.draftEdited(messageInput.text)
    }

    function insertEmoji(emojiText) {
        if (!root.enabledComposer || !emojiText || emojiText.length === 0) {
            return
        }
        const pos = messageInput.cursorPosition
        const source = messageInput.text
        const before = source.substring(0, pos)
        const after = source.substring(pos)
        messageInput.text = before + emojiText + after
        messageInput.cursorPosition = pos + emojiText.length
        root.draftEdited(messageInput.text)
    }

    function showTransientTip(message) {
        root.transientTipText = message
        transientTipTimer.restart()
    }

    onDraftTextChanged: {
        if (messageInput.text === draftText) {
            return
        }
        syncingDraftText = true
        messageInput.text = draftText
        messageInput.cursorPosition = messageInput.length
        syncingDraftText = false
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ToolButton {
                id: emojiButton
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                enabled: root.enabledComposer
                hoverEnabled: true
                scale: down ? 0.96 : (hovered ? 1.02 : 1.0)
                onClicked: root.insertEmoji("😀")

                Behavior on scale {
                    NumberAnimation {
                        duration: 110
                        easing.type: Easing.OutQuad
                    }
                }

                HoverHandler {
                    cursorShape: emojiButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                }

                contentItem: Image {
                    source: "qrc:/icons/emoji.png"
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: 16
                    sourceSize.height: 16
                    opacity: root.enabledComposer ? 0.95 : 0.45
                }

                background: Rectangle {
                    radius: 8
                    color: !emojiButton.enabled ? Qt.rgba(1, 1, 1, 0.08)
                                                : emojiButton.down ? Qt.rgba(0.35, 0.61, 0.90, 0.30)
                                                                   : emojiButton.hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                                                                                         : "transparent"

                    Behavior on color {
                        ColorAnimation {
                            duration: 110
                            easing.type: Easing.OutQuad
                        }
                    }
                }
            }

            ToolButton {
                id: imageButton
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                enabled: root.enabledComposer
                hoverEnabled: true
                scale: down ? 0.96 : (hovered ? 1.02 : 1.0)
                onClicked: root.sendImage()

                Behavior on scale {
                    NumberAnimation {
                        duration: 110
                        easing.type: Easing.OutQuad
                    }
                }

                HoverHandler {
                    cursorShape: imageButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                }

                contentItem: Image {
                    source: "qrc:/icons/image.png"
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: 16
                    sourceSize.height: 16
                    opacity: root.enabledComposer ? 0.95 : 0.45
                }

                background: Rectangle {
                    radius: 8
                    color: !imageButton.enabled ? Qt.rgba(1, 1, 1, 0.08)
                                                : imageButton.down ? Qt.rgba(0.35, 0.61, 0.90, 0.30)
                                                                   : imageButton.hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                                                                                         : "transparent"

                    Behavior on color {
                        ColorAnimation {
                            duration: 110
                            easing.type: Easing.OutQuad
                        }
                    }
                }
            }

            ToolButton {
                id: fileButton
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                enabled: root.enabledComposer
                hoverEnabled: true
                scale: down ? 0.96 : (hovered ? 1.02 : 1.0)
                onClicked: root.sendFile()

                Behavior on scale {
                    NumberAnimation {
                        duration: 110
                        easing.type: Easing.OutQuad
                    }
                }

                HoverHandler {
                    cursorShape: fileButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                }

                contentItem: Image {
                    source: "qrc:/icons/file.png"
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: 16
                    sourceSize.height: 16
                    opacity: root.enabledComposer ? 0.95 : 0.45
                }

                background: Rectangle {
                    radius: 8
                    color: !fileButton.enabled ? Qt.rgba(1, 1, 1, 0.08)
                                               : fileButton.down ? Qt.rgba(0.35, 0.61, 0.90, 0.30)
                                                                 : fileButton.hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                                                                                      : "transparent"

                    Behavior on color {
                        ColorAnimation {
                            duration: 110
                            easing.type: Easing.OutQuad
                        }
                    }
                }
            }

            ToolButton {
                id: voiceButton
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                enabled: root.enabledComposer
                hoverEnabled: true
                scale: down ? 0.96 : (hovered ? 1.02 : 1.0)
                onClicked: {
                    if (root.isGroupChat) {
                        root.showTransientTip("群聊通话暂未开放")
                        return
                    }
                    root.sendVoiceCall()
                }

                Behavior on scale {
                    NumberAnimation {
                        duration: 110
                        easing.type: Easing.OutQuad
                    }
                }

                HoverHandler {
                    cursorShape: voiceButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                }

                contentItem: Image {
                    source: "qrc:/icons/phone.png"
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: 16
                    sourceSize.height: 16
                    opacity: root.enabledComposer ? 0.95 : 0.45
                }

                background: Rectangle {
                    radius: 8
                    color: !voiceButton.enabled ? Qt.rgba(1, 1, 1, 0.08)
                                                : voiceButton.down ? Qt.rgba(0.35, 0.61, 0.90, 0.30)
                                                                   : voiceButton.hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                                                                                         : "transparent"

                    Behavior on color {
                        ColorAnimation {
                            duration: 110
                            easing.type: Easing.OutQuad
                        }
                    }
                }
            }

            ToolButton {
                id: videoButton
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                enabled: root.enabledComposer
                hoverEnabled: true
                scale: down ? 0.96 : (hovered ? 1.02 : 1.0)
                onClicked: {
                    if (root.isGroupChat) {
                        root.showTransientTip("群聊通话暂未开放")
                        return
                    }
                    root.sendVideoCall()
                }

                Behavior on scale {
                    NumberAnimation {
                        duration: 110
                        easing.type: Easing.OutQuad
                    }
                }

                HoverHandler {
                    cursorShape: videoButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                }

                contentItem: Image {
                    source: "qrc:/icons/video.png"
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: 16
                    sourceSize.height: 16
                    opacity: root.enabledComposer ? 0.95 : 0.45
                }

                background: Rectangle {
                    radius: 8
                    color: !videoButton.enabled ? Qt.rgba(1, 1, 1, 0.08)
                                                : videoButton.down ? Qt.rgba(0.35, 0.61, 0.90, 0.30)
                                                                   : videoButton.hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                                                                                         : "transparent"

                    Behavior on color {
                        ColorAnimation {
                            duration: 110
                            easing.type: Easing.OutQuad
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }

        Label {
            Layout.fillWidth: true
            visible: root.transientTipText.length > 0
            text: root.transientTipText
            color: "#b24f4f"
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        GlassSurface {
            Layout.fillWidth: true
            Layout.preferredHeight: root.hasReplyContext ? 44 : 0
            visible: root.hasReplyContext
            backdrop: root.backdrop !== null ? root.backdrop : root
            cornerRadius: 9
            blurRadius: 14
            fillColor: Qt.rgba(0.30, 0.39, 0.52, 0.18)
            strokeColor: Qt.rgba(0.61, 0.74, 0.92, 0.36)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 8
                spacing: 8

                Rectangle {
                    Layout.preferredWidth: 3
                    Layout.fillHeight: true
                    radius: 2
                    color: Qt.rgba(0.40, 0.66, 0.96, 0.74)
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1
                    Label {
                        text: replyTargetName.length > 0 ? ("回复 " + replyTargetName) : "回复"
                        color: "#38547b"
                        font.pixelSize: 12
                        font.bold: true
                    }
                    Label {
                        text: replyPreviewText
                        color: "#556d89"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                ToolButton {
                    id: clearReplyButton
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    hoverEnabled: true
                    scale: down ? 0.96 : (hovered ? 1.02 : 1.0)
                    onClicked: root.clearReplyRequested()

                    Behavior on scale {
                        NumberAnimation {
                            duration: 110
                            easing.type: Easing.OutQuad
                        }
                    }

                    HoverHandler {
                        cursorShape: clearReplyButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    }
                    contentItem: Label {
                        text: "x"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: "#4d5f76"
                        font.pixelSize: 13
                    }
                    background: Rectangle {
                        radius: 7
                        color: clearReplyButton.down ? Qt.rgba(1, 1, 1, 0.34)
                                                     : clearReplyButton.hovered ? Qt.rgba(1, 1, 1, 0.24)
                                                                               : Qt.rgba(1, 1, 1, 0.16)
                        border.color: Qt.rgba(1, 1, 1, 0.30)

                        Behavior on color {
                            ColorAnimation {
                                duration: 110
                                easing.type: Easing.OutQuad
                            }
                        }
                    }
                }
            }
        }

        GlassSurface {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 106
            backdrop: root.backdrop !== null ? root.backdrop : root
            cornerRadius: 10
            blurRadius: 18
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
                onTextChanged: {
                    if (!root.syncingDraftText) {
                        root.draftEdited(text)
                    }
                }
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
                    root.draftEdited("")
                }
            }
        }
    }

    Timer {
        id: transientTipTimer
        interval: 2000
        repeat: false
        onTriggered: root.transientTipText = ""
    }
}
