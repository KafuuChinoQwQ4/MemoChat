import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "contact"

Rectangle {
    id: root
    color: "#f1f2f3"

    property int paneIndex: 0
    property string contactName: ""
    property string contactNick: ""
    property string contactIcon: "qrc:/res/head_1.jpg"
    property string contactBack: ""
    property int contactSex: 0
    property bool hasCurrentContact: false
    property var applyModel
    property string authStatusText: ""
    property bool authStatusError: false

    signal approveFriendRequested(int uid, string backName, var tags)
    signal authStatusCleared()
    signal messageChatRequested()
    signal voiceChatRequested()
    signal videoChatRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: "#f1f2f3"
            border.color: "#ede9e7"

            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 12
                text: root.paneIndex === 0 ? "新的朋友" : "联系人信息"
                color: "#2f3a4a"
                font.pixelSize: 18
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: authTip.visible ? authTip.implicitHeight + 12 : 0
            color: "#f1f2f3"
            visible: authTip.visible

            Label {
                id: authTip
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: root.authStatusText
                visible: text.length > 0
                color: root.authStatusError ? "#c64040" : "#2a7f62"
                wrapMode: Text.Wrap
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.paneIndex

            ApplyRequestList {
                applyModel: root.applyModel
                onApproveClicked: authDialog.openFor(uid, name)
            }

            ChatFriendInfoPane {
                hasContact: root.hasCurrentContact
                contactName: root.contactName
                contactNick: root.contactNick
                contactIcon: root.contactIcon
                contactBack: root.contactBack
                contactSex: root.contactSex
                onMessageChatClicked: root.messageChatRequested()
                onVoiceChatClicked: root.voiceChatRequested()
                onVideoChatClicked: root.videoChatRequested()
            }
        }
    }

    AuthFriendDialog {
        id: authDialog
        anchors.centerIn: Overlay.overlay
        onSubmitted: {
            root.authStatusCleared()
            root.approveFriendRequested(uid, backName, tags)
        }
    }
}
