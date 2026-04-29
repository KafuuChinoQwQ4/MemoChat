import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root
    color: "transparent"
    clip: true

    property var agentController: null
    property var kbList: []
    property bool busy: false
    property string statusText: ""
    property string errorText: ""
    property string _searchQuery: ""
    property string searchResult: ""

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
                    text: root.busy ? "处理中" : "搜索"
                    textPixelSize: 12
                    cornerRadius: 6
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                    enabled: searchField.text.length > 0 && !root.busy
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
            Layout.preferredHeight: root.searchResult.length > 0 ? Math.min(150, Math.max(92, root.height * 0.24)) : 0
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.20)
            border.color: Qt.rgba(1, 1, 1, 0.42)
            visible: root.searchResult.length > 0

            ScrollView {
                anchors.fill: parent
                ScrollBar.vertical: GlassScrollBar { }
                anchors.margins: 8
                clip: true

                TextArea {
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    text: root.searchResult
                    font.pixelSize: 13
                    color: "#253247"
                    textFormat: Text.PlainText
                    background: null
                    selectByMouse: true
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? statusLabel.implicitHeight + 18 : 0
            radius: 10
            color: root.errorText.length > 0 ? Qt.rgba(0.89, 0.27, 0.27, 0.12) : Qt.rgba(0.35, 0.61, 0.90, 0.14)
            border.color: root.errorText.length > 0 ? Qt.rgba(0.89, 0.27, 0.27, 0.24) : Qt.rgba(0.35, 0.61, 0.90, 0.24)
            visible: root.busy || root.errorText.length > 0 || root.statusText.length > 0

            Label {
                id: statusLabel
                anchors.fill: parent
                anchors.margins: 9
                text: root.errorText.length > 0 ? root.errorText : root.statusText
                color: root.errorText.length > 0 ? "#c14d4d" : "#4e5d74"
                font.pixelSize: 12
                wrapMode: Text.Wrap
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
                    width: Math.min(220, parent.width - 24)
                    height: 64
                    radius: 10
                    color: Qt.rgba(1, 1, 1, 0.20)
                    visible: !root.busy && root.errorText.length === 0 && root.kbList.length === 0

                    Label {
                        anchors.centerIn: parent
                                text: "暂无知识库\n点击下方上传文档添加"
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
                            enabled: !root.busy
                            normalColor: Qt.rgba(0.89, 0.27, 0.27, 0.20)
                            hoverColor: Qt.rgba(0.89, 0.27, 0.27, 0.30)
                            pressedColor: Qt.rgba(0.89, 0.27, 0.27, 0.40)
                            onClicked: {
                                if (root.agentController && modelData.kb_id) {
                                    root.agentController.deleteKnowledgeBase(modelData.kb_id)
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
            text: root.busy ? "处理中..." : "上传文档"
            textPixelSize: 14
            cornerRadius: 10
            enabled: !root.busy
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
                    width: Math.min(340, Math.max(0, root.width - 32))
                    height: Math.min(220, Math.max(0, root.height - 32))
                    radius: 12
                    color: Qt.rgba(0.96, 0.98, 1.0, 0.86)
                    border.color: Qt.rgba(1, 1, 1, 0.48)

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
                                enabled: !root.busy
                                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                                onClicked: {
                                    if (root.agentController) {
                                        root.agentController.chooseAndUploadDocument()
                                    }
                                    uploadDialogLoader.active = false
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        if (root.agentController) {
            root.agentController.listKnowledgeBases()
        }
    }
}
