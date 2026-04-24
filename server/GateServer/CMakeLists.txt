add_executable(GateServer
    GateServer.cpp
    CServer.cpp
    GateHttpJsonSupport.cpp
    GateServerH1Routes.cpp
    GateWorkerPool.cpp
    HttpConnection.cpp
    LogicSystem.cpp
    MediaHttpService.cpp
    AIServiceClient.cpp
    MomentsRouteModules.cpp
    AIRouteModules.cpp
    MomentsRouteModules.h
    GateRouteModules.h
    AIRouteModules.h
    ../GateServerHttp3/GateHttp3ServiceRoutes.h
    CertUtil.cpp
    CertUtil.h
)

# When nghttp2 is available, embed NgHttp2Server (HTTP/2) directly inside GateServer.
# NOTE: GateServer's CertUtil.cpp is already in the main target above; we do NOT
# compile GateServerHttp2/CertUtil.cpp here to avoid duplicate symbol errors.
if(MEMOCHAT_HAVE_NGHTTP2)
    target_sources(GateServer PRIVATE
        NgHttp2Server.cpp
        ../GateServerHttp2/Http2Routes.cpp
        ../GateServerHttp2/Http2Handlers.cpp
        ../GateServerHttp2/Http2AuthHandlers.cpp
        ../GateServerHttp2/Http2AuthSupport.cpp
        ../GateServerHttp2/Http2CallHandlers.cpp
        ../GateServerHttp2/Http2CallSupport.cpp
        ../GateServerHttp2/Http2MediaHandlers.cpp
        ../GateServerHttp2/Http2ProfileHandlers.cpp
        ../GateServerHttp2/Http2MomentsSupport.cpp
    )
    # Header-only support modules
    target_sources(GateServer PRIVATE
        ../GateServerHttp2/Http2MediaSupport.h
        ../GateServerHttp2/Http2ProfileSupport.h
    )
    target_include_directories(GateServer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../GateServerHttp2
    )
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServerCore
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServerHttp3
)

if(MEMOCHAT_ENABLE_HTTP3)
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
