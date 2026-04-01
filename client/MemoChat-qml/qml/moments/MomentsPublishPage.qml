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

        // ── 顶部导航栏 ──────────────────────────────────
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
                anchors.leftMargin: 8
                anchors.rightMargin: 12
                spacing: 4

                ToolButton {
                    text: "←"
                    font.pixelSize: 18
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
                    leftPadding: 16
                    rightPadding: 16
                    topPadding: 6
                    bottomPadding: 6
                    background: Rectangle {
                        radius: height / 2
                        color: postButton.enabled
                            ? Qt.rgba(0.16, 0.48, 0.89, 1.0)
                            : Qt.rgba(0.78, 0.82, 0.88, 1.0)
                    }
                    contentItem: Label {
                        anchors.centerIn: parent
                        text: postButton.text
                        color: "#ffffff"
                        font.pixelSize: 13
                        font.weight: Font.Medium
                    }
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

        // ── 主体内容 ────────────────────────────────────
        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 14
            Layout.bottomMargin: 14
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            clip: true
            ScrollBar.vertical.interactive: true

            ColumnLayout {
                width: scrollView.availableWidth
                spacing: 12

                // ── 文字输入区 ──────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    radius: 12
                    color: Qt.rgba(1, 1, 1, 0.92)
                    border.color: Qt.rgba(0.8, 0.8, 0.85, 0.6)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8

                        Label {
                            text: "这一刻的想法"
                            font.pixelSize: 13
                            font.weight: Font.Medium
                            color: "#555555"
                        }

                        TextArea {
                            id: postInput
                            Layout.fillWidth: true
                            Layout.preferredHeight: Math.max(120, scrollView.availableHeight * 0.35)
                            placeholderText: "这一刻的想法..."
                            wrapMode: TextEdit.Wrap
                            font.pixelSize: 15
                            color: "#1a1a1a"
                            placeholderTextColor: "#bbbbbb"
                            selectByMouse: true
                            background: Rectangle { color: "transparent" }
                        }
                    }
                }

                // ── 图片上传占位区（待集成）──────────────
                Rectangle {
                    Layout.fillWidth: true
                    radius: 12
                    height: 80
                    color: Qt.rgba(0.94, 0.96, 1.0, 0.8)
                    border.color: Qt.rgba(0.8, 0.82, 0.9, 0.5)

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 8

                        Label {
                            text: "🖼"
                            font.pixelSize: 22
                        }
                        Label {
                            text: "图片上传功能待集成"
                            font.pixelSize: 13
                            color: "#999999"
                        }
                    }
                }

                // ── 位置 ────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    radius: 10
                    height: 44
                    color: Qt.rgba(1, 1, 1, 0.88)
                    border.color: Qt.rgba(0.8, 0.8, 0.85, 0.5)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        spacing: 10

                        Label {
                            text: "📍"
                            font.pixelSize: 16
                        }

                        TextField {
                            id: locationField
                            Layout.fillWidth: true
                            placeholderText: "所在位置（选填）"
                            placeholderTextColor: "#cccccc"
                            font.pixelSize: 14
                            color: "#1a1a1a"
                            background: Rectangle { color: "transparent" }
                        }
                    }
                }

                // ── 可见性 ──────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    radius: 10
                    height: 44
                    color: Qt.rgba(1, 1, 1, 0.88)
                    border.color: Qt.rgba(0.8, 0.8, 0.85, 0.5)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        spacing: 10

                        Label {
                            text: "👁"
                            font.pixelSize: 16
                        }

                        ComboBox {
                            id: visibilityCombo
                            Layout.fillWidth: true
                            model: ["公开", "仅好友可见", "仅自己可见"]
                            currentIndex: 0
                            font.pixelSize: 14
                            background: Rectangle { color: "transparent" }
                        }
                    }
                }

                // ── 留空底部填充 ────────────────────────
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 20
                }
            }
        }
    }

    property bool loading: root.controller ? root.controller.loading : false
}
