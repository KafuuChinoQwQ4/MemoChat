import QtQuick 2.15
import MemoChat 1.0
import "qrc:/features/contact/view"
import "qrc:/features/settings/view"
import "qrc:/features/chat/view"
import "qrc:/features/group/view"
import "qrc:/features/moments/view"
import "qrc:/features/agent/view"
import "qrc:/features/pet/view"

Item {
    id: root

    property Item backdrop: null
    property int viewMode: 0
    property bool groupManagementPanelActive: false
    property int momentsSelectedUid: 0
    property string momentsSelectedName: ""
    readonly property var momentsControllerContext: moments
    readonly property var momentsDataModelContext: momentsModel

    signal groupManagementPanelActiveChangedByUser(bool active)
    signal switchAccountToLoginRequested()
    signal agentGameSetupRequested(string kind)
    signal petPreviewRequested(var petAssetSettings)
    signal avatarProfileRequested(int uid, string name, string icon)

    visible: root.viewMode === 0

    Loader {
        anchors.fill: parent
        active: root.viewMode === 0 && shell.chatTab === ShellViewModel.ChatTabPage
        asynchronous: true
        sourceComponent: Component {
            ChatConversationPane {
                backdrop: root.backdrop
                peerName: chat.currentChatPeerName
                selfName: shell.currentUserNick && shell.currentUserNick.length > 0
                          ? shell.currentUserNick
                          : shell.currentUserName
                selfAvatar: shell.currentUserIcon
                peerAvatar: chat.currentChatPeerIcon
                hasCurrentChat: chat.hasCurrentChat
                isGroupChat: chat.hasCurrentGroup
                currentGroupRole: chat.currentGroupRole
                messageModel: chat.messageModel
                agentController: agent
                imeBridgeController: pet
                currentDraftText: chat.currentDraftText
                currentPendingAttachments: chat.currentPendingAttachments
                currentDialogPinned: chat.currentDialogPinned
                currentDialogMuted: chat.currentDialogMuted
                hasPendingReply: chat.hasPendingReply
                replyTargetName: chat.replyTargetName
                replyPreviewText: chat.replyPreviewText
                privateHistoryLoading: chat.privateHistoryLoading
                canLoadMorePrivateHistory: chat.canLoadMorePrivateHistory
                mediaUploadInProgress: chat.mediaUploadInProgress
                mediaUploadProgressText: chat.mediaUploadProgressText
                dialogsReady: chat.dialogsReady
                onSendComposer: function(text) { chat.sendCurrentComposerPayload(text) }
                onSendImage: chat.sendImageMessage()
                onSendFile: chat.sendFileMessage()
                onRemovePendingAttachment: function(attachmentId) { chat.removePendingAttachment(attachmentId) }
                onSendVoiceCall: call.startVoiceChat()
                onSendVideoCall: call.startVideoChat()
                onDraftEdited: function(text) { chat.updateCurrentDraft(text) }
                onRefreshGroupRequested: group.refreshGroupList()
                onToggleDialogPinned: chat.toggleCurrentDialogPinned()
                onToggleDialogMuted: chat.toggleCurrentDialogMuted()
                onOpenAttachment: function(url) { shell.openExternalResource(url) }
                onRequestLoadMoreHistory: chat.loadMorePrivateHistory()
                onForwardMessage: function(msgId) { group.forwardGroupMessage(msgId) }
                onRevokeMessage: function(msgId) { group.revokeGroupMessage(msgId) }
                onEditMessage: function(msgId, text) { group.editGroupMessage(msgId, text) }
                onReplyMessage: function(msgId, senderName, previewText) { chat.beginReplyMessage(msgId, senderName, previewText) }
                onAvatarProfileRequested: function(uid, name, icon) {
                    if (uid > 0) {
                        root.avatarProfileRequested(uid,
                                                    name || chat.currentChatPeerName || "用户",
                                                    icon || chat.currentChatPeerIcon || "qrc:/res/head_1.jpg")
                    }
                }
                onCancelReplyMessage: chat.cancelReplyMessage()
                onOpenGroupManageRequested: {
                    if (chat.hasCurrentGroup) {
                        root.groupManagementPanelActiveChangedByUser(true)
                        chat.ensureGroupsInitialized()
                    }
                }
            }
        }
    }

    Loader {
        anchors.fill: parent
        active: root.viewMode === 0 && shell.chatTab === ShellViewModel.ContactTabPage
        asynchronous: true
        sourceComponent: Component {
            ContactPane {
                backdrop: root.backdrop
                paneIndex: contact.contactPane
                contactName: contact.currentContactName
                contactNick: contact.currentContactNick
                contactIcon: contact.currentContactIcon
                contactBack: contact.currentContactBack
                contactSex: contact.currentContactSex
                contactUserId: contact.currentContactUserId
                hasCurrentContact: contact.hasCurrentContact
                applyModel: contact.applyRequestModel
                authStatusText: contact.authStatusText
                authStatusError: contact.authStatusError
                onApproveFriendRequested: function(uid, backName, tags) { contact.approveFriend(uid, backName, tags) }
                onAuthStatusCleared: contact.clearAuthStatus()
                onMessageChatRequested: contact.jumpChatWithCurrentContact()
                onVoiceChatRequested: call.startVoiceChat()
                onVideoChatRequested: call.startVideoChat()
                onDeleteContactRequested: contact.deleteFriend(contact.currentContactUid)
            }
        }
    }

    Loader {
        anchors.fill: parent
        active: root.viewMode === 0 && shell.chatTab === ShellViewModel.SettingsTabPage
        asynchronous: true
        sourceComponent: Component {
            MorePane {
                backdrop: root.backdrop
                onSwitchAccountRequested: root.switchAccountToLoginRequested()
                onLogoutRequested: root.switchAccountToLoginRequested()
            }
        }
    }

    Loader {
        anchors.fill: parent
        active: root.viewMode === 0 && shell.chatTab === ShellViewModel.AgentTabPage
        asynchronous: true
        sourceComponent: Component {
            AgentPane {
                agentController: agent
                messageModel: agentMessages
                sessions: agent ? agent.sessions : []
                currentSessionId: agent ? agent.currentSessionId : ""
                currentModel: agent ? agent.currentModel : ""
                availableModels: agent ? agent.availableModels : []
                modelRefreshBusy: agent ? agent.modelRefreshBusy : false
                apiProviderBusy: agent ? agent.apiProviderBusy : false
                apiProviderStatus: agent ? agent.apiProviderStatus : ""
                loading: agent ? agent.loading : false
                streaming: agent ? agent.streaming : false
                errorMsg: agent ? agent.error : ""
                knowledgeBusy: agent ? agent.knowledgeBusy : false
                knowledgeStatusText: agent ? agent.knowledgeStatusText : ""
                knowledgeError: agent ? agent.knowledgeError : ""
                gameState: agent ? agent.gameState : ({})
                currentGameRoomId: agent ? agent.currentGameRoomId : ""
                gameBusy: agent ? agent.gameBusy : false
                gameStatusText: agent ? agent.gameStatusText : ""
                gameError: agent ? agent.gameError : ""
                selfName: shell.currentUserNick && shell.currentUserNick.length > 0
                          ? shell.currentUserNick
                          : shell.currentUserName
                selfAvatar: shell.currentUserIcon
                onGameModeRequested: root.agentGameSetupRequested("game")
            }
        }
    }

    Loader {
        anchors.fill: parent
        active: root.viewMode === 0 && shell.chatTab === ShellViewModel.Live2DTabPage
        asynchronous: true
        sourceComponent: Component {
            Live2DCharacterPane {
                backdrop: root.backdrop
                petController: pet
                onPetPreviewRequested: function(petAssetSettings) {
                    root.petPreviewRequested(petAssetSettings)
                }
            }
        }
    }

    Loader {
        anchors.fill: parent
        active: root.viewMode === 0 && shell.chatTab === ShellViewModel.MomentsTabPage
        asynchronous: true
        sourceComponent: Component {
            MomentsFeedPane {
                backdrop: root.backdrop
                currentUserUid: shell.currentUserUid
                contactController: contact
                momentsModel: root.momentsDataModelContext
                momentsController: root.momentsControllerContext
                selectedFriendUid: root.momentsSelectedUid
                selectedFriendName: root.momentsSelectedName
            }
        }
    }

    Loader {
        anchors.fill: parent
        active: root.viewMode === 0
                && shell.chatTab === ShellViewModel.ChatTabPage
                && root.groupManagementPanelActive
                && chat.hasCurrentGroup
        visible: active
        z: 10
        asynchronous: true
        sourceComponent: Component {
            GroupManagementPanel {
                backdrop: root.backdrop
                groupName: group.currentGroupName
                groupCode: group.currentGroupCode
                groupIcon: chat.currentChatPeerIcon
                currentGroupRole: chat.currentGroupRole
                currentDialogPinned: chat.currentDialogPinned
                currentDialogMuted: chat.currentDialogMuted
                canUpdateIcon: group.currentGroupCanChangeInfo
                canUpdateAnnouncement: group.currentGroupCanChangeInfo
                canDeleteMessages: group.currentGroupCanDeleteMessages
                canInviteUsers: group.currentGroupCanInviteUsers
                canManageAdmins: group.currentGroupCanManageAdmins
                canPinMessages: group.currentGroupCanPinMessages
                canBanUsers: group.currentGroupCanBanUsers
                canManageTopics: group.currentGroupCanManageTopics
                friendModel: contact.contactListModel
                statusText: group.groupStatusText
                statusError: group.groupStatusError
                onBackRequested: root.groupManagementPanelActiveChangedByUser(false)
                onRefreshRequested: group.refreshGroupList()
                onLoadHistoryRequested: group.loadGroupHistory()
                onUpdateAnnouncementRequested: function(announcement) { group.updateGroupAnnouncement(announcement) }
                onUpdateGroupIconRequested: function(source) { group.updateGroupIcon(source) }
                onToggleDialogPinned: chat.toggleCurrentDialogPinned()
                onToggleDialogMuted: chat.toggleCurrentDialogMuted()
                onQuitRequested: {
                    group.quitCurrentGroup()
                    root.groupManagementPanelActiveChangedByUser(false)
                }
                onDissolveRequested: {
                    group.dissolveCurrentGroup()
                    root.groupManagementPanelActiveChangedByUser(false)
                }
                onInviteRequested: function(userId, reason) { group.inviteGroupMember(userId, reason) }
                onSetAdminRequested: function(userId, isAdmin, permissionBits) { group.setGroupAdmin(userId, isAdmin, permissionBits) }
                onMuteRequested: function(userId, muteSeconds) { group.muteGroupMember(userId, muteSeconds) }
                onKickRequested: function(userId) { group.kickGroupMember(userId) }
                onReviewRequested: function(applyId, agree) { group.reviewGroupApply(applyId, agree) }
            }
        }
    }
}
