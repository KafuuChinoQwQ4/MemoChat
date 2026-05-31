import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "../components"
import "../app"
import "../app/ChatShellRuntime.js" as ChatShellRuntime

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
            currentTab: controller.chatTab
            userIcon: controller.currentUserIcon
            hasPendingApply: controller.hasPendingApply
            onR18Toggled: root.r18Toggled()
            onTabSelected: function(tab) {
                root.viewModeChangedByUser(0)
                root.lastMainTabChangedByUser(tab)
                controller.switchChatTab(tab)
            }
            onAvatarClicked: {
                root.lastMainTabChangedByUser(controller.chatTab)
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
            agentGameRooms: controller.agentController ? controller.agentController.gameRooms : []
            agentCurrentGameRoomId: controller.agentController ? controller.agentController.currentGameRoomId : ""
            agentCurrentModel: controller.agentController ? controller.agentController.currentModel : ""
            agentBusy: controller.agentController ? (controller.agentController.loading
                                                     || controller.agentController.streaming
                                                     || controller.agentController.gameBusy) : false
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
            onCreateGroupRequested: root.createGroupRequested()
            onAgentRefreshRequested: {
                if (controller.agentController) {
                    controller.agentController.loadSessions()
                    controller.agentController.listGameRooms()
                    controller.agentController.listGameRulesets()
                    controller.agentController.listGameTemplates()
                    controller.agentController.refreshModelList()
                }
            }
            onAgentNewChatRequested: root.agentChatSessionRequested()
            onAgentNewSessionRequested: root.agentGameSetupRequested("multi")
            onAgentNewGameRequested: root.agentGameSetupRequested("game")
            onAgentSessionSelected: function(sessionId) {
                if (controller.agentController) {
                    root.agentGameClosedRequested()
                    controller.agentController.switchSession(sessionId)
                }
            }
            onAgentGameRoomSelected: function(roomId) {
                if (controller.agentController) {
                    root.agentGameClosedRequested()
                    controller.agentController.loadGameRoom(roomId)
                }
            }
            onAgentSessionDeleted: function(sessionId) {
                if (controller.agentController) {
                    controller.agentController.deleteSession(sessionId)
                }
            }
            onAgentGameRoomDeleted: function(roomId) {
                if (controller.agentController) {
                    if (roomId === controller.agentController.currentGameRoomId) {
                        root.agentGameClosedRequested()
                    }
                    controller.agentController.deleteGameRoom(roomId)
                }
            }
            onMomentFriendSelected: function(uid, displayName) {
                root.momentFriendSelected(uid, displayName || "")
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
                ChatProfileCenterPane {
                    backdrop: root.backdrop
                    userIcon: controller.currentUserIcon
                    userNick: controller.currentUserNick
                    userName: controller.currentUserName
                    userDesc: controller.currentUserDesc
                    userId: controller.currentUserId
                    statusText: controller.settingsStatusText
                    statusError: controller.settingsStatusError
                    onBackRequested: {
                        root.viewModeChangedByUser(0)
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
