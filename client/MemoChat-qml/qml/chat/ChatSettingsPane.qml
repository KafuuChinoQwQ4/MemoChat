import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "settings"

Rectangle {
    id: root
    color: "transparent"

    property Item backdrop: null
    property string userIcon: "qrc:/res/head_1.jpg"
    property string userNick: ""
    property string userName: ""
    property string userDesc: ""
    property string statusText: ""
    property bool statusError: false
    signal chooseAvatarRequested()
    signal saveProfileRequested(string nick, string desc)
    signal statusCleared()

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 18

        SettingsAvatarCard {
            Layout.preferredWidth: 260
            Layout.fillHeight: true
            backdrop: root.backdrop
            iconSource: root.userIcon
            onChooseAvatarRequested: root.chooseAvatarRequested()
        }

        SettingsProfileForm {
            Layout.fillWidth: true
            Layout.fillHeight: true
            backdrop: root.backdrop
            userName: root.userName
            userNick: root.userNick
            userDesc: root.userDesc
            statusText: root.statusText
            statusError: root.statusError
            onSaveRequested: root.saveProfileRequested(nick, desc)
            onStatusCleared: root.statusCleared()
        }
    }
}
