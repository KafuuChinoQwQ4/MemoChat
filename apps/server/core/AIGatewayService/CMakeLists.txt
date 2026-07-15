# AIGatewayService owns the AI gateway domain code (relocated from GateShared in
# slice j). GateAiDomain is the AI route library; target name unchanged.
#
# The AI domain is a pure proxy to the AI orchestrator / AIServer (no Postgres/
# Mongo/storage), so it needs no Gate<X>Core support library — just the runtime
# framework + the shared infra base (for ConfigMgr) + proto for the gRPC client.

add_library(GateAiDomain STATIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/GateRouteProfileRegistrar.hpp
    domain/profiles/GateAiProfile.cpp
    domain/modules/ai/AIRouteModule.cpp
    domain/modules/ai/AIRouteSchemas.cpp
    domain/services/ai/AIPublicDtos.cpp
    domain/services/ai/AIPublicDtos.hpp
    domain/services/ai/AIService.cpp
    domain/AIServiceClient.cpp
    domain/AIRouteModules.cpp
    domain/AIRouteModules.hpp
    domain/AIHttpServiceRoutes.hpp
)
memochat_configure_gate_app_lib(GateAiDomain)
target_include_directories(GateAiDomain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain)
# The AI gateway is a pure proxy: it owns no database, so it must NOT drag in the
# persistence slice (libpq/mongoc/bson). It links only the infra slices it truly
# uses — GateInfraSupport (bearer/utoken auth; pulls cache+config transitively)
# for AIService's token checks and the AIGatewayServer main's RedisMgr use. This
# is the decoupling the split enables: proof is `ldd AIGatewayServer` showing no
# libpq/libmongoc. The slimmed GateRuntimeCore no longer force-links persistence.
target_link_libraries(GateAiDomain PUBLIC GateRuntimeCore GateInfraSupport memochat_proto)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateAiDomain
        MODULES
            memochat.ai.route_module_algorithms=domain/modules/cxx_modules/AIRouteModule.cppm
            memochat.ai.route_registration_algorithms=domain/modules/cxx_modules/AIRouteRegistration.cppm
            memochat.ai.route_schema_algorithms=domain/modules/cxx_modules/AIRouteSchema.cppm
            memochat.ai.gateway_service_algorithms=domain/services/ai/cxx_modules/AIService.cppm
            memochat.ai.public_dto_algorithms=domain/services/ai/cxx_modules/AIPublicDto.cppm
        CONSUMER_SOURCES
            domain/AIRouteModules.cpp
            domain/modules/ai/AIRouteModule.cpp
            domain/modules/ai/AIRouteSchemas.cpp
            domain/services/ai/AIPublicDtos.cpp
            domain/services/ai/AIService.cpp
    )
endif()

add_executable(AIGatewayServer
    app/AIGatewayRuntime.cpp
    app/AIGatewayRuntime.hpp
    app/AIGatewayServer.cpp
)
memochat_configure_gate_focused_service(AIGatewayServer DOMAINS GateAiDomain)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(AIGatewayServer
        MODULES
            memochat.ai.gateway_runtime_algorithms=app/cxx_modules/AIGatewayRuntime.cppm
        CONSUMER_SOURCES
            app/AIGatewayRuntime.cpp
    )
endif()
