add_executable(GateServer
    # H1-specific entry point and listeners
    CServer.cpp
    GateHttpJsonSupport.cpp
    GateServerH1Routes.cpp
    GateWorkerPool.cpp
    HttpConnection.cpp
    LogicSystem.cpp
    GateServer.cpp
    MediaHttpService.cpp
    MomentsRouteModules.cpp
    MomentsRouteModules.h
    # GateServerCore header for H3 service interface
    GateRouteModules.h
    # GateHttp3Service is defined in GateServerHttp3
    ../GateServerHttp3/GateHttp3ServiceRoutes.h
    config.ini
    # Drogon media/profile support used by both H1 and H3 routes
    ../GateServerDrogon/DrogonMediaSupport.cpp
    ../GateServerDrogon/DrogonProfileSupport.cpp
)

set_target_properties(GateServer PROPERTIES FOLDER "Server")
memochat_configure_server_target(GateServer)

# GateServer links against GateServerCore (all shared services)
# and GateServerHttp3 (H3 listener + routes)
target_link_libraries(GateServer PRIVATE
    GateServerCore
    GateServerHttp3
)

target_include_directories(GateServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServerCore
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServerHttp3
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServerDrogon
)

if(WIN32)
    target_link_libraries(GateServer PRIVATE
        "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/bcrypt.lib")
    target_link_options(GateServer PRIVATE
        "/LIBPATH:C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64"
        "/LIBPATH:C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64"
        "/LIBPATH:D:/VS2026stable/VC/Tools/MSVC/14.50.35717/lib/x64")
endif()

if(MEMOCHAT_ENABLE_HTTP3)
    # msquic.dll is in vcpkg_installed/x64-windows/bin, copy to output
    find_path(MSQUIC_BIN_DIR NAMES msquic.dll PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg_installed/x64-windows/bin"
        NO_DEFAULT_PATH)
    if(MSQUIC_BIN_DIR)
        add_custom_command(TARGET GateServer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MSQUIC_BIN_DIR}/msquic.dll"
                "$<TARGET_FILE_DIR:GateServer>")
        message(STATUS "HTTP/3: will copy msquic.dll from ${MSQUIC_BIN_DIR}")
    else()
        message(WARNING "HTTP/3: msquic.dll not found in vcpkg_installed, "
            "copy it manually if QUIC listener fails to start")
    endif()
endif()
