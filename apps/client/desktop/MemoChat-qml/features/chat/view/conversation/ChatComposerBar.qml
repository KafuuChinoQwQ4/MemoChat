import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"
import "qrc:/features/agent/view"
import "."

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
    property var imeBridgeController: null
    property bool pendingDraftSync: false
    property string pendingDraftText: ""
    property string pendingDraftBaseText: ""
    property bool inputMethodUpdatePending: false
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

    function focusMessageInput() {
        messageInput.forceActiveFocus()
        scheduleInputMethodUpdate()
    }

    function applyDraftText(text) {
        if (messageInput.text === text) {
            return
        }
        syncingDraftText = true
        messageInput.text = text
        messageInput.cursorPosition = messageInput.length
        syncingDraftText = false
        scheduleInputMethodUpdate()
    }

    function syncDraftTextFromBinding() {
        if (messageInput.activeFocus && messageInput.preeditText.length > 0) {
            pendingDraftBaseText = messageInput.text
            pendingDraftText = draftText
            pendingDraftSync = true
            return
        }
        pendingDraftSync = false
        pendingDraftBaseText = ""
        applyDraftText(draftText)
    }

    function scheduleInputMethodUpdate() {
        if (!messageInput.activeFocus || inputMethodUpdatePending) {
            return
        }
        inputMethodUpdatePending = true
        Qt.callLater(function() {
            root.inputMethodUpdatePending = false
            if (messageInput.activeFocus) {
                Qt.inputMethod.update(Qt.ImQueryAll)
            }
        })
    }

    function showTransientTip(message) {
        root.transientTipText = message
        transientTipTimer.restart()
    }

    onDraftTextChanged: syncDraftTextFromBinding()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ComposerIconButton {
                iconSource: "qrc:/icons/emoji.png"
                iconActive: root.enabledComposer
                enabled: root.enabledComposer && !root.mediaUploadInProgress
                onClicked: root.insertEmoji("😀")
            }

            ComposerIconButton {
                iconSource: "qrc:/icons/image.png"
                iconActive: root.enabledComposer
                enabled: root.enabledComposer && !root.mediaUploadInProgress
                onClicked: root.sendImage()
            }

            ComposerIconButton {
                iconSource: "qrc:/icons/file.png"
                iconActive: root.enabledComposer
                enabled: root.enabledComposer && !root.mediaUploadInProgress
                onClicked: root.sendFile()
            }

            ComposerIconButton {
                iconSource: "qrc:/icons/phone.png"
                iconActive: root.enabledComposer
                enabled: root.enabledComposer && !root.mediaUploadInProgress
                onClicked: {
                    if (root.isGroupChat) {
                        root.showTransientTip("群聊通话暂未开放")
                        return
                    }
                    root.sendVoiceCall()
                }
            }

            ComposerIconButton {
                iconSource: "qrc:/icons/video.png"
                iconActive: root.enabledComposer
                enabled: root.enabledComposer && !root.mediaUploadInProgress
                onClicked: {
                    if (root.isGroupChat) {
                        root.showTransientTip("群聊通话暂未开放")
                        return
                    }
                    root.sendVideoCall()
                }
            }

            Item {
                Layout.fillWidth: true
            }

            SmartFeatureBar {
                Layout.preferredWidth: 84
                Layout.preferredHeight: 30
                compactMode: true
                backdrop: root.backdrop !== null ? root.backdrop : root
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

        ChatComposerReplyBar {
            hasReplyContext: root.hasReplyContext
            replyTargetName: root.replyTargetName
            replyPreviewText: root.replyPreviewText
            backdrop: root.backdrop !== null ? root.backdrop : root
            onClearRequested: root.clearReplyRequested()
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

                ChatComposerAttachmentStrip {
                    pendingAttachments: root.pendingAttachments
                    mediaUploadInProgress: root.mediaUploadInProgress
                    onRemoveRequested: function(attachmentId) { root.removePendingAttachment(attachmentId) }
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
                        inputMethodHints: Qt.ImhMultiLine
                        background: Item { }
                        onActiveFocusChanged: {
                            if (activeFocus) {
                                root.scheduleInputMethodUpdate()
                            }
                        }
                        onCursorPositionChanged: root.scheduleInputMethodUpdate()
                        onCursorRectangleChanged: root.scheduleInputMethodUpdate()
                        onPreeditTextChanged: {
                            root.scheduleInputMethodUpdate()
                            if (preeditText.length > 0) {
                                return
                            }
                            if (root.pendingDraftSync) {
                                const shouldApplyPendingDraft = text === root.pendingDraftBaseText
                                if (shouldApplyPendingDraft) {
                                    root.applyDraftText(root.pendingDraftText)
                                }
                                root.pendingDraftSync = false
                                root.pendingDraftBaseText = ""
                            }
                            if (!root.syncingDraftText && root.draftText !== text) {
                                root.draftEdited(text)
                            }
                        }
                        onTextChanged: {
                            root.scheduleInputMethodUpdate()
                            if (!root.syncingDraftText && preeditText.length === 0) {
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
