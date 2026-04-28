import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "../components"

Popup {
    id: root
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: Math.min(560, (parent ? parent.width : 800) - 48)
    height: Math.min(520, (parent ? parent.height : 600) - 48)
    anchors.centerIn: parent

    property var backdrop: null
    property var controller: null

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        cornerRadius: 14
        blurRadius: 22
        fillColor: Qt.rgba(1, 1, 1, 0.96)
        strokeColor: Qt.rgba(0.85, 0.85, 0.90, 0.6)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Title bar
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: "发布朋友圈"
                font.pixelSize: 16
                font.weight: Font.Medium
                color: "#1a1a1a"
            }
            Item { Layout.fillWidth: true }

            ToolButton {
                icon.source: "qrc:/icons/close.png"
                icon.width: 18
                icon.height: 18
                onClicked: root.closed()
            }
        }

        // Text area
        TextArea {
            id: textArea
            Layout.fillWidth: true
            Layout.preferredHeight: 140
            placeholderText: "这一刻的想法..."
            placeholderTextColor: "#aaaaaa"
            font.pixelSize: 14
            color: "#1a1a1a"
            wrapMode: TextEdit.Wrap
            background: Rectangle {
                color: Qt.rgba(0.95, 0.95, 0.97, 0.8)
                radius: 8
                border.color: Qt.rgba(0.8, 0.8, 0.85, 0.5)
            }
        }

        // Image preview grid (up to 9 images)
        GridLayout {
            id: imageGrid
            Layout.fillWidth: true
            Layout.preferredHeight: gridHeight
            columns: 3
            rowSpacing: 6
            columnSpacing: 6

            property int gridHeight: {
                var count = imageList.count
                if (count === 0) return 0
                var rows = Math.ceil(count / 3)
                return rows * 90
            }

            Repeater {
                model: imageList
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 84
                    radius: 6
                    color: Qt.rgba(0.92, 0.94, 0.97, 1.0)
                    clip: true
                    Image {
                        anchors.fill: parent
                        fillMode: Image.Cover
                        source: model.url
                        cache: false
                    }
                    ToolButton {
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: 2
                        icon.source: "qrc:/icons/close.png"
                        icon.width: 14
                        icon.height: 14
                        icon.color: "#ffffff"
                        background: Rectangle {
                            anchors.fill: parent
                            color: Qt.rgba(0, 0, 0, 0.4)
                            radius: 7
                        }
                        onClicked: {
                            var idx = imageList.count - 1 - (parent.Parent ? parent.Parent : this)
                            // Find the index
                            for (var i = 0; i < imageList.count; i++) {
                                if (imageList.get(i).url === model.url) {
                                    imageList.remove(i)
                                    break
                                }
                            }
                        }
                    }
                }
            }
        }

        // Image count indicator and add button
        RowLayout {
            Layout.fillWidth: true
            visible: imageList.count > 0 || canAddMore

            Label {
                text: imageList.count + " 张图片"
                font.pixelSize: 12
                color: "#888888"
                visible: imageList.count > 0
            }

            Item { Layout.fillWidth: true }

            Label {
                text: "图片上传功能待集成"
                font.pixelSize: 11
                color: "#aaaaaa"
                visible: canAddMore
            }
        }

        // Location input
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: "📍"
                font.pixelSize: 14
            }
            TextField {
                id: locationField
                Layout.fillWidth: true
                placeholderText: "所在位置（选填）"
                placeholderTextColor: "#aaaaaa"
                font.pixelSize: 13
                color: "#1a1a1a"
                background: Rectangle {
                    color: Qt.rgba(0.95, 0.95, 0.97, 0.6)
                    radius: 6
                }
            }
        }

        // Visibility selector
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: "👁"
                font.pixelSize: 14
            }
            ComboBox {
                id: visibilityCombo
                Layout.fillWidth: true
                model: ["公开", "仅好友可见", "仅自己可见"]
                currentIndex: 0
                font.pixelSize: 13
            }
        }

        Item { Layout.fillWidth: true; Layout.preferredHeight: 4 }

        // Post button
        Button {
            id: postButton
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            enabled: textArea.text.length > 0 && !isLoading
            background: Rectangle {
                radius: 8
                color: postButton.enabled ? "#2a7ae2" : Qt.rgba(0.8, 0.85, 0.9, 1.0)
            }
            contentItem: Label {
                anchors.centerIn: parent
                text: isLoading ? "发布中..." : "发布"
                color: postButton.enabled ? "#ffffff" : "#aaaaaa"
                horizontalAlignment: Text.AlignHCenter
            }
            onClicked: {
                var items = []
                if (textArea.text.length > 0) {
                    items.push({"media_type": "text", "content": textArea.text})
                }
                for (var i = 0; i < imageList.count; i++) {
                    var img = imageList.get(i)
                    items.push({
                        "media_type": "image",
                        "media_key": img.key || "",
                        "thumb_key": img.key || "",
                        "width": img.width || 200,
                        "height": img.height || 200
                    })
                }
                root.controller.publishMoment(locationField.text, visibilityCombo.currentIndex, items)
                root.closed()
            }
        }
    }

    ListModel { id: imageList }

    property bool canAddMore: imageList.count < 9
    property bool isLoading: root.controller ? root.controller.loading : false

    Component.onCompleted: imageList.clear()
}
