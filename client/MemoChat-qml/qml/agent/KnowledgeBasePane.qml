import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root
    color: "transparent"

    property var agentController: null
    property var kbList: []
    property string _searchQuery: ""
    property string _searchResult: ""

    signal reloadRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        // 搜索栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.30)
            border.color: Qt.rgba(1, 1, 1, 0.44)

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                Label {
                    text: "🔍"
                    font.pixelSize: 14
                    Layout.preferredWidth: 20
                }

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    placeholderText: "搜索知识库..."
                    placeholderTextColor: Qt.rgba(106, 123, 146, 0.6)
                    color: "#253247"
                    background: Rectangle { color: "transparent" }
                    font.pixelSize: 14
                    onTextChanged: root._searchQuery = text
                }

                GlassButton {
                    Layout.preferredWidth: 64
                    Layout.preferredHeight: 28
                    text: "搜索"
                    textPixelSize: 12
                    cornerRadius: 6
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                    enabled: searchField.text.length > 0
                    onClicked: {
                        if (root.agentController) {
                            root.agentController.searchKnowledgeBase(searchField.text)
                        }
                    }
                }
            }
        }

        // 搜索结果
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: root._searchResult.length > 0 ? 180 : 0
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.20)
            border.color: Qt.rgba(1, 1, 1, 0.42)
            visible: root._searchResult.length > 0

            Label {
                anchors.fill: parent
                anchors.margins: 12
                text: root._searchResult
                wrapMode: Text.Wrap
                font.pixelSize: 13
                color: "#253247"
                textFormat: Text.PlainText
                ScrollBar.vertical: GlassScrollBar { }
                elide: Text.ElideNone
            }
        }

        // 知识库列表
        Label {
            text: "已上传文档"
            font.pixelSize: 12
            font.bold: true
            color: "#6a7b92"
            Layout.topMargin: 4
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.12)
            border.color: Qt.rgba(1, 1, 1, 0.3)

            ListView {
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                model: root.kbList
                ScrollBar.vertical: GlassScrollBar { }
                spacing: 6

                Rectangle {
                    anchors.centerIn: parent
                    width: 220
                    height: 64
                    radius: 10
                    color: Qt.rgba(1, 1, 1, 0.20)
                    visible: root.kbList.length === 0

                    Label {
                        anchors.centerIn: parent
                        text: "暂无知识库\n点击上方「上传文档」添加"
                        color: "#6a7b92"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                delegate: Rectangle {
                    width: ListView.view.width
                    implicitHeight: 60
                    radius: 8
                    color: mouseArea.containsMouse ? Qt.rgba(1, 1, 1, 0.15) : "transparent"
                    border.color: Qt.rgba(1, 1, 1, 0.2)

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Label {
                            text: "📄"
                            font.pixelSize: 20
                            Layout.preferredWidth: 28
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                text: modelData.name || modelData.kb_id || "未知文档"
                                font.pixelSize: 13
                                font.bold: true
                                color: "#2a3649"
                                elide: Text.ElideRight
                            }

                            Label {
                                text: (modelData.chunk_count || 0) + " 个片段 | 状态: " + (modelData.status || "未知")
                                font.pixelSize: 11
                                color: "#6a7b92"
                            }
                        }

                        GlassButton {
                            Layout.preferredWidth: 56
                            Layout.preferredHeight: 26
                            text: "删除"
                            textPixelSize: 11
                            cornerRadius: 6
                            normalColor: Qt.rgba(0.89, 0.27, 0.27, 0.20)
                            hoverColor: Qt.rgba(0.89, 0.27, 0.27, 0.30)
                            pressedColor: Qt.rgba(0.89, 0.27, 0.27, 0.40)
                            onClicked: {
                                if (root.agentController && modelData.kb_id) {
                                    root.agentController.deleteKnowledgeBase(modelData.kb_id)
                                    root.reloadRequested()
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                    }
                }
            }
        }

        // 上传按钮
        GlassButton {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            text: "📤 上传文档"
            textPixelSize: 14
            cornerRadius: 10
            normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.28)
            hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.38)
            pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.48)
            onClicked: {
                uploadDialogLoader.active = true
            }
        }

        // 上传说明
        Label {
            text: "支持 PDF / TXT / MD / DOCX 格式"
            font.pixelSize: 11
            color: "#6a7b92"
            Layout.alignment: Qt.AlignHCenter
        }
    }

    // 上传对话框
    Loader {
        id: uploadDialogLoader
        anchors.fill: parent
        active: false
        sourceComponent: Component {
            Rectangle {
                anchors.fill: parent
                color: Qt.rgba(0, 0, 0, 0.4)
                MouseArea {
                    anchors.fill: parent
                    onClicked: uploadDialogLoader.active = false
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: 340
                    height: 200
                    radius: 12
                    color: Qt.rgba(1, 1, 1, 0.95)
                    border.color: Qt.rgba(1, 1, 1, 0.6)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Label {
                            text: "上传知识库文档"
                            font.pixelSize: 15
                            font.bold: true
                            color: "#2a3649"
                        }

                        Label {
                            text: "选择要上传的文档（PDF、TXT、Markdown、DOCX），系统将自动分块并向量化存入知识库。"
                            wrapMode: Text.Wrap
                            font.pixelSize: 12
                            color: "#6a7b92"
                        }

                        Item { Layout.fillHeight: true }

                        RowLayout {
                            spacing: 8

                            Item { Layout.fillWidth: true }

                            GlassButton {
                                text: "取消"
                                Layout.preferredWidth: 72
                                Layout.preferredHeight: 32
                                textPixelSize: 13
                                cornerRadius: 8
                                normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.24)
                                onClicked: uploadDialogLoader.active = false
                            }

                            GlassButton {
                                text: "选择文件"
                                Layout.preferredWidth: 96
                                Layout.preferredHeight: 32
                                textPixelSize: 13
                                cornerRadius: 8
                                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                                onClicked: {
                                    // TODO: 调用 QML FileDialog
                                    uploadDialogLoader.active = false
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
