cmake_minimum_required(VERSION 3.16)

project(GateServerHttp2 LANGUAGES CXX)

# libh2o must be found before entering this subdirectory
# (server/CMakeLists.txt calls find_package(libh2o CONFIG QUIET) before add_subdirectory)
if(NOT libh2o_FOUND)
    message(FATAL_ERROR "GateServerHttp2: libh2o not found — this subdirectory should not be processed")
endif()

# WinCompat.h provides Windows SDK fixes (see that file for details).
# GateServerHttp2 .cpp files manually include it as their first line
# to ensure it is parsed before any SDK header.
# This avoids CMake /FI escaping issues on Windows.

add_executable(GateServerHttp2
    standalone/main.cpp
    standalone/GateServerHttp2.cpp
    standalone/GateServerHttp2.h
    standalone/Http2App.h
    standalone/Http2App.cpp
    standalone/CertUtil.h
    standalone/CertUtil.cpp
    standalone/WinCompat.h
    adapters/h2/H2RouteAdapter.cpp
    routes/Http2Routes.h
    routes/Http2Routes.cpp
    handlers/Http2Handlers.h
    handlers/Http2Handlers.cpp
    handlers/Http2AuthHandlers.h
    handlers/Http2AuthHandlers.cpp
    handlers/Http2ProfileHandlers.h
    handlers/Http2ProfileHandlers.cpp
    handlers/Http2CallHandlers.h
    handlers/Http2CallHandlers.cpp
    handlers/Http2MediaHandlers.h
    handlers/Http2MediaHandlers.cpp
    support/Http2AuthSupport.h
    support/Http2AuthSupport.cpp
    support/Http2CallSupport.h
    support/Http2CallSupport.cpp
    support/Http2MomentsSupport.h
    support/Http2MomentsSupport.cpp
    support/Http2MediaSupport.h
    support/Http2ProfileSupport.h
    ../../routing/RouteRegistry.cpp
    ../../modules/health/HealthRouteModule.cpp
)

set_target_properties(GateServerHttp2 PROPERTIES FOLDER "Server")
memochat_configure_server_target(GateServerHttp2)

# GateServerHttp2 links against GateServerCore for all shared services
target_link_libraries(GateServerHttp2 PRIVATE GateServerCore memochat_boost_stubs)

# Include directory for WinCompat.h (local dir), GateServerCore, and H3 compatibility headers
target_include_directories(GateServerHttp2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}          # for WinCompat.h include
    ${CMAKE_CURRENT_SOURCE_DIR}/adapters
    ${CMAKE_CURRENT_SOURCE_DIR}/routes
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers
    ${CMAKE_CURRENT_SOURCE_DIR}/support
    ${CMAKE_CURRENT_SOURCE_DIR}/standalone
    ${CMAKE_CURRENT_SOURCE_DIR}/../h3/listener
    ${CMAKE_CURRENT_SOURCE_DIR}/../h3/legacy_routes
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../common
)

target_compile_features(GateServerHttp2 PRIVATE cxx_std_23)
set_target_properties(GateServerHttp2 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(GateServerHttp2 PRIVATE ${MEMOCHAT_PQXX_TARGET})

if(MEMOCHAT_ENABLE_KAFKA)
    target_link_libraries(GateServerHttp2 PRIVATE ${MEMOCHAT_CPPKAFKA_TARGET})
endif()

if(MEMOCHAT_ENABLE_RABBITMQ)
    target_link_libraries(GateServerHttp2 PRIVATE ${MEMOCHAT_RABBITMQ_TARGET})
endif()

if(WIN32)
    # Environment variables: WINSDK_LIB_UM, VC_TOOLS_LIB
    target_link_libraries(GateServerHttp2 PRIVATE
        "$ENV{WINSDK_LIB_UM}/bcrypt.lib"
        "$ENV{WINSDK_LIB_UM}/ws2_32.lib")
    target_link_options(GateServerHttp2 PRIVATE
        "/LIBPATH:$ENV{WINSDK_LIB_UM}"
        "/LIBPATH:$ENV{WINSDK_LIB_UCRT}"
        "/LIBPATH:$ENV{VC_TOOLS_LIB}")
endif()

# OpenSSL is REQUIRED for HTTP/2 (TLS with ALPN negotiation)
find_package(OpenSSL CONFIG REQUIRED)
if(TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
    target_link_libraries(GateServerHttp2 PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    # vcpkg OpenSSL DLL on Windows requires OPENSSL_Applink to be registered.
    # OpenSSL::applink adds openssl/applink.c as a source, providing OPENSSL_Applink().
    # Without this, libcrypto-3-x64.dll crashes with "no OPENSSL_Applink".
    if(TARGET OpenSSL::applink)
        target_link_libraries(GateServerHttp2 PRIVATE OpenSSL::applink)
    endif()
    message(STATUS "GateServerHttp2: OpenSSL found for TLS/HTTP2")
else()
    message(FATAL_ERROR "GateServerHttp2: OpenSSL not found - HTTP/2 requires TLS")
endif()

# libh2o provides HTTP/2 server functionality (found REQUIRED above)
target_link_libraries(GateServerHttp2 PRIVATE h2o::h2o)
message(STATUS "GateServerHttp2: libh2o found and linked")
