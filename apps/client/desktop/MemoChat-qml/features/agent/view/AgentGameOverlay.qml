import QtQuick 2.15
import MemoChat 1.0

Loader {
    id: root
    active: root.viewMode === 0
            && shell.chatTab === ShellViewModel.AgentTabPage
            && root.agentGameActive
    visible: active
    z: 50
    asynchronous: true

    property int viewMode: 0
    property bool agentGameActive: false
    property int setupModeToken: 0
    property string setupKind: "multi"

    signal closeRequested()

    sourceComponent: Component {
        AgentGamePane {
            agentController: agent
            availableModels: agent ? agent.availableModels : []
            currentModel: agent ? agent.currentModel : ""
            apiProviderBusy: agent ? agent.apiProviderBusy : false
            apiProviderStatus: agent ? agent.apiProviderStatus : ""
            gameRooms: agent ? agent.gameRooms : []
            gameTemplates: agent ? agent.gameTemplates : []
            gameTemplatePresets: agent ? agent.gameTemplatePresets : []
            gameState: agent ? agent.gameState : ({})
            currentGameRoomId: agent ? agent.currentGameRoomId : ""
            setupModeToken: root.setupModeToken
            setupKind: root.setupKind
            gameRulesets: agent ? agent.gameRulesets : []
            gameRolePresets: agent ? agent.gameRolePresets : []
            gameBusy: agent ? agent.gameBusy : false
            gameStatusText: agent ? agent.gameStatusText : ""
            gameError: agent ? agent.gameError : ""
            humanDisplayName: shell.currentUserNick && shell.currentUserNick.length > 0
                              ? shell.currentUserNick
                              : shell.currentUserName
            onCloseRequested: root.closeRequested()
        }
    }
}
