import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "components"
import "chat"
import "chat/group"

Rectangle {
    id: root
    color: "transparent"
    property int topInset: 0
    property real revealProgress: 1.0

    function stageValue(start, span) {
        return Math.max(0, Math.min(1, (revealProgress - start) / span))
    }

    GlassBackdrop {
        id: backdropLayer
        anchors.fill: parent
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.bottomMargin: 12
        anchors.topMargin: 12 + root.topInset
        spacing: 10

        GlassSurface {
            Layout.preferredWidth: 56
            Layout.fillHeight: true
            backdrop: backdropLayer
            cornerRadius: 14
            blurRadius: 30
            fillColor: Qt.rgba(0.20, 0.28, 0.38, 0.34)
            strokeColor: Qt.rgba(1, 1, 1, 0.36)
            glowTopColor: Qt.rgba(1, 1, 1, 0.16)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.02)
            opacity: root.stageValue(0.02, 0.20)

            ChatSideBar {
                anchors.fill: parent
                currentTab: controller.chatTab
                userIcon: controller.currentUserIcon
                hasPendingApply: controller.hasPendingApply
                onTabSelected: controller.switchChatTab(tab)
            }
        }

        GlassSurface {
            Layout.preferredWidth: 250
            Layout.fillHeight: true
            backdrop: backdropLayer
            cornerRadius: 14
            blurRadius: 30
            fillColor: Qt.rgba(1, 1, 1, 0.16)
            strokeColor: Qt.rgba(1, 1, 1, 0.48)
            glowTopColor: Qt.rgba(1, 1, 1, 0.24)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.04)
            opacity: root.stageValue(0.11, 0.20)

            ChatLeftPanel {
                anchors.fill: parent
                backdrop: backdropLayer
                currentTab: controller.chatTab
                chatModel: controller.chatListModel
                groupModel: controller.groupListModel
                contactModel: controller.contactListModel
                searchModel: controller.searchResultModel
                searchPending: controller.searchPending
                searchStatusText: controller.searchStatusText
                searchStatusError: controller.searchStatusError
                canLoadMoreChats: controller.canLoadMoreChats
                chatLoadingMore: controller.chatLoadingMore
                hasPendingApply: controller.hasPendingApply
                canLoadMoreContacts: controller.canLoadMoreContacts
                contactLoadingMore: controller.contactLoadingMore
                groupStatusText: controller.groupStatusText
                groupStatusError: controller.groupStatusError
                onChatIndexSelected: controller.selectChatIndex(index)
                onGroupIndexSelected: controller.selectGroupIndex(index)
                onRequestChatLoadMore: controller.loadMoreChats()
                onContactIndexSelected: controller.selectContactIndex(index)
                onOpenApplyRequested: controller.showApplyRequests()
                onRequestContactLoadMore: controller.loadMoreContacts()
                onSearchRequested: controller.searchUser(uidText)
                onSearchCleared: controller.clearSearchState()
                onAddFriendRequested: controller.requestAddFriend(uid, bakName, tags)
                onCreateGroupRequested: createGroupDialog.open()
                onRefreshGroupRequested: controller.refreshGroupList()
            }
        }

        GlassSurface {
            Layout.fillWidth: true
            Layout.fillHeight: true
            backdrop: backdropLayer
            cornerRadius: 16
            blurRadius: 34
            fillColor: Qt.rgba(1, 1, 1, 0.14)
            strokeColor: Qt.rgba(1, 1, 1, 0.54)
            glowTopColor: Qt.rgba(1, 1, 1, 0.25)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.04)
            opacity: root.stageValue(0.20, 0.24)

            StackLayout {
                anchors.fill: parent
                anchors.margins: 8
                currentIndex: controller.chatTab

                Item {
                    anchors.fill: parent

                    ChatConversationPane {
                        anchors.fill: parent
                        backdrop: backdropLayer
                        peerName: controller.currentChatPeerName
                        selfAvatar: controller.currentUserIcon
                        peerAvatar: controller.currentChatPeerIcon
                        hasCurrentChat: controller.hasCurrentChat
                        messageModel: controller.messageModel
                        onSendText: controller.sendTextMessage(text)
                        onSendImage: controller.sendImageMessage()
                        onSendFile: controller.sendFileMessage()
                        onOpenAttachment: controller.openExternalResource(url)
                    }

                    GlassButton {
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.topMargin: 12
                        anchors.rightMargin: 14
                        visible: controller.hasCurrentGroup
                        text: "群管理"
                        implicitWidth: 84
                        implicitHeight: 32
                        cornerRadius: 9
                        normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                        hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                        pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                        disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                        onClicked: groupManagePopup.open()
                    }
                }

                ChatContactPane {
                    backdrop: backdropLayer
                    paneIndex: controller.contactPane
                    contactName: controller.currentContactName
                    contactNick: controller.currentContactNick
                    contactIcon: controller.currentContactIcon
                    contactBack: controller.currentContactBack
                    contactSex: controller.currentContactSex
                    hasCurrentContact: controller.hasCurrentContact
                    applyModel: controller.applyRequestModel
                    authStatusText: controller.authStatusText
                    authStatusError: controller.authStatusError
                    onApproveFriendRequested: controller.approveFriend(uid, backName, tags)
                    onAuthStatusCleared: controller.clearAuthStatus()
                    onMessageChatRequested: controller.jumpChatWithCurrentContact()
                    onVoiceChatRequested: controller.startVoiceChat()
                    onVideoChatRequested: controller.startVideoChat()
                }

                ChatSettingsPane {
                    backdrop: backdropLayer
                    userIcon: controller.currentUserIcon
                    userNick: controller.currentUserNick
                    userName: controller.currentUserName
                    userDesc: controller.currentUserDesc
                    statusText: controller.settingsStatusText
                    statusError: controller.settingsStatusError
                    onChooseAvatarRequested: controller.chooseAvatar()
                    onSaveProfileRequested: controller.saveProfile(nick, desc)
                    onStatusCleared: controller.clearSettingsStatus()
                }
            }
        }
    }

    CreateGroupDialog {
        id: createGroupDialog
        anchors.centerIn: Overlay.overlay
        backdrop: backdropLayer
        onSubmitted: controller.createGroup(name, memberUids)
    }

    Popup {
        id: groupManagePopup
        modal: true
        focus: true
        width: 360
        height: Math.min(root.height - 48, 740)
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        visible: controller.hasCurrentGroup

        background: GlassSurface {
            anchors.fill: parent
            backdrop: backdropLayer
            cornerRadius: 12
            blurRadius: 30
            fillColor: Qt.rgba(1, 1, 1, 0.24)
            strokeColor: Qt.rgba(1, 1, 1, 0.46)
        }

        Flickable {
            anchors.fill: parent
            anchors.margins: 12
            clip: true
            contentWidth: width
            contentHeight: toolsColumn.implicitHeight

            Column {
                id: toolsColumn
                width: parent.width
                spacing: 8

                GroupInfoPane {
                    width: parent.width
                    height: 210
                    backdrop: backdropLayer
                    groupName: controller.currentGroupName
                    statusText: controller.groupStatusText
                    statusError: controller.groupStatusError
                    onRefreshRequested: controller.refreshGroupList()
                    onLoadHistoryRequested: controller.loadGroupHistory()
                    onUpdateAnnouncementRequested: controller.updateGroupAnnouncement(announcement)
                    onQuitRequested: {
                        controller.quitCurrentGroup()
                        groupManagePopup.close()
                    }
                }

                GroupManagePane {
                    width: parent.width
                    height: 270
                    backdrop: backdropLayer
                    onInviteRequested: controller.inviteGroupMember(uid, reason)
                    onSetAdminRequested: controller.setGroupAdmin(uid, isAdmin)
                    onMuteRequested: controller.muteGroupMember(uid, muteSeconds)
                    onKickRequested: controller.kickGroupMember(uid)
                }

                GroupApplyReviewPane {
                    width: parent.width
                    height: 250
                    backdrop: backdropLayer
                    onApplyJoinRequested: controller.applyJoinGroup(groupId, reason)
                    onReviewRequested: controller.reviewGroupApply(applyId, agree)
                }
            }
        }
    }
}
