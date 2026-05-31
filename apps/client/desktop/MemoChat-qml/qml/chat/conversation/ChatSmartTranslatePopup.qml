pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Popup {
    id: root

    property Item backdrop: null
    property real popupAvailableWidth: 0
    property string pendingTranslateText: ""
    property bool smartBusy: false

    signal translateConfirmed(string sourceLanguage, string targetLanguage)

    function openWithDefaults() {
        translateSourceBox.currentIndex = 0
        translateTargetBox.currentIndex = 1
        root.open()
    }

    modal: true
    focus: true
    width: Math.min(root.popupAvailableWidth - 48, 420)
    height: 248
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop !== null ? root.backdrop : root.contentItem
        cornerRadius: 12
        blurRadius: 18
        fillColor: Qt.rgba(1, 1, 1, 0.88)
        strokeColor: Qt.rgba(1, 1, 1, 0.62)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Label {
            Layout.fillWidth: true
            text: "翻译消息"
            color: "#26364d"
            font.pixelSize: 15
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ComboBox {
                id: translateSourceBox
                Layout.fillWidth: true
                editable: true
                model: ["自动检测", "中文", "英语", "日语", "韩语", "法语", "德语"]
                currentIndex: 0
            }

            Label {
                text: "到"
                color: "#60718a"
                font.pixelSize: 13
            }

            ComboBox {
                id: translateTargetBox
                Layout.fillWidth: true
                editable: true
                model: ["英语", "中文", "日语", "韩语", "法语", "德语"]
                currentIndex: 1
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: Qt.rgba(1, 1, 1, 0.34)
            border.color: Qt.rgba(1, 1, 1, 0.42)
            clip: true

            Text {
                anchors.fill: parent
                anchors.margins: 10
                text: root.pendingTranslateText
                color: "#34445c"
                font.pixelSize: 12
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                elide: Text.ElideRight
                maximumLineCount: 3
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Item { Layout.fillWidth: true }

            GlassButton {
                text: "取消"
                implicitWidth: 72
                implicitHeight: 32
                textPixelSize: 12
                cornerRadius: 8
                normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.22)
                onClicked: root.close()
            }

            GlassButton {
                text: "翻译"
                implicitWidth: 72
                implicitHeight: 32
                textPixelSize: 12
                cornerRadius: 8
                enabled: !root.smartBusy && root.pendingTranslateText.length > 0
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: {
                    root.translateConfirmed(translateSourceBox.currentText, translateTargetBox.currentText)
                    root.close()
                }
            }
        }
    }
}
