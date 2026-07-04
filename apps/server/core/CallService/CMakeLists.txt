# CallService owns the call domain code (relocated from GateShared in slice j).
# GateCallCore (call session/token support) and GateCallDomain (call route
# module); target names unchanged.

# --- GateCallCore: call session/token service. -------------------------------
add_library(GateCallCore STATIC
    core/support/CallPersistence.cpp
    core/support/CallPersistence.hpp
    core/support/CallPublicDtos.cpp
    core/support/CallPublicDtos.hpp
    core/support/CallSessionCacheDto.cpp
    core/support/CallSessionCacheDto.hpp
    core/support/CallService.cpp
    core/support/CallService.hpp
)
memochat_configure_gate_core_lib(GateCallCore)
target_include_directories(GateCallCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/support)
target_link_libraries(GateCallCore PUBLIC GateInfraCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateCallCore
        MODULES
            memochat.call.public_dto_algorithms=core/support/cxx_modules/CallPublicDto.cppm
            memochat.call.service_algorithms=core/support/cxx_modules/CallService.cppm
            memochat.call.session_cache_algorithms=core/support/cxx_modules/CallSessionCache.cppm
            memochat.call.session_math_algorithms=core/support/cxx_modules/CallSessionMath.cppm
            memochat.call.token_algorithms=core/support/cxx_modules/CallToken.cppm
        CONSUMER_SOURCES
            core/support/CallPublicDtos.cpp
            core/support/CallSessionCacheDto.cpp
            core/support/CallService.cpp
    )
endif()

# --- GateCallDomain: call signalling routes. ---------------------------------
add_library(GateCallDomain STATIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/GateRouteProfileRegistrar.hpp
    domain/profiles/GateCallProfile.cpp
    domain/modules/call/CallRouteModule.cpp
    domain/modules/call/CallRouteSchemas.cpp
)
memochat_configure_gate_app_lib(GateCallDomain)
target_include_directories(GateCallDomain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain)
target_link_libraries(GateCallDomain PUBLIC GateRuntimeCore GateCallCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateCallDomain
        MODULES
            memochat.call.route_module_algorithms=domain/modules/call/cxx_modules/CallRouteModule.cppm
            memochat.call.route_response_algorithms=domain/modules/call/cxx_modules/CallRouteResponse.cppm
            memochat.call.route_schema_algorithms=domain/modules/call/cxx_modules/CallRouteSchema.cppm
        CONSUMER_SOURCES
            domain/modules/call/CallRouteModule.cpp
            domain/modules/call/CallRouteSchemas.cpp
    )
endif()

add_executable(CallGatewayServer
    app/CallGatewayServer.cpp
)
memochat_configure_gate_focused_service(CallGatewayServer DOMAINS GateCallDomain)
