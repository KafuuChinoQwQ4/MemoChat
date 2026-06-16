cmake_minimum_required(VERSION 3.16)

project(MemoChatServer LANGUAGES CXX)

option(BUILD_SERVER_SERVICES "Build backend service executables" ON)
option(MEMOCHAT_AUTO_DEPLOY_RUNTIME "Copy built server executables into infra/Memo_ops/runtime/services during build" OFF)
option(MEMOCHAT_BUILD_LEGACY_GATE_HTTP2_STANDALONE "Build the legacy standalone GateServerHttp2 executable" OFF)

# Require C++23 for std::expected, std::unreachable, std::byteswap (glaze library requires C++23).
# Must be set BEFORE any add_subdirectory() calls so it takes effect for all targets.
if(NOT DEFINED MEMOCHAT_CXX_STANDARD)
    set(MEMOCHAT_CXX_STANDARD "23" CACHE STRING "C++ standard used by MemoChat targets")
endif()
set(CMAKE_CXX_STANDARD ${MEMOCHAT_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(bsoncxx CONFIG REQUIRED)
find_package(mongocxx CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(CppKafka CONFIG QUIET)
find_package(cppkafka CONFIG QUIET)
find_package(unofficial-librabbitmq CONFIG QUIET)
find_package(rabbitmq-c CONFIG QUIET)
find_package(msquic CONFIG QUIET)
find_package(OpenSSL CONFIG QUIET)
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

# HTTP/2 requires nghttp2 (Boost.Asio + OpenSSL for TLS/ALPN, nghttp2 for HPACK/H2 framing)
set(MEMOCHAT_HAVE_NGHTTP2 0)
find_package(nghttp2 CONFIG QUIET)
# Use nghttp2::nghttp2_static (the actual .lib/.a) to avoid DLL dependency.
# nghttp2::nghttp2 maps to a SHARED library and requires nghttp2.dll at runtime.
if((TARGET nghttp2::nghttp2_static OR TARGET nghttp2::nghttp2) AND TARGET OpenSSL::SSL)
    set(MEMOCHAT_HAVE_NGHTTP2 1)
    message(STATUS "HTTP/2: nghttp2 (static) found, enabling HTTP/2 support")
else()
    message(STATUS "HTTP/2: nghttp2 or OpenSSL not found — HTTP/2 support disabled")
endif()

if(TARGET spdlog::spdlog)
    set(MEMOCHAT_SPDLOG_TARGET spdlog::spdlog)
elseif(TARGET spdlog::spdlog_header_only)
    set(MEMOCHAT_SPDLOG_TARGET spdlog::spdlog_header_only)
else()
    message(FATAL_ERROR "spdlog target not found")
endif()

if(TARGET mongo::mongocxx_shared)
    set(MEMOCHAT_MONGOCXX_TARGET mongo::mongocxx_shared)
elseif(TARGET mongo::mongocxx_static)
    set(MEMOCHAT_MONGOCXX_TARGET mongo::mongocxx_static)
else()
    message(FATAL_ERROR "mongocxx target not found")
endif()

if(TARGET mongo::bsoncxx_shared)
    set(MEMOCHAT_BSONCXX_TARGET mongo::bsoncxx_shared)
elseif(TARGET mongo::bsoncxx_static)
    set(MEMOCHAT_BSONCXX_TARGET mongo::bsoncxx_static)
else()
    message(FATAL_ERROR "bsoncxx target not found")
endif()

if(TARGET libpqxx::pqxx)
    set(MEMOCHAT_PQXX_TARGET libpqxx::pqxx)
else()
    message(FATAL_ERROR "libpqxx target not found")
endif()

set(MEMOCHAT_ENABLE_KAFKA 0)
if(TARGET CppKafka::cppkafka)
    set(MEMOCHAT_CPPKAFKA_TARGET CppKafka::cppkafka)
    set(MEMOCHAT_ENABLE_KAFKA 1)
elseif(TARGET cppkafka)
    set(MEMOCHAT_CPPKAFKA_TARGET cppkafka)
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
    "${MEMOCHAT_VCPKG_TRIPLET_DIR}/include/mongocxx/v_noabi"
    "${MEMOCHAT_VCPKG_TRIPLET_DIR}/include/bsoncxx/v_noabi"
)
target_link_libraries(memochat_server_deps INTERFACE
    memochat_logging
    memochat_runtime
    memochat_proto
    memochat_boost_stubs
    Boost::filesystem
    Boost::headers
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    hiredis::hiredis
    ${MEMOCHAT_SPDLOG_TARGET}
)
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

if(MEMOCHAT_HAVE_NGHTTP2)
    # Prefer the static library target to avoid DLL dependency.
    if(TARGET nghttp2::nghttp2_static)
        target_link_libraries(memochat_server_deps INTERFACE nghttp2::nghttp2_static)
    else()
        target_link_libraries(memochat_server_deps INTERFACE nghttp2::nghttp2)
    endif()
    target_compile_definitions(memochat_server_deps INTERFACE MEMOCHAT_HAVE_NGHTTP2=1)
else()
    target_compile_definitions(memochat_server_deps INTERFACE MEMOCHAT_HAVE_NGHTTP2=0)
endif()

if(MEMOCHAT_ENABLE_KAFKA)
    target_compile_definitions(memochat_server_deps INTERFACE CPPKAFKA_STATIC)
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
    if(MSVC)
        target_compile_options(memochat_server_deps INTERFACE /EHsc)
    endif()

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

function(memochat_configure_gate_focused_service target_name)
    set_target_properties(${target_name} PROPERTIES FOLDER "Server")
    memochat_configure_server_target(${target_name})
    target_link_libraries(${target_name} PRIVATE
        GateAppCore
        GateServerCore
        GateServerHttp3
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
    add_subdirectory(GateShared/core)
    # Legacy H1.1 standalone transport remains opt-in; R18 plugin helpers stay default-buildable.
    add_subdirectory(GateShared/transports/h1/legacy_standalone)
    add_subdirectory(GateShared/plugins/r18)
    add_subdirectory(GateShared/transports/h3)
    if(MEMOCHAT_BUILD_LEGACY_GATE_HTTP2_STANDALONE AND libh2o_FOUND)
        add_subdirectory(GateShared/transports/h2)
    elseif(MEMOCHAT_BUILD_LEGACY_GATE_HTTP2_STANDALONE)
        message(WARNING "MEMOCHAT_BUILD_LEGACY_GATE_HTTP2_STANDALONE=ON but libh2o was not found; skipping GateServerHttp2")
    endif()
    add_subdirectory(GateShared)
    add_subdirectory(AIGatewayService)
    add_subdirectory(MediaService)
    add_subdirectory(MomentsService)
    add_subdirectory(CallService)
    add_subdirectory(R18Service)
    add_subdirectory(RegisterService)
    add_subdirectory(LoginService)
    add_subdirectory(AccountService)
    add_subdirectory(VarifyServer)
    add_subdirectory(AIServer)
endif()
