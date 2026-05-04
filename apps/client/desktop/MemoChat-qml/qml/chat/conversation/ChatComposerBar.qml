import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"
import "../../agent"

Item {
    id: root

    property Item backdrop: null
    property bool enabledComposer: false
    property bool isGroupChat: false
    property bool mediaUploadInProgress: false
    property string mediaUploadProgressText: ""
    property string draftText: ""
    property var pendingAttachments: []
    property bool syncingDraftText: false
    property string transientTipText: ""
    property bool hasReplyContext: false
    property string replyTargetName: ""
    property string replyPreviewText: ""
    property var smartResult: null
    property bool smartBusy: false
    property string smartStatusText: ""
    property string smartResultTitle: "智能结果"
    readonly property bool hasPendingAttachmentItems: pendingAttachments && pendingAttachments.length > 0
    signal sendComposer(string text)
    signal sendImage()
    signal sendFile()
    signal removePendingAttachment(string attachmentId)
    signal sendVoiceCall()
    signal sendVideoCall()
    signal draftEdited(string text)
    signal clearReplyRequested()
    signal summaryRequested()
    signal suggestRequested()
    signal translateRequested()
    signal clearSmartResultRequested()

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

    function applySuggestedText(text) {
        if (!root.enabledComposer || !text || text.length === 0) {
            return
        }
        messageInput.text = text
        messageInput.cursorPosition = messageInput.text.length
        messageInput.forceActiveFocus()
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
        anchors.margins: 8
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ToolButton {
                id: emojiButton
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                enabled: root.enabledComposer && !root.mediaUploadInProgress
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
                enabled: root.enabledComposer && !root.mediaUploadInProgress
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
                enabled: root.enabledComposer && !root.mediaUploadInProgress
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
                enabled: root.enabledComposer && !root.mediaUploadInProgress
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
                enabled: root.enabledComposer && !root.mediaUploadInProgress
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

            SmartFeatureBar {
                Layout.preferredWidth: 256
                Layout.preferredHeight: 30
                compactMode: true
                smartResult: root.smartResult
                busy: root.smartBusy
                statusText: root.smartStatusText
                resultTitle: root.smartResultTitle
                onSummaryRequested: root.summaryRequested()
                onSuggestRequested: root.suggestRequested()
                onTranslateRequested: root.translateRequested()
                onClearResultRequested: root.clearSmartResultRequested()
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root.transientTipText.length > 0
            text: root.transientTipText
            color: "#b24f4f"
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        Label {
            Layout.fillWidth: true
            visible: root.mediaUploadInProgress
            text: root.mediaUploadProgressText.length > 0 ? root.mediaUploadProgressText : "上传中..."
            color: "#3e6f9f"
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
            Layout.minimumHeight: 86
            backdrop: root.backdrop !== null ? root.backdrop : root
            cornerRadius: 10
            blurRadius: 18
            fillColor: Qt.rgba(1, 1, 1, 0.16)
            strokeColor: Qt.rgba(1, 1, 1, 0.34)
            glowTopColor: Qt.rgba(1, 1, 1, 0.12)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.01)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.hasPendingAttachmentItems ? 76 : 0
                    visible: root.hasPendingAttachmentItems
                    radius: 10
                    color: Qt.rgba(0.88, 0.93, 0.98, 0.42)
                    border.color: Qt.rgba(0.49, 0.67, 0.89, 0.30)

                    Flickable {
                        anchors.fill: parent
                        anchors.margins: 8
                        clip: true
                        contentWidth: attachmentRow.implicitWidth
                        contentHeight: attachmentRow.height

                        Row {
                            id: attachmentRow
                            spacing: 8

                            Repeater {
                                model: root.pendingAttachments

                                delegate: Rectangle {
                                    width: modelData.type === "image" ? 96 : 164
                                    height: 52
                                    radius: 8
                                    color: Qt.rgba(1, 1, 1, 0.50)
                                    border.color: Qt.rgba(0.44, 0.61, 0.82, 0.28)

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 6
                                        anchors.rightMargin: 4
                                        spacing: 6

                                        Rectangle {
                                            Layout.preferredWidth: 40
                                            Layout.preferredHeight: 40
                                            radius: 6
                                            color: Qt.rgba(0.80, 0.88, 0.96, 0.65)
                                            border.color: Qt.rgba(1, 1, 1, 0.44)
                                            clip: true

                                            Image {
                                                anchors.fill: parent
                                                anchors.margins: modelData.type === "image" ? 0 : 10
                                                source: modelData.type === "image" ? modelData.previewUrl : "qrc:/icons/file.png"
                                                fillMode: modelData.type === "image" ? Image.PreserveAspectCrop : Image.PreserveAspectFit
                                                sourceSize.width: 80
                                                sourceSize.height: 80
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 1

                                            Label {
                                                Layout.fillWidth: true
                                                text: modelData.fileName
                                                color: "#2a3950"
                                                font.pixelSize: 12
                                                elide: Text.ElideMiddle
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: modelData.type === "image" ? "图片待发送" : "文件待发送"
                                                color: "#667b95"
                                                font.pixelSize: 11
                                                elide: Text.ElideRight
                                            }
                                        }

                                        ToolButton {
                                            id: removeButton
                                            Layout.preferredWidth: 24
                                            Layout.preferredHeight: 24
                                            enabled: !root.mediaUploadInProgress
                                            onClicked: root.removePendingAttachment(modelData.attachmentId)
                                            contentItem: Label {
                                                text: "x"
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                                color: "#516378"
                                                font.pixelSize: 12
                                            }
                                            background: Rectangle {
                                                radius: 6
                                                color: removeButton.down ? Qt.rgba(0.35, 0.61, 0.90, 0.18)
                                                                         : removeButton.hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.12)
                                                                                                : "transparent"
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 10

                    TextArea {
                        id: messageInput
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        placeholderText: "输入消息..."
                        enabled: root.enabledComposer && !root.mediaUploadInProgress
                        color: "#253247"
                        selectionColor: "#7baee899"
                        selectedTextColor: "#ffffff"
                        wrapMode: Text.Wrap
                        font.pixelSize: 15
                        background: Item { }
                        onTextChanged: {
                            if (!root.syncingDraftText) {
                                root.draftEdited(text)
                            }
                        }
                    }

                    GlassButton {
                        id: sendButton
                        Layout.alignment: Qt.AlignBottom
                        implicitWidth: 80
                        implicitHeight: 32
                        textPixelSize: 13
                        cornerRadius: 9
                        text: "发送"
                        normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.26)
                        hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.36)
                        pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.44)
                        disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                        enabled: root.enabledComposer
                                 && !root.mediaUploadInProgress
                                 && (messageInput.text.trim().length > 0 || root.hasPendingAttachmentItems)
                        onClicked: {
                            const shouldClearText = !root.hasPendingAttachmentItems
                            root.sendComposer(messageInput.text)
                            if (shouldClearText) {
                                messageInput.clear()
                                root.draftEdited("")
                            }
                        }
                    }
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
