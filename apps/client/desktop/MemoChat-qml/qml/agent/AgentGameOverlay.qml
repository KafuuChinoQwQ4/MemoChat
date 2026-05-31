import QtQuick 2.15
import MemoChat 1.0

Loader {
    id: root
    active: root.viewMode === 0
            && controller.chatTab === AppController.AgentTabPage
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
            agentController: controller.agentController
            availableModels: controller.agentController ? controller.agentController.availableModels : []
            currentModel: controller.agentController ? controller.agentController.currentModel : ""
            apiProviderBusy: controller.agentController ? controller.agentController.apiProviderBusy : false
            apiProviderStatus: controller.agentController ? controller.agentController.apiProviderStatus : ""
            gameRooms: controller.agentController ? controller.agentController.gameRooms : []
            gameTemplates: controller.agentController ? controller.agentController.gameTemplates : []
            gameTemplatePresets: controller.agentController ? controller.agentController.gameTemplatePresets : []
            gameState: controller.agentController ? controller.agentController.gameState : ({})
            currentGameRoomId: controller.agentController ? controller.agentController.currentGameRoomId : ""
            setupModeToken: root.setupModeToken
            setupKind: root.setupKind
            gameRulesets: controller.agentController ? controller.agentController.gameRulesets : []
            gameRolePresets: controller.agentController ? controller.agentController.gameRolePresets : []
            gameBusy: controller.agentController ? controller.agentController.gameBusy : false
            gameStatusText: controller.agentController ? controller.agentController.gameStatusText : ""
            gameError: controller.agentController ? controller.agentController.gameError : ""
            humanDisplayName: controller.currentUserNick && controller.currentUserNick.length > 0
                              ? controller.currentUserNick
                              : controller.currentUserName
            onCloseRequested: root.closeRequested()
        }
    }
}
