import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"
import "settings"

Item {
    id: root
    property Item backdrop: null
    property string userIcon: "qrc:/res/head_1.jpg"
    property string userNick: ""
    property string userName: ""
    property string userDesc: ""
    property string userId: ""
    property string statusText: ""
    property bool statusError: false
    signal backRequested()
    signal chooseAvatarRequested(int source)
    signal saveProfileRequested(string nick, string desc)
    signal statusCleared()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        GlassSurface {
            Layout.fillWidth: true
            Layout.preferredHeight: 58
            backdrop: root.backdrop !== null ? root.backdrop : root
            cornerRadius: 12
            blurRadius: 18
            fillColor: Qt.rgba(1, 1, 1, 0.22)
            strokeColor: Qt.rgba(1, 1, 1, 0.48)
            glowTopColor: Qt.rgba(1, 1, 1, 0.24)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.05)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 12

                GlassButton {
                    text: "返回"
                    implicitWidth: 68
                    implicitHeight: 30
                    textPixelSize: 13
                    cornerRadius: 9
                    normalColor: Qt.rgba(0.33, 0.56, 0.82, 0.24)
                    hoverColor: Qt.rgba(0.33, 0.56, 0.82, 0.33)
                    pressedColor: Qt.rgba(0.33, 0.56, 0.82, 0.44)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    onClicked: root.backRequested()
                }

                Label {
                    Layout.fillWidth: true
                    text: "个人中心"
                    color: "#223247"
                    font.pixelSize: 18
                    font.bold: true
                    elide: Text.ElideRight
                }
            }
        }

        GlassSurface {
            Layout.fillWidth: true
            Layout.fillHeight: true
            backdrop: root.backdrop !== null ? root.backdrop : root
            cornerRadius: 12
            blurRadius: 18
            fillColor: Qt.rgba(1, 1, 1, 0.18)
            strokeColor: Qt.rgba(1, 1, 1, 0.46)
            glowTopColor: Qt.rgba(1, 1, 1, 0.22)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.04)

            RowLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 14

                SettingsAvatarCard {
                    Layout.preferredWidth: 260
                    Layout.fillHeight: true
                    backdrop: root.backdrop
                    iconSource: root.userIcon
                    onChooseAvatarRequested: root.chooseAvatarRequested(source)
                }

                SettingsProfileForm {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    backdrop: root.backdrop
                    userName: root.userName
                    userNick: root.userNick
                    userDesc: root.userDesc
                    userId: root.userId
                    statusText: root.statusText
                    statusError: root.statusError
                    onSaveRequested: root.saveProfileRequested(nick, desc)
                    onStatusCleared: root.statusCleared()
                }
            }
        }
    }
}
