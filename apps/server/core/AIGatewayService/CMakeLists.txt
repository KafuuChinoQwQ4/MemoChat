# AIGatewayService owns the AI gateway domain code (relocated from GateShared in
# slice j). GateAiDomain is the AI route library; target name unchanged.
#
# The AI domain is a pure proxy to the AI orchestrator / AIServer (no Postgres/
# Mongo/storage), so it needs no Gate<X>Core support library — just the runtime
# framework + the shared infra base (for ConfigMgr) + proto for the gRPC client.

add_library(GateAiDomain STATIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/GateRouteProfileRegistrar.h
    domain/profiles/GateAiProfile.cpp
    domain/modules/ai/AIRouteModule.cpp
    domain/services/ai/AIService.cpp
    domain/AIServiceClient.cpp
    domain/AIRouteModules.cpp
    domain/AIRouteModules.h
    domain/AIHttpServiceRoutes.h
)
memochat_configure_gate_app_lib(GateAiDomain)
target_include_directories(GateAiDomain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain)
target_link_libraries(GateAiDomain PUBLIC GateRuntimeCore memochat_proto)

add_executable(AIGatewayServer
    app/AIGatewayServer.cpp
)
memochat_configure_gate_focused_service(AIGatewayServer DOMAINS GateAiDomain)
