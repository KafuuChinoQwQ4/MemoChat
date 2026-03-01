import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Popup {
    id: root
    modal: true
    focus: true
    width: 420
    height: 360
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property Item backdrop: null
    property int targetUid: 0
    property string targetName: ""
    signal submitted(int uid, string backName, var tags)

    function openFor(uid, name) {
        targetUid = uid
        targetName = name
        backField.text = name
        tagEditor.selectedTags = []
        open()
    }

    background: GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        blurRadius: 30
        cornerRadius: 10
        fillColor: Qt.rgba(1, 1, 1, 0.22)
        strokeColor: Qt.rgba(1, 1, 1, 0.50)
        glowTopColor: Qt.rgba(1, 1, 1, 0.24)
        glowBottomColor: Qt.rgba(1, 1, 1, 0.05)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Label {
            text: "好友认证"
            color: "#28364a"
            font.pixelSize: 18
            font.bold: true
        }

        Label {
            text: "通过 " + (root.targetName.length > 0 ? root.targetName : ("UID " + root.targetUid)) + " 的好友申请"
            color: "#5f7088"
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        GlassTextField {
            id: backField
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            backdrop: root.backdrop
            blurRadius: 30
            cornerRadius: 9
            textHorizontalAlignment: TextInput.AlignLeft
            leftInset: 12
            rightInset: 12
            textPixelSize: 14
            placeholderText: root.targetName.length > 0 ? root.targetName : "备注名"
        }

        TagEditor {
            id: tagEditor
            Layout.fillWidth: true
            Layout.fillHeight: true
            backdrop: root.backdrop
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 8

            GlassButton {
                implicitWidth: 74
                implicitHeight: 34
                cornerRadius: 9
                text: "取消"
                onClicked: root.close()
            }

            GlassButton {
                implicitWidth: 74
                implicitHeight: 34
                cornerRadius: 9
                text: "确定"
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.26)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.36)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.44)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: {
                    root.submitted(root.targetUid, backField.text, tagEditor.selectedTags)
                    root.close()
                }
            }
        }
    }
}
