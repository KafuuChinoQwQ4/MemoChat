import QtQuick 2.15
import MemoChat 1.0
import "../chat"
import "../chat/group"
import "../moments"
import "../agent"
import "../pet"

Item {
    id: root

    property Item backdrop: null
    property int viewMode: 0
    property bool groupManagementPanelActive: false
    property int momentsSelectedUid: 0
    property string momentsSelectedName: ""

    signal groupManagementPanelActiveChangedByUser(bool active)
    signal switchAccountToLoginRequested()
    signal agentGameSetupRequested(string kind)
    signal petPreviewRequested(var petAssetSettings)
    signal avatarProfileRequested(int uid, string name, string icon)

    visible: root.viewMode === 0

    Loader {
        anchors.fill: parent
        active: root.viewMode === 0 && controller.chatTab === AppController.ChatTabPage
        asynchronous: true
        sourceComponent: Component {
            ChatConversationPane {
                backdrop: root.backdrop
                peerName: controller.currentChatPeerName
                selfName: controller.currentUserNick && controller.currentUserNick.length > 0
                          ? controller.currentUserNick
                          : controller.currentUserName
                selfAvatar: controller.currentUserIcon
                peerAvatar: controller.currentChatPeerIcon
                hasCurrentChat: controller.hasCurrentChat
                isGroupChat: controller.hasCurrentGroup
                currentGroupRole: controller.currentGroupRole
                messageModel: controller.messageModel
                agentController: controller.agentController
                imeBridgeController: controller.petController
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
                        root.avatarProfileRequested(uid,
                                                    name || controller.currentChatPeerName || "用户",
                                                    icon || controller.currentChatPeerIcon || "qrc:/res/head_1.jpg")
                    }
                }
                onCancelReplyMessage: controller.cancelReplyMessage()
                onOpenGroupManageRequested: {
                    if (controller.hasCurrentGroup) {
                        root.groupManagementPanelActiveChangedByUser(true)
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
                backdrop: root.backdrop
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
                backdrop: root.backdrop
                onSwitchAccountRequested: root.switchAccountToLoginRequested()
                onLogoutRequested: root.switchAccountToLoginRequested()
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
                gameState: controller.agentController ? controller.agentController.gameState : ({})
                currentGameRoomId: controller.agentController ? controller.agentController.currentGameRoomId : ""
                gameBusy: controller.agentController ? controller.agentController.gameBusy : false
                gameStatusText: controller.agentController ? controller.agentController.gameStatusText : ""
                gameError: controller.agentController ? controller.agentController.gameError : ""
                selfName: controller.currentUserNick && controller.currentUserNick.length > 0
                          ? controller.currentUserNick
                          : controller.currentUserName
                selfAvatar: controller.currentUserIcon
                onGameModeRequested: root.agentGameSetupRequested("game")
            }
        }
    }

    Loader {
        anchors.fill: parent
        active: root.viewMode === 0 && controller.chatTab === AppController.Live2DTabPage
        asynchronous: true
        sourceComponent: Component {
            Live2DCharacterPane {
                backdrop: root.backdrop
                petController: controller.petController
                onPetPreviewRequested: function(petAssetSettings) {
                    root.petPreviewRequested(petAssetSettings)
                }
            }
        }
    }

    Loader {
        anchors.fill: parent
        active: root.viewMode === 0 && controller.chatTab === AppController.MomentsTabPage
        asynchronous: true
        sourceComponent: Component {
            MomentsFeedPane {
                backdrop: root.backdrop
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
                backdrop: root.backdrop
                groupName: controller.currentGroupName
                groupCode: controller.currentGroupCode
                groupIcon: controller.currentChatPeerIcon
                currentGroupRole: controller.currentGroupRole
                currentDialogPinned: controller.currentDialogPinned
                currentDialogMuted: controller.currentDialogMuted
                canUpdateIcon: controller.currentGroupCanChangeInfo
                canUpdateAnnouncement: controller.currentGroupCanChangeInfo
                canDeleteMessages: controller.currentGroupCanDeleteMessages
                canInviteUsers: controller.currentGroupCanInviteUsers
                canManageAdmins: controller.currentGroupCanManageAdmins
                canPinMessages: controller.currentGroupCanPinMessages
                canBanUsers: controller.currentGroupCanBanUsers
                canManageTopics: controller.currentGroupCanManageTopics
                friendModel: controller.contactListModel
                statusText: controller.groupStatusText
                statusError: controller.groupStatusError
                onBackRequested: root.groupManagementPanelActiveChangedByUser(false)
                onRefreshRequested: controller.refreshGroupList()
                onLoadHistoryRequested: controller.loadGroupHistory()
                onUpdateAnnouncementRequested: function(announcement) { controller.updateGroupAnnouncement(announcement) }
                onUpdateGroupIconRequested: function(source) { controller.updateGroupIcon(source) }
                onToggleDialogPinned: controller.toggleCurrentDialogPinned()
                onToggleDialogMuted: controller.toggleCurrentDialogMuted()
                onQuitRequested: {
                    controller.quitCurrentGroup()
                    root.groupManagementPanelActiveChangedByUser(false)
                }
                onDissolveRequested: {
                    controller.dissolveCurrentGroup()
                    root.groupManagementPanelActiveChangedByUser(false)
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
