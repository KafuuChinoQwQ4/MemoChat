import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: root
    modal: true
    focus: true
    width: 420
    height: 360
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

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

    background: Rectangle {
        color: "#f7f7f8"
        border.color: "#e3e7ee"
        radius: 8
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Label {
            text: "好友认证"
            color: "#2f3a4a"
            font.pixelSize: 18
            font.bold: true
        }

        Label {
            text: "通过 " + (root.targetName.length > 0 ? root.targetName : ("UID " + root.targetUid)) + " 的好友申请"
            color: "#67768a"
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        TextField {
            id: backField
            Layout.fillWidth: true
            placeholderText: root.targetName.length > 0 ? root.targetName : "备注名"
        }

        TagEditor {
            id: tagEditor
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 8

            Button {
                text: "取消"
                onClicked: root.close()
            }

            Button {
                text: "确定"
                onClicked: {
                    root.submitted(root.targetUid, backField.text, tagEditor.selectedTags)
                    root.close()
                }
            }
        }
    }
}
