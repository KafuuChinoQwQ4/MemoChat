import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

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

        TextField {
            id: inputField
            Layout.fillWidth: true
            placeholderText: "Search or add labels"
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
                    color: "#daf6e7"
                    border.color: "#b9e5cb"
                    height: 24
                    width: tagLabel.implicitWidth + 26

                    Text {
                        id: tagLabel
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        text: modelData
                        color: "#2f3a4a"
                        font.pixelSize: 12
                    }

                    Button {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: 2
                        text: "x"
                        flat: true
                        onClicked: root.removeTag(modelData)
                    }
                }
            }
        }

        Label {
            text: "建议标签"
            color: "#6f7d91"
            font.pixelSize: 12
        }

        Flow {
            Layout.fillWidth: true
            spacing: 6

            Repeater {
                model: root.suggestionTags
                delegate: Button {
                    text: modelData
                    highlighted: root.selectedTags.indexOf(modelData) !== -1
                    onClicked: {
                        if (root.selectedTags.indexOf(modelData) !== -1) {
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
