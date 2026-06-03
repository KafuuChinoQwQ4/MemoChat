pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Rectangle {
    id: root
    radius: 10
    color: Qt.rgba(1, 1, 1, 0.30)
    border.color: Qt.rgba(1, 1, 1, 0.38)

    property string currentModelLabel: ""
    property bool apiProviderBusy: false

    signal registerRequested(string name, string baseUrl, string apiKey)

    Layout.preferredHeight: apiColumn.implicitHeight + 20

    ColumnLayout {
        id: apiColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        spacing: 7

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: "模型"
                color: "#243145"
                font.pixelSize: 13
                font.bold: true
            }

            Label {
                Layout.maximumWidth: 240
                text: root.currentModelLabel
                color: "#65758b"
                font.pixelSize: 11
                elide: Text.ElideRight
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: apiProviderNameField
                Layout.preferredWidth: 96
                Layout.preferredHeight: 30
                text: "chat-test"
                placeholderText: "名称"
                selectByMouse: true
            }

            TextField {
                id: apiBaseUrlField
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                text: "https://api.openai.com/v1"
                placeholderText: "API 地址"
                selectByMouse: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: apiKeyField
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                placeholderText: "API Key"
                echoMode: TextInput.Password
                selectByMouse: true
            }

            GlassButton {
                Layout.preferredWidth: 72
                Layout.preferredHeight: 30
                text: root.apiProviderBusy ? "解析" : "接入"
                textPixelSize: 12
                cornerRadius: 8
                enabled: !root.apiProviderBusy
                normalColor: Qt.rgba(0.33, 0.56, 0.84, 0.22)
                hoverColor: Qt.rgba(0.33, 0.56, 0.84, 0.32)
                onClicked: root.registerRequested(apiProviderNameField.text, apiBaseUrlField.text, apiKeyField.text)
            }
        }
    }
}
