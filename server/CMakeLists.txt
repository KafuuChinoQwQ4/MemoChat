cmake_minimum_required(VERSION 3.16)

project(MemoChatServer LANGUAGES CXX)

if(MSVC)
    if(DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES "static")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
endif()

find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
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

# HTTP/3 requires ngtcp2 + nghttp3 + openssl (always try for server feature)
set(MEMOCHAT_ENABLE_HTTP3 0)
if(TARGET ngtcp2::ngtcp2 AND TARGET nghttp3::nghttp3 AND TARGET OpenSSL::SSL)
    set(MEMOCHAT_ENABLE_HTTP3 1)
    message(STATUS "HTTP/3: ngtcp2 + nghttp3 + OpenSSL found, enabling HTTP/3 support")
else()
    find_package(ngtcp2 CONFIG QUIET)
    find_package(nghttp3 CONFIG QUIET)
    if(TARGET ngtcp2::ngtcp2 AND TARGET nghttp3::nghttp3)
        find_package(OpenSSL CONFIG QUIET)
        if(TARGET OpenSSL::SSL)
            set(MEMOCHAT_ENABLE_HTTP3 1)
            message(STATUS "HTTP/3: ngtcp2 + nghttp3 + OpenSSL found, enabling HTTP/3 support")
        else()
            message(STATUS "HTTP/3: OpenSSL not found, HTTP/3 support disabled")
        endif()
    else()
        message(STATUS "HTTP/3: ngtcp2 or nghttp3 not found, HTTP/3 support disabled")
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

add_library(memochat_logging STATIC
    common/logging/LogConfig.cpp
    common/logging/Logger.cpp
    common/logging/Redaction.cpp
    common/logging/Telemetry.cpp
    common/logging/TelemetryConfig.cpp
    common/logging/GrpcTrace.cpp
)
target_compile_features(memochat_logging PUBLIC cxx_std_17)
target_include_directories(memochat_logging PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)
target_link_libraries(memochat_logging PUBLIC
    JsonCpp::JsonCpp
    ${MEMOCHAT_SPDLOG_TARGET}
)

add_library(memochat_runtime STATIC
    common/runtime/IniConfig.cpp
    common/runtime/IoContextPool.cpp
    common/runtime/EtcdConfig.cpp
)
target_compile_features(memochat_runtime PUBLIC cxx_std_17)
target_include_directories(memochat_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)
target_link_libraries(memochat_runtime PUBLIC
    Boost::filesystem
)

set(MEMOCHAT_PROTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(MEMOCHAT_PROTO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/common/proto/message.proto")
set(MEMOCHAT_PROTO_GEN_DIR "${CMAKE_BINARY_DIR}/generated")
set(MEMOCHAT_PROTO_CC "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/message.pb.cc")
set(MEMOCHAT_PROTO_H "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/message.pb.h")
set(MEMOCHAT_GRPC_CC "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/message.grpc.pb.cc")
set(MEMOCHAT_GRPC_H "${MEMOCHAT_PROTO_GEN_DIR}/common/proto/message.grpc.pb.h")

add_custom_command(
    OUTPUT
        "${MEMOCHAT_PROTO_CC}"
        "${MEMOCHAT_PROTO_H}"
        "${MEMOCHAT_GRPC_CC}"
        "${MEMOCHAT_GRPC_H}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MEMOCHAT_PROTO_GEN_DIR}/common/proto"
    COMMAND $<TARGET_FILE:protobuf::protoc> --proto_path=${MEMOCHAT_PROTO_ROOT} --cpp_out=${MEMOCHAT_PROTO_GEN_DIR} ${MEMOCHAT_PROTO_SRC}
    COMMAND $<TARGET_FILE:protobuf::protoc> --proto_path=${MEMOCHAT_PROTO_ROOT} --grpc_out=${MEMOCHAT_PROTO_GEN_DIR} --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin> ${MEMOCHAT_PROTO_SRC}
    DEPENDS
        "${MEMOCHAT_PROTO_SRC}"
        protobuf::protoc
        gRPC::grpc_cpp_plugin
    VERBATIM
)

add_library(memochat_proto STATIC
    "${MEMOCHAT_PROTO_CC}"
    "${MEMOCHAT_PROTO_H}"
    "${MEMOCHAT_GRPC_CC}"
    "${MEMOCHAT_GRPC_H}"
)
target_compile_features(memochat_proto PUBLIC cxx_std_17)
target_include_directories(memochat_proto PUBLIC
    ${MEMOCHAT_PROTO_GEN_DIR}
)
target_link_libraries(memochat_proto PUBLIC
    gRPC::grpc++
    protobuf::libprotobuf
)

add_library(memochat_server_deps INTERFACE)
target_compile_features(memochat_server_deps INTERFACE cxx_std_17)
target_include_directories(memochat_server_deps INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)
target_link_libraries(memochat_server_deps INTERFACE
    memochat_logging
    memochat_runtime
    memochat_proto
    Boost::filesystem
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    hiredis::hiredis
    JsonCpp::JsonCpp
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
    target_link_libraries(memochat_server_deps INTERFACE ngtcp2::ngtcp2 nghttp3::nghttp3 OpenSSL::SSL OpenSSL::Crypto)
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
        set(_VCPKG_AWS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg_installed/x64-windows/lib")
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
    target_link_libraries(memochat_server_deps INTERFACE bcrypt ws2_32 wsock32 crypt32 advapi32 iphlpapi)
endif()

# Require C++20 for std::span and other standard library features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fix: Override Qt's -external:W0 which excludes system include directories
# Try /Zc:twoPhase- to fix conformance mode issues with VS2026 + Windows SDK
# CRITICAL: Define NOMINMAX and use std::byte namespace to fix byte ambiguity
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /external:I \"D:/VS2026stable/VC/Tools/MSVC/14.50.35717/ATLMFC/include\" /external:I \"D:/VS2026stable/VC/Tools/MSVC/14.50.35717/include\" /external:I \"C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/ucrt\" /external:I \"C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared\" /external:I \"C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um\" /external:W0 /D_WIN32_WINNT=0x0601 /Zc:__cplusplus /Zc:twoPhase-")

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

    # Auto-deploy to Memo_ops runtime directory after each build.
    if(WIN32)
        set(RUNTIME_BASE "D:\\MemoChat-Qml-Drogon\\Memo_ops\\runtime\\services")
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${target_name}>"
                "${RUNTIME_BASE}\\${target_name}\\"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE_DIR:${target_name}>/config.ini"
                "${RUNTIME_BASE}\\${target_name}\\"
            COMMENT "Auto-deploying ${target_name} -> ${RUNTIME_BASE}\\${target_name}\\"
        )
    endif()
endfunction()

if(BUILD_TESTS)
    # GTest package is loaded and GTest_TestLib is defined in the root tests/CMakeLists.txt.
    # Test subdirectories are registered there (including the ones below).
    # Keep this guard so "cmake --build server/" without BUILD_TESTS still works.
    find_package(GTest CONFIG REQUIRED)
endif()

add_subdirectory(ChatServer)
add_subdirectory(GateServerCore)
add_subdirectory(GateServerHttp3)
add_subdirectory(GateServer)
add_subdirectory(StatusServer)
add_subdirectory(VarifyServer)
