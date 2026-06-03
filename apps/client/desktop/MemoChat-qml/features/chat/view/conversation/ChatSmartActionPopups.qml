pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Item {
    id: root

    property Item backdrop: null
    property var summaryModel
    property var suggestionModel
    property bool smartBusy: false
    property string activeSmartAction: ""
    property string selectedSuggestionText: ""
    property string pendingTranslateText: ""
    property string editMsgId: ""
    property string editText: ""

    signal suggestionSelected(string text)
    signal suggestedTextAccepted(string text)
    signal translateConfirmed(string sourceLanguage, string targetLanguage)
    signal editConfirmed(string msgId, string text)

    function openSuggestionDialog() {
        suggestionDialog.open()
    }

    function openTranslateDialog() {
        translateDialog.openWithDefaults()
    }

    function openEditDialog(msgId, text) {
        root.editMsgId = msgId
        root.editText = text
        editDialog.open()
    }

    Repeater {
        model: root.summaryModel

        delegate: ChatSmartSummaryPopup {
            availableWidth: root.width
            backdrop: root.backdrop !== null ? root.backdrop : root
            onCloseRequested: function(index) { root.summaryModel.remove(index) }
        }
    }

    Popup {
        id: suggestionDialog
        modal: false
        focus: true
        width: Math.min(root.width - 48, 460)
        height: 260
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: GlassSurface {
            anchors.fill: parent
            backdrop: root.backdrop !== null ? root.backdrop : root
            cornerRadius: 12
            blurRadius: 18
            fillColor: Qt.rgba(1, 1, 1, 0.88)
            strokeColor: Qt.rgba(1, 1, 1, 0.62)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    text: "建议回复"
                    color: "#26364d"
                    font.pixelSize: 15
                    font.bold: true
                }

                Label {
                    text: root.smartBusy && root.activeSmartAction === "suggest" ? "生成中" : ""
                    color: "#60718a"
                    font.pixelSize: 12
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                Repeater {
                    model: root.suggestionModel

                    delegate: Rectangle {
                        id: suggestionOption

                        required property string optionText

                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        radius: 8
                        color: root.selectedSuggestionText === suggestionOption.optionText
                               ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                               : optionMouse.containsMouse ? Qt.rgba(0.35, 0.61, 0.90, 0.12)
                                                           : Qt.rgba(1, 1, 1, 0.30)
                        border.color: root.selectedSuggestionText === suggestionOption.optionText
                                      ? Qt.rgba(0.35, 0.61, 0.90, 0.52)
                                      : Qt.rgba(1, 1, 1, 0.42)

                        Text {
                            anchors.fill: parent
                            anchors.margins: 10
                            text: suggestionOption.optionText
                            color: "#223047"
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        MouseArea {
                            id: optionMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.suggestionSelected(suggestionOption.optionText)
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: root.suggestionModel ? root.suggestionModel.count === 0 : true
                    text: root.smartBusy ? "正在生成 3 条候选回复..." : "暂无建议"
                    color: "#60718a"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
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
                    onClicked: suggestionDialog.close()
                }

                GlassButton {
                    text: "确定"
                    implicitWidth: 72
                    implicitHeight: 32
                    textPixelSize: 12
                    cornerRadius: 8
                    enabled: root.selectedSuggestionText.length > 0
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    onClicked: {
                        root.suggestedTextAccepted(root.selectedSuggestionText)
                        suggestionDialog.close()
                    }
                }
            }
        }
    }

    ChatSmartTranslatePopup {
        id: translateDialog
        popupAvailableWidth: root.width
        backdrop: root.backdrop !== null ? root.backdrop : root
        pendingTranslateText: root.pendingTranslateText
        smartBusy: root.smartBusy
        onTranslateConfirmed: function(sourceLanguage, targetLanguage) {
            root.translateConfirmed(sourceLanguage, targetLanguage)
        }
    }

    Popup {
        id: editDialog
        modal: true
        focus: true
        width: Math.min(root.width - 40, 420)
        height: 220
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onOpened: editInput.text = root.editText

        background: GlassSurface {
            anchors.fill: parent
            backdrop: root.backdrop !== null ? root.backdrop : root
            cornerRadius: 12
            blurRadius: 18
            fillColor: Qt.rgba(1, 1, 1, 0.24)
            strokeColor: Qt.rgba(1, 1, 1, 0.46)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Label {
                text: "编辑消息"
                color: "#2a3649"
                font.pixelSize: 15
                font.bold: true
            }

            TextArea {
                id: editInput
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: root.editText
                wrapMode: Text.Wrap
                color: "#253247"
                selectionColor: "#7baee899"
                selectedTextColor: "#ffffff"
                background: Rectangle {
                    radius: 8
                    color: Qt.rgba(1, 1, 1, 0.30)
                    border.color: Qt.rgba(1, 1, 1, 0.44)
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
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.24)
                    hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.34)
                    pressedColor: Qt.rgba(0.54, 0.60, 0.68, 0.42)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    onClicked: editDialog.close()
                }
                GlassButton {
                    text: "保存"
                    implicitWidth: 72
                    implicitHeight: 32
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    enabled: editInput.text.trim().length > 0
                    onClicked: {
                        root.editConfirmed(root.editMsgId, editInput.text)
                        editDialog.close()
                    }
                }
            }
        }
    }
}
