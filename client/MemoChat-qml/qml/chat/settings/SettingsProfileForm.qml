import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Item {
    id: root

    property Item backdrop: null
    property string userName: ""
    property string userNick: ""
    property string userDesc: ""
    property string statusText: ""
    property bool statusError: false
    signal saveRequested(string nick, string desc)
    signal statusCleared()

    implicitWidth: 460
    implicitHeight: 260

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop !== null ? root.backdrop : root
        cornerRadius: 10
        blurRadius: 30
        fillColor: Qt.rgba(1, 1, 1, 0.22)
        strokeColor: Qt.rgba(1, 1, 1, 0.46)
        glowTopColor: Qt.rgba(1, 1, 1, 0.22)
        glowBottomColor: Qt.rgba(1, 1, 1, 0.03)

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            Label {
                text: "资料"
                color: "#28364a"
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
                    color: "#536276"
                    background: Rectangle {
                        radius: 8
                        color: Qt.rgba(1, 1, 1, 0.22)
                        border.color: Qt.rgba(1, 1, 1, 0.44)
                    }
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
                    color: "#233247"
                    selectionColor: "#7baee899"
                    selectedTextColor: "#ffffff"
                    background: Rectangle {
                        radius: 8
                        color: Qt.rgba(1, 1, 1, 0.30)
                        border.color: Qt.rgba(1, 1, 1, 0.50)
                    }
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
                    color: "#233247"
                    selectionColor: "#7baee899"
                    selectedTextColor: "#ffffff"
                    wrapMode: Text.Wrap
                    background: Rectangle {
                        radius: 8
                        color: Qt.rgba(1, 1, 1, 0.30)
                        border.color: Qt.rgba(1, 1, 1, 0.50)
                    }
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

            GlassButton {
                Layout.alignment: Qt.AlignRight
                text: "保存"
                textPixelSize: 13
                cornerRadius: 9
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.26)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.36)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.44)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.saveRequested(nickField.text, descField.text)
            }
        }
    }
}
