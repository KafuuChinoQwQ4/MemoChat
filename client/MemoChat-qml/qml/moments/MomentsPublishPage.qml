import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "../components"

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
        visibilityCombo.currentIndex = 0
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

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 56

            GlassSurface {
                anchors.fill: parent
                backdrop: root.backdrop
                cornerRadius: 0
                blurRadius: 12
                fillColor: Qt.rgba(1, 1, 1, 0.82)
                strokeColor: Qt.rgba(0.85, 0.85, 0.90, 0.5)
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 14
                spacing: 8

                ToolButton {
                    text: "←"
                    font.pixelSize: 18
                    implicitWidth: 36
                    implicitHeight: 36
                    background: Rectangle {
                        radius: width / 2
                        color: parent.down ? "#dbe7f7" : Qt.rgba(1, 1, 1, 0.86)
                        border.width: 1
                        border.color: "#d5dfec"
                    }
                    contentItem: Label {
                        text: "←"
                        color: "#2d3b4f"
                        font.pixelSize: 18
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.backRequested()
                }

                ColumnLayout {
                    spacing: 1
                    Label {
                        text: "发布朋友圈"
                        font.pixelSize: 17
                        font.weight: Font.Medium
                        color: "#1c2027"
                    }
                    Label {
                        text: attachmentsModel.count > 0 ? ("已选 " + attachmentsModel.count + " 个素材") : ""
                        font.pixelSize: 11
                        color: "#79818f"
                        visible: text.length > 0
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    id: publishButton
                    text: loading ? "发布中..." : "发布"
                    enabled: root.hasDraftContent && !loading
                    implicitHeight: 38
                    leftPadding: 18
                    rightPadding: 18
                    topPadding: 7
                    bottomPadding: 7
                    background: Rectangle {
                        radius: 19
                        color: publishButton.enabled ? "#2b6ff2" : "#b9c4d6"
                        border.width: 1
                        border.color: publishButton.enabled ? "#2a67de" : "#b9c4d6"
                    }
                    contentItem: Label {
                        text: publishButton.text
                        color: "#ffffff"
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        root.statusText = ""
                        root.statusIsError = false
                        if (root.controller) {
                            root.controller.publishDraftMoment(
                                postInput.text,
                                visibilityCombo.currentIndex,
                                root.draftAttachments())
                        }
                    }
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

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 84
                    radius: 18
                    color: "#ffffff"
                    border.color: "#dce4ef"
                    border.width: 1

                    Flickable {
                        anchors.fill: parent
                        anchors.margins: 10
                        clip: true
                        contentWidth: thumbRow.width
                        contentHeight: height
                        boundsBehavior: Flickable.StopAtBounds

                        Row {
                            id: thumbRow
                            spacing: 8
                            height: parent.height

                            Repeater {
                                model: attachmentsModel
                                delegate: Rectangle {
                                    width: 62
                                    height: 62
                                    radius: 14
                                    color: index === root.selectedMediaIndex ? "#d8e6ff" : "#edf2f8"
                                    border.width: 1
                                    border.color: index === root.selectedMediaIndex ? "#4e87f6" : "#cfdae8"
                                    clip: true

                                    Image {
                                        anchors.fill: parent
                                        visible: type === "image"
                                        source: visible ? (previewUrl || fileUrl || "") : ""
                                        fillMode: Image.PreserveAspectCrop
                                        asynchronous: true
                                    }

                                    Rectangle {
                                        anchors.fill: parent
                                        visible: type === "video"
                                        color: "#243245"
                                        Label {
                                            anchors.centerIn: parent
                                            text: "▶"
                                            color: "#ffffff"
                                            font.pixelSize: 16
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.selectedMediaIndex = index
                                    }

                                    Rectangle {
                                        width: 18
                                        height: 18
                                        radius: 9
                                        anchors.top: parent.top
                                        anchors.right: parent.right
                                        anchors.margins: 4
                                        color: Qt.rgba(0.09, 0.12, 0.18, 0.78)

                                        Label {
                                            anchors.centerIn: parent
                                            text: "×"
                                            color: "#ffffff"
                                            font.pixelSize: 12
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: root.removeAttachment(index)
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                width: 62
                                height: 62
                                radius: 14
                                color: "#ffffff"
                                border.width: 1
                                border.color: "#c7d3e3"

                                Label {
                                    anchors.centerIn: parent
                                    text: "+"
                                    font.pixelSize: 28
                                    color: "#2b6ff2"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.pickMedia()
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 58
                    radius: 14
                    color: "#ffffff"
                    border.color: "#dce4ef"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        spacing: 10

                        Label {
                            text: "谁可见"
                            font.pixelSize: 14
                            color: "#202733"
                        }

                        Item { Layout.fillWidth: true }

                        ComboBox {
                            id: visibilityCombo
                            Layout.preferredWidth: 150
                            model: ["公开", "仅好友可见", "仅自己可见"]
                            currentIndex: 0
                            font.pixelSize: 13
                            leftPadding: 12
                            rightPadding: 28
                            topPadding: 6
                            bottomPadding: 6
                            background: Rectangle {
                                radius: 12
                                color: "#f8fbff"
                                border.width: 1
                                border.color: "#d5dfec"
                            }
                            contentItem: Text {
                                leftPadding: 0
                                rightPadding: 0
                                text: visibilityCombo.displayText
                                font: visibilityCombo.font
                                color: "#243246"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            indicator: Canvas {
                                x: visibilityCombo.width - width - 12
                                y: (visibilityCombo.height - height) / 2
                                width: 10
                                height: 6
                                contextType: "2d"
                                onPaint: {
                                    context.reset()
                                    context.moveTo(0, 0)
                                    context.lineTo(width, 0)
                                    context.lineTo(width / 2, height)
                                    context.closePath()
                                    context.fillStyle = "#607086"
                                    context.fill()
                                }
                            }
                            popup: Popup {
                                y: visibilityCombo.height + 6
                                width: visibilityCombo.width
                                padding: 6
                                background: Rectangle {
                                    radius: 14
                                    color: "#ffffff"
                                    border.width: 1
                                    border.color: "#d5dfec"
                                }
                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: contentHeight
                                    model: visibilityCombo.popup.visible ? visibilityCombo.delegateModel : null
                                    currentIndex: visibilityCombo.highlightedIndex
                                }
                            }
                            delegate: ItemDelegate {
                                width: visibilityCombo.width - 12
                                height: 38
                                text: modelData
                                font.pixelSize: 13
                                highlighted: visibilityCombo.highlightedIndex === index
                                background: Rectangle {
                                    radius: 10
                                    color: parent.highlighted ? "#eef4ff" : "transparent"
                                }
                                contentItem: Text {
                                    text: parent.text
                                    font: parent.font
                                    color: "#243246"
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 18
                }
            }
        }
    }
}
