cmake_minimum_required(VERSION 3.16)

project(MemoChatServer LANGUAGES CXX)

option(BUILD_SERVER_SERVICES "Build backend service executables" ON)
option(MEMOCHAT_AUTO_DEPLOY_RUNTIME "Copy built server executables into infra/Memo_ops/runtime/services during build" OFF)
option(MEMOCHAT_ENABLE_CPP26_REFLECTION "Enable experimental C++26 static reflection for supported server targets" OFF)
option(MEMOCHAT_ENABLE_GNU_MODULES "Enable project-owned GNU C++ module CMI construction for selected server targets" OFF)
option(MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER "Build ChatServer WebTransport provider with an HTTP/3-capable libwebsockets" OFF)

# Require at least C++23 for std::expected, std::unreachable, std::byteswap (glaze library requires C++23).
# Must be set BEFORE any add_subdirectory() calls so it takes effect for all targets.
if(NOT DEFINED MEMOCHAT_CXX_STANDARD)
    set(MEMOCHAT_CXX_STANDARD "26" CACHE STRING "C++ standard used by MemoChat targets")
endif()
set(CMAKE_CXX_STANDARD ${MEMOCHAT_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(memochat_cpp26_reflection INTERFACE)
if(MEMOCHAT_ENABLE_CPP26_REFLECTION)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "MEMOCHAT_ENABLE_CPP26_REFLECTION requires GNU g++ because C++26 reflection is compiler-specific and experimental.")
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
        message(FATAL_ERROR "MEMOCHAT_ENABLE_CPP26_REFLECTION requires g++ >= 16.")
    endif()
    if(NOT "${MEMOCHAT_CXX_STANDARD}" STREQUAL "26")
        message(FATAL_ERROR "MEMOCHAT_ENABLE_CPP26_REFLECTION requires MEMOCHAT_CXX_STANDARD=26.")
    endif()

    include(CheckCXXSourceCompiles)
    set(_MEMOCHAT_REFLECTION_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    string(APPEND CMAKE_REQUIRED_FLAGS " -freflection")
    check_cxx_source_compiles([=[
#include <meta>
#include <cstddef>

struct Probe {
    int id;
    const char* name;
};

consteval std::size_t CountFields()
{
    return std::meta::nonstatic_data_members_of(^^Probe, std::meta::access_context::current()).size();
}

static_assert(CountFields() == 2);

int main()
{
    return 0;
}
]=] MEMOCHAT_CPP26_REFLECTION_PROBE_OK)
    set(CMAKE_REQUIRED_FLAGS "${_MEMOCHAT_REFLECTION_REQUIRED_FLAGS}")
    if(NOT MEMOCHAT_CPP26_REFLECTION_PROBE_OK)
        message(FATAL_ERROR "MEMOCHAT_ENABLE_CPP26_REFLECTION was requested, but the compiler failed the <meta> -freflection probe.")
    endif()

    target_compile_options(memochat_cpp26_reflection INTERFACE -freflection)
    target_compile_definitions(memochat_cpp26_reflection INTERFACE MEMOCHAT_ENABLE_CPP26_REFLECTION=1 GLZ_REFLECTION26=1)
    message(STATUS "C++26 static reflection: enabled for server targets via -freflection")
else()
    target_compile_definitions(memochat_cpp26_reflection INTERFACE MEMOCHAT_ENABLE_CPP26_REFLECTION=0)
endif()

if(MSVC)
    if(DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES "static")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
    
    # Add MSVC system include paths for vcpkg's /external mode compatibility
    # vcpkg's toolchain adds -external:I and -external:W0 which requires explicit system include paths
    if(DEFINED ENV{VC_TOOLS_INCLUDE} AND NOT "$ENV{VC_TOOLS_INCLUDE}" STREQUAL "")
        set(_VC_TOOLS_INCLUDE "$ENV{VC_TOOLS_INCLUDE}")
    else()
        set(_VC_TOOLS_INCLUDE "D:/VS2026stable/VC/Tools/MSVC/14.50.35717/include")
    endif()
    if(DEFINED ENV{WINSDK_INCLUDE} AND NOT "$ENV{WINSDK_INCLUDE}" STREQUAL "")
        set(_WINSDK_INCLUDE "$ENV{WINSDK_INCLUDE}")
    else()
        set(_WINSDK_INCLUDE "C:/Program Files (x86)/Windows Kits/10/Include/10.0.28000.0")
    endif()
    
    # Store paths in a global variable for use by subdirectories
    set(MEMOCHAT_MSVC_SYSTEM_INCLUDES
        "${_VC_TOOLS_INCLUDE}"
        "${_WINSDK_INCLUDE}/ucrt"
        "${_WINSDK_INCLUDE}/shared"
        "${_WINSDK_INCLUDE}/um"
        "${_WINSDK_INCLUDE}/winrt"
        "${_WINSDK_INCLUDE}/cppwinrt"
    )
    
    # Add these as external includes using CMAKE_CXX_FLAGS
    foreach(_path IN LISTS MEMOCHAT_MSVC_SYSTEM_INCLUDES)
        if(EXISTS "${_path}")
            string(APPEND CMAKE_CXX_FLAGS " /external:I \"${_path}\"")
        endif()
    endforeach()
endif()

find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(Threads REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(bson-1.0 CONFIG REQUIRED)
find_package(mongoc-1.0 CONFIG REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(RdKafka CONFIG QUIET)
find_package(unofficial-librabbitmq CONFIG QUIET)
find_package(rabbitmq-c CONFIG QUIET)
find_package(msquic CONFIG QUIET)
find_package(OpenSSL CONFIG QUIET)
find_package(unofficial-sodium CONFIG QUIET)
find_package(PkgConfig QUIET)
find_package(libh2o CONFIG QUIET)
find_package(AWSSDK CONFIG QUIET)
if(AWSSDK_FOUND)
    # AWSSDK found but doesn't provide CMake IMPORTED targets.
    # Fall back to per-service vcpkg packages which have proper targets.
    unset(AWSSDK_FOUND)
endif()

# vcpkg aws-sdk-cpp installs per-service packages with proper CMake IMPORTED targets.
find_package(aws-cpp-sdk-s3 CONFIG QUIET)
if(aws-cpp-sdk-s3_FOUND)
    set(AWSSDK_FOUND TRUE)
endif()

# Standard HTTP/3 is terminated by the local Envoy edge. This flag only keeps
# the legacy embedded Gate QUIC listener buildable for explicit experiments.
set(MEMOCHAT_ENABLE_HTTP3 0)
if(TARGET msquic AND TARGET OpenSSL::SSL)
    set(MEMOCHAT_ENABLE_HTTP3 1)
    message(STATUS "Embedded Gate QUIC listener: MsQuic + OpenSSL found")
else()
    if(NOT TARGET msquic)
        message(STATUS "Embedded Gate QUIC listener: MsQuic not found, listener disabled")
    else()
        message(STATUS "Embedded Gate QUIC listener: OpenSSL not found, listener disabled")
    endif()
endif()

if(MEMOCHAT_NO_EXCEPTIONS AND TARGET spdlog::spdlog_header_only)
    set(MEMOCHAT_SPDLOG_TARGET spdlog::spdlog_header_only)
elseif(TARGET spdlog::spdlog)
    set(MEMOCHAT_SPDLOG_TARGET spdlog::spdlog)
elseif(TARGET spdlog::spdlog_header_only)
    set(MEMOCHAT_SPDLOG_TARGET spdlog::spdlog_header_only)
else()
    message(FATAL_ERROR "spdlog target not found")
endif()

if(TARGET mongo::mongoc_static)
    set(MEMOCHAT_MONGOC_TARGET mongo::mongoc_static)
else()
    message(FATAL_ERROR "mongo::mongoc_static target not found")
endif()

if(TARGET mongo::bson_static)
    set(MEMOCHAT_BSON_TARGET mongo::bson_static)
else()
    message(FATAL_ERROR "mongo::bson_static target not found")
endif()

if(TARGET PostgreSQL::PostgreSQL)
    set(MEMOCHAT_POSTGRES_TARGET PostgreSQL::PostgreSQL)
else()
    message(FATAL_ERROR "PostgreSQL::PostgreSQL target not found")
endif()

set(MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER_COMPILED 0)
if(MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER)
    find_package(libwebsockets CONFIG REQUIRED)
    if(TARGET websockets)
        set(MEMOCHAT_LWS_WEBTRANSPORT_TARGET websockets)
    elseif(TARGET websockets_shared)
        set(MEMOCHAT_LWS_WEBTRANSPORT_TARGET websockets_shared)
    else()
        message(FATAL_ERROR "MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER requires a libwebsockets CMake target named websockets or websockets_shared.")
    endif()

    get_target_property(_MEMOCHAT_LWS_WEBTRANSPORT_INCLUDES ${MEMOCHAT_LWS_WEBTRANSPORT_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT _MEMOCHAT_LWS_WEBTRANSPORT_INCLUDES)
        set(_MEMOCHAT_LWS_WEBTRANSPORT_INCLUDES "")
    endif()

    include(CheckCXXSourceCompiles)
    set(_MEMOCHAT_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES}")
    set(_MEMOCHAT_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
    set(CMAKE_REQUIRED_INCLUDES ${_MEMOCHAT_LWS_WEBTRANSPORT_INCLUDES})
    set(CMAKE_REQUIRED_LIBRARIES ${MEMOCHAT_LWS_WEBTRANSPORT_TARGET})
    check_cxx_source_compiles([=[
#include <libwebsockets.h>
extern "C" {
#include <libwebsockets/lws-webtransport.h>
}

#if !defined(LWS_WITH_HTTP3)
#error "libwebsockets was not built with HTTP/3"
#endif

#if !defined(LWS_ROLE_WT)
#error "libwebsockets was not built with WebTransport role support"
#endif

int main()
{
    (void) &lws_wt_create_stream;
    (void) &lws_wt_is_session;
    (void) &lws_hdr_total_length;
    (void) &lws_hdr_copy;
    (void) WSI_TOKEN_HTTP_COLON_PATH;
    return 0;
}
]=] MEMOCHAT_LWS_WEBTRANSPORT_READY)
    set(CMAKE_REQUIRED_INCLUDES "${_MEMOCHAT_REQUIRED_INCLUDES}")
    set(CMAKE_REQUIRED_LIBRARIES "${_MEMOCHAT_REQUIRED_LIBRARIES}")

    if(NOT MEMOCHAT_LWS_WEBTRANSPORT_READY)
        message(FATAL_ERROR "MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER requires libwebsockets built with LWS_WITH_HTTP3 and LWS_ROLE_WT.")
    endif()

    set(MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER_COMPILED 1)
    message(STATUS "ChatServer WebTransport provider: libwebsockets HTTP/3 WebTransport enabled")
else()
    message(STATUS "ChatServer WebTransport provider: libwebsockets disabled; unavailable provider remains the default")
endif()

if(TARGET unofficial-sodium::sodium)
    set(MEMOCHAT_SODIUM_TARGET unofficial-sodium::sodium)
elseif(PkgConfig_FOUND)
    pkg_check_modules(MEMOCHAT_SODIUM IMPORTED_TARGET libsodium)
    if(TARGET PkgConfig::MEMOCHAT_SODIUM)
        set(MEMOCHAT_SODIUM_TARGET PkgConfig::MEMOCHAT_SODIUM)
    endif()
endif()
if(NOT MEMOCHAT_SODIUM_TARGET)
    message(FATAL_ERROR "libsodium target not found. Install vcpkg package libsodium or system package libsodium.")
endif()

set(MEMOCHAT_ENABLE_KAFKA 0)
if(TARGET RdKafka::rdkafka)
    set(MEMOCHAT_RDKAFKA_TARGET RdKafka::rdkafka)
    set(MEMOCHAT_ENABLE_KAFKA 1)
endif()

set(MEMOCHAT_ENABLE_RABBITMQ 0)
if(TARGET unofficial::librabbitmq::rabbitmq)
    set(MEMOCHAT_RABBITMQ_TARGET unofficial::librabbitmq::rabbitmq)
    set(MEMOCHAT_ENABLE_RABBITMQ 1)
elseif(TARGET rabbitmq::rabbitmq-static)
    set(MEMOCHAT_RABBITMQ_TARGET rabbitmq::rabbitmq-static)
    set(MEMOCHAT_ENABLE_RABBITMQ 1)
elseif(TARGET rabbitmq::rabbitmq)
    set(MEMOCHAT_RABBITMQ_TARGET rabbitmq::rabbitmq)
    set(MEMOCHAT_ENABLE_RABBITMQ 1)
endif()

if(NOT TARGET protobuf::protoc)
    message(FATAL_ERROR "protobuf::protoc target not found. Install protobuf host tools.")
endif()

if(NOT TARGET gRPC::grpc_cpp_plugin)
    message(FATAL_ERROR "gRPC::grpc_cpp_plugin target not found. Install grpc[codegen].")
endif()

add_library(memochat_boost_stubs STATIC
    common/BoostStubs.cpp
)
target_compile_features(memochat_boost_stubs PUBLIC cxx_std_23)
target_include_directories(memochat_boost_stubs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)
target_link_libraries(memochat_boost_stubs PUBLIC
    Boost::headers
)

add_library(memochat_logging STATIC
    common/logging/LogConfig.cpp
    common/logging/Logger.cpp
    common/logging/Redaction.cpp
    common/logging/Telemetry.cpp
    common/logging/TelemetryConfig.cpp
    common/logging/GrpcTrace.cpp
)
target_compile_features(memochat_logging PUBLIC cxx_std_23)
target_include_directories(memochat_logging PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)
target_link_libraries(memochat_logging PUBLIC
    ${MEMOCHAT_SPDLOG_TARGET}
    Boost::headers
    memochat_boost_stubs
)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(memochat_logging
        MODULES
            memochat.logging.config_algorithms=common/logging/cxx_modules/LoggingConfig.cppm
            memochat.logging.grpc_trace_algorithms=common/logging/cxx_modules/GrpcTrace.cppm
            memochat.logging.log_config_algorithms=common/logging/cxx_modules/LogConfig.cppm
            memochat.logging.logger_algorithms=common/logging/cxx_modules/Logger.cppm
            memochat.logging.redaction_algorithms=common/logging/cxx_modules/Redaction.cppm
            memochat.logging.telemetry_algorithms=common/logging/cxx_modules/Telemetry.cppm
            memochat.logging.telemetry_config_algorithms=common/logging/cxx_modules/TelemetryConfig.cppm
        CONSUMER_SOURCES
            common/logging/GrpcTrace.cpp
            common/logging/LogConfig.cpp
            common/logging/Logger.cpp
            common/logging/Redaction.cpp
            common/logging/Telemetry.cpp
            common/logging/TelemetryConfig.cpp
    )
endif()

add_library(memochat_runtime STATIC
    common/runtime/IniConfig.cpp
    common/runtime/IoContextPool.cpp
    common/runtime/EtcdConfig.cpp
)
target_compile_features(memochat_runtime PUBLIC cxx_std_23)
target_include_directories(memochat_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)
target_link_libraries(memochat_runtime PUBLIC
    Boost::filesystem
    Boost::headers
    memochat_boost_stubs
)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(memochat_runtime
        MODULES
            memochat.runtime.etcd_config_algorithms=common/runtime/cxx_modules/EtcdConfig.cppm
            memochat.runtime.io_context_pool_algorithms=common/runtime/cxx_modules/IoContextPool.cppm
            memochat.runtime.ini_config_algorithms=common/runtime/cxx_modules/IniConfig.cppm
        CONSUMER_SOURCES
            common/runtime/EtcdConfig.cpp
            common/runtime/IniConfig.cpp
            common/runtime/IoContextPool.cpp
    )
endif()

add_library(memochat_auth_security STATIC
    common/auth/JwtAccessToken.cpp
    common/auth/JwtAccessToken.hpp
    common/auth/PasswordHasher.cpp
    common/auth/PasswordHasher.hpp
    common/auth/RefreshToken.cpp
    common/auth/RefreshToken.hpp
)
target_compile_features(memochat_auth_security PUBLIC cxx_std_23)
target_include_directories(memochat_auth_security PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)
target_link_libraries(memochat_auth_security PUBLIC ${MEMOCHAT_SODIUM_TARGET})

set(MEMOCHAT_PROTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(MEMOCHAT_PROTO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/common/proto/message.proto")
set(MEMOCHAT_PROTO_AI_SRC "${CMAKE_CURRENT_SOURCE_DIR}/common/proto/ai_message.proto")
set(MEMOCHAT_PROTO_CHAT_INTERNAL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/common/proto/chat_internal.proto")
set(MEMOCHAT_PROTO_GEN_DIR "${CMAKE_BINARY_DIR}/generated")
set(MEMOCHAT_PROTO_CC "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/message.pb.cc")
set(MEMOCHAT_PROTO_H "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/message.pb.h")
set(MEMOCHAT_GRPC_CC "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/message.grpc.pb.cc")
set(MEMOCHAT_GRPC_H "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/message.grpc.pb.h")
set(MEMOCHAT_PROTO_AI_CC "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/ai_message.pb.cc")
set(MEMOCHAT_PROTO_AI_H "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/ai_message.pb.h")
set(MEMOCHAT_GRPC_AI_CC "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/ai_message.grpc.pb.cc")
set(MEMOCHAT_GRPC_AI_H "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/ai_message.grpc.pb.h")
set(MEMOCHAT_PROTO_CHAT_INTERNAL_CC "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/chat_internal.pb.cc")
set(MEMOCHAT_PROTO_CHAT_INTERNAL_H "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/chat_internal.pb.h")
set(MEMOCHAT_GRPC_CHAT_INTERNAL_CC "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/chat_internal.grpc.pb.cc")
set(MEMOCHAT_GRPC_CHAT_INTERNAL_H "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/chat_internal.grpc.pb.h")

add_custom_command(
    OUTPUT
        "${MEMOCHAT_PROTO_CC}"
        "${MEMOCHAT_PROTO_H}"
        "${MEMOCHAT_GRPC_CC}"
        "${MEMOCHAT_GRPC_H}"
        "${MEMOCHAT_PROTO_AI_CC}"
        "${MEMOCHAT_PROTO_AI_H}"
        "${MEMOCHAT_GRPC_AI_CC}"
        "${MEMOCHAT_GRPC_AI_H}"
        "${MEMOCHAT_PROTO_CHAT_INTERNAL_CC}"
        "${MEMOCHAT_PROTO_CHAT_INTERNAL_H}"
        "${MEMOCHAT_GRPC_CHAT_INTERNAL_CC}"
        "${MEMOCHAT_GRPC_CHAT_INTERNAL_H}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MEMOCHAT_PROTO_GEN_DIR}/common/proto"
    COMMAND $<TARGET_FILE:protobuf::protoc> --proto_path=${MEMOCHAT_PROTO_ROOT} --cpp_out=${MEMOCHAT_PROTO_GEN_DIR} ${MEMOCHAT_PROTO_SRC}
    COMMAND $<TARGET_FILE:protobuf::protoc> --proto_path=${MEMOCHAT_PROTO_ROOT} --cpp_out=${MEMOCHAT_PROTO_GEN_DIR} ${MEMOCHAT_PROTO_AI_SRC}
    COMMAND $<TARGET_FILE:protobuf::protoc> --proto_path=${MEMOCHAT_PROTO_ROOT} --cpp_out=${MEMOCHAT_PROTO_GEN_DIR} ${MEMOCHAT_PROTO_CHAT_INTERNAL_SRC}
    COMMAND $<TARGET_FILE:protobuf::protoc> --proto_path=${MEMOCHAT_PROTO_ROOT} --grpc_out=${MEMOCHAT_PROTO_GEN_DIR} --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin> ${MEMOCHAT_PROTO_SRC}
    COMMAND $<TARGET_FILE:protobuf::protoc> --proto_path=${MEMOCHAT_PROTO_ROOT} --grpc_out=${MEMOCHAT_PROTO_GEN_DIR} --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin> ${MEMOCHAT_PROTO_AI_SRC}
    COMMAND $<TARGET_FILE:protobuf::protoc> --proto_path=${MEMOCHAT_PROTO_ROOT} --grpc_out=${MEMOCHAT_PROTO_GEN_DIR} --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin> ${MEMOCHAT_PROTO_CHAT_INTERNAL_SRC}
    DEPENDS
        "${MEMOCHAT_PROTO_SRC}"
        "${MEMOCHAT_PROTO_AI_SRC}"
        "${MEMOCHAT_PROTO_CHAT_INTERNAL_SRC}"
        protobuf::protoc
        gRPC::grpc_cpp_plugin
    VERBATIM
)

add_library(memochat_proto STATIC
    "${MEMOCHAT_PROTO_CC}"
    "${MEMOCHAT_PROTO_H}"
    "${MEMOCHAT_GRPC_CC}"
    "${MEMOCHAT_GRPC_H}"
    "${MEMOCHAT_PROTO_AI_CC}"
    "${MEMOCHAT_PROTO_AI_H}"
    "${MEMOCHAT_GRPC_AI_CC}"
    "${MEMOCHAT_GRPC_AI_H}"
    "${MEMOCHAT_PROTO_CHAT_INTERNAL_CC}"
    "${MEMOCHAT_PROTO_CHAT_INTERNAL_H}"
    "${MEMOCHAT_GRPC_CHAT_INTERNAL_CC}"
    "${MEMOCHAT_GRPC_CHAT_INTERNAL_H}"
)
target_compile_features(memochat_proto PUBLIC cxx_std_23)
target_include_directories(memochat_proto PUBLIC
    ${MEMOCHAT_PROTO_GEN_DIR}
)
target_link_libraries(memochat_proto PUBLIC
    gRPC::grpc++
    protobuf::libprotobuf
)

if(DEFINED VCPKG_TARGET_TRIPLET AND NOT "${VCPKG_TARGET_TRIPLET}" STREQUAL "")
    set(MEMOCHAT_VCPKG_TARGET_TRIPLET "${VCPKG_TARGET_TRIPLET}")
elseif(WIN32)
    set(MEMOCHAT_VCPKG_TARGET_TRIPLET "x64-windows")
else()
    set(MEMOCHAT_VCPKG_TARGET_TRIPLET "x64-linux")
endif()

if(DEFINED VCPKG_INSTALLED_DIR AND NOT "${VCPKG_INSTALLED_DIR}" STREQUAL "")
    set(MEMOCHAT_VCPKG_INSTALLED_DIR "${VCPKG_INSTALLED_DIR}")
elseif(DEFINED ENV{VCPKG_INSTALLED_DIR} AND NOT "$ENV{VCPKG_INSTALLED_DIR}" STREQUAL "")
    set(MEMOCHAT_VCPKG_INSTALLED_DIR "$ENV{VCPKG_INSTALLED_DIR}")
else()
    set(MEMOCHAT_VCPKG_INSTALLED_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed")
endif()
set(MEMOCHAT_VCPKG_TRIPLET_DIR "${MEMOCHAT_VCPKG_INSTALLED_DIR}/${MEMOCHAT_VCPKG_TARGET_TRIPLET}")

add_library(memochat_server_deps INTERFACE)
target_compile_features(memochat_server_deps INTERFACE cxx_std_23)
target_include_directories(memochat_server_deps INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)
target_link_libraries(memochat_server_deps INTERFACE
    memochat_cpp26_reflection
    memochat_logging
    memochat_runtime
    memochat_auth_security
    memochat_proto
    memochat_boost_stubs
    Boost::filesystem
    Boost::headers
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    hiredis::hiredis
    Threads::Threads
    ${MEMOCHAT_SPDLOG_TARGET}
)
# nghttp2 支持已移除;显式置 0 防止 target_compile_definitions 传出空值。
if(NOT DEFINED MEMOCHAT_HAVE_NGHTTP2)
    set(MEMOCHAT_HAVE_NGHTTP2 0)
endif()
target_compile_definitions(memochat_server_deps INTERFACE MEMOCHAT_ENABLE_KAFKA=${MEMOCHAT_ENABLE_KAFKA} MEMOCHAT_ENABLE_RABBITMQ=${MEMOCHAT_ENABLE_RABBITMQ} MEMOCHAT_ENABLE_HTTP3=${MEMOCHAT_ENABLE_HTTP3} MEMOCHAT_HAVE_NGHTTP2=${MEMOCHAT_HAVE_NGHTTP2})

if(TARGET msquic)
    target_link_libraries(memochat_server_deps INTERFACE msquic)
    target_compile_definitions(memochat_server_deps INTERFACE MEMOCHAT_ENABLE_MSQUIC=1)
else()
    target_compile_definitions(memochat_server_deps INTERFACE MEMOCHAT_ENABLE_MSQUIC=0)
endif()

if(MEMOCHAT_ENABLE_HTTP3)
    target_link_libraries(memochat_server_deps INTERFACE msquic OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(memochat_server_deps INTERFACE MEMOCHAT_ENABLE_HTTP3=1)
else()
    target_compile_definitions(memochat_server_deps INTERFACE MEMOCHAT_ENABLE_HTTP3=0)
endif()

if(MEMOCHAT_ENABLE_RABBITMQ)
    target_link_libraries(memochat_server_deps INTERFACE ${MEMOCHAT_RABBITMQ_TARGET})
endif()

if(AWSSDK_FOUND)
    # vcpkg builds aws-sdk-cpp and aws-crt-cpp as SHARED DLLs. The .lib files are
    # DLL import stubs (NULL_THUNK_DATA). We need USE_IMPORT_EXPORT=1 so our code
    # uses __declspec(dllimport), and we must explicitly link all aws .lib files
    # since Ninja doesn't reliably propagate transitive IMPORTED_LINK_DEPENDENT_LIBRARIES.
    target_compile_definitions(memochat_server_deps INTERFACE USE_IMPORT_EXPORT=1)
    if(WIN32)
        # VCPKG_INSTALLED_DIR is set by CMakePresets.json cacheVariables or environment
        if(DEFINED VCPKG_INSTALLED_DIR)
            set(_VCPKG_AWS_LIB_DIR "${VCPKG_INSTALLED_DIR}/x64-windows/lib")
        elseif(DEFINED ENV{VCPKG_INSTALLED_DIR})
            set(_VCPKG_AWS_LIB_DIR "$ENV{VCPKG_INSTALLED_DIR}/x64-windows/lib")
        else()
            set(_VCPKG_AWS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg_installed/x64-windows/lib")
        endif()
        foreach(_aws_lib aws-cpp-sdk-core aws-cpp-sdk-s3 aws-crt-cpp aws-c-http aws-c-mqtt aws-c-auth aws-c-cal aws-c-common aws-c-io aws-checksums aws-c-event-stream aws-c-s3 aws-c-compression aws-c-sdkutils)
            set(_lib_path "${_VCPKG_AWS_LIB_DIR}/${_aws_lib}.lib")
            if(EXISTS "${_lib_path}")
                target_link_libraries(memochat_server_deps INTERFACE "${_lib_path}")
            endif()
        endforeach()
    endif()
    message(STATUS "MinIO/S3: AWS SDK found — object storage support enabled")
else()
    message(STATUS "MinIO/S3: AWS SDK not found — S3 storage will be unavailable")
endif()

if(WIN32)
    target_compile_definitions(memochat_server_deps INTERFACE
        _WIN32_WINNT=0x0A00
        WINVER=0x0A00)
    # Use full path for bcrypt so the Windows SDK directory is always found,
    # regardless of how vcpkg or other deps configure LIBPATH.
    target_link_directories(memochat_server_deps INTERFACE
        "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/um/x64")
    target_link_libraries(memochat_server_deps INTERFACE
        "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/um/x64/bcrypt.lib"
        ws2_32 wsock32 crypt32 advapi32 iphlpapi)
endif()

function(memochat_configure_server_target target_name)
    target_link_libraries(${target_name} PRIVATE memochat_server_deps)
    target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

    file(GLOB local_dlls "${CMAKE_CURRENT_SOURCE_DIR}/*.dll")
    foreach(local_dll IN LISTS local_dlls)
        add_custom_command(
            TARGET ${target_name}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${local_dll}"
            "$<TARGET_FILE_DIR:${target_name}>"
        )
    endforeach()

    # Auto-deploy is opt-in. Runtime service executables are often running and
    # locked on Windows; deploying from POST_BUILD makes otherwise successful
    # builds fail. Use tools/scripts/status/deploy_services.bat for deployment.
    # Environment variable: MEMOCHAT_RUNTIME_BASE (defaults to ${sourceDir}/infra/Memo_ops/runtime/services)
    if(WIN32 AND MEMOCHAT_AUTO_DEPLOY_RUNTIME)
        if(DEFINED ENV{MEMOCHAT_RUNTIME_BASE})
            file(TO_CMAKE_PATH "$ENV{MEMOCHAT_RUNTIME_BASE}" RUNTIME_BASE_NATIVE)
        else()
            file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/infra/Memo_ops/runtime/services" RUNTIME_BASE_NATIVE)
        endif()
        get_target_property(_memochat_target_source_dir ${target_name} SOURCE_DIR)
        set(_memochat_target_config "${CMAKE_SOURCE_DIR}/apps/server/core/${target_name}/config.ini")
        if(NOT EXISTS "${_memochat_target_config}" AND _memochat_target_source_dir)
            set(_memochat_target_config "${_memochat_target_source_dir}/config.ini")
        endif()
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RUNTIME_BASE_NATIVE}/${target_name}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${target_name}>"
                "${RUNTIME_BASE_NATIVE}/${target_name}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_memochat_target_config}"
                "${RUNTIME_BASE_NATIVE}/${target_name}"
            COMMENT "Auto-deploying ${target_name} -> ${RUNTIME_BASE_NATIVE}/${target_name}"
        )
    endif()
endfunction()

# Absolute path to apps/server/core, captured here so helper functions can build
# include roots that do not depend on CMAKE_CURRENT_SOURCE_DIR (which, inside a
# function(), is the calling subdirectory).
set(MEMOCHAT_SERVER_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# --------------------------------------------------------------------------
# Shared Gate library helpers.
#
# Slice j of the gateserver split makes each microservice physically own its
# domain code: the per-domain static libraries (Gate*Core / Gate*Domain) are
# defined in their owning service directory (AIGatewayService/, MediaService/,
# AccountShared/, ...) instead of all living under GateShared/. These two
# helpers carry the common dependency + include wiring every such library
# needs, so they are defined here at the server/core root (before any
# add_subdirectory) and reused by every service CMakeLists.
#
# NOTE: inside a function() CMAKE_CURRENT_SOURCE_DIR is the CALLER's subdir, so
# the shared GateShared framework/infra include roots are anchored to
# MEMOCHAT_SERVER_CORE_DIR. Each domain library adds its own local domain/core
# include roots on top in its own CMakeLists (where CMAKE_CURRENT_SOURCE_DIR is
# that service dir).

# Infra-tier include roots: the GateInfraCore persistence/cache/config/clients/
# runtime layer that lives in GateShared/core plus shared common headers.
set(MEMOCHAT_GATE_CORE_INCLUDE_DIRS
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/core
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/core/cache
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/core/clients
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/core/config
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/core/persistence
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/core/runtime
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/core/support
    ${MEMOCHAT_SERVER_CORE_DIR}/common
)

# Apply the shared dependency/compile settings every gate *core* library (infra
# base + per-domain support cores) needs. Mirrors the old gate-core helper.
function(memochat_configure_gate_core_lib target_name)
    target_compile_features(${target_name} PUBLIC cxx_std_23)
    target_include_directories(${target_name} PUBLIC ${MEMOCHAT_GATE_CORE_INCLUDE_DIRS})
    target_link_libraries(${target_name} PUBLIC memochat_server_deps)
    target_link_libraries(${target_name} PRIVATE ${MEMOCHAT_POSTGRES_TARGET})
    target_link_libraries(${target_name} PUBLIC ${MEMOCHAT_MONGOC_TARGET})
    target_link_libraries(${target_name} PRIVATE ${MEMOCHAT_BSON_TARGET})
    target_link_libraries(${target_name} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    if(AWSSDK_FOUND)
        target_link_libraries(${target_name} PUBLIC aws-cpp-sdk-s3)
    endif()
    if(MEMOCHAT_ENABLE_KAFKA)
        target_link_libraries(${target_name} PRIVATE ${MEMOCHAT_RDKAFKA_TARGET})
    endif()
    if(MEMOCHAT_ENABLE_RABBITMQ)
        target_link_libraries(${target_name} PRIVATE ${MEMOCHAT_RABBITMQ_TARGET})
    endif()
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
        target_link_libraries(${target_name} PRIVATE "${_WINSDK_LIB_UM}/bcrypt.lib")
        target_link_options(${target_name} PRIVATE
            "/LIBPATH:${_WINSDK_LIB_UM}"
            "/LIBPATH:${_WINSDK_LIB_UCRT}")
    endif()
    set_target_properties(${target_name} PROPERTIES FOLDER "Server/GateShared")
endfunction()

# App-tier include roots: the GateRuntimeCore framework (LogicSystem, routing,
# transports glue) that lives in GateShared.
set(MEMOCHAT_GATE_APP_INCLUDE_DIRS
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/app
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/core
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/transports/h3
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/transports/h3/listener
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/transports/h3/legacy_routes
)

# Apply the shared settings every gate *app/domain* library (the framework
# runtime core + each Gate<X>Domain route library) needs.
#
# Links only the LEAN infra base every gate app needs — GateInfraConfig (which
# pulls GateInfraRuntime: ConfigMgr, GateGlobals, the IO pool, the readiness-probe
# contract) — NOT the full GateInfraCore facade. This is what stops the framework
# from force-linking persistence/cache/clients into every service: the domain-
# server bootstrap no longer references any concrete DB/Redis manager, so a domain
# that actually touches a database (Moments, ...) now declares that slice itself
# instead of inheriting libpq/mongoc/bson silently through the framework. AWS is
# kept here because GateDomainServer owns the process-wide Aws::InitAPI lifecycle.
function(memochat_configure_gate_app_lib target_name)
    set_target_properties(${target_name} PROPERTIES FOLDER "Server/GateShared")
    memochat_configure_server_target(${target_name})
    target_include_directories(${target_name} PUBLIC ${MEMOCHAT_GATE_APP_INCLUDE_DIRS})
    target_link_libraries(${target_name} PUBLIC
        GateInfraConfig
        memochat_boost_stubs
    )
    if(AWSSDK_FOUND)
        target_link_libraries(${target_name} PUBLIC aws-cpp-sdk-s3)
    endif()
endfunction()

function(memochat_configure_gate_focused_service target_name)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs DOMAINS)
    cmake_parse_arguments(MEMOCHAT_GATE_FOCUSED "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(NOT MEMOCHAT_GATE_FOCUSED_DOMAINS)
        message(FATAL_ERROR "memochat_configure_gate_focused_service(${target_name}) requires DOMAINS")
    endif()

    set_target_properties(${target_name} PROPERTIES FOLDER "Server")
    memochat_configure_server_target(${target_name})
    target_link_libraries(${target_name} PRIVATE
        GateRuntimeCore
        ${MEMOCHAT_GATE_FOCUSED_DOMAINS}
        memochat_boost_stubs
    )
    # NOTE: inside a function() CMAKE_CURRENT_SOURCE_DIR resolves to the CALLER's
    # subdirectory (the per-service dir), so the shared gateway framework
    # include roots must be anchored to MEMOCHAT_SERVER_CORE_DIR (server/core),
    # captured at the top of this file.
    target_include_directories(${target_name} PRIVATE
        ${MEMOCHAT_SERVER_CORE_DIR}/GateShared
        ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/core
        ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/app
        ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/transports/h3
        ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/transports/h3/listener
        ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/transports/h3/legacy_routes
    )
endfunction()

if(BUILD_TESTS)
    # GTest package is loaded and GTest_TestLib is defined in the root tests/CMakeLists.txt.
    # Test subdirectories are registered there (including the ones below).
    # Keep this guard so "cmake --build server/" without BUILD_TESTS still works.
    find_package(GTest CONFIG REQUIRED)
endif()

if(BUILD_SERVER_SERVICES)
    add_subdirectory(ChatServer)

    # --- Shared gate infrastructure base (GateInfraCore + GateServerCore iface).
    add_subdirectory(GateShared/core)

    # --- Microservices: each owns its domain code (Gate<X>Domain) and, where it
    # has domain-specific support, its own core library (Gate<X>Core). These are
    # processed before the legacy transports / GateServerCore-aggregate consumers
    # so every domain-core target name exists when those aggregates reference it.
    # AccountShared holds the auth+account aggregate (GateAccountCore +
    # GateAuthDomain + GateAccountDomain) that Register/Login/Account all link.
    add_subdirectory(AccountShared)
    add_subdirectory(AIGatewayService)
    add_subdirectory(MediaService)
    add_subdirectory(MomentsService)
    add_subdirectory(CallService)
    add_subdirectory(R18Service)
    add_subdirectory(RegisterService)
    add_subdirectory(LoginService)
    add_subdirectory(AccountService)

    # --- Shared gate runtime framework + compat transports. These depend
    # on the domain-core/domain targets defined above (GateServerCore aggregate,
    # GateAppCore compat). Default-buildable: GateRuntimeCore and GateServerHttp3.
    add_subdirectory(GateShared)
    add_subdirectory(GateShared/transports/h3)

    add_subdirectory(VarifyServer)
    add_subdirectory(AIServer)
endif()
