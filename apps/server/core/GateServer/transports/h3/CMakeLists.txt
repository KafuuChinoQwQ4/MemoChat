cmake_minimum_required(VERSION 3.16)

project(GateServerHttp3 LANGUAGES CXX)

# GateServerHttp3 - HTTP/3 specific library
# Contains: GateHttp3Listener, GateHttp3Connection, GateHttp3JsonSupport
# Depends on GateServerCore for LogicSystem and shared route registration.

add_library(GateServerHttp3 STATIC
    listener/GateHttp3Listener.cpp
    listener/GateHttp3Listener.h
    listener/GateHttp3Connection.cpp
    listener/GateHttp3Connection.h
    listener/GateHttp3JsonSupport.cpp
    listener/GateHttp3JsonSupport.h
    legacy_routes/GateHttp3ServiceRoutes.cpp
    legacy_routes/GateHttp3ServiceRoutes.h
    adapters/h3/H3RouteAdapter.cpp
)

target_compile_features(GateServerHttp3 PUBLIC cxx_std_17)
target_include_directories(GateServerHttp3 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/listener
    ${CMAKE_CURRENT_SOURCE_DIR}/legacy_routes
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../common
)

# MongoDB include paths (transitively needed by GateHttp3ServiceRoutes.cpp via MongoMgr.h)
# Environment variables: VCPKG_INSTALLED_DIR (defined via CMake cache)
target_include_directories(GateServerHttp3 PRIVATE
    "${MEMOCHAT_VCPKG_TRIPLET_DIR}/include"
    "${MEMOCHAT_VCPKG_TRIPLET_DIR}/include/mongocxx/v_noabi"
    "${MEMOCHAT_VCPKG_TRIPLET_DIR}/include/bsoncxx/v_noabi"
)

target_link_libraries(GateServerHttp3 PUBLIC
    GateServerCore
    memochat_boost_stubs
)

# HTTP/3 specific libraries
if(MEMOCHAT_ENABLE_HTTP3)
    if(WIN32)
        # Environment variables: WINSDK_LIB_UM, WINSDK_LIB_UCRT, VC_TOOLS_LIB
        target_link_libraries(GateServerHttp3 PRIVATE
            "$ENV{WINSDK_LIB_UM}/bcrypt.lib")
        target_link_options(GateServerHttp3 PRIVATE
            "/LIBPATH:$ENV{WINSDK_LIB_UM}"
            "/LIBPATH:$ENV{WINSDK_LIB_UCRT}"
            "/LIBPATH:$ENV{VC_TOOLS_LIB}")
    endif()

    # OpenSSL required by CertUtil (PKCS12/self-signed cert generation)
    find_package(OpenSSL CONFIG)
    if(TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
        target_link_libraries(GateServerHttp3 PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()

    if(WIN32)
        # msquic.dll is in vcpkg_installed/x64-windows/bin, copy to output.
        find_path(MSQUIC_BIN_DIR NAMES msquic.dll PATHS
            "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/bin"
            NO_DEFAULT_PATH)
        if(MSQUIC_BIN_DIR)
            message(STATUS "GateServerHttp3: msquic.dll found at ${MSQUIC_BIN_DIR}")
            add_custom_command(TARGET GateServerHttp3 POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MSQUIC_BIN_DIR}/msquic.dll"
                "$<TARGET_FILE_DIR:GateServerHttp3>/../../bin"
                COMMAND_EXPAND_LISTS
            )
        else()
            message(WARNING "GateServerHttp3: msquic.dll not found in vcpkg_installed")
        endif()
    endif()
endif()

set_target_properties(GateServerHttp3 PROPERTIES FOLDER "Server/GateServer")
