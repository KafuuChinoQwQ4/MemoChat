# Slice j of the gateserver microservice split: each microservice now physically
# owns its domain code. GateShared keeps ONLY the shared runtime framework
# (GateRuntimeCore: LogicSystem, HttpConnection, CServer, worker pool, the H1
# route adapter, the health module, the domain-server bootstrap) plus the
# legacy/compat aggregate (GateAppCore). The per-domain route libraries
# (Gate<X>Domain) moved into their owning service directories:
#   GateAiDomain      -> AIGatewayService/
#   GateAuthDomain    -> AccountShared/   (linked by Register + Login)
#   GateAccountDomain -> AccountShared/   (linked by Account)
#   GateMediaDomain   -> MediaService/
#   GateMomentsDomain -> MomentsService/
#   GateCallDomain    -> CallService/
#   GateR18Domain     -> R18Service/
#
# Target names are unchanged so the focused service executables, the legacy
# transports, and the structure tests keep referencing the same names. The
# shared configure helpers (memochat_configure_gate_app_lib /
# memochat_configure_gate_core_lib) and their include roots live at the
# server/core root so every service dir can reuse them.

add_library(GateRuntimeCore STATIC
    CServer.cpp
    GateHttpJsonSupport.cpp
    GateWorkerPool.cpp
    HttpConnection.cpp
    LogicSystem.cpp
    app/GateDomainServer.cpp
    app/GateDomainRuntime.cpp
    app/GateDomainRuntime.hpp
    adapters/h1/H1RouteAdapter.cpp
    modules/health/HealthRouteModule.cpp
    routing/RouteRegistry.cpp
)
memochat_configure_gate_app_lib(GateRuntimeCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateRuntimeCore
        MODULES
            memochat.gate.domain_runtime_algorithms=modules/cxx_modules/GateDomainRuntime.cppm
            memochat.gate.health_route_algorithms=modules/cxx_modules/HealthRoute.cppm
            memochat.gate.h1_route_adapter_algorithms=adapters/h1/cxx_modules/H1RouteAdapter.cppm
            memochat.gate.http_json_support_algorithms=modules/cxx_modules/GateHttpJsonSupport.cppm
            memochat.gate.module_probe=modules/cxx_modules/GateModuleProbe.cppm
            memochat.gate.routing_algorithms=modules/cxx_modules/GateRouting.cppm
            memochat.gate.worker_pool_algorithms=modules/cxx_modules/GateWorkerPool.cppm
        PRIVATE_SOURCES
            modules/cxx_modules/GateModuleProbeConsumer.cpp
        CONSUMER_SOURCES
            adapters/h1/H1RouteAdapter.cpp
            app/GateDomainRuntime.cpp
            GateHttpJsonSupport.cpp
            GateWorkerPool.cpp
            LogicSystem.cpp
            modules/health/HealthRouteModule.cpp
            routing/RouteRegistry.cpp
    )
    if(MEMOCHAT_ENABLE_GNU_STD_MODULES AND MEMOCHAT_ENABLE_CPP26_REFLECTION)
        # GCC 16 currently conflicts when -freflection consumers import a module that imports std
        # after textual standard-library includes from the consumer's declaration header.
        set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/app/GateDomainRuntime.cpp" APPEND_STRING PROPERTY COMPILE_FLAGS " -fno-reflection")
    endif()
endif()

# Compatibility aggregate for retired monolithic Gate paths and the legacy
# transports. Focused services link GateRuntimeCore plus their own Gate<X>Domain
# target instead of this. GateFullProfile registers every domain's routes, so it
# depends on all the relocated domain libraries (resolved by target name through
# GateServerCore's aggregate, which also propagates their include roots).
add_library(GateAppCore STATIC
    profiles/GateFullProfile.cpp
    GateServerH1Routes.cpp
    GateRouteModules.hpp
    transports/h3/legacy_routes/GateHttp3ServiceRoutes.hpp
    CertUtil.cpp
    CertUtil.hpp
)
memochat_configure_gate_app_lib(GateAppCore)
target_link_libraries(GateAppCore PUBLIC
    GateRuntimeCore
    GateServerCore
    GateAiDomain
    GateAuthDomain
    GateMediaDomain
    GateMomentsDomain
    GateCallDomain
    GateR18Domain
    GateAccountDomain
    GateServerHttp3
)

# The GateServer monolith executable is RETIRED and removed: every business
# domain is served by its own peeled microservice (AIGatewayService, MediaService,
# MomentsService, CallService, R18Service, RegisterService, LoginService,
# AccountService), and Envoy routes each prefix directly to those services. This
# directory now owns only the shared runtime framework + the compat aggregate;
# there is no GateServer entrypoint, config.ini, or embedded HTTP/2 monolith here.
