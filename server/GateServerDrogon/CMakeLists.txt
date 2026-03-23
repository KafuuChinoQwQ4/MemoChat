cmake_minimum_required(VERSION 3.16)

project(GateServerDrogon LANGUAGES CXX)

# WinCompat.h provides Windows SDK fixes (see that file for details).
# GateServerDrogon .cpp files manually include it as their first line
# to ensure it is parsed before any SDK header.
# This avoids CMake /FI escaping issues on Windows.

add_executable(GateServerDrogon
    main.cpp
    GateServerDrogon.cpp
    GateServerDrogon.h
    DrogonAppCfg.h
    DrogonAppCfg.cpp
    DrogonRoutes.h
    DrogonRoutes.cpp
    DrogonHttpHandlers.h
    DrogonHttpHandlers.cpp
    DrogonAuthHandlers.h
    DrogonAuthHandlers.cpp
    DrogonAuthSupport.h
    DrogonAuthSupport.cpp
    DrogonProfileHandlers.h
    DrogonProfileHandlers.cpp
    DrogonProfileSupport.h
    DrogonProfileSupport.cpp
    DrogonCallHandlers.h
    DrogonCallHandlers.cpp
    DrogonCallSupport.h
    DrogonCallSupport.cpp
    DrogonMediaHandlers.h
    DrogonMediaHandlers.cpp
    DrogonMediaSupport.h
    DrogonMediaSupport.cpp
    CertUtil.h
    CertUtil.cpp
    WinCompat.h
    config.ini
)

set_target_properties(GateServerDrogon PROPERTIES FOLDER "Server")
memochat_configure_server_target(GateServerDrogon)

# GateServerDrogon links against GateServerCore for all shared services
target_link_libraries(GateServerDrogon PRIVATE GateServerCore)

# Include directory for WinCompat.h (local dir), GateServerCore, and GateServerHttp3
target_include_directories(GateServerDrogon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}          # for WinCompat.h include
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServerCore
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServerHttp3
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServerDrogon
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

target_compile_features(GateServerDrogon PRIVATE cxx_std_20)
set_target_properties(GateServerDrogon PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(GateServerDrogon PRIVATE ${MEMOCHAT_PQXX_TARGET})

if(MEMOCHAT_ENABLE_KAFKA)
    target_link_libraries(GateServerDrogon PRIVATE ${MEMOCHAT_CPPKAFKA_TARGET})
endif()

if(MEMOCHAT_ENABLE_RABBITMQ)
    target_link_libraries(GateServerDrogon PRIVATE ${MEMOCHAT_RABBITMQ_TARGET})
endif()

if(WIN32)
    target_link_libraries(GateServerDrogon PRIVATE
        "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/bcrypt.lib"
        "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/ws2_32.lib")
    target_link_options(GateServerDrogon PRIVATE
        "/LIBPATH:C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64"
        "/LIBPATH:C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64"
        "/LIBPATH:D:/VS2026stable/VC/Tools/MSVC/14.50.35717/lib/x64")
endif()

# OpenSSL is REQUIRED for HTTP/2 (TLS with ALPN negotiation)
find_package(OpenSSL CONFIG REQUIRED)
if(TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
    target_link_libraries(GateServerDrogon PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    message(STATUS "GateServerDrogon: OpenSSL found for TLS/HTTP2")
else()
    message(FATAL_ERROR "GateServerDrogon: OpenSSL not found - HTTP/2 requires TLS")
endif()
