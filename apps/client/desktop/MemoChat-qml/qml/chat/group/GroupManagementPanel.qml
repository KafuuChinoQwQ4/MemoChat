import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Rectangle {
    id: root
    color: "transparent"

    property Item backdrop: null
    property string groupName: ""
    property string groupCode: ""
    property string groupIcon: "qrc:/res/chat_icon.png"
    property int currentGroupRole: 1
    property bool currentDialogPinned: false
    property bool currentDialogMuted: false
    property bool canUpdateIcon: false
    property bool canUpdateAnnouncement: false
    property bool canDeleteMessages: false
    property bool canInviteUsers: false
    property bool canManageAdmins: false
    property bool canPinMessages: false
    property bool canBanUsers: false
    property bool canManageTopics: false
    property var friendModel: null
    property string statusText: ""
    property bool statusError: false

    signal backRequested()
    signal refreshRequested()
    signal loadHistoryRequested()
    signal updateAnnouncementRequested(string announcement)
    signal updateGroupIconRequested(int source)
    signal toggleDialogPinned()
    signal toggleDialogMuted()
    signal quitRequested()
    signal dissolveRequested()
    signal inviteRequested(string userId, string reason)
    signal setAdminRequested(string userId, bool isAdmin, int permissionBits)
    signal muteRequested(string userId, int muteSeconds)
    signal kickRequested(string userId)
    signal reviewRequested(var applyId, bool agree)

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        cornerRadius: 14
        blurRadius: 24
        fillColor: Qt.rgba(0.98, 0.99, 1.0, 0.90)
        strokeColor: Qt.rgba(1, 1, 1, 0.60)
        glowTopColor: Qt.rgba(1, 1, 1, 0.28)
        glowBottomColor: Qt.rgba(1, 1, 1, 0.06)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            radius: 12
            color: Qt.rgba(1, 1, 1, 0.18)
            border.color: Qt.rgba(1, 1, 1, 0.36)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10

                GlassButton {
                    Layout.preferredWidth: 76
                    Layout.preferredHeight: 34
                    text: "返回"
                    textPixelSize: 13
                    cornerRadius: 10
                    normalColor: Qt.rgba(0.42, 0.56, 0.74, 0.20)
                    hoverColor: Qt.rgba(0.42, 0.56, 0.74, 0.30)
                    pressedColor: Qt.rgba(0.42, 0.56, 0.74, 0.40)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    onClicked: root.backRequested()
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: "群管理"
                        color: "#2a3649"
                        font.pixelSize: 18
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.groupName.length > 0 ? root.groupName : "群聊"
                        color: "#60738a"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }
            }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: panelColumn.implicitHeight

            Column {
                id: panelColumn
                width: parent.width
                spacing: 10

                GroupInfoPane {
                    width: parent.width
                    height: implicitHeight
                    backdrop: root.backdrop
                    groupName: root.groupName
                    groupCode: root.groupCode
                    groupIcon: root.groupIcon
                    currentGroupRole: root.currentGroupRole
                    canUpdateIcon: root.canUpdateIcon
                    canUpdateAnnouncement: root.canUpdateAnnouncement
                    statusText: root.statusText
                    statusError: root.statusError
                    currentDialogPinned: root.currentDialogPinned
                    currentDialogMuted: root.currentDialogMuted
                    onRefreshRequested: root.refreshRequested()
                    onLoadHistoryRequested: root.loadHistoryRequested()
                    onUpdateAnnouncementRequested: function(announcement) { root.updateAnnouncementRequested(announcement) }
                    onUpdateGroupIconRequested: function(source) { root.updateGroupIconRequested(source) }
                    onToggleDialogPinned: root.toggleDialogPinned()
                    onToggleDialogMuted: root.toggleDialogMuted()
                    onQuitRequested: root.quitRequested()
                    onDissolveRequested: root.dissolveRequested()
                }

                GroupManagePane {
                    width: parent.width
                    height: implicitHeight
                    backdrop: root.backdrop
                    friendModel: root.friendModel
                    canUpdateAnnouncement: root.canUpdateAnnouncement
                    canDeleteMessages: root.canDeleteMessages
                    canInviteUsers: root.canInviteUsers
                    canManageAdmins: root.canManageAdmins
                    canPinMessages: root.canPinMessages
                    canBanUsers: root.canBanUsers
                    canManageTopics: root.canManageTopics
                    onInviteRequested: function(userId, reason) { root.inviteRequested(userId, reason) }
                    onSetAdminRequested: function(userId, isAdmin, permissionBits) { root.setAdminRequested(userId, isAdmin, permissionBits) }
                    onMuteRequested: function(userId, muteSeconds) { root.muteRequested(userId, muteSeconds) }
                    onKickRequested: function(userId) { root.kickRequested(userId) }
                }

                GroupApplyReviewPane {
                    width: parent.width
                    height: implicitHeight
                    backdrop: root.backdrop
                    onReviewRequested: function(applyId, agree) { root.reviewRequested(applyId, agree) }
                }
            }

            ScrollBar.vertical: GlassScrollBar { }
        }
    }
}
