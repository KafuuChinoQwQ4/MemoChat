cmake_minimum_required(VERSION 3.16)

project(GateServerCore LANGUAGES CXX)

# Phase 1 of the gateserver microservice split: GateServerCore is decomposed
# into a base infrastructure library (GateInfraCore) plus per-domain static
# libraries (Account/Media/Moments/Call/R18), each layered on top of the base.
#
# GateServerCore is kept as an INTERFACE aggregate that links all of them, so
# the existing consumers (GateServer executable, H1/H2/H3 transport targets)
# keep linking a single "GateServerCore" with no change. Later phases peel the
# per-domain libraries into independent services.
#
# Dependency direction (no cycles): every domain core depends only on
# GateInfraCore; GateInfraCore depends on nothing in this tree. The one
# cross-domain edge (GateAsyncSideEffects -> AuthLoginSupport) is resolved by
# placing async in GateAccountCore alongside the auth-login support.
#
# NOTE on persistence: PostgresMgr/MongoMgr each hold a PostgresDao/MongoDao
# member and forward to ALL their methods, and PostgresDao is a single class
# whose definitions are split across PostgresDao.cpp (broad) and
# PostgresDaoAccount.cpp (account). The Dao+Mgr persistence layer is therefore a
# single shared infrastructure unit and lives wholly in GateInfraCore; it cannot
# be library-split per domain without splitting the Dao/Mgr classes themselves
# (a larger refactor, out of scope for this compile-time split). Domain cores own
# only the domain logic layered ABOVE persistence.

# GlazeCompat.h (glaze) requires C++23; enforce it for all libs defined here
# regardless of inherited CMAKE_CXX_STANDARD in subdirectory builds.
set(CMAKE_CXX_STANDARD ${MEMOCHAT_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MEMOCHAT_GATE_CORE_INCLUDE_DIRS
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

# Apply the shared dependency/compile settings every gate core library needs.
function(memochat_configure_gate_core_lib target_name)
    target_compile_features(${target_name} PUBLIC cxx_std_23)
    target_include_directories(${target_name} PUBLIC ${MEMOCHAT_GATE_CORE_INCLUDE_DIRS})
    target_link_libraries(${target_name} PUBLIC memochat_server_deps)
    target_link_libraries(${target_name} PRIVATE ${MEMOCHAT_PQXX_TARGET})
    target_link_libraries(${target_name} PRIVATE ${MEMOCHAT_MONGOCXX_TARGET} ${MEMOCHAT_BSONCXX_TARGET})
    target_link_libraries(${target_name} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    if(AWSSDK_FOUND)
        target_link_libraries(${target_name} PUBLIC aws-cpp-sdk-s3)
    endif()
    if(MEMOCHAT_ENABLE_KAFKA)
        target_link_libraries(${target_name} PRIVATE ${MEMOCHAT_CPPKAFKA_TARGET})
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
    set_target_properties(${target_name} PROPERTIES FOLDER "Server/GateServer")
endfunction()

# --- GateInfraCore: shared base. Includes the whole persistence layer (broad +
# account DAO, Postgres/Mongo managers) because the Dao/Mgr classes are a single
# shared unit (see NOTE above), plus redis/config/runtime/gRPC clients. Every
# domain core depends on this. ------------------------------------------------
add_library(GateInfraCore STATIC
    cache/RedisMgr.cpp
    cache/RedisMgr.h
    cache/RedisPipelines.cpp
    cache/RedisPipelines.h
    clients/AuthVerifyClient.cpp
    clients/AuthVerifyClient.h
    clients/ChatGrpcClient.cpp
    clients/ChatGrpcClient.h
    clients/VerifyGrpcClient.cpp
    clients/VerifyGrpcClient.h
    config/ConfigMgr.cpp
    config/ConfigMgr.h
    config/const.h
    persistence/PostgresDao.cpp
    persistence/PostgresDaoAccount.cpp
    persistence/PostgresDao.h
    persistence/PostgresMgr.cpp
    persistence/PostgresMgr.h
    persistence/MongoDao.cpp
    persistence/MongoDao.h
    persistence/MongoMgr.cpp
    persistence/MongoMgr.h
    runtime/AsioIOServicePool.cpp
    runtime/AsioIOServicePool.h
    runtime/GateGlobals.cpp
    runtime/GateGlobals.h
    runtime/Singleton.h
    support/GateRouteModules.h           # Shared route registration interface (H1+H3 handlers)
)
memochat_configure_gate_core_lib(GateInfraCore)

# --- GateAccountCore: memo_account aggregate logic above persistence (auth
# cache, login support, profile support, async side-effects). -----------------
add_library(GateAccountCore STATIC
    cache/AuthCache.cpp
    cache/AuthCache.h
    support/AuthLoginSupport.cpp
    support/AuthLoginSupport.h
    support/Http2ProfileSupport.cpp
    support/Http2ProfileSupport.h
    async/GateAsyncSideEffects.cpp
    async/GateAsyncSideEffects.h
)
memochat_configure_gate_core_lib(GateAccountCore)
target_link_libraries(GateAccountCore PUBLIC GateInfraCore)

# --- GateMediaCore: media storage (local + S3) and media support. ------------
add_library(GateMediaCore STATIC
    storage/MediaStorage.cpp
    storage/MediaStorage.h
    storage/S3MediaStorage.cpp
    storage/S3MediaStorage.h
    support/Http2MediaSupport.cpp
    support/Http2MediaSupport.h
    support/MediaHttpService.h
)
memochat_configure_gate_core_lib(GateMediaCore)
target_link_libraries(GateMediaCore PUBLIC GateInfraCore)

# --- GateCallCore: call session/token service. -------------------------------
add_library(GateCallCore STATIC
    support/CallService.cpp
    support/CallService.h
)
memochat_configure_gate_core_lib(GateCallCore)
target_link_libraries(GateCallCore PUBLIC GateInfraCore)

# --- GateR18Core: R18 source service (shared common/r18 implementation). ------
add_library(GateR18Core STATIC
    ../../common/r18/R18SourceService.cpp
    ../../common/r18/R18SourceService.h
)
memochat_configure_gate_core_lib(GateR18Core)
target_link_libraries(GateR18Core PUBLIC GateInfraCore)

# --- GateServerCore: INTERFACE aggregate of all gate core libraries. Existing
# consumers (GateServer + H1/H2/H3 transports) link this one name unchanged. --
add_library(GateServerCore INTERFACE)
target_link_libraries(GateServerCore INTERFACE
    GateInfraCore
    GateAccountCore
    GateMediaCore
    GateCallCore
    GateR18Core
)
