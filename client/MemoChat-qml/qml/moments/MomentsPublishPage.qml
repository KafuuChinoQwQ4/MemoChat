import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "../components"

Item {
    id: root

    property var backdrop: null
    property var controller: null

    signal backRequested()
    signal publishRequested()

    function resetForm() {
        postInput.text = ""
    }

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        cornerRadius: 16
        blurRadius: 20
        fillColor: Qt.rgba(0.95, 0.95, 0.97, 0.90)
        strokeColor: Qt.rgba(0.8, 0.8, 0.85, 0.6)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 0

        Item {
            Layout.preferredHeight: 52
            Layout.fillWidth: true

            GlassSurface {
                anchors.fill: parent
                backdrop: root.backdrop
                cornerRadius: 0
                blurRadius: 12
                fillColor: Qt.rgba(1, 1, 1, 0.80)
                strokeColor: Qt.rgba(0.85, 0.85, 0.90, 0.5)
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                ToolButton {
                    text: "返回"
                    onClicked: root.backRequested()
                }

                Label {
                    text: "发布朋友圈"
                    font.pixelSize: 17
                    font.weight: Font.Medium
                    color: "#1a1a1a"
                }

                Item { Layout.fillWidth: true }

                Button {
                    id: publishButton
                    text: loading ? "发布中..." : "发布"
                    enabled: postInput.text.trim().length > 0 && !loading
                    onClicked: {
                        var items = [{"media_type": "text", "content": postInput.text.trim()}]
                        if (root.controller) {
                            root.controller.publishMoment("", 0, items)
                        }
                        root.publishRequested()
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10

                Label {
                    text: "这一刻的想法"
                    font.pixelSize: 14
                    color: "#555555"
                }

                TextArea {
                    id: postInput
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    placeholderText: "这一刻的想法..."
                    wrapMode: TextEdit.Wrap
                    font.pixelSize: 14
                    color: "#1a1a1a"
                    selectByMouse: true
                    background: Rectangle {
                        radius: 10
                        color: Qt.rgba(1, 1, 1, 0.95)
                        border.color: Qt.rgba(0.8, 0.8, 0.85, 0.7)
                    }
                }
            }
        }
    }

    property bool loading: root.controller ? root.controller.loading : false
}
