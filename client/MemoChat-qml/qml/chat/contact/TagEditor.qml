import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Item {
    id: root

    property Item backdrop: null
    property var selectedTags: []
    property var suggestionTags: [
        "Classmates", "Family", "Guide", "C++ Primer", "Rust",
        "Python", "Node.js", "Go", "Game", "Investment",
        "WeChatRead", "Shopping"
    ]
    signal tagsChanged(var tags)

    implicitHeight: editorLayout.implicitHeight

    function normalizeTag(tagText) {
        return tagText ? tagText.trim() : ""
    }

    function addTag(tagText) {
        const normalized = normalizeTag(tagText)
        if (normalized.length === 0) {
            return
        }
        if (root.selectedTags.indexOf(normalized) !== -1) {
            return
        }
        const next = root.selectedTags.slice(0)
        next.push(normalized)
        root.selectedTags = next
        root.tagsChanged(next)
    }

    function removeTag(tagText) {
        const idx = root.selectedTags.indexOf(tagText)
        if (idx < 0) {
            return
        }
        const next = root.selectedTags.slice(0)
        next.splice(idx, 1)
        root.selectedTags = next
        root.tagsChanged(next)
    }

    ColumnLayout {
        id: editorLayout
        anchors.fill: parent
        spacing: 8

        GlassTextField {
            id: inputField
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            backdrop: root.backdrop
            blurRadius: 30
            cornerRadius: 9
            textHorizontalAlignment: TextInput.AlignLeft
            leftInset: 12
            rightInset: 12
            textPixelSize: 13
            placeholderText: "搜索或添加标签"
            onAccepted: {
                root.addTag(text)
                text = ""
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: 6
            visible: root.selectedTags.length > 0

            Repeater {
                model: root.selectedTags
                delegate: Rectangle {
                    radius: 12
                    color: Qt.rgba(0.56, 0.85, 0.70, 0.30)
                    border.color: Qt.rgba(0.50, 0.79, 0.66, 0.56)
                    height: 24
                    width: tagLabel.implicitWidth + 30

                    Text {
                        id: tagLabel
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        text: modelData
                        color: "#2d3b50"
                        font.pixelSize: 12
                    }

                    Text {
                        id: removeLabel
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: 8
                        text: "\u00D7"
                        color: "#45566f"
                        font.pixelSize: 12
                    }

                    MouseArea {
                        anchors.fill: removeLabel
                        anchors.margins: -4
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.removeTag(modelData)
                    }
                }
            }
        }

        Label {
            text: "建议标签"
            color: "#5e7088"
            font.pixelSize: 12
        }

        Flow {
            Layout.fillWidth: true
            spacing: 6

            Repeater {
                model: root.suggestionTags
                delegate: GlassButton {
                    id: suggestionBtn
                    property bool selected: root.selectedTags.indexOf(modelData) !== -1
                    text: modelData
                    textPixelSize: 12
                    cornerRadius: 12
                    implicitHeight: 28
                    implicitWidth: tagMetrics.advanceWidth + 24
                    normalColor: selected ? Qt.rgba(0.52, 0.82, 0.67, 0.30) : Qt.rgba(1, 1, 1, 0.22)
                    hoverColor: selected ? Qt.rgba(0.52, 0.82, 0.67, 0.38) : Qt.rgba(1, 1, 1, 0.30)
                    pressedColor: selected ? Qt.rgba(0.52, 0.82, 0.67, 0.46) : Qt.rgba(1, 1, 1, 0.40)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)

                    TextMetrics {
                        id: tagMetrics
                        text: suggestionBtn.text
                        font.pixelSize: suggestionBtn.textPixelSize
                    }

                    onClicked: {
                        if (selected) {
                            root.removeTag(modelData)
                        } else {
                            root.addTag(modelData)
                        }
                    }
                }
            }
        }
    }
}
