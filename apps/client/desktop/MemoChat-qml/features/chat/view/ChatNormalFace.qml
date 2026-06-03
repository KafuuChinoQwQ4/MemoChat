import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "qrc:/features/chat/view"
import "qrc:/features/profile/view"
import "sidebar"
import "qrc:/qml/components"
import "qrc:/qml/app/ChatShellRuntime.js" as ChatShellRuntime

RowLayout {
    id: root
    spacing: 10
    visible: root.flipAngle < 90
    enabled: visible
    layer.enabled: visible && root.flipInProgress

    property Item backdrop: null
    property real revealProgress: 1.0
    property real flipAngle: 0
    property bool flipInProgress: false
    property bool glassEffectsEnabled: false
    property int viewMode: 0
    property int lastMainTab: 0
    property var chatViewModel: chat
    property bool groupManagementPanelActive: false
    property int momentsSelectedUid: 0
    property string momentsSelectedName: ""

    signal r18Toggled()
    signal viewModeChangedByUser(int mode)
    signal lastMainTabChangedByUser(int tab)
    signal createGroupRequested()
    signal agentGameClosedRequested()
    signal agentChatSessionRequested()
    signal agentGameSetupRequested(string kind)
    signal momentFriendSelected(int uid, string displayName)
    signal groupManagementPanelActiveChangedByUser(bool active)
    signal switchAccountToLoginRequested()
    signal petPreviewRequested(var petAssetSettings)
    signal avatarProfileRequested(int uid, string name, string icon)

    function stageValue(start, span) {
        return ChatShellRuntime.stageValue(root.revealProgress, start, span)
    }

    transform: Rotation {
        origin.x: root.width / 2
        origin.y: root.height / 2
        axis.x: 0
        axis.y: 1
        axis.z: 0
        angle: root.flipAngle
    }

    GlassSurface {
        Layout.preferredWidth: 72
        Layout.fillHeight: true
        backdrop: root.backdrop
        cornerRadius: 14
        blurEnabled: root.glassEffectsEnabled
        blurRadius: 18
        fillColor: Qt.rgba(1, 1, 1, 0.18)
        strokeColor: Qt.rgba(1, 1, 1, 0.56)
        glowTopColor: Qt.rgba(1, 1, 1, 0.28)
        glowBottomColor: Qt.rgba(1, 1, 1, 0.02)
        opacity: root.stageValue(0.02, 0.20)

        ChatSideBar {
            anchors.fill: parent
            currentTab: root.chatViewModel.chatTab
            userIcon: shell.currentUserIcon
            hasPendingApply: contact.hasPendingApply
            onR18Toggled: root.r18Toggled()
            onTabSelected: function(tab) {
                root.viewModeChangedByUser(0)
                root.lastMainTabChangedByUser(tab)
                shell.switchChatTab(tab)
            }
            onAvatarClicked: {
                root.lastMainTabChangedByUser(root.chatViewModel.chatTab)
                root.viewModeChangedByUser(1)
            }
        }
    }

    GlassSurface {
        Layout.preferredWidth: 250
        Layout.fillHeight: true
        backdrop: root.backdrop
        cornerRadius: 14
        blurEnabled: root.glassEffectsEnabled
        blurRadius: 18
        fillColor: Qt.rgba(1, 1, 1, 0.16)
        strokeColor: Qt.rgba(1, 1, 1, 0.48)
        glowTopColor: Qt.rgba(1, 1, 1, 0.24)
        glowBottomColor: Qt.rgba(1, 1, 1, 0.04)
        opacity: root.stageValue(0.11, 0.20)

        ChatLeftPanel {
            anchors.fill: parent
            backdrop: root.backdrop
            chatViewModel: root.chatViewModel
            currentTab: root.chatViewModel.chatTab
            currentDialogUid: root.chatViewModel.currentDialogUid
            dialogsReady: root.chatViewModel.dialogsReady
            contactsReady: contact.contactsReady
            groupsReady: group.groupsReady
            dialogModel: root.chatViewModel.dialogListModel
            contactModel: contact.contactListModel
            searchModel: contact.searchResultModel
            searchPending: contact.searchPending
            searchStatusText: contact.searchStatusText
            searchStatusError: contact.searchStatusError
            canLoadMoreChats: root.chatViewModel.canLoadMoreChats
            chatLoadingMore: root.chatViewModel.chatLoadingMore
            hasPendingApply: contact.hasPendingApply
            canLoadMoreContacts: contact.canLoadMoreContacts
            contactLoadingMore: contact.contactLoadingMore
            groupStatusText: group.groupStatusText
            groupStatusError: group.groupStatusError
            agentSessions: agent ? agent.sessions : []
            agentCurrentSessionId: agent ? agent.currentSessionId : ""
            agentGameRooms: agent ? agent.gameRooms : []
            agentCurrentGameRoomId: agent ? agent.currentGameRoomId : ""
            agentCurrentModel: agent ? agent.currentModel : ""
            agentBusy: agent ? (agent.loading
                                || agent.streaming
                                || agent.gameBusy) : false
            momentsSelectedUid: root.momentsSelectedUid
            momentsSelectedName: root.momentsSelectedName
            onDialogUidSelected: function(uid) { root.chatViewModel.selectDialogByUid(uid) }
            onChatIndexSelected: function(index) { root.chatViewModel.selectChatIndex(index) }
            onGroupIndexSelected: function(index) { root.chatViewModel.selectGroupIndex(index) }
            onRequestChatLoadMore: root.chatViewModel.loadMoreChats()
            onContactIndexSelected: function(index) { contact.selectContactIndex(index) }
            onOpenApplyRequested: contact.showApplyRequests()
            onRequestContactLoadMore: contact.loadMoreContacts()
            onSearchRequested: function(uidText) { contact.searchUser(uidText) }
            onSearchCleared: contact.clearSearchState()
            onAddFriendRequested: function(uid, bakName, tags) { contact.requestAddFriend(uid, bakName, tags) }
            onApplyJoinGroupRequested: function(groupCode, reason) { group.applyJoinGroup(groupCode, reason) }
            onCreateGroupRequested: root.createGroupRequested()
            onAgentRefreshRequested: {
                if (agent) {
                    agent.loadSessions()
                    agent.listGameRooms()
                    agent.listGameRulesets()
                    agent.listGameTemplates()
                    agent.refreshModelList()
                }
            }
            onAgentNewChatRequested: root.agentChatSessionRequested()
            onAgentNewSessionRequested: root.agentGameSetupRequested("multi")
            onAgentNewGameRequested: root.agentGameSetupRequested("game")
            onAgentSessionSelected: function(sessionId) {
                if (agent) {
                    root.agentGameClosedRequested()
                    agent.switchSession(sessionId)
                }
            }
            onAgentGameRoomSelected: function(roomId) {
                if (agent) {
                    root.agentGameClosedRequested()
                    agent.loadGameRoom(roomId)
                }
            }
            onAgentSessionDeleted: function(sessionId) {
                if (agent) {
                    agent.deleteSession(sessionId)
                }
            }
            onAgentGameRoomDeleted: function(roomId) {
                if (agent) {
                    if (roomId === agent.currentGameRoomId) {
                        root.agentGameClosedRequested()
                    }
                    agent.deleteGameRoom(roomId)
                }
            }
            onMomentFriendSelected: function(uid, displayName) {
                root.momentFriendSelected(uid, displayName || "")
            }
            onDialogPinToggled: function(uid) { root.chatViewModel.toggleDialogPinnedByUid(uid) }
            onDialogMuteToggled: function(uid) { root.chatViewModel.toggleDialogMutedByUid(uid) }
            onDialogMarkRead: function(uid) { root.chatViewModel.markDialogReadByUid(uid) }
            onDialogDraftCleared: function(uid) { root.chatViewModel.clearDialogDraftByUid(uid) }
            Component.onCompleted: root.chatViewModel.ensureGroupsInitialized()
        }
    }

    GlassSurface {
        Layout.fillWidth: true
        Layout.fillHeight: true
        backdrop: root.backdrop
        cornerRadius: 16
        blurEnabled: root.glassEffectsEnabled
        blurRadius: 20
        fillColor: Qt.rgba(1, 1, 1, 0.14)
        strokeColor: Qt.rgba(1, 1, 1, 0.54)
        glowTopColor: Qt.rgba(1, 1, 1, 0.25)
        glowBottomColor: Qt.rgba(1, 1, 1, 0.04)
        opacity: root.stageValue(0.20, 0.24)

        ChatShellContent {
            anchors.fill: parent
            anchors.margins: 8
            backdrop: root.backdrop
            viewMode: root.viewMode
            groupManagementPanelActive: root.groupManagementPanelActive
            momentsSelectedUid: root.momentsSelectedUid
            momentsSelectedName: root.momentsSelectedName
            onGroupManagementPanelActiveChangedByUser: function(active) {
                root.groupManagementPanelActiveChangedByUser(active)
            }
            onSwitchAccountToLoginRequested: root.switchAccountToLoginRequested()
            onAgentGameSetupRequested: function(kind) { root.agentGameSetupRequested(kind) }
            onPetPreviewRequested: function(petAssetSettings) {
                root.petPreviewRequested(petAssetSettings)
            }
            onAvatarProfileRequested: function(uid, name, icon) {
                root.avatarProfileRequested(uid, name, icon)
            }
        }

        Loader {
            anchors.fill: parent
            anchors.margins: 8
            active: root.viewMode === 1
            asynchronous: true
            sourceComponent: Component {
                ProfileCenterPane {
                    backdrop: root.backdrop
                    userIcon: shell.currentUserIcon
                    userNick: shell.currentUserNick
                    userName: shell.currentUserName
                    userDesc: shell.currentUserDesc
                    userId: shell.currentUserId
                    statusText: profile.statusText
                    statusError: profile.statusError
                    onBackRequested: {
                        root.viewModeChangedByUser(0)
                        shell.switchChatTab(root.lastMainTab)
                    }
                    onChooseAvatarRequested: function(source) { profile.chooseAvatar(source) }
                    onSaveProfileRequested: function(nick, desc) { profile.saveProfile(nick, desc) }
                    onStatusCleared: profile.clearStatus()
                }
            }
        }
    }
}
