# CallService owns the call domain code (relocated from GateShared in slice j).
# GateCallCore (call session/token support) and GateCallDomain (call route
# module); target names unchanged.

# --- GateCallCore: call session/token service. -------------------------------
add_library(GateCallCore STATIC
    core/support/CallPublicDtos.cpp
    core/support/CallPublicDtos.h
    core/support/CallSessionCacheDto.cpp
    core/support/CallSessionCacheDto.h
    core/support/CallService.cpp
    core/support/CallService.h
)
memochat_configure_gate_core_lib(GateCallCore)
target_include_directories(GateCallCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/support)
target_link_libraries(GateCallCore PUBLIC GateInfraCore)

# --- GateCallDomain: call signalling routes. ---------------------------------
add_library(GateCallDomain STATIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/GateRouteProfileRegistrar.h
    domain/profiles/GateCallProfile.cpp
    domain/modules/call/CallRouteModule.cpp
    domain/modules/call/CallRouteSchemas.cpp
)
memochat_configure_gate_app_lib(GateCallDomain)
target_include_directories(GateCallDomain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain)
target_link_libraries(GateCallDomain PUBLIC GateRuntimeCore GateCallCore)

add_executable(CallGatewayServer
    app/CallGatewayServer.cpp
)
memochat_configure_gate_focused_service(CallGatewayServer DOMAINS GateCallDomain)
