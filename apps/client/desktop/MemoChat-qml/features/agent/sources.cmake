list(APPEND MEMOCHAT_QML_FEATURE_SOURCES
    features/agent/controller/AgentControllerChat.cpp
    features/agent/controller/AgentController.cpp
    features/agent/controller/AgentControllerAgentTasks.cpp
    features/agent/controller/AgentControllerKnowledge.cpp
    features/agent/controller/AgentControllerMemory.cpp
    features/agent/controller/AgentControllerModels.cpp
    features/agent/controller/AgentControllerSessions.cpp
    features/agent/controller/AgentControllerState.cpp
    features/agent/controller/AgentControllerStream.cpp
    features/agent/game/AgentGameClient.cpp
    features/agent/game/AgentControllerGameNetwork.cpp
    features/agent/game/AgentControllerGameResponses.cpp
    features/agent/game/AgentControllerGameRooms.cpp
    features/agent/game/AgentControllerGameTemplates.cpp
    features/agent/model/AgentMessageModel.cpp
    features/agent/transport/AgentRequestTracker.cpp
    features/agent/transport/AgentStreamClient.cpp
)

list(APPEND MEMOCHAT_QML_FEATURE_HEADERS
    features/agent/controller/AgentController.h
    features/agent/game/AgentGameClient.h
    features/agent/model/AgentMessageModel.h
    features/agent/model/AgentRequestTypes.h
    features/agent/transport/AgentNetworkRequestUtils.h
    features/agent/transport/AgentRequestTracker.h
    features/agent/transport/AgentStreamClient.h
)

list(APPEND MEMOCHAT_QML_FEATURE_RESOURCES
    features/agent/resources/agent.qrc
)
