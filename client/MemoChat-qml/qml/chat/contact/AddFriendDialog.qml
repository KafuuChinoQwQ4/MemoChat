import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: root
    modal: true
    focus: true
    width: 420
    height: 390
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property int targetUid: 0
    property string targetName: ""
    property string targetDesc: ""
    property string targetIcon: "qrc:/res/head_1.jpg"
    signal submitted(int uid, string backName, var tags)

    function openFor(uid, name, desc, icon) {
        targetUid = uid
        targetName = name
        targetDesc = desc
        targetIcon = icon && icon.length > 0 ? icon : "qrc:/res/head_1.jpg"
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
            text: "添加好友"
            color: "#2f3a4a"
            font.pixelSize: 18
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Rectangle {
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
                radius: 6
                clip: true
                color: "#dfe4eb"

                Image {
                    anchors.fill: parent
                    source: root.targetIcon
                    fillMode: Image.PreserveAspectCrop
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    text: root.targetName
                    color: "#2f3a4a"
                    font.pixelSize: 14
                    font.bold: true
                }

                Label {
                    text: root.targetDesc
                    color: "#6f7d91"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        TextField {
            id: backField
            Layout.fillWidth: true
            placeholderText: "备注名"
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
                text: "发送申请"
                onClicked: {
                    root.submitted(root.targetUid, backField.text, tagEditor.selectedTags)
                    root.close()
                }
            }
        }
    }
}
