# MomentsService owns the moments domain code (relocated from GateShared in
# slice j). GateMomentsDomain (moments route module + service facade); target
# name unchanged. Moments reads its own Mongo + the one known cross-domain user
# lookup, so it needs no dedicated Gate<X>Core support library beyond the shared
# infra base (Postgres/Mongo managers live in GateInfraCore).

add_library(GateMomentsDomain STATIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/GateRouteProfileRegistrar.h
    domain/profiles/GateMomentsProfile.cpp
    domain/modules/moments/MomentsRouteModule.cpp
    domain/modules/moments/MomentsRouteSchemas.cpp
    domain/services/moments/MomentsOutputDtos.cpp
    domain/services/moments/MomentsOutputDtos.h
    domain/services/moments/MomentsPublicDtos.cpp
    domain/services/moments/MomentsPublicDtos.h
    domain/services/moments/MomentsService.cpp
    domain/MomentsRouteModules.cpp
    domain/MomentsRouteModules.h
    clients/MomentsRelationClient.cpp
    clients/MomentsRelationClient.h
)
memochat_configure_gate_app_lib(GateMomentsDomain)
target_include_directories(GateMomentsDomain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(GateMomentsDomain PUBLIC GateRuntimeCore)

add_executable(MomentsGatewayServer
    app/MomentsGatewayServer.cpp
)
memochat_configure_gate_focused_service(MomentsGatewayServer DOMAINS GateMomentsDomain)
