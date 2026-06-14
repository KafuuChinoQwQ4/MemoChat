cmake_minimum_required(VERSION 3.16)

project(GateServerCore LANGUAGES CXX)

# GateServerCore - shared library for all GateServer variants (H1, H2, H3)
# Contains: gRPC clients, Postgres/Redis managers, Auth/Call services, etc.
# This library does NOT include any HTTP protocol-specific code.

add_library(GateServerCore STATIC
    async/GateAsyncSideEffects.cpp
    async/GateAsyncSideEffects.h
    cache/AuthCache.cpp
    cache/AuthCache.h
    cache/RedisMgr.cpp
    cache/RedisMgr.h
    cache/RedisPipelines.cpp
    cache/RedisPipelines.h
    clients/AuthVerifyClient.cpp
    clients/AuthVerifyClient.h
    clients/ChatGrpcClient.cpp
    clients/ChatGrpcClient.h
    clients/StatusGrpcClient.cpp
    clients/StatusGrpcClient.h
    clients/VerifyGrpcClient.cpp
    clients/VerifyGrpcClient.h
    config/ConfigMgr.cpp
    config/ConfigMgr.h
    config/const.h
    persistence/MongoDao.cpp
    persistence/MongoDao.h
    persistence/MongoMgr.cpp
    persistence/MongoMgr.h
    persistence/PostgresDaoAccount.cpp
    persistence/PostgresDao.cpp
    persistence/PostgresDao.h
    persistence/PostgresMgr.cpp
    persistence/PostgresMgr.h
    runtime/AsioIOServicePool.cpp
    runtime/AsioIOServicePool.h
    runtime/GateGlobals.cpp
    runtime/GateGlobals.h
    runtime/Singleton.h
    storage/MediaStorage.cpp
    storage/MediaStorage.h
    storage/S3MediaStorage.cpp
    storage/S3MediaStorage.h
    support/AuthLoginSupport.cpp
    support/AuthLoginSupport.h
    support/CallService.cpp
    support/CallService.h
    support/GateRouteModules.h           # Shared route registration interface (H1+H3 handlers)
    support/Http2MediaSupport.cpp
    support/Http2MediaSupport.h
    support/Http2ProfileSupport.cpp
    support/Http2ProfileSupport.h
    support/MediaHttpService.h
    ../../common/r18/R18SourceService.cpp
    ../../common/r18/R18SourceService.h
)

target_compile_features(GateServerCore PUBLIC cxx_std_23)

# GateServerCore pulls in GlazeCompat.h (glaze library) which requires C++23.
# The parent CMAKE_CXX_STANDARD may not propagate correctly to static libraries
# in subdirectory builds — enforce it here to override any inherited value.
set(CMAKE_CXX_STANDARD ${MEMOCHAT_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
target_include_directories(GateServerCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/async
    ${CMAKE_CURRENT_SOURCE_DIR}/cache
    ${CMAKE_CURRENT_SOURCE_DIR}/clients
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/persistence
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/support
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

# GateServerCore links against memochat_server_deps (proto, logging, runtime, gRPC, etc.)
target_link_libraries(GateServerCore PUBLIC
    memochat_server_deps
)

# Additional libs needed by GateServerCore
target_link_libraries(GateServerCore PRIVATE ${MEMOCHAT_PQXX_TARGET})
target_link_libraries(GateServerCore PRIVATE ${MEMOCHAT_MONGOCXX_TARGET} ${MEMOCHAT_BSONCXX_TARGET})
target_link_libraries(GateServerCore PRIVATE OpenSSL::SSL OpenSSL::Crypto)
if(AWSSDK_FOUND)
    target_link_libraries(GateServerCore PUBLIC aws-cpp-sdk-s3)
endif()
if(MEMOCHAT_ENABLE_KAFKA)
    target_link_libraries(GateServerCore PRIVATE ${MEMOCHAT_CPPKAFKA_TARGET})
endif()
if(MEMOCHAT_ENABLE_RABBITMQ)
    target_link_libraries(GateServerCore PRIVATE ${MEMOCHAT_RABBITMQ_TARGET})
endif()

if(WIN32)
    # Environment variables: WINSDK_LIB_UM, WINSDK_LIB_UCRT, VC_TOOLS_LIB
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
    target_link_libraries(GateServerCore PRIVATE "${_WINSDK_LIB_UM}/bcrypt.lib")
    target_link_options(GateServerCore PRIVATE
        "/LIBPATH:${_WINSDK_LIB_UM}"
        "/LIBPATH:${_WINSDK_LIB_UCRT}")
endif()

set_target_properties(GateServerCore PROPERTIES FOLDER "Server/GateServer")
