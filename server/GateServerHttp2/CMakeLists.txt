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
    main.cpp
    GateServerHttp2.cpp
    GateServerHttp2.h
    Http2App.h
    Http2App.cpp
    Http2Routes.h
    Http2Routes.cpp
    Http2Handlers.h
    Http2Handlers.cpp
    Http2AuthHandlers.h
    Http2AuthHandlers.cpp
    Http2AuthSupport.h
    Http2AuthSupport.cpp
    Http2ProfileHandlers.h
    Http2ProfileHandlers.cpp
    Http2CallHandlers.h
    Http2CallHandlers.cpp
    Http2CallSupport.h
    Http2CallSupport.cpp
    Http2MediaHandlers.h
    Http2MediaHandlers.cpp
    CertUtil.h
    CertUtil.cpp
    WinCompat.h
)

set_target_properties(GateServerHttp2 PROPERTIES FOLDER "Server")
memochat_configure_server_target(GateServerHttp2)

# GateServerHttp2 links against GateServerCore for all shared services
target_link_libraries(GateServerHttp2 PRIVATE GateServerCore)

# Include directory for WinCompat.h (local dir), GateServerCore, and GateServerHttp3
target_include_directories(GateServerHttp2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}          # for WinCompat.h include
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServerCore
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServerHttp3
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

target_compile_features(GateServerHttp2 PRIVATE cxx_std_20)
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
    target_link_libraries(GateServerHttp2 PRIVATE
        "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/bcrypt.lib"
        "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/ws2_32.lib")
    target_link_options(GateServerHttp2 PRIVATE
        "/LIBPATH:C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64"
        "/LIBPATH:C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64"
        "/LIBPATH:D:/VS2026stable/VC/Tools/MSVC/14.50.35717/lib/x64")
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
