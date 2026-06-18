import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "qrc:/features/contact/view"
import "qrc:/qml/components"
import "sidebar"
import "qrc:/features/chat/view"
import "qrc:/features/moments/view"

Rectangle {
    id: root
    color: "transparent"
    width: 250

    property Item backdrop: null
    property int currentTab: 0
    property int currentDialogUid: 0
    property bool dialogsReady: false
    property bool contactsReady: false
    property bool groupsReady: false
    property var chatViewModel: null
    property var dialogModel
    property var contactModel
    property var searchModel
    property bool searchPending: false
    property string searchStatusText: ""
    property bool searchStatusError: false
    property bool canLoadMoreChats: false
    property bool chatLoadingMore: false
    property bool hasPendingApply: false
    property bool canLoadMoreContacts: false
    property bool contactLoadingMore: false
    property string groupStatusText: ""
    property bool groupStatusError: false
    property var agentSessions: []
    property string agentCurrentSessionId: ""
    property var agentGameRooms: []
    property string agentCurrentGameRoomId: ""
    property string agentCurrentModel: ""
    property bool agentBusy: false
    property int currentUserUid: 0
    property string currentUserId: ""
    property int momentsSelectedUid: 0
    property string momentsSelectedName: ""
    property real _sessionLastContentY: 0
    property bool _chatLoadBottomArmed: true
    property real _chatLoadThresholdPx: 48
    property real _chatLoadRearmOffsetPx: 96
    readonly property string live2dAvatarFallback: "qrc:/icons/modelive2d.png"
    property string live2dAvatarSource: ""
    property string live2dCharacterName: ""
    property string live2dModelPath: ""
    property bool live2dModelImported: false

    PetAssetSettings {
        id: live2dPetSettings
    }

    signal dialogUidSelected(int uid)
    signal chatIndexSelected(int index)
    signal groupIndexSelected(int index)
    signal requestChatLoadMore()
    signal searchRequested(string uidText)
    signal searchCleared()
    signal addFriendRequested(int uid, string bakName, var tags)
    signal openApplyRequested()
    signal contactIndexSelected(int index)
    signal requestContactLoadMore()
    signal createGroupRequested()
    signal applyJoinGroupRequested(string groupCode, string reason)
    signal agentRefreshRequested()
    signal agentNewChatRequested()
    signal agentNewSessionRequested()
    signal agentNewGameRequested()
    signal agentSessionSelected(string sessionId)
    signal agentSessionDeleted(string sessionId)
    signal agentSessionRenamed(string sessionId, string title)
    signal agentGameRoomSelected(string roomId)
    signal agentGameRoomDeleted(string roomId)
    signal momentFriendSelected(int uid, string displayName)
    signal dialogPinToggled(int uid)
    signal dialogMuteToggled(int uid)
    signal dialogMarkRead(int uid)
    signal dialogDraftCleared(int uid)

    function currentSessionListView() { return sessionPaneLoader.item ? sessionPaneLoader.item.sessionListView : null }
    function currentSessionModel() { return dialogModel }
    function usesSearchHeader() { return false }
    function trimmed(value) {
        return (value || "").toString().replace(/^\s+|\s+$/g, "")
    }
    function refreshLive2DEntryState() {
        var modelJson = root.trimmed(live2dPetSettings.modelJson)
        var modelRoot = root.trimmed(live2dPetSettings.modelRoot)
        var nextAvatar = ""
        root.live2dCharacterName = root.trimmed(live2dPetSettings.characterName)
        root.live2dModelPath = modelJson.length > 0 ? modelJson : modelRoot
        root.live2dModelImported = modelJson.length > 0
        if (root.live2dModelImported && live2dPetSettings && live2dPetSettings.resolveLive2DAvatarUrl) {
            nextAvatar = live2dPetSettings.resolveLive2DAvatarUrl(modelJson, modelRoot)
        }
        root.live2dAvatarSource = nextAvatar && nextAvatar.length > 0 ? nextAvatar : ""
    }
    function clearLive2DEntryState() {
        root.live2dCharacterName = ""
        root.live2dModelPath = ""
        root.live2dModelImported = false
        root.live2dAvatarSource = ""
    }
    function bindLive2DEntrySettingsToCurrentUser() {
        if (root.currentUserUid > 0) {
            live2dPetSettings.bindAccount(root.currentUserUid, root.currentUserId)
            live2dPetSettings.load()
            root.refreshLive2DEntryState()
            return
        }
        live2dPetSettings.clearAccountBinding()
        root.clearLive2DEntryState()
    }
    function contextualTitle() {
        if (currentTab === ShellViewModel.MomentsTabPage) {
            return "朋友圈"
        }
        if (currentTab === ShellViewModel.AgentTabPage) {
            return "AI 助手"
        }
        if (currentTab === ShellViewModel.Live2DTabPage) {
            return "Live2D 角色"
        }
        return "更多"
    }
    function contextualSubtitle() {
        if (currentTab === ShellViewModel.MomentsTabPage) {
            return momentsSelectedUid > 0
                   ? ((momentsSelectedName.length > 0 ? momentsSelectedName : "好友") + " 的朋友圈")
                   : "按好友查看朋友圈动态。"
        }
        if (currentTab === ShellViewModel.AgentTabPage) {
            var sessionCount = agentSessions ? agentSessions.length : 0
            var roomCount = agentGameRooms ? agentGameRooms.length : 0
            if (agentCurrentGameRoomId.length > 0) {
                return "已选择对话房间"
            }
            if (agentCurrentSessionId.length > 0) {
                return "已选择会话"
            }
            return "会话 " + sessionCount + " · 对话房间 " + roomCount
        }
        if (currentTab === ShellViewModel.Live2DTabPage) {
            return "模型、语音、人设和说话风格配置。"
        }
        return "在这里管理账户信息和应用设置。"
    }

    Connections {
        target: live2dPetSettings
        ignoreUnknownSignals: true
        function onSettingsChanged() {
            root.refreshLive2DEntryState()
        }
    }

    function ensureCurrentSessionSource() {
        if (currentTab !== ShellViewModel.ChatTabPage) {
            return
        }
        if (!root.chatViewModel) {
            return
        }
        root.chatViewModel.ensureChatListInitialized()
        root.chatViewModel.ensureGroupsInitialized()
    }
    function activateSession(uidValue, indexValue) {
        const sessionView = currentSessionListView()
        if (sessionView) {
            sessionView.currentIndex = indexValue
        }
        dialogUidSelected(uidValue)
    }
    function syncCurrentSelection() {
        const modelRef = currentSessionModel()
        const sessionView = currentSessionListView()
        const modelCount = modelRef && modelRef.count !== undefined ? modelRef.count : 0
        if (!modelRef || modelCount <= 0) {
            if (sessionView) {
                sessionView.currentIndex = -1
            }
            return
        }
        const targetUid = currentDialogUid
        const targetIndex = targetUid === 0 ? -1 : (modelRef.indexOfUid ? modelRef.indexOfUid(targetUid) : -1)
        if (sessionView) {
            sessionView.currentIndex = targetIndex
        }
    }

    onCurrentTabChanged: {
        if (currentTab === ShellViewModel.ChatTabPage) {
            root.ensureCurrentSessionSource()
        }

        if (currentTab === ShellViewModel.AgentTabPage) {
            agentRefreshRequested()
        }

        if (currentTab === ShellViewModel.Live2DTabPage) {
            root.bindLive2DEntrySettingsToCurrentUser()
        }
    }
    onCurrentDialogUidChanged: syncCurrentSelection()
    onCurrentUserUidChanged: root.bindLive2DEntrySettingsToCurrentUser()
    onCurrentUserIdChanged: root.bindLive2DEntrySettingsToCurrentUser()
    Component.onCompleted: {
        root.ensureCurrentSessionSource()
        root.bindLive2DEntrySettingsToCurrentUser()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ChatLeftHeader {
            Layout.fillWidth: true
            Layout.preferredHeight: root.currentTab === ShellViewModel.ChatTabPage ? 42 : 84
            backdrop: root.backdrop !== null ? root.backdrop : root
            currentTab: root.currentTab
            title: root.contextualTitle()
            subtitle: root.contextualSubtitle()
            agentBusy: root.agentBusy
            onCreateGroupRequested: root.createGroupRequested()
            onAddFriendRequested: findFriendDialog.openFresh()
            onJoinGroupRequested: joinGroupDialog.openFresh()
            onOpenApplyRequested: root.openApplyRequested()
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Qt.rgba(1, 1, 1, 0.08)
            border.color: Qt.rgba(1, 1, 1, 0.28)

            Loader {
                id: sessionPaneLoader
                anchors.fill: parent
                active: currentTab === ShellViewModel.ChatTabPage
                asynchronous: true
                sourceComponent: sessionPaneComponent
            }

            Loader {
                anchors.fill: parent
                active: currentTab === ShellViewModel.ContactTabPage
                asynchronous: true
                sourceComponent: contactPaneComponent
            }

            Loader {
                anchors.fill: parent
                active: currentTab === ShellViewModel.MomentsTabPage
                asynchronous: true
                sourceComponent: momentsPaneComponent
            }

            Loader {
                anchors.fill: parent
                active: currentTab === ShellViewModel.AgentTabPage
                asynchronous: true
                sourceComponent: agentPaneComponent
            }

            Loader {
                anchors.fill: parent
                active: currentTab === ShellViewModel.Live2DTabPage
                asynchronous: true
                sourceComponent: live2dPaneComponent
            }
        }
    }

    Component {
        id: sessionPaneComponent
        Item {
            property alias sessionListView: sessionList

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                ListView {
                    id: sessionList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    interactive: contentHeight > height
                    visible: root.dialogsReady
                    model: root.currentSessionModel()
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: GlassScrollBar { }
                    cacheBuffer: 200
                    maximumFlickVelocity: 4000

                    onContentYChanged: {
                        const movingDown = contentY > root._sessionLastContentY + 0.5
                        const nearBottom = (contentY + height) >= (contentHeight - root._chatLoadThresholdPx)
                        root._sessionLastContentY = contentY
                        if ((contentY + height) < (contentHeight - root._chatLoadRearmOffsetPx)) {
                            root._chatLoadBottomArmed = true
                        }
                        if (!root.canLoadMoreChats || root.chatLoadingMore || contentHeight <= height + 1) {
                            return
                        }
                        if (movingDown && nearBottom && root._chatLoadBottomArmed) {
                            root._chatLoadBottomArmed = false
                            root.requestChatLoadMore()
                        }
                    }

                    onCountChanged: root.syncCurrentSelection()

                    footer: Item {
                        width: sessionList.width
                        height: (root.chatLoadingMore || root.canLoadMoreChats) ? 40 : 0

                        Label {
                            anchors.centerIn: parent
                            text: root.chatLoadingMore ? "加载中..." : (root.canLoadMoreChats ? "上滑加载更多" : "")
                            color: "#8e9aac"
                            font.pixelSize: 12
                        }
                    }

                    delegate: ChatDialogRow {
                        width: ListView.view.width
                        current: ListView.isCurrentItem
                        rowIndex: index
                        uidValue: uid
                        title: name || ""
                        iconSource: icon || ""
                        lastMessage: lastMsg || ""
                        description: desc || ""
                        draftText: draftText || ""
                        pinnedRank: pinnedRank || 0
                        muteState: muteState || 0
                        unreadCount: unreadCount || 0
                        onActivated: function(uid, rowIndex) { root.activateSession(uid, rowIndex) }
                        onPinToggled: function(uid) { root.dialogPinToggled(uid) }
                        onMuteToggled: function(uid) { root.dialogMuteToggled(uid) }
                        onMarkRead: function(uid) { root.dialogMarkRead(uid) }
                        onDraftCleared: function(uid) { root.dialogDraftCleared(uid) }
                    }
                }
            }

            GlassSurface {
                anchors.centerIn: parent
                width: 220
                height: 68
                visible: !sessionList.visible || (sessionList.count === 0 && !root.chatLoadingMore)
                backdrop: root.backdrop !== null ? root.backdrop : root
                cornerRadius: 10
                blurRadius: 16
                fillColor: Qt.rgba(1, 1, 1, 0.20)
                strokeColor: Qt.rgba(1, 1, 1, 0.42)

                Label {
                    anchors.centerIn: parent
                        text: !root.dialogsReady ? "正在加载最近会话" : "暂无会话"
                    color: "#6a7b92"
                    font.pixelSize: 13
                }
            }
        }
    }

    Component {
        id: contactPaneComponent
        ContactListPane {
            anchors.fill: parent
            backdrop: root.backdrop
            contactModel: root.contactModel
            hasPendingApply: root.hasPendingApply
            canLoadMore: root.canLoadMoreContacts
            loadingMore: root.contactLoadingMore
            onOpenApplyRequested: root.openApplyRequested()
            onContactIndexSelected: function(index) { root.contactIndexSelected(index) }
            onLoadMoreRequested: root.requestContactLoadMore()
            Component.onCompleted: contact.ensureContactsInitialized()
        }
    }

    Component {
        id: momentsPaneComponent
        MomentsFriendSidePane {
            anchors.fill: parent
            backdrop: root.backdrop !== null ? root.backdrop : root
            contactModel: root.contactModel
            selectedUid: root.momentsSelectedUid
            onFriendSelected: function(uid, displayName) { root.momentFriendSelected(uid, displayName) }

            Component.onCompleted: contact.ensureContactsInitialized()
        }
    }

    Component {
        id: agentPaneComponent
        ChatAgentSidePane {
            anchors.fill: parent
            backdrop: root.backdrop !== null ? root.backdrop : root
            agentSessions: root.agentSessions
            agentGameRooms: root.agentGameRooms
            currentSessionId: root.agentCurrentSessionId
            currentGameRoomId: root.agentCurrentGameRoomId
            currentModel: root.agentCurrentModel
            busy: root.agentBusy
            onNewChatRequested: root.agentNewChatRequested()
            onNewSessionRequested: root.agentNewSessionRequested()
            onNewGameRequested: root.agentNewGameRequested()
            onSessionSelected: function(sessionId) { root.agentSessionSelected(sessionId) }
            onSessionDeleted: function(sessionId) { root.agentSessionDeleted(sessionId) }
            onSessionRenamed: function(sessionId, title) { root.agentSessionRenamed(sessionId, title) }
            onGameRoomSelected: function(roomId) { root.agentGameRoomSelected(roomId) }
            onGameRoomDeleted: function(roomId) { root.agentGameRoomDeleted(roomId) }
        }
    }

    Component {
        id: live2dPaneComponent
        ChatLive2DEntryPane {
            anchors.fill: parent
            backdrop: root.backdrop !== null ? root.backdrop : root
            avatarSource: root.live2dAvatarSource
            avatarFallback: root.live2dAvatarFallback
            characterName: root.live2dCharacterName
            modelPath: root.live2dModelPath
            hasImportedModel: root.live2dModelImported
        }
    }

    AddFriendDialog {
        id: addFriendDialog
        anchors.centerIn: Overlay.overlay
        backdrop: root.backdrop !== null ? root.backdrop : root
        onSubmitted: function(uid, backName, tags) { root.addFriendRequested(uid, backName, tags) }
    }

    ChatFindFriendPopup {
        id: findFriendDialog
        backdrop: root.backdrop !== null ? root.backdrop : root
        searchModel: root.searchModel
        searchPending: root.searchPending
        searchStatusText: root.searchStatusText
        searchStatusError: root.searchStatusError
        onSearchRequested: function(uidText) { root.searchRequested(uidText) }
        onSearchCleared: root.searchCleared()
        onAddFriendDialogRequested: function(uid, name, description, icon) {
            addFriendDialog.openFor(uid, name, description, icon)
        }
    }

    ChatJoinGroupPopup {
        id: joinGroupDialog
        backdrop: root.backdrop !== null ? root.backdrop : root
        groupStatusText: root.groupStatusText
        groupStatusError: root.groupStatusError
        onApplyJoinGroupRequested: function(groupCode, reason) {
            root.applyJoinGroupRequested(groupCode, reason)
        }
    }
}
