import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Popup {
    id: root
    modal: true
    focus: true
    width: 360
    height: root.groupStatusText.length > 0 ? 262 : 236
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property Item backdrop: null
    property string groupStatusText: ""
    property bool groupStatusError: false

    signal applyJoinGroupRequested(string groupCode, string reason)

    function openFresh() {
        joinGroupCodeInput.text = ""
        joinGroupReasonInput.text = ""
        open()
        joinGroupCodeInput.forceActiveFocus()
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(24, 32, 44, 0.28)
    }

    background: GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        blurRadius: 30
        cornerRadius: 12
        fillColor: Qt.rgba(0.98, 0.99, 1.0, 0.90)
        strokeColor: Qt.rgba(1, 1, 1, 0.62)
        glowTopColor: Qt.rgba(1, 1, 1, 0.30)
        glowBottomColor: Qt.rgba(1, 1, 1, 0.06)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: "申请加群"
            color: "#263448"
            font.pixelSize: 18
            font.bold: true
            elide: Text.ElideRight
        }

        GlassTextField {
            id: joinGroupCodeInput
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            backdrop: root.backdrop
            blurRadius: 30
            cornerRadius: 9
            leftInset: 12
            rightInset: 12
            textPixelSize: 14
            textHorizontalAlignment: TextInput.AlignLeft
            placeholderText: "群ID（g#########）"
        }

        GlassTextField {
            id: joinGroupReasonInput
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            backdrop: root.backdrop
            blurRadius: 30
            cornerRadius: 9
            leftInset: 12
            rightInset: 12
            textPixelSize: 14
            textHorizontalAlignment: TextInput.AlignLeft
            placeholderText: "验证信息（可选）"
            onAccepted: submitJoinGroupButton.clicked()
        }

        Label {
            Layout.fillWidth: true
            visible: root.groupStatusText.length > 0
            text: root.groupStatusText
            color: root.groupStatusError ? "#cc4a4a" : "#2a7f62"
            font.pixelSize: 12
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            GlassButton {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                text: "取消"
                textPixelSize: 13
                cornerRadius: 9
                normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.18)
                hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.26)
                pressedColor: Qt.rgba(0.54, 0.60, 0.68, 0.34)
                onClicked: root.close()
            }

            GlassButton {
                id: submitJoinGroupButton
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                text: "发送申请"
                textPixelSize: 13
                cornerRadius: 9
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.26)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.36)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.44)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: {
                    root.applyJoinGroupRequested(joinGroupCodeInput.text.trim(), joinGroupReasonInput.text)
                    root.close()
                }
            }
        }
    }
}
