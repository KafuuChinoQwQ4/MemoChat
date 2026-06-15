option(MEMOCHAT_BUILD_EMBEDDED_GATE_HTTP2 "Compile the legacy embedded HTTP/2 listener into GateServer" OFF)

# GateAppCore: the shared H1 application layer (transport + routing + every
# domain module/service) used by the GateServer monolith AND by focused
# single-domain processes peeled off during the microservice split (Phase 3+).
# It holds everything the GateServer executable used to compile directly EXCEPT
# the entrypoint (GateServer.cpp), so multiple executables can link the same
# application core and select a LogicSystem::RouteProfile at startup.
add_library(GateAppCore STATIC
    CServer.cpp
    GateHttpJsonSupport.cpp
    GateServerH1Routes.cpp
    GateR18Routes.cpp
    GateWorkerPool.cpp
    HttpConnection.cpp
    LogicSystem.cpp
    app/GateDomainServer.cpp
    adapters/h1/H1RouteAdapter.cpp
    modules/ai/AIRouteModule.cpp
    modules/auth/AuthRouteModule.cpp
    modules/call/CallRouteModule.cpp
    modules/health/HealthRouteModule.cpp
    modules/media/MediaRouteModule.cpp
    modules/moments/MomentsRouteModule.cpp
    modules/profile/ProfileRouteModule.cpp
    modules/r18/R18RouteModule.cpp
    routing/RouteRegistry.cpp
    services/ai/AIService.cpp
    services/account/AccountPersistence.cpp
    services/auth/AuthService.cpp
    services/media/MediaService.cpp
    services/moments/MomentsService.cpp
    services/r18/R18Service.cpp
    MediaHttpService.cpp
    AIServiceClient.cpp
    MomentsRouteModules.cpp
    AIRouteModules.cpp
    MomentsRouteModules.h
    GateRouteModules.h
    AIRouteModules.h
    transports/h3/legacy_routes/GateHttp3ServiceRoutes.h
    CertUtil.cpp
    CertUtil.h
)
set_target_properties(GateAppCore PROPERTIES FOLDER "Server/GateServer")
memochat_configure_server_target(GateAppCore)
target_link_libraries(GateAppCore PUBLIC
    GateServerCore
    GateServerHttp3
    memochat_boost_stubs
)
target_include_directories(GateAppCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3
    ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3/listener
    ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3/legacy_routes
)

add_executable(GateServer
    GateServer.cpp
)

# The embedded HTTP/2 listener is legacy/experimental. Envoy owns production
# HTTP/2 ingress, so this source block must stay behind an explicit opt-in.
# NOTE: GateServer's CertUtil.cpp is already in the main target above; we do NOT
# compile transports/h2/standalone/CertUtil.cpp here to avoid duplicate symbol errors.
if(MEMOCHAT_BUILD_EMBEDDED_GATE_HTTP2 AND MEMOCHAT_HAVE_NGHTTP2)
    target_sources(GateServer PRIVATE
        NgHttp2Server.cpp
        transports/h2/adapters/h2/H2RouteAdapter.cpp
        transports/h2/routes/Http2Routes.cpp
        transports/h2/handlers/Http2Handlers.cpp
        transports/h2/handlers/Http2AuthHandlers.cpp
        transports/h2/support/Http2AuthSupport.cpp
        transports/h2/handlers/Http2CallHandlers.cpp
        transports/h2/support/Http2CallSupport.cpp
        transports/h2/handlers/Http2MediaHandlers.cpp
        transports/h2/handlers/Http2ProfileHandlers.cpp
        transports/h2/support/Http2MomentsSupport.cpp
    )
    # Header-only support modules
    target_sources(GateServer PRIVATE
        transports/h2/support/Http2MediaSupport.h
        transports/h2/support/Http2ProfileSupport.h
    )
    target_include_directories(GateServer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/transports/h2
        ${CMAKE_CURRENT_SOURCE_DIR}/transports/h2/adapters
        ${CMAKE_CURRENT_SOURCE_DIR}/transports/h2/routes
        ${CMAKE_CURRENT_SOURCE_DIR}/transports/h2/handlers
        ${CMAKE_CURRENT_SOURCE_DIR}/transports/h2/support
        ${CMAKE_CURRENT_SOURCE_DIR}/transports/h2/standalone
    )
elseif(MEMOCHAT_BUILD_EMBEDDED_GATE_HTTP2)
    message(WARNING "MEMOCHAT_BUILD_EMBEDDED_GATE_HTTP2=ON but nghttp2/OpenSSL support is unavailable; skipping embedded Gate HTTP/2 sources")
endif()

set_target_properties(GateServer PROPERTIES FOLDER "Server")
memochat_configure_server_target(GateServer)

# GateServer links the shared application core (which transitively brings in
# GateServerCore + GateServerHttp3). The default LogicSystem::RouteProfile is
# Full, so GateServer registers every domain module exactly as before.
target_link_libraries(GateServer PRIVATE
    GateAppCore
    GateServerCore
    GateServerHttp3
    memochat_boost_stubs
)

if(WIN32)
    if(DEFINED ENV{WINSDK_LIB_UM} AND NOT "$ENV{WINSDK_LIB_UM}" STREQUAL "")
        set(_WINSDK_LIB_UM "$ENV{WINSDK_LIB_UM}")
    else()
        set(_WINSDK_LIB_UM "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/um/x64")
    endif()
    if(DEFINED ENV{WINSDK_LIB_UCRT} AND NOT "$ENV{WINSDK_LIB_UCRT}" STREQUAL "")
        set(_WINSDK_LIB_UCRT "$ENV{WINSDK_LIB_UCRT}")
    else()
        set(_WINSDK_LIB_UCRT "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/ucrt/x64")
    endif()
    target_link_options(GateServer PRIVATE
        "/LIBPATH:${_WINSDK_LIB_UM}"
        "/LIBPATH:${_WINSDK_LIB_UCRT}")
endif()

target_include_directories(GateServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3
    ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3/listener
    ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3/legacy_routes
)

if(WIN32 AND MEMOCHAT_ENABLE_HTTP3)
    find_path(MSQUIC_BIN_DIR NAMES msquic.dll PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg_installed/x64-windows/bin"
        NO_DEFAULT_PATH)
    if(MSQUIC_BIN_DIR)
        add_custom_command(TARGET GateServer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MSQUIC_BIN_DIR}/msquic.dll"
                "$<TARGET_FILE_DIR:GateServer>")
    endif()
endif()

# ── AIGatewayServer: first peeled microservice (gateserver split Phase 3) ─────
# A standalone process that reuses GateAppCore but selects the AIGateway route
# profile, so it serves only /healthz, /readyz and /ai/* (a pure proxy to
# AIServer/AIOrchestrator — no DB of its own). It is built by default but is NOT
# part of the default runtime topology: it stays opt-in until runtime smoke
# proves it, per the split guardrails. Its entrypoint is the only source unique
# to it; all behavior comes from the shared GateAppCore.
add_executable(AIGatewayServer
    app/AIGatewayServer.cpp
)
set_target_properties(AIGatewayServer PROPERTIES FOLDER "Server")
memochat_configure_server_target(AIGatewayServer)
target_link_libraries(AIGatewayServer PRIVATE
    GateAppCore
    GateServerCore
    GateServerHttp3
    memochat_boost_stubs
)
target_include_directories(AIGatewayServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3
    ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3/listener
    ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3/legacy_routes
)

# ── Phase 4 domain gateways (Media/Moments/Call/R18) ─────────────────────────
# Each is a standalone process reusing GateAppCore + a LogicSystem::RouteProfile,
# serving only its own domain routes (+ health). All are built by default but are
# opt-in in the runtime topology (NOT in MEMOCHAT_CORE_START_GROUPS) until their
# runtime smoke + Envoy routing are flipped on. Entry points are the only sources
# unique to each; all behavior comes from the shared GateAppCore.
function(memochat_add_gate_domain_server target_name entry_src)
    add_executable(${target_name} ${entry_src})
    set_target_properties(${target_name} PROPERTIES FOLDER "Server")
    memochat_configure_server_target(${target_name})
    target_link_libraries(${target_name} PRIVATE
        GateAppCore
        GateServerCore
        GateServerHttp3
        memochat_boost_stubs
    )
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3
        ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3/listener
        ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3/legacy_routes
    )
endfunction()

memochat_add_gate_domain_server(MediaGatewayServer app/MediaGatewayServer.cpp)
memochat_add_gate_domain_server(MomentsGatewayServer app/MomentsGatewayServer.cpp)
memochat_add_gate_domain_server(CallGatewayServer app/CallGatewayServer.cpp)
memochat_add_gate_domain_server(R18GatewayServer app/R18GatewayServer.cpp)

# ── Phase 5 account aggregate (Register/Login/Account) ───────────────────────
# The memo_account aggregate split (D-ACCOUNT): RegisterServer + LoginServer are
# separate processes but reach account data only via account-core
# (AccountPersistence); AccountServer owns the profile write path. All opt-in
# (default OFF) and reuse the same GateAppCore + RouteProfile template.
memochat_add_gate_domain_server(RegisterServer app/RegisterServer.cpp)
memochat_add_gate_domain_server(LoginServer app/LoginServer.cpp)
memochat_add_gate_domain_server(AccountServer app/AccountServer.cpp)
