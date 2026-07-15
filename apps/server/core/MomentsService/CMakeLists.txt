# MomentsService owns the moments domain code (relocated from GateShared in
# slice j). GateMomentsDomain (moments route module + service facade); target
# name unchanged. A narrow local persistence Module owns the remaining Postgres
# adapter calls for moment rows and user-profile reads.

add_library(GateMomentsDomain STATIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/GateRouteProfileRegistrar.hpp
    domain/profiles/GateMomentsProfile.cpp
    domain/modules/moments/MomentsRouteModule.cpp
    domain/modules/moments/MomentsRouteSchemas.cpp
    domain/services/moments/MomentsOutputDtos.cpp
    domain/services/moments/MomentsOutputDtos.hpp
    domain/services/moments/MomentsPersistence.cpp
    domain/services/moments/MomentsPersistence.hpp
    domain/services/moments/MomentsPublicDtos.cpp
    domain/services/moments/MomentsPublicDtos.hpp
    domain/services/moments/MomentsService.cpp
    domain/MomentsRouteModules.cpp
    domain/MomentsRouteModules.hpp
    clients/MomentsRelationClient.cpp
    clients/MomentsRelationClient.hpp
)
memochat_configure_gate_app_lib(GateMomentsDomain)
target_include_directories(GateMomentsDomain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain ${CMAKE_CURRENT_SOURCE_DIR})
# Moments owns no Gate<X>Core facade, so it declares its infra slices explicitly
# instead of inheriting the full closure the framework used to force on everyone:
# MomentsPersistence calls PostgresMgr/MongoMgr (GateInfraPersistence) and
# MomentsService delegates to the shared user-token validator (GateInfraSupport,
# which brings GateInfraCache for the Redis readiness probe). The AI/Media/etc
# services get these transitively through their own cores; Moments is explicit.
target_link_libraries(GateMomentsDomain PUBLIC GateRuntimeCore GateInfraPersistence GateInfraSupport)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateMomentsDomain
        MODULES
            memochat.moments.output_algorithms=domain/services/moments/cxx_modules/MomentsOutput.cppm
            memochat.moments.public_algorithms=domain/services/moments/cxx_modules/MomentsPublic.cppm
            memochat.moments.route_registration_algorithms=domain/modules/moments/cxx_modules/MomentsRouteRegistration.cppm
            memochat.moments.route_schema_algorithms=domain/modules/moments/cxx_modules/MomentsRouteSchema.cppm
            memochat.moments.service_algorithms=domain/services/moments/cxx_modules/MomentsService.cppm
        CONSUMER_SOURCES
            domain/modules/moments/MomentsRouteModule.cpp
            domain/modules/moments/MomentsRouteSchemas.cpp
            domain/services/moments/MomentsOutputDtos.cpp
            domain/services/moments/MomentsPublicDtos.cpp
            domain/services/moments/MomentsService.cpp
    )
endif()

add_executable(MomentsGatewayServer
    app/MomentsGatewayServer.cpp
)
memochat_configure_gate_focused_service(MomentsGatewayServer DOMAINS GateMomentsDomain)
