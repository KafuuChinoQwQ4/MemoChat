pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Item {
    id: root

    property string currentModel: ""
    property var availableModels: []
    property bool modelRefreshBusy: false
    property bool apiProviderBusy: false
    property string apiProviderStatus: ""
    property bool thinkingEnabled: false
    property bool currentModelSupportsThinking: false

    signal closeRequested()
    signal thinkingToggled(bool checked)
    signal apiProviderRegistered(string name, string baseUrl, string apiKey)
    signal modelSelected(string modelType, string modelName)
    signal modelDeleted(string modelType)
    signal refreshModelsRequested()

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(20, 28, 40, 0.22)

        MouseArea {
            anchors.fill: parent
            onClicked: root.closeRequested()
        }

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(460, Math.max(0, root.width - 32))
            height: Math.min(560, Math.max(0, root.height - 32))
            radius: 14
            clip: true
            color: Qt.rgba(0.96, 0.98, 1.0, 0.84)
            border.color: Qt.rgba(1, 1, 1, 0.48)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Label {
                    text: "选择模型"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#2a3649"
                }

                Label {
                    Layout.fillWidth: true
                    text: root.currentModel.length > 0 ? ("当前使用: " + root.currentModel) : "当前模型尚未加载"
                    color: "#6a7b92"
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: root.currentModelSupportsThinking
                    spacing: 10

                    Label {
                        Layout.fillWidth: true
                        text: "Think"
                        color: "#2a3649"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    Switch {
                        checked: root.thinkingEnabled
                        text: checked ? "开" : "关"
                        onToggled: root.thinkingToggled(checked)
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: apiFormColumn.implicitHeight + 18
                    radius: 10
                    color: Qt.rgba(1, 1, 1, 0.22)
                    border.width: 1
                    border.color: Qt.rgba(1, 1, 1, 0.30)

                    ColumnLayout {
                        id: apiFormColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 9
                        spacing: 7

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                text: "API 接入"
                                color: "#2a3649"
                                font.pixelSize: 13
                                font.bold: true
                            }

                            Label {
                                text: root.apiProviderBusy ? "解析中" : ""
                                color: "#6a7b92"
                                font.pixelSize: 11
                                visible: root.apiProviderBusy
                            }
                        }

                        TextField {
                            id: apiProviderNameField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            placeholderText: "名称，例如 gpt"
                            text: "gpt"
                            font.pixelSize: 12
                            selectByMouse: true
                        }

                        TextField {
                            id: apiBaseUrlField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            placeholderText: "API 地址，例如 https://api.openai.com/v1"
                            text: "https://api.openai.com/v1"
                            font.pixelSize: 12
                            selectByMouse: true
                        }

                        TextField {
                            id: apiKeyField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            placeholderText: "API Key"
                            echoMode: TextInput.Password
                            font.pixelSize: 12
                            selectByMouse: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                text: root.apiProviderStatus
                                color: root.apiProviderStatus.indexOf("已接入") >= 0 ? "#4d7f5c" : "#6a7b92"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }

                            GlassButton {
                                Layout.preferredWidth: 90
                                Layout.preferredHeight: 30
                                text: root.apiProviderBusy ? "解析中" : "接入"
                                textPixelSize: 12
                                cornerRadius: 8
                                enabled: !root.apiProviderBusy
                                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                                onClicked: root.apiProviderRegistered(
                                               apiProviderNameField.text,
                                               apiBaseUrlField.text,
                                               apiKeyField.text)
                            }
                        }
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.availableModels
                    spacing: 6

                    delegate: Rectangle {
                        id: modelDelegate
                        required property var modelData

                        width: ListView.view.width
                        height: 48
                        radius: 8
                        color: {
                            const fullName = (modelData.model_type || "") + ":" + (modelData.model_name || "")
                            if (fullName === root.currentModel) {
                                return Qt.rgba(0.54, 0.70, 0.93, 0.18)
                            }
                            return modelMouseArea.containsMouse ? Qt.rgba(0.54, 0.70, 0.93, 0.10) : "transparent"
                        }
                        border.color: Qt.rgba(1, 1, 1, 0.32)

                        MouseArea {
                            id: modelMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (modelDelegate.modelData.model_type && modelDelegate.modelData.model_name) {
                                    root.modelSelected(modelDelegate.modelData.model_type,
                                                       modelDelegate.modelData.model_name)
                                }
                                root.closeRequested()
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    Layout.fillWidth: true
                                    text: modelDelegate.modelData.display_name || modelDelegate.modelData.model_name || ""
                                    color: "#2a3649"
                                    font.pixelSize: 14
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: (modelDelegate.modelData.model_type || "") + (modelDelegate.modelData.supports_thinking ? " · Think" : "")
                                    color: "#6a7b92"
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }

                            GlassButton {
                                Layout.preferredWidth: 54
                                Layout.preferredHeight: 28
                                visible: (modelDelegate.modelData.model_type || "").indexOf("api-") === 0
                                text: "删除"
                                textPixelSize: 12
                                cornerRadius: 7
                                normalColor: Qt.rgba(0.82, 0.38, 0.38, 0.16)
                                hoverColor: Qt.rgba(0.82, 0.38, 0.38, 0.26)
                                pressedColor: Qt.rgba(0.82, 0.38, 0.38, 0.34)
                                onClicked: root.modelDeleted(modelDelegate.modelData.model_type || "")
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 72
                    radius: 10
                    color: Qt.rgba(1, 1, 1, 0.24)
                    border.color: Qt.rgba(1, 1, 1, 0.30)
                    visible: root.availableModels.length === 0

                    Label {
                        anchors.centerIn: parent
                        width: parent.width - 24
                        text: root.modelRefreshBusy ? "正在刷新模型列表..." : "当前没有可用模型。请刷新模型列表，或检查 AIServer / AIOrchestrator 配置。"
                        color: "#6a7b92"
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    Item { Layout.fillWidth: true }

                    GlassButton {
                        Layout.preferredWidth: 96
                        Layout.preferredHeight: 32
                        text: "刷新模型"
                        textPixelSize: 13
                        cornerRadius: 8
                        enabled: !root.modelRefreshBusy
                        normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                        hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                        pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                        onClicked: root.refreshModelsRequested()
                    }
                }
            }
        }
    }
}
