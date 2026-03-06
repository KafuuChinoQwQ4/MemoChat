import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import MemoChat 1.0
import "components"
import "chat"
import "chat/group"

Rectangle {
    id: root
    color: "transparent"
    property int topInset: 0
    property real revealProgress: 1.0
    property int viewMode: 0 // 0 = main tabs, 1 = profile center
    property int lastMainTab: controller.chatTab

    Connections {
        target: controller
        function onHasCurrentGroupChanged() {
            if (!controller.hasCurrentGroup && groupManagePopup.opened) {
                groupManagePopup.close()
            }
        }
    }

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
            blurRadius: 18
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
                onTabSelected: {
                    root.viewMode = 0
                    root.lastMainTab = tab
                    controller.switchChatTab(tab)
                }
                onAvatarClicked: {
                    root.lastMainTab = controller.chatTab
                    root.viewMode = 1
                }
            }

            Rectangle {
                id: sideDragBar
                z: 10
                anchors.top: parent.top
                anchors.topMargin: 11
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 11
                anchors.right: parent.right
                property bool hovering: dragArea.containsMouse
                property bool pressed: dragArea.pressed
                width: pressed || hovering ? 6 : 4
                radius: 3
                color: pressed ? Qt.rgba(0.56, 0.74, 0.96, 0.42)
                               : hovering ? Qt.rgba(0.56, 0.74, 0.96, 0.30)
                                          : Qt.rgba(0.56, 0.74, 0.96, 0.18)
                border.width: 1
                border.color: (pressed || hovering)
                              ? Qt.rgba(1, 1, 1, 0.24)
                              : Qt.rgba(1, 1, 1, 0.10)

                Behavior on color {
                    ColorAnimation {
                        duration: 120
                        easing.type: Easing.OutQuad
                    }
                }
                Behavior on width {
                    NumberAnimation {
                        duration: 120
                        easing.type: Easing.OutQuad
                    }
                }

                MouseArea {
                    id: dragArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    hoverEnabled: true
                    cursorShape: Qt.SizeAllCursor
                    onPressed: function(mouse) {
                        mouse.accepted = true
                        if (Window.window) {
                            Window.window.startSystemMove()
                        }
                    }
                }
            }
        }

        GlassSurface {
            Layout.preferredWidth: 250
            Layout.fillHeight: true
            backdrop: backdropLayer
            cornerRadius: 14
            blurRadius: 18
            fillColor: Qt.rgba(1, 1, 1, 0.16)
            strokeColor: Qt.rgba(1, 1, 1, 0.48)
            glowTopColor: Qt.rgba(1, 1, 1, 0.24)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.04)
            opacity: root.stageValue(0.11, 0.20)

            ChatLeftPanel {
                anchors.fill: parent
                backdrop: backdropLayer
                currentTab: controller.chatTab
                dialogModel: controller.dialogListModel
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
                onDialogUidSelected: controller.selectDialogByUid(uid)
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
                onDialogPinToggled: controller.toggleDialogPinnedByUid(uid)
                onDialogMuteToggled: controller.toggleDialogMutedByUid(uid)
                onDialogMarkRead: controller.markDialogReadByUid(uid)
                onDialogDraftCleared: controller.clearDialogDraftByUid(uid)
            }
        }

        GlassSurface {
            Layout.fillWidth: true
            Layout.fillHeight: true
            backdrop: backdropLayer
            cornerRadius: 16
            blurRadius: 20
            fillColor: Qt.rgba(1, 1, 1, 0.14)
            strokeColor: Qt.rgba(1, 1, 1, 0.54)
            glowTopColor: Qt.rgba(1, 1, 1, 0.25)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.04)
            opacity: root.stageValue(0.20, 0.24)

            StackLayout {
                anchors.fill: parent
                anchors.margins: 8
                visible: root.viewMode === 0
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
                        isGroupChat: controller.hasCurrentGroup
                        currentGroupRole: controller.currentGroupRole
                        messageModel: controller.messageModel
                        currentDraftText: controller.currentDraftText
                        currentDialogPinned: controller.currentDialogPinned
                        currentDialogMuted: controller.currentDialogMuted
                        hasPendingReply: controller.hasPendingReply
                        replyTargetName: controller.replyTargetName
                        replyPreviewText: controller.replyPreviewText
                        privateHistoryLoading: controller.privateHistoryLoading
                        canLoadMorePrivateHistory: controller.canLoadMorePrivateHistory
                        mediaUploadInProgress: controller.mediaUploadInProgress
                        mediaUploadProgressText: controller.mediaUploadProgressText
                        onSendText: controller.sendTextMessage(text)
                        onSendImage: controller.sendImageMessage()
                        onSendFile: controller.sendFileMessage()
                        onSendVoiceCall: controller.startVoiceChat()
                        onSendVideoCall: controller.startVideoChat()
                        onDraftEdited: controller.updateCurrentDraft(text)
                        onToggleDialogPinned: controller.toggleCurrentDialogPinned()
                        onToggleDialogMuted: controller.toggleCurrentDialogMuted()
                        onOpenAttachment: controller.openExternalResource(url)
                        onRequestLoadMoreHistory: controller.loadMorePrivateHistory()
                        onForwardMessage: controller.forwardGroupMessage(msgId)
                        onRevokeMessage: controller.revokeGroupMessage(msgId)
                        onEditMessage: controller.editGroupMessage(msgId, text)
                        onReplyMessage: controller.beginReplyMessage(msgId, senderName, previewText)
                        onCancelReplyMessage: controller.cancelReplyMessage()
                        onOpenGroupManageRequested: {
                            if (controller.hasCurrentGroup) {
                                groupManagePopup.open()
                            }
                        }
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
                    contactUserId: controller.currentContactUserId
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

                ChatMorePane {
                    backdrop: backdropLayer
                    onSwitchAccountRequested: controller.switchToLogin()
                    onLogoutRequested: controller.switchToLogin()
                }
            }

            ChatProfileCenterPane {
                anchors.fill: parent
                anchors.margins: 8
                visible: root.viewMode === 1
                backdrop: backdropLayer
                userIcon: controller.currentUserIcon
                userNick: controller.currentUserNick
                userName: controller.currentUserName
                userDesc: controller.currentUserDesc
                userId: controller.currentUserId
                statusText: controller.settingsStatusText
                statusError: controller.settingsStatusError
                onBackRequested: {
                    root.viewMode = 0
                    controller.switchChatTab(root.lastMainTab)
                }
                onChooseAvatarRequested: controller.chooseAvatar()
                onSaveProfileRequested: controller.saveProfile(nick, desc)
                onStatusCleared: controller.clearSettingsStatus()
            }
        }
    }

    CreateGroupDialog {
        id: createGroupDialog
        anchors.centerIn: Overlay.overlay
        backdrop: backdropLayer
        onSubmitted: controller.createGroup(name, memberUserIds)
    }

    Popup {
        id: groupManagePopup
        modal: true
        focus: true
        width: 360
        height: Math.min(root.height - 48, 740)
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        enabled: controller.hasCurrentGroup
        onOpened: {
            if (!controller.hasCurrentGroup) {
                close()
            }
        }

        background: GlassSurface {
            anchors.fill: parent
            backdrop: backdropLayer
            cornerRadius: 12
            blurRadius: 18
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
                    groupCode: controller.currentGroupCode
                    groupIcon: controller.currentChatPeerIcon
                    canUpdateIcon: controller.currentGroupCanChangeInfo
                    canUpdateAnnouncement: controller.currentGroupCanChangeInfo
                    statusText: controller.groupStatusText
                    statusError: controller.groupStatusError
                    onRefreshRequested: controller.refreshGroupList()
                    onLoadHistoryRequested: controller.loadGroupHistory()
                    onUpdateAnnouncementRequested: controller.updateGroupAnnouncement(announcement)
                    onUpdateGroupIconRequested: controller.updateGroupIcon()
                    onQuitRequested: {
                        controller.quitCurrentGroup()
                        groupManagePopup.close()
                    }
                }

                GroupManagePane {
                    width: parent.width
                    height: 340
                    backdrop: backdropLayer
                    canInviteUsers: controller.currentGroupCanInviteUsers
                    canManageAdmins: controller.currentGroupCanManageAdmins
                    canBanUsers: controller.currentGroupCanBanUsers
                    onInviteRequested: controller.inviteGroupMember(userId, reason)
                    onSetAdminRequested: controller.setGroupAdmin(userId, isAdmin, permissionBits)
                    onMuteRequested: controller.muteGroupMember(userId, muteSeconds)
                    onKickRequested: controller.kickGroupMember(userId)
                }

                GroupApplyReviewPane {
                    width: parent.width
                    height: 250
                    backdrop: backdropLayer
                    onApplyJoinRequested: controller.applyJoinGroup(groupCode, reason)
                    onReviewRequested: controller.reviewGroupApply(applyId, agree)
                }
            }
        }
    }
}
