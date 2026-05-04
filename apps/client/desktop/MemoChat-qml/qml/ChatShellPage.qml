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
import "r18"

Rectangle {
    id: root
    color: "transparent"
    property int topInset: 0
    property real revealProgress: 1.0
    property int viewMode: 0 // 0 = main tabs, 1 = profile center
    property int lastMainTab: controller.chatTab
    property bool createGroupDialogActivated: false
    property bool groupManagementPanelActive: false
    property int momentsSelectedUid: 0
    property string momentsSelectedName: ""
    property bool r18Mode: false
    property real flipAngle: 0
    property int r18ViewMode: 0
    readonly property real acrylicPinkProgress: 0
    readonly property var r18NavigationItems: [
        { "label": "主页", "icon": "qrc:/icons/r18_home.png", "mode": 0 },
        { "label": "本地", "icon": "qrc:/icons/r18_local.png", "mode": 5 },
        { "label": "导航", "icon": "qrc:/icons/r18_navigation.png", "mode": 1 },
        { "label": "数据源", "icon": "qrc:/icons/r18_datasource.png", "mode": 4 }
    ]

    Connections {
        target: controller
        function onCurrentGroupChanged() {
            if (!controller.hasCurrentGroup) {
                groupManagementPanelActive = false
            }
        }

        function onGroupCreated(groupId) {
            root.viewMode = 0
            controller.switchChatTab(AppController.ChatTabPage)
        }
    }

    function stageValue(start, span) {
        return Math.max(0, Math.min(1, (revealProgress - start) / span))
    }

    function syncWindowAcrylicTint() {
        if (Window.window && Window.window.hasOwnProperty("acrylicPinkProgress")) {
            Window.window.acrylicPinkProgress = root.acrylicPinkProgress
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

    function switchAccountToLogin() {
        Qt.callLater(function() {
            controller.switchToLogin()
        })
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
        pinkProgress: root.acrylicPinkProgress
    }

    Component.onCompleted: syncWindowAcrylicTint()
    onAcrylicPinkProgressChanged: syncWindowAcrylicTint()

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
                Layout.preferredWidth: 72
                Layout.fillHeight: true
                backdrop: backdropLayer
                cornerRadius: 14
                blurRadius: 18
                fillColor: Qt.rgba(1, 1, 1, 0.18)
                strokeColor: Qt.rgba(1, 1, 1, 0.56)
                glowTopColor: Qt.rgba(1, 1, 1, 0.28)
                glowBottomColor: Qt.rgba(1, 1, 1, 0.02)
                opacity: root.stageValue(0.02, 0.20)

            ChatSideBar {
                anchors.fill: parent
                currentTab: controller.chatTab
                userIcon: controller.currentUserIcon
                hasPendingApply: controller.hasPendingApply
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
                id: chatLeftPanel
                anchors.fill: parent
                backdrop: backdropLayer
                currentTab: controller.chatTab
                currentDialogUid: controller.currentDialogUid
                dialogsReady: controller.dialogsReady
                contactsReady: controller.contactsReady
                groupsReady: controller.groupsReady
                dialogModel: controller.dialogListModel
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
                momentsSelectedUid: root.momentsSelectedUid
                momentsSelectedName: root.momentsSelectedName
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
                onApplyJoinGroupRequested: function(groupCode, reason) { controller.applyJoinGroup(groupCode, reason) }
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
                onMomentFriendSelected: function(uid, displayName) {
                    root.momentsSelectedUid = uid
                    root.momentsSelectedName = displayName || ""
                    if (controller.momentsController) {
                        controller.momentsController.loadFeedForAuthor(uid)
                    }
                }
                onDialogPinToggled: function(uid) { controller.toggleDialogPinnedByUid(uid) }
                onDialogMuteToggled: function(uid) { controller.toggleDialogMutedByUid(uid) }
                onDialogMarkRead: function(uid) { controller.markDialogReadByUid(uid) }
                onDialogDraftCleared: function(uid) { controller.clearDialogDraftByUid(uid) }
                Component.onCompleted: controller.ensureGroupsInitialized()
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
                            agentController: controller.agentController
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
                                    groupManagementPanelActive = true
                                    controller.ensureGroupsInitialized()
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
                            onSwitchAccountRequested: root.switchAccountToLogin()
                            onLogoutRequested: root.switchAccountToLogin()
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
                            sessions: controller.agentController ? controller.agentController.sessions : []
                            currentSessionId: controller.agentController ? controller.agentController.currentSessionId : ""
                            currentModel: controller.agentController ? controller.agentController.currentModel : ""
                            availableModels: controller.agentController ? controller.agentController.availableModels : []
                            modelRefreshBusy: controller.agentController ? controller.agentController.modelRefreshBusy : false
                            apiProviderBusy: controller.agentController ? controller.agentController.apiProviderBusy : false
                            apiProviderStatus: controller.agentController ? controller.agentController.apiProviderStatus : ""
                            loading: controller.agentController ? controller.agentController.loading : false
                            streaming: controller.agentController ? controller.agentController.streaming : false
                            errorMsg: controller.agentController ? controller.agentController.error : ""
                            knowledgeBusy: controller.agentController ? controller.agentController.knowledgeBusy : false
                            knowledgeStatusText: controller.agentController ? controller.agentController.knowledgeStatusText : ""
                            knowledgeError: controller.agentController ? controller.agentController.knowledgeError : ""
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
                            selectedFriendUid: root.momentsSelectedUid
                            selectedFriendName: root.momentsSelectedName
                        }
                    }
                }

                Loader {
                    anchors.fill: parent
                    active: root.viewMode === 0
                            && controller.chatTab === AppController.ChatTabPage
                            && root.groupManagementPanelActive
                            && controller.hasCurrentGroup
                    visible: active
                    z: 10
                    asynchronous: true
                    sourceComponent: Component {
                            GroupManagementPanel {
                                backdrop: backdropLayer
                                groupName: controller.currentGroupName
                                groupCode: controller.currentGroupCode
                                groupIcon: controller.currentChatPeerIcon
                                currentGroupRole: controller.currentGroupRole
                                currentDialogPinned: controller.currentDialogPinned
                                currentDialogMuted: controller.currentDialogMuted
                                canUpdateIcon: controller.currentGroupCanChangeInfo
                                canUpdateAnnouncement: controller.currentGroupCanChangeInfo
                                canInviteUsers: controller.currentGroupCanInviteUsers
                                canManageAdmins: controller.currentGroupCanManageAdmins
                                canBanUsers: controller.currentGroupCanBanUsers
                                friendModel: controller.contactListModel
                                statusText: controller.groupStatusText
                                statusError: controller.groupStatusError
                                onBackRequested: root.groupManagementPanelActive = false
                                onRefreshRequested: controller.refreshGroupList()
                                onLoadHistoryRequested: controller.loadGroupHistory()
                                onUpdateAnnouncementRequested: function(announcement) { controller.updateGroupAnnouncement(announcement) }
                                onUpdateGroupIconRequested: function(source) { controller.updateGroupIcon(source) }
                                onToggleDialogPinned: controller.toggleCurrentDialogPinned()
                                onToggleDialogMuted: controller.toggleCurrentDialogMuted()
                                onQuitRequested: {
                                    controller.quitCurrentGroup()
                                    root.groupManagementPanelActive = false
                                }
                            onDissolveRequested: {
                                controller.dissolveCurrentGroup()
                                root.groupManagementPanelActive = false
                            }
                            onInviteRequested: function(userId, reason) { controller.inviteGroupMember(userId, reason) }
                            onSetAdminRequested: function(userId, isAdmin, permissionBits) { controller.setGroupAdmin(userId, isAdmin, permissionBits) }
                            onMuteRequested: function(userId, muteSeconds) { controller.muteGroupMember(userId, muteSeconds) }
                            onKickRequested: function(userId) { controller.kickGroupMember(userId) }
                            onReviewRequested: function(applyId, agree) { controller.reviewGroupApply(applyId, agree) }
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
                Layout.preferredWidth: 72
                Layout.fillHeight: true
                backdrop: backdropLayer
                cornerRadius: 14
                blurRadius: 18
                fillColor: Qt.rgba(1, 1, 1, 0.18)
                strokeColor: Qt.rgba(1, 1, 1, 0.56)
                glowTopColor: Qt.rgba(1, 1, 1, 0.28)
                glowBottomColor: Qt.rgba(1, 1, 1, 0.02)

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.SizeAllCursor
                    onPressed: function(mouse) {
                        mouse.accepted = false
                        if (Window.window) {
                            Window.window.startSystemMove()
                        }
                    }
                }

                ColumnLayout {
                    z: 1
                    anchors.fill: parent
                    anchors.topMargin: 12
                    anchors.bottomMargin: 12
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 14

                    Repeater {
                        model: root.r18NavigationItems
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 52
                            radius: 13
                            color: root.r18ViewMode === modelData.mode ? Qt.rgba(0.56, 0.70, 0.86, 0.26)
                                                                       : navMouse.containsMouse ? Qt.rgba(0.56, 0.70, 0.86, 0.16)
                                                                                                : "transparent"
                            border.color: root.r18ViewMode === modelData.mode ? Qt.rgba(1, 1, 1, 0.54) : "transparent"

                            Image {
                                anchors.centerIn: parent
                                width: 27
                                height: 27
                                source: modelData.icon
                                fillMode: Image.PreserveAspectFit
                                opacity: root.r18ViewMode === modelData.mode ? 0.98 : 0.62
                                mipmap: true
                            }

                            MouseArea {
                                id: navMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.r18ViewMode = modelData.mode
                                    if (r18PaneLoader.item) {
                                        r18PaneLoader.item.viewMode = modelData.mode
                                    }
                                }
                            }

                            ToolTip.visible: navMouse.containsMouse
                            ToolTip.delay: 350
                            ToolTip.text: modelData.label
                        }
                    }

                    Item { Layout.fillHeight: true }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 52
                        property bool hovering: chatReturnMouse.containsMouse
                        property bool pressed: chatReturnMouse.pressed
                        scale: pressed ? 0.96 : (hovering ? 1.02 : 1.0)

                        Behavior on scale {
                            NumberAnimation {
                                duration: 110
                                easing.type: Easing.OutQuad
                            }
                        }

                        Rectangle {
                            anchors.fill: parent
                            radius: 13
                            color: chatReturnMouse.pressed ? Qt.rgba(0.56, 0.70, 0.86, 0.22)
                                                           : chatReturnMouse.containsMouse ? Qt.rgba(0.56, 0.70, 0.86, 0.16)
                                                                                           : "transparent"

                            Behavior on color {
                                ColorAnimation {
                                    duration: 110
                                    easing.type: Easing.OutQuad
                                }
                            }
                        }

                        Image {
                            anchors.centerIn: parent
                            width: 28
                            height: 28
                            source: "qrc:/icons/r18.png"
                            fillMode: Image.PreserveAspectFit
                            opacity: 0.72
                            mipmap: true
                        }

                        MouseArea {
                            id: chatReturnMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.toggleR18Mode()
                        }

                        ToolTip.visible: chatReturnMouse.containsMouse
                        ToolTip.delay: 120
                        ToolTip.text: "返回聊天"
                    }
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

                Loader {
                    id: r18PaneLoader
                    anchors.fill: parent
                    anchors.margins: 8
                    active: root.r18Mode
                    asynchronous: true
                    sourceComponent: Component {
                        R18ShellPane {
                            backdrop: backdropLayer
                            r18Controller: controller.r18Controller
                            viewMode: root.r18ViewMode
                            onViewModeChanged: root.r18ViewMode = viewMode
                        }
                    }
                    onLoaded: if (item) { item.viewMode = root.r18ViewMode }
                }
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
                friendModel: controller.contactListModel
                onSubmitted: function(name, memberUserIds) { controller.createGroup(name, memberUserIds) }
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
