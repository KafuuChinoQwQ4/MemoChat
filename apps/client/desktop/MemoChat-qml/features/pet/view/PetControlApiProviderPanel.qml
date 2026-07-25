pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"
import "../runtime/PetControlRuntime.js" as PetControlRuntime

ColumnLayout {
    id: root

    property var availableModels: []
    property string currentModel: ""
    property string apiProviderStatus: ""
    property bool apiProviderBusy: false
    property var apiProviderCandidates: []
    property bool modelRefreshBusy: false
    property bool agentAvailable: false
    property color accentColor: "#74b2ba"
    property color borderColor: "#ead6e1"

    signal discoverRequested(string providerName, string baseUrl, string apiKey)
    signal registerRequested(string modelName)
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

    GlassTextField {
        id: apiProviderNameField
        Layout.fillWidth: true
        Layout.preferredHeight: 34
        placeholderText: "名称，例如 gpt"
        text: "gpt"
        textPixelSize: 12
        textHorizontalAlignment: TextInput.AlignLeft
        fillColor: Qt.rgba(1, 1, 1, 0.55)
        strokeColor: Qt.rgba(0.84, 0.74, 0.82, 0.45)
        focusStrokeColor: Qt.rgba(0.45, 0.70, 0.73, 0.80)
        cornerRadius: 8
    }

    GlassTextField {
        id: apiBaseUrlField
        Layout.fillWidth: true
        Layout.preferredHeight: 34
        placeholderText: "API 地址，例如 https://api.openai.com/v1"
        text: "https://api.openai.com/v1"
        textPixelSize: 12
        textHorizontalAlignment: TextInput.AlignLeft
        fillColor: Qt.rgba(1, 1, 1, 0.55)
        strokeColor: Qt.rgba(0.84, 0.74, 0.82, 0.45)
        focusStrokeColor: Qt.rgba(0.45, 0.70, 0.73, 0.80)
        cornerRadius: 8
    }

    GlassTextField {
        id: apiKeyField
        Layout.fillWidth: true
        Layout.preferredHeight: 34
        placeholderText: "API Key"
        echoMode: TextInput.Password
        textPixelSize: 12
        textHorizontalAlignment: TextInput.AlignLeft
        fillColor: Qt.rgba(1, 1, 1, 0.55)
        strokeColor: Qt.rgba(0.84, 0.74, 0.82, 0.45)
        focusStrokeColor: Qt.rgba(0.45, 0.70, 0.73, 0.80)
        cornerRadius: 8
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

        GlassButton {
            Layout.preferredWidth: 82
            Layout.preferredHeight: 32
            text: root.apiProviderBusy ? "检测中" : "检测模型"
            textPixelSize: 12
            textColor: enabled ? "#4b3042" : "#a997a3"
            cornerRadius: 8
            normalColor: Qt.rgba(0.91, 0.95, 0.95, 0.50)
            hoverColor: Qt.rgba(0.83, 0.93, 0.94, 0.72)
            pressedColor: Qt.rgba(0.72, 0.87, 0.89, 0.88)
            disabledColor: Qt.rgba(0, 0, 0, 0.04)
            enableScaleFeedback: false
            enabled: root.agentAvailable && !root.apiProviderBusy
            onClicked: root.discoverRequested(apiProviderNameField.text,
                                              apiBaseUrlField.text,
                                              apiKeyField.text)
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        visible: root.apiProviderCandidates.length > 0

        ComboBox {
            id: apiCandidateModelBox
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            model: root.apiProviderCandidates
            textRole: "display_name"
            enabled: !root.apiProviderBusy
        }

        GlassButton {
            Layout.preferredWidth: 104
            Layout.preferredHeight: 32
            text: "接入所选"
            textPixelSize: 12
            textColor: enabled ? "#4b3042" : "#a997a3"
            cornerRadius: 8
            normalColor: Qt.rgba(0.83, 0.93, 0.89, 0.58)
            hoverColor: Qt.rgba(0.72, 0.88, 0.80, 0.72)
            pressedColor: Qt.rgba(0.62, 0.80, 0.70, 0.86)
            enabled: root.agentAvailable && !root.apiProviderBusy
                     && apiCandidateModelBox.currentIndex >= 0
            onClicked: {
                const candidate = root.apiProviderCandidates[apiCandidateModelBox.currentIndex] || {}
                root.registerRequested(candidate.model_name || "")
            }
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

        GlassButton {
            Layout.preferredWidth: 82
            Layout.preferredHeight: 32
            text: root.modelRefreshBusy ? "刷新中" : "刷新"
            textPixelSize: 12
            textColor: enabled ? "#4b3042" : "#a997a3"
            cornerRadius: 8
            normalColor: Qt.rgba(0.91, 0.95, 0.95, 0.50)
            hoverColor: Qt.rgba(0.83, 0.93, 0.94, 0.72)
            pressedColor: Qt.rgba(0.72, 0.87, 0.89, 0.88)
            disabledColor: Qt.rgba(0, 0, 0, 0.04)
            enableScaleFeedback: false
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

}
