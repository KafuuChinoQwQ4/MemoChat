import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    property string userName: ""
    property string userNick: ""
    property string userDesc: ""
    property string statusText: ""
    property bool statusError: false
    signal saveRequested(string nick, string desc)
    signal statusCleared()

    implicitWidth: 460
    implicitHeight: 260

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: "#ffffff"
        border.color: "#dfe5ee"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            Label {
                text: "资料"
                color: "#2f3a4a"
                font.pixelSize: 16
                font.bold: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Label {
                    text: "用户名"
                    Layout.preferredWidth: 56
                }
                TextField {
                    Layout.fillWidth: true
                    text: root.userName
                    enabled: false
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Label {
                    text: "昵称"
                    Layout.preferredWidth: 56
                }
                TextField {
                    id: nickField
                    Layout.fillWidth: true
                    text: root.userNick
                    onTextChanged: root.statusCleared()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Label {
                    text: "签名"
                    Layout.preferredWidth: 56
                    Layout.alignment: Qt.AlignTop
                }
                TextArea {
                    id: descField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 86
                    text: root.userDesc
                    wrapMode: Text.Wrap
                    onTextChanged: root.statusCleared()
                }
            }

            Label {
                Layout.fillWidth: true
                visible: root.statusText.length > 0
                text: root.statusText
                color: root.statusError ? "#c64040" : "#2a7f62"
                wrapMode: Text.Wrap
            }

            Item { Layout.fillHeight: true }

            Button {
                Layout.alignment: Qt.AlignRight
                text: "保存"
                onClicked: root.saveRequested(nickField.text, descField.text)
            }
        }
    }
}
