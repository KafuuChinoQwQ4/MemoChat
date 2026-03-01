import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Popup {
    id: root
    modal: true
    focus: true
    width: 420
    height: 390
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property Item backdrop: null
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
            text: "添加好友"
            color: "#28364a"
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
                color: Qt.rgba(0.74, 0.83, 0.93, 0.52)

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
                    color: "#2b3950"
                    font.pixelSize: 14
                    font.bold: true
                }

                Label {
                    text: root.targetDesc
                    color: "#5f7088"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
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
            placeholderText: "备注名"
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
                implicitWidth: 92
                implicitHeight: 34
                cornerRadius: 9
                text: "发送申请"
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
