cmake_minimum_required(VERSION 3.16)

project(GateServerCore LANGUAGES CXX)

# GateServerCore - shared library for all GateServer variants (H1, H2, H3)
# Contains: gRPC clients, Postgres/Redis managers, Auth/Call services, etc.
# This library does NOT include any HTTP protocol-specific code.

add_library(GateServerCore STATIC
    AsioIOServicePool.cpp
    AsioIOServicePool.h
    AuthLoginSupport.cpp
    AuthLoginSupport.h
    CallService.cpp
    CallService.h
    ChatGrpcClient.cpp
    ChatGrpcClient.h
    ConfigMgr.cpp
    ConfigMgr.h
    const.h
    GateAsyncSideEffects.cpp
    GateAsyncSideEffects.h
    GateGlobals.cpp
    GateGlobals.h
    GateRouteModules.h           # Shared route registration interface (H1+H3 handlers)
    MediaHttpService.h
    MediaStorage.cpp
    MediaStorage.h
    MongoDao.cpp
    MongoDao.h
    MongoMgr.cpp
    MongoMgr.h
    PostgresDao.cpp
    PostgresDao.h
    PostgresMgr.cpp
    PostgresMgr.h
    RedisMgr.cpp
    RedisMgr.h
    RedisPipelines.cpp
    RedisPipelines.h
    Singleton.h
    StatusGrpcClient.cpp
    StatusGrpcClient.h
    VerifyGrpcClient.cpp
    VerifyGrpcClient.h
    Http2MediaSupport.cpp
    Http2MediaSupport.h
    Http2ProfileSupport.cpp
    Http2ProfileSupport.h
    S3MediaStorage.cpp
    S3MediaStorage.h
)

target_compile_features(GateServerCore PUBLIC cxx_std_23)

# GateServerCore pulls in GlazeCompat.h (glaze library) which requires C++23.
# The parent CMAKE_CXX_STANDARD may not propagate correctly to static libraries
# in subdirectory builds — enforce it here to override any inherited value.
set(CMAKE_CXX_STANDARD 23)
target_include_directories(GateServerCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

# GateServerCore links against memochat_server_deps (proto, logging, runtime, gRPC, etc.)
target_link_libraries(GateServerCore PUBLIC
    memochat_server_deps
)

# Additional libs needed by GateServerCore
target_link_libraries(GateServerCore PRIVATE ${MEMOCHAT_PQXX_TARGET})
target_link_libraries(GateServerCore PRIVATE ${MEMOCHAT_MONGOCXX_TARGET} ${MEMOCHAT_BSONCXX_TARGET})
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
