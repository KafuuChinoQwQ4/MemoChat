import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import MemoChat 1.0
import "components"
import "chat"
import "chat/group"
import "call"
import "moments"
import "agent"

Rectangle {
    id: root
    color: "transparent"
    property int topInset: 0
    property real revealProgress: 1.0
    property int viewMode: 0 // 0 = main tabs, 1 = profile center
    property int lastMainTab: controller.chatTab
    property bool createGroupDialogActivated: false
    property bool groupManagePopupActivated: false
    property bool r18Mode: false
    property real flipAngle: 0
    readonly property real pinkProgress: Math.max(0, Math.min(1, root.flipAngle / 180))

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

    function mixColor(fromColor, toColor) {
        return Qt.rgba(fromColor.r + (toColor.r - fromColor.r) * root.pinkProgress,
                       fromColor.g + (toColor.g - fromColor.g) * root.pinkProgress,
                       fromColor.b + (toColor.b - fromColor.b) * root.pinkProgress,
                       fromColor.a + (toColor.a - fromColor.a) * root.pinkProgress)
    }

    function syncWindowAcrylicTint() {
        if (Window.window && Window.window.hasOwnProperty("acrylicPinkProgress")) {
            Window.window.acrylicPinkProgress = root.pinkProgress
        }
    }

    function toggleR18Mode() {
        if (flipAnimation.running) {
            return
        }
        r18Mode = !r18Mode
        flipAnimation.to = r18Mode ? 180 : 0
        flipAnimation.start()
    }

    NumberAnimation {
        id: flipAnimation
        target: root
        property: "flipAngle"
        duration: 520
        easing.type: Easing.InOutCubic
    }

    GlassBackdrop {
        id: backdropLayer
        anchors.fill: parent
        pinkProgress: root.pinkProgress
    }

    Component.onCompleted: syncWindowAcrylicTint()
    onPinkProgressChanged: syncWindowAcrylicTint()

    Item {
        id: flipDeck
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.bottomMargin: 12
        anchors.topMargin: 12 + root.topInset

        RowLayout {
            id: normalFace
            anchors.fill: parent
            spacing: 10
            visible: root.flipAngle < 90
            enabled: visible
            layer.enabled: true
            transform: Rotation {
                origin.x: normalFace.width / 2
                origin.y: normalFace.height / 2
                axis.x: 0
                axis.y: 1
                axis.z: 0
                angle: root.flipAngle
            }

            GlassSurface {
                Layout.preferredWidth: 56
                Layout.fillHeight: true
                backdrop: backdropLayer
                cornerRadius: 14
                blurRadius: 18
                fillColor: root.mixColor(Qt.rgba(0.20, 0.28, 0.38, 0.34), Qt.rgba(1.0, 0.58, 0.76, 0.22))
                strokeColor: root.mixColor(Qt.rgba(1, 1, 1, 0.36), Qt.rgba(1.0, 0.78, 0.88, 0.42))
                glowTopColor: root.mixColor(Qt.rgba(1, 1, 1, 0.16), Qt.rgba(1.0, 0.88, 0.94, 0.28))
                glowBottomColor: root.mixColor(Qt.rgba(1, 1, 1, 0.02), Qt.rgba(1.0, 0.56, 0.74, 0.08))
                opacity: root.stageValue(0.02, 0.20)

            ChatSideBar {
                anchors.fill: parent
                currentTab: controller.chatTab
                userIcon: controller.currentUserIcon
                hasPendingApply: controller.hasPendingApply
                backgroundColor: root.mixColor(Qt.rgba(0.16, 0.22, 0.31, 0.44), Qt.rgba(1.0, 0.62, 0.78, 0.18))
                borderColor: root.mixColor(Qt.rgba(1, 1, 1, 0.16), Qt.rgba(1.0, 0.82, 0.90, 0.26))
                buttonHoverColor: root.mixColor(Qt.rgba(0.75, 0.87, 1.0, 0.18), Qt.rgba(1.0, 0.46, 0.68, 0.16))
                buttonPressedColor: root.mixColor(Qt.rgba(0.75, 0.87, 1.0, 0.26), Qt.rgba(1.0, 0.42, 0.66, 0.24))
                buttonSelectedColor: root.mixColor(Qt.rgba(0.75, 0.87, 1.0, 0.30), Qt.rgba(1.0, 0.48, 0.70, 0.22))
                buttonSelectedPressedColor: root.mixColor(Qt.rgba(0.75, 0.87, 1.0, 0.44), Qt.rgba(1.0, 0.42, 0.66, 0.32))
                onR18Toggled: root.toggleR18Mode()
                onTabSelected: function(tab) {
                    root.viewMode = 0
                    root.lastMainTab = tab
                    controller.switchChatTab(tab)
                }
                onAvatarClicked: {
                    root.lastMainTab = controller.chatTab
                    root.viewMode = 1
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
                currentDialogUid: controller.currentDialogUid
                dialogsReady: controller.dialogsReady
                contactsReady: controller.contactsReady
                groupsReady: controller.groupsReady
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
                agentSessions: controller.agentController ? controller.agentController.sessions : []
                agentCurrentSessionId: controller.agentController ? controller.agentController.currentSessionId : ""
                agentCurrentModel: controller.agentController ? controller.agentController.currentModel : ""
                agentBusy: controller.agentController ? (controller.agentController.loading || controller.agentController.streaming) : false
                onDialogUidSelected: function(uid) { controller.selectDialogByUid(uid) }
                onChatIndexSelected: function(index) { controller.selectChatIndex(index) }
                onGroupIndexSelected: function(index) { controller.selectGroupIndex(index) }
                onRequestChatLoadMore: controller.loadMoreChats()
                onContactIndexSelected: function(index) { controller.selectContactIndex(index) }
                onOpenApplyRequested: controller.showApplyRequests()
                onRequestContactLoadMore: controller.loadMoreContacts()
                onSearchRequested: function(uidText) { controller.searchUser(uidText) }
                onSearchCleared: controller.clearSearchState()
                onAddFriendRequested: function(uid, bakName, tags) { controller.requestAddFriend(uid, bakName, tags) }
                onCreateGroupRequested: {
                    createGroupDialogActivated = true
                    if (createGroupDialogLoader.item) {
                        createGroupDialogLoader.item.open()
                    }
                }
                onAgentRefreshRequested: {
                    if (controller.agentController) {
                        controller.agentController.loadSessions()
                        controller.agentController.refreshModelList()
                    }
                }
                onAgentNewSessionRequested: {
                    if (controller.agentController) {
                        controller.agentController.createSession()
                    }
                }
                onAgentSessionSelected: function(sessionId) {
                    if (controller.agentController) {
                        controller.agentController.switchSession(sessionId)
                    }
                }
                onAgentSessionDeleted: function(sessionId) {
                    if (controller.agentController) {
                        controller.agentController.deleteSession(sessionId)
                    }
                }
                onDialogPinToggled: function(uid) { controller.toggleDialogPinnedByUid(uid) }
                onDialogMuteToggled: function(uid) { controller.toggleDialogMutedByUid(uid) }
                onDialogMarkRead: function(uid) { controller.markDialogReadByUid(uid) }
                onDialogDraftCleared: function(uid) { controller.clearDialogDraftByUid(uid) }
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

            Item {
                anchors.fill: parent
                anchors.margins: 8
                visible: root.viewMode === 0

                Loader {
                    anchors.fill: parent
                    active: root.viewMode === 0 && controller.chatTab === AppController.ChatTabPage
                    asynchronous: true
                    sourceComponent: Component {
                        ChatConversationPane {
                            backdrop: backdropLayer
                            peerName: controller.currentChatPeerName
                            selfAvatar: controller.currentUserIcon
                            peerAvatar: controller.currentChatPeerIcon
                            hasCurrentChat: controller.hasCurrentChat
                            isGroupChat: controller.hasCurrentGroup
                            currentGroupRole: controller.currentGroupRole
                            messageModel: controller.messageModel
                            currentDraftText: controller.currentDraftText
                            currentPendingAttachments: controller.currentPendingAttachments
                            currentDialogPinned: controller.currentDialogPinned
                            currentDialogMuted: controller.currentDialogMuted
                            hasPendingReply: controller.hasPendingReply
                            replyTargetName: controller.replyTargetName
                            replyPreviewText: controller.replyPreviewText
                            privateHistoryLoading: controller.privateHistoryLoading
                            canLoadMorePrivateHistory: controller.canLoadMorePrivateHistory
                            mediaUploadInProgress: controller.mediaUploadInProgress
                            mediaUploadProgressText: controller.mediaUploadProgressText
                            dialogsReady: controller.dialogsReady
                            onSendComposer: function(text) { controller.sendCurrentComposerPayload(text) }
                            onSendImage: controller.sendImageMessage()
                            onSendFile: controller.sendFileMessage()
                            onRemovePendingAttachment: function(attachmentId) { controller.removePendingAttachment(attachmentId) }
                            onSendVoiceCall: controller.startVoiceChat()
                            onSendVideoCall: controller.startVideoChat()
                            onDraftEdited: function(text) { controller.updateCurrentDraft(text) }
                            onRefreshGroupRequested: controller.refreshGroupList()
                            onToggleDialogPinned: controller.toggleCurrentDialogPinned()
                            onToggleDialogMuted: controller.toggleCurrentDialogMuted()
                            onOpenAttachment: function(url) { controller.openExternalResource(url) }
                            onRequestLoadMoreHistory: controller.loadMorePrivateHistory()
                            onForwardMessage: function(msgId) { controller.forwardGroupMessage(msgId) }
                            onRevokeMessage: function(msgId) { controller.revokeGroupMessage(msgId) }
                            onEditMessage: function(msgId, text) { controller.editGroupMessage(msgId, text) }
                            onReplyMessage: function(msgId, senderName, previewText) { controller.beginReplyMessage(msgId, senderName, previewText) }
                            onAvatarProfileRequested: function(uid, name, icon) {
                                if (uid > 0) {
                                    contactProfilePopup.openProfile(uid, name || controller.currentChatPeerName || "用户", icon || controller.currentChatPeerIcon || "qrc:/res/head_1.jpg", "")
                                }
                            }
                            onCancelReplyMessage: controller.cancelReplyMessage()
                            onOpenGroupManageRequested: {
                                if (controller.hasCurrentGroup) {
                                    groupManagePopupActivated = true
                                    if (groupManagePopupLoader.item) {
                                        groupManagePopupLoader.item.open()
                                    }
                                }
                            }
                        }
                    }
                }

                Loader {
                    anchors.fill: parent
                    active: root.viewMode === 0 && controller.chatTab === AppController.ContactTabPage
                    asynchronous: true
                    sourceComponent: Component {
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
                            onApproveFriendRequested: function(uid, backName, tags) { controller.approveFriend(uid, backName, tags) }
                            onAuthStatusCleared: controller.clearAuthStatus()
                            onMessageChatRequested: controller.jumpChatWithCurrentContact()
                            onVoiceChatRequested: controller.startVoiceChat()
                            onVideoChatRequested: controller.startVideoChat()
                            onDeleteContactRequested: controller.deleteFriend(controller.currentContactUid)
                        }
                    }
                }

                Loader {
                    anchors.fill: parent
                    active: root.viewMode === 0 && controller.chatTab === AppController.SettingsTabPage
                    asynchronous: true
                    sourceComponent: Component {
                        ChatMorePane {
                            backdrop: backdropLayer
                            onSwitchAccountRequested: controller.switchToLogin()
                            onLogoutRequested: controller.switchToLogin()
                        }
                    }
                }

                Loader {
                    anchors.fill: parent
                    active: root.viewMode === 0 && controller.chatTab === AppController.AgentTabPage
                    asynchronous: true
                    sourceComponent: Component {
                        AgentPane {
                            agentController: controller.agentController
                            messageModel: controller.agentMessageModel
                            currentSessionId: controller.agentController ? controller.agentController.currentSessionId : ""
                            currentModel: controller.agentController ? controller.agentController.currentModel : ""
                            availableModels: controller.agentController ? controller.agentController.availableModels : []
                            loading: controller.agentController ? controller.agentController.loading : false
                            streaming: controller.agentController ? controller.agentController.streaming : false
                            errorMsg: controller.agentController ? controller.agentController.error : ""
                            selfAvatar: controller.currentUserIcon
                        }
                    }
                }

                Loader {
                    anchors.fill: parent
                    active: root.viewMode === 0 && controller.chatTab === AppController.MomentsTabPage
                    asynchronous: true
                    sourceComponent: Component {
                        MomentsFeedPane {
                            backdrop: backdropLayer
                            appController: controller
                            momentsModel: controller.momentsModel
                            momentsController: controller.momentsController
                        }
                    }
                }
            }

            Loader {
                anchors.fill: parent
                anchors.margins: 8
                active: root.viewMode === 1
                asynchronous: true
                sourceComponent: Component {
                    ChatProfileCenterPane {
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
                        onChooseAvatarRequested: function(source) { controller.chooseAvatar(source) }
                        onSaveProfileRequested: function(nick, desc) { controller.saveProfile(nick, desc) }
                        onStatusCleared: controller.clearSettingsStatus()
                    }
                }
            }
            }
        }

        RowLayout {
            id: r18Face
            anchors.fill: parent
            spacing: 10
            visible: root.flipAngle >= 90
            enabled: visible
            layer.enabled: true
            transform: Rotation {
                origin.x: r18Face.width / 2
                origin.y: r18Face.height / 2
                axis.x: 0
                axis.y: 1
                axis.z: 0
                angle: root.flipAngle - 180
            }

            GlassSurface {
                Layout.preferredWidth: 56
                Layout.fillHeight: true
                backdrop: backdropLayer
                cornerRadius: 14
                blurRadius: 18
                fillColor: root.mixColor(Qt.rgba(0.20, 0.28, 0.38, 0.34), Qt.rgba(1.0, 0.58, 0.76, 0.22))
                strokeColor: root.mixColor(Qt.rgba(1, 1, 1, 0.36), Qt.rgba(1.0, 0.78, 0.88, 0.42))
                glowTopColor: root.mixColor(Qt.rgba(1, 1, 1, 0.16), Qt.rgba(1.0, 0.88, 0.94, 0.28))
                glowBottomColor: root.mixColor(Qt.rgba(1, 1, 1, 0.02), Qt.rgba(1.0, 0.56, 0.74, 0.08))

                ChatSideBar {
                    anchors.fill: parent
                    minimalMode: true
                    currentTab: controller.chatTab
                    userIcon: controller.currentUserIcon
                    hasPendingApply: controller.hasPendingApply
                    backgroundColor: root.mixColor(Qt.rgba(0.16, 0.22, 0.31, 0.44), Qt.rgba(1.0, 0.62, 0.78, 0.18))
                    borderColor: root.mixColor(Qt.rgba(1, 1, 1, 0.16), Qt.rgba(1.0, 0.82, 0.90, 0.26))
                    buttonHoverColor: root.mixColor(Qt.rgba(0.75, 0.87, 1.0, 0.18), Qt.rgba(1.0, 0.46, 0.68, 0.16))
                    buttonPressedColor: root.mixColor(Qt.rgba(0.75, 0.87, 1.0, 0.26), Qt.rgba(1.0, 0.42, 0.66, 0.24))
                    buttonSelectedColor: root.mixColor(Qt.rgba(0.75, 0.87, 1.0, 0.30), Qt.rgba(1.0, 0.48, 0.70, 0.22))
                    buttonSelectedPressedColor: root.mixColor(Qt.rgba(0.75, 0.87, 1.0, 0.44), Qt.rgba(1.0, 0.42, 0.66, 0.32))
                    onR18Toggled: root.toggleR18Mode()
                    onTabSelected: function(tab) {
                        controller.switchChatTab(tab)
                    }
                }
            }

            GlassSurface {
                Layout.fillWidth: true
                Layout.fillHeight: true
                backdrop: backdropLayer
                cornerRadius: 16
                blurRadius: 22
                fillColor: Qt.rgba(1.0, 0.68, 0.82, 0.14)
                strokeColor: Qt.rgba(1.0, 0.84, 0.92, 0.42)
                glowTopColor: Qt.rgba(1.0, 0.92, 0.96, 0.30)
                glowBottomColor: Qt.rgba(1.0, 0.64, 0.80, 0.06)
            }
        }
    }

    Loader {
        id: createGroupDialogLoader
        active: root.createGroupDialogActivated
        asynchronous: true
        sourceComponent: Component {
            CreateGroupDialog {
                anchors.centerIn: Overlay.overlay
                backdrop: backdropLayer
                onSubmitted: function(name, memberUserIds) { controller.createGroup(name, memberUserIds) }
            }
        }
        onLoaded: if (item) { item.open() }
    }

    Loader {
        id: groupManagePopupLoader
        active: root.groupManagePopupActivated
        asynchronous: true
        sourceComponent: Component {
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
                    } else {
                        controller.ensureGroupsInitialized()
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
                            onUpdateAnnouncementRequested: function(announcement) { controller.updateGroupAnnouncement(announcement) }
                            onUpdateGroupIconRequested: function(source) { controller.updateGroupIcon(source) }
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
                            onInviteRequested: function(userId, reason) { controller.inviteGroupMember(userId, reason) }
                            onSetAdminRequested: function(userId, isAdmin, permissionBits) { controller.setGroupAdmin(userId, isAdmin, permissionBits) }
                            onMuteRequested: function(userId, muteSeconds) { controller.muteGroupMember(userId, muteSeconds) }
                            onKickRequested: function(userId) { controller.kickGroupMember(userId) }
                        }

                        GroupApplyReviewPane {
                            width: parent.width
                            height: 250
                            backdrop: backdropLayer
                            onApplyJoinRequested: function(groupCode, reason) { controller.applyJoinGroup(groupCode, reason) }
                            onReviewRequested: function(applyId, agree) { controller.reviewGroupApply(applyId, agree) }
                        }
                    }
                }
            }
        }
        onLoaded: if (item) { item.open() }
    }

    CallOverlay {
        anchors.fill: parent
        sessionModel: controller.callSession
        onAcceptRequested: controller.acceptIncomingCall()
        onRejectRequested: controller.rejectIncomingCall()
        onEndRequested: controller.endCurrentCall()
        onMuteToggled: controller.toggleCallMuted()
        onCameraToggled: controller.toggleCallCamera()
    }

    ContactProfilePopup {
        id: contactProfilePopup
        appController: controller
    }
}
