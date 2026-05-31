import QtQuick 2.15
import QtQuick.Window 2.15
import MemoChat 1.0
import "../components"
import "../chat"
import "../agent"
import "../r18"
import "ChatShellRuntime.js" as ChatShellRuntime

Rectangle {
    id: root
    color: "transparent"

    property int topInset: 0
    property real revealProgress: 1.0
    property int viewMode: 0 // 0 = main tabs, 1 = profile center
    property int lastMainTab: controller.chatTab
    property bool groupCreationDialogActivated: false
    property bool groupManagementPanelActive: false
    property int momentsSelectedUid: 0
    property string momentsSelectedName: ""
    property bool r18Mode: false
    property bool r18PaneWarm: false
    property bool pendingR18Flip: false
    property bool targetR18Mode: false
    property real flipAngle: 0
    property int r18ViewMode: 0
    property bool agentGameActive: false
    property int agentGameSetupToken: 0
    property string agentGameSetupKind: "multi"
    readonly property bool flipInProgress: flipAnimation.running || pendingR18Flip
    readonly property bool glassEffectsEnabled: Qt.platform.os !== "linux"
    readonly property real acrylicPinkProgress: 0
    readonly property var r18NavigationItems: ChatShellRuntime.r18NavigationItems()

    signal petPreviewRequested(var petAssetSettings)

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
        return ChatShellRuntime.stageValue(revealProgress, start, span)
    }

    function syncWindowAcrylicTint() {
        if (Window.window && Window.window.hasOwnProperty("acrylicPinkProgress")) {
            Window.window.acrylicPinkProgress = root.acrylicPinkProgress
        }
    }

    function toggleR18Mode() {
        if (flipAnimation.running || pendingR18Flip) {
            return
        }
        beginR18Flip(!r18Mode)
    }

    function beginR18Flip(nextMode) {
        targetR18Mode = nextMode
        if (nextMode && !r18PaneWarm) {
            pendingR18Flip = true
            r18PaneWarm = true
            r18WarmupTimer.restart()
            return
        }
        r18Mode = nextMode
        flipAnimation.to = nextMode ? 180 : 0
        flipAnimation.start()
    }

    function finishPendingR18Flip() {
        if (!pendingR18Flip || !r18Face.r18Ready) {
            return
        }
        pendingR18Flip = false
        Qt.callLater(function() {
            root.beginR18Flip(root.targetR18Mode)
        })
    }

    function switchAccountToLogin() {
        Qt.callLater(function() {
            controller.switchToLogin()
        })
    }

    function openGroupCreationDialog() {
        root.groupCreationDialogActivated = true
        modalLayer.openGroupCreationDialog()
    }

    function openAgentGameSetup(kind) {
        root.agentGameSetupKind = ChatShellRuntime.defaultAgentGameSetupKind(kind)
        root.agentGameSetupToken += 1
        root.agentGameActive = true
        if (controller.agentController) {
            controller.agentController.refreshModelList()
            controller.agentController.listGameRooms()
            controller.agentController.listGameRulesets()
            controller.agentController.listGameTemplates()
        }
    }

    function createAgentChatSession() {
        root.agentGameActive = false
        if (controller.agentController) {
            controller.agentController.createSession()
        }
    }

    NumberAnimation {
        id: flipAnimation
        target: root
        property: "flipAngle"
        duration: 320
        easing.type: Easing.OutCubic
    }

    Timer {
        id: r18WarmupTimer
        interval: 40
        repeat: true
        onTriggered: {
            if (!root.pendingR18Flip) {
                stop()
                return
            }
            if (r18Face.r18Ready) {
                stop()
                root.finishPendingR18Flip()
            }
        }
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

        ChatNormalFace {
            id: normalFace
            anchors.fill: parent
            backdrop: backdropLayer
            revealProgress: root.revealProgress
            flipAngle: root.flipAngle
            flipInProgress: root.flipInProgress
            glassEffectsEnabled: root.glassEffectsEnabled
            viewMode: root.viewMode
            lastMainTab: root.lastMainTab
            groupManagementPanelActive: root.groupManagementPanelActive
            momentsSelectedUid: root.momentsSelectedUid
            momentsSelectedName: root.momentsSelectedName
            onR18Toggled: root.toggleR18Mode()
            onViewModeChangedByUser: function(mode) { root.viewMode = mode }
            onLastMainTabChangedByUser: function(tab) { root.lastMainTab = tab }
            onCreateGroupRequested: root.openGroupCreationDialog()
            onAgentGameClosedRequested: root.agentGameActive = false
            onAgentChatSessionRequested: root.createAgentChatSession()
            onAgentGameSetupRequested: function(kind) { root.openAgentGameSetup(kind) }
            onMomentFriendSelected: function(uid, displayName) {
                root.momentsSelectedUid = uid
                root.momentsSelectedName = displayName || ""
                if (controller.momentsController) {
                    controller.momentsController.loadFeedForAuthor(uid)
                }
            }
            onGroupManagementPanelActiveChangedByUser: function(active) {
                root.groupManagementPanelActive = active
            }
            onSwitchAccountToLoginRequested: root.switchAccountToLogin()
            onPetPreviewRequested: function(petAssetSettings) {
                root.petPreviewRequested(petAssetSettings)
            }
            onAvatarProfileRequested: function(uid, name, icon) {
                modalLayer.openProfile(uid, name, icon, "")
            }
        }

        AgentGameOverlay {
            anchors.fill: normalFace
            viewMode: root.viewMode
            agentGameActive: root.agentGameActive
            setupModeToken: root.agentGameSetupToken
            setupKind: root.agentGameSetupKind
            onCloseRequested: root.agentGameActive = false
        }

        R18FlipFace {
            id: r18Face
            anchors.fill: parent
            backdrop: backdropLayer
            flipAngle: root.flipAngle
            flipInProgress: root.flipInProgress
            glassEffectsEnabled: root.glassEffectsEnabled
            r18PaneWarm: root.r18PaneWarm
            r18ViewMode: root.r18ViewMode
            r18NavigationItems: root.r18NavigationItems
            r18Controller: controller.r18Controller
            onViewModeChangedByUser: function(mode) { root.r18ViewMode = mode }
            onReturnToChatRequested: root.toggleR18Mode()
            onPendingFlipReady: root.finishPendingR18Flip()
        }
    }

    ChatModalLayer {
        id: modalLayer
        z: 100
        anchors.fill: parent
        backdrop: backdropLayer
        groupCreationDialogActivated: root.groupCreationDialogActivated
        friendModel: controller.contactListModel
        callSession: controller.callSession
        appController: controller
        onCreateGroupSubmitted: function(name, memberUserIds) { controller.createGroup(name, memberUserIds) }
        onAcceptRequested: controller.acceptIncomingCall()
        onRejectRequested: controller.rejectIncomingCall()
        onEndRequested: controller.endCurrentCall()
        onMuteToggled: controller.toggleCallMuted()
        onCameraToggled: controller.toggleCallCamera()
    }
}
