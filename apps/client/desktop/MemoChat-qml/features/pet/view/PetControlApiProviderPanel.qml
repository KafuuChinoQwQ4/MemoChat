pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../runtime/PetControlRuntime.js" as PetControlRuntime

ColumnLayout {
    id: root

    property var availableModels: []
    property string currentModel: ""
    property string apiProviderStatus: ""
    property bool apiProviderBusy: false
    property bool modelRefreshBusy: false
    property bool agentAvailable: false
    property color accentColor: "#74b2ba"
    property color borderColor: "#ead6e1"

    signal registerRequested(string providerName, string baseUrl, string apiKey)
    signal refreshRequested()
    signal modelSelected(string modelType, string modelName)

    Layout.fillWidth: true
    spacing: 7

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: "AI API 接入"
            color: "#4b3042"
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

    ProviderTextField {
        id: apiProviderNameField
        Layout.fillWidth: true
        placeholderText: "名称，例如 gpt"
        text: "gpt"
    }

    ProviderTextField {
        id: apiBaseUrlField
        Layout.fillWidth: true
        placeholderText: "API 地址，例如 https://api.openai.com/v1"
        text: "https://api.openai.com/v1"
    }

    ProviderTextField {
        id: apiKeyField
        Layout.fillWidth: true
        placeholderText: "API Key"
        echoMode: TextInput.Password
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: root.apiProviderStatus
            color: PetControlRuntime.apiProviderStatusColor(root.apiProviderStatus)
            font.pixelSize: 11
            elide: Text.ElideRight
        }

        ProviderButton {
            Layout.preferredWidth: 82
            text: root.apiProviderBusy ? "解析中" : "接入"
            enabled: root.agentAvailable && !root.apiProviderBusy
            onClicked: root.registerRequested(apiProviderNameField.text,
                                              apiBaseUrlField.text,
                                              apiKeyField.text)
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: root.currentModel.length > 0 ? root.currentModel : "未选择模型"
            color: "#6a7b92"
            font.pixelSize: 11
            elide: Text.ElideRight
        }

        ProviderButton {
            Layout.preferredWidth: 82
            text: root.modelRefreshBusy ? "刷新中" : "刷新"
            enabled: root.agentAvailable && !root.modelRefreshBusy
            onClicked: root.refreshRequested()
        }
    }

    ListView {
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(148, Math.max(64, contentHeight))
        clip: true
        model: root.availableModels
        spacing: 6

        delegate: Rectangle {
            id: modelRow
            required property var modelData

            width: ListView.view.width
            height: 42
            radius: 8
            antialiasing: true
            color: {
                const fullName = PetControlRuntime.modelFullName(modelRow.modelData)
                if (fullName === root.currentModel) {
                    return Qt.rgba(0.45, 0.70, 0.73, 0.18)
                }
                return modelMouse.containsMouse ? "#e8f6f4" : "#fffafd"
            }
            border.color: root.borderColor

            MouseArea {
                id: modelMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (modelRow.modelData.model_type && modelRow.modelData.model_name) {
                        root.modelSelected(modelRow.modelData.model_type, modelRow.modelData.model_name)
                    }
                }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 1

                Label {
                    Layout.fillWidth: true
                    text: modelRow.modelData.display_name || modelRow.modelData.model_name || ""
                    color: "#4b3042"
                    font.pixelSize: 12
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: modelRow.modelData.model_type || ""
                    color: "#8f7c88"
                    font.pixelSize: 10
                    elide: Text.ElideRight
                }
            }
        }
    }

    component ProviderTextField: TextField {
        id: field
        Layout.preferredHeight: 32
        font.pixelSize: 12
        selectByMouse: true
        color: "#2d2630"
        placeholderTextColor: "#8f7c88"
        background: Rectangle {
            radius: 8
            antialiasing: true
            color: "#ffffff"
            border.color: field.activeFocus ? root.accentColor : "#dcc8d4"
        }
    }

    component ProviderButton: Button {
        id: button
        font.pixelSize: 12
        padding: 8
        background: Rectangle {
            radius: 8
            antialiasing: true
            color: button.down ? "#d6f0ed" : button.hovered ? "#e8f6f4" : "#fffafd"
            border.color: button.enabled ? root.borderColor : "#eadfe6"
        }
        contentItem: Label {
            text: button.text
            color: button.enabled ? "#4b3042" : "#a997a3"
            font: button.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }
}
