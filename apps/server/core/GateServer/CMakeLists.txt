option(MEMOCHAT_BUILD_EMBEDDED_GATE_HTTP2 "Compile the legacy embedded HTTP/2 listener into GateServer" OFF)

add_executable(GateServer
    GateServer.cpp
    CServer.cpp
    GateHttpJsonSupport.cpp
    GateServerH1Routes.cpp
    GateR18Routes.cpp
    GateWorkerPool.cpp
    HttpConnection.cpp
    LogicSystem.cpp
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

target_link_libraries(GateServer PRIVATE
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
