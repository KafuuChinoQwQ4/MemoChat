import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Popup {
    id: root
    modal: true
    focus: true
    width: 360
    height: 320
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 0

    property Item backdrop: null
    signal submitted(string name, var memberUids)

    background: GlassSurface {
        backdrop: root.backdrop
        cornerRadius: 12
        blurRadius: 30
        fillColor: Qt.rgba(1, 1, 1, 0.24)
        strokeColor: Qt.rgba(1, 1, 1, 0.46)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Label {
            text: "创建群聊"
            font.pixelSize: 18
            font.bold: true
            color: "#2a3649"
        }

        GlassTextField {
            id: groupNameInput
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            backdrop: root.backdrop
            placeholderText: "群名称（1-64）"
        }

        Label {
            text: "成员 UID（英文逗号分隔，可选）"
            color: "#56677d"
            font.pixelSize: 12
        }

        TextArea {
            id: membersInput
            Layout.fillWidth: true
            Layout.fillHeight: true
            placeholderText: "例如：10002,10003,10004"
            wrapMode: TextArea.Wrap
            color: "#2a3649"
            selectByMouse: true
            background: Rectangle {
                radius: 10
                color: Qt.rgba(1, 1, 1, 0.26)
                border.color: Qt.rgba(1, 1, 1, 0.46)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            GlassButton {
                Layout.fillWidth: true
                text: "取消"
                implicitHeight: 34
                cornerRadius: 8
                normalColor: Qt.rgba(0.48, 0.56, 0.66, 0.20)
                hoverColor: Qt.rgba(0.48, 0.56, 0.66, 0.30)
                pressedColor: Qt.rgba(0.48, 0.56, 0.66, 0.38)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.close()
            }

            GlassButton {
                Layout.fillWidth: true
                text: "创建"
                implicitHeight: 34
                cornerRadius: 8
                normalColor: Qt.rgba(0.28, 0.70, 0.58, 0.24)
                hoverColor: Qt.rgba(0.28, 0.70, 0.58, 0.34)
                pressedColor: Qt.rgba(0.28, 0.70, 0.58, 0.42)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: {
                    var ids = []
                    var parts = membersInput.text.split(",")
                    for (var i = 0; i < parts.length; ++i) {
                        var num = parseInt(parts[i].trim())
                        if (!isNaN(num) && num > 0) {
                            ids.push(num)
                        }
                    }
                    root.submitted(groupNameInput.text, ids)
                    root.close()
                }
            }
        }
    }

    onOpened: {
        groupNameInput.text = ""
        membersInput.text = ""
        groupNameInput.forceActiveFocus()
    }
}
