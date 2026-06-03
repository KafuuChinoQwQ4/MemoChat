import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "qrc:/qml/components"

Item {
    id: root

    property var backdrop: null
    property var controller: null
    property int selectedMediaIndex: 0
    property string statusText: ""
    property bool statusIsError: false
    property bool loading: root.controller ? root.controller.loading : false
    property bool hasDraftContent: postInput.text.trim().length > 0 || attachmentsModel.count > 0

    signal backRequested()
    signal publishRequested()

    function resetForm() {
        postInput.text = ""
        attachmentsModel.clear()
        visibilitySelector.currentIndex = 0
        selectedMediaIndex = 0
        statusText = ""
        statusIsError = false
    }

    function appendPickedAttachments(picked) {
        if (!picked || picked.length === 0) {
            return
        }
        for (var i = 0; i < picked.length; ++i) {
            attachmentsModel.append(picked[i])
        }
        if (attachmentsModel.count > 0 && selectedMediaIndex >= attachmentsModel.count) {
            selectedMediaIndex = attachmentsModel.count - 1
        }
        if (attachmentsModel.count === picked.length) {
            selectedMediaIndex = 0
        }
    }

    function pickMedia() {
        statusText = ""
        statusIsError = false
        if (!root.controller) {
            return
        }
        var picked = root.controller.pickMomentMedia()
        if (picked && picked.length > 0) {
            appendPickedAttachments(picked)
            return
        }
        if (root.controller.errorText && root.controller.errorText.length > 0) {
            statusText = root.controller.errorText
            statusIsError = true
        }
    }

    function removeAttachment(index) {
        if (index < 0 || index >= attachmentsModel.count) {
            return
        }
        attachmentsModel.remove(index)
        if (attachmentsModel.count === 0) {
            selectedMediaIndex = 0
            return
        }
        if (selectedMediaIndex >= attachmentsModel.count) {
            selectedMediaIndex = attachmentsModel.count - 1
        } else if (index < selectedMediaIndex) {
            selectedMediaIndex = Math.max(0, selectedMediaIndex - 1)
        }
    }

    function draftAttachments() {
        var list = []
        for (var i = 0; i < attachmentsModel.count; ++i) {
            list.push(attachmentsModel.get(i))
        }
        return list
    }

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        cornerRadius: 16
        blurRadius: 20
        fillColor: Qt.rgba(0.95, 0.95, 0.97, 0.90)
        strokeColor: Qt.rgba(0.8, 0.8, 0.85, 0.6)
    }

    ListModel {
        id: attachmentsModel
    }

    Connections {
        target: root.controller
        function onProgressTextChanged() {
            if (!root.controller) {
                return
            }
            if (root.controller.progressText && root.controller.progressText.length > 0) {
                root.statusText = root.controller.progressText
                root.statusIsError = false
            }
        }
        function onPublishError(msg) {
            root.statusText = msg
            root.statusIsError = true
        }
        function onPublishSuccess() {
            root.statusText = ""
            root.statusIsError = false
            root.publishRequested()
            root.resetForm()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

            MomentsPublishHeader {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                backdrop: root.backdrop
                attachmentCount: attachmentsModel.count
                loading: root.loading
                publishEnabled: root.hasDraftContent
                onBackRequested: root.backRequested()
                onPublishRequested: {
                    root.statusText = ""
                    root.statusIsError = false
                    if (root.controller) {
                        root.controller.publishDraftMoment(
                            postInput.text,
                            visibilitySelector.currentIndex,
                            root.draftAttachments())
                    }
                }
            }

        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 16
            clip: true
            ScrollBar.vertical.interactive: true

            ColumnLayout {
                width: scrollView.availableWidth
                spacing: 14

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 228
                    radius: 18
                    color: "#ffffff"
                    border.color: "#dce4ef"
                    border.width: 1

                    TextArea {
                        id: postInput
                        anchors.fill: parent
                        anchors.margins: 12
                        placeholderText: "写点什么..."
                        placeholderTextColor: "#9aa5b5"
                        wrapMode: TextEdit.Wrap
                        selectByMouse: true
                        font.pixelSize: 15
                        color: "#1a1f28"
                        background: Rectangle {
                            radius: 14
                            color: "#f8fbff"
                            border.color: "#dce4ef"
                        }
                        padding: 14
                    }
                }

                MomentsAttachmentStrip {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 84
                    attachmentsModel: attachmentsModel
                    selectedMediaIndex: root.selectedMediaIndex
                    onSelectedMediaIndexChangedRequested: function(index) {
                        root.selectedMediaIndex = index
                    }
                    onRemoveAttachmentRequested: function(index) {
                        root.removeAttachment(index)
                    }
                    onPickMediaRequested: root.pickMedia()
                }

                MomentsVisibilitySelector {
                    id: visibilitySelector
                    Layout.fillWidth: true
                    Layout.preferredHeight: 58
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 18
                }
            }
        }
    }
}
