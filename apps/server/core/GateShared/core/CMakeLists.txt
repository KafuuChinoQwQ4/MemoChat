cmake_minimum_required(VERSION 3.16)

project(GateInfraCore LANGUAGES CXX)

# Slice j of the gateserver microservice split: each microservice now physically
# owns its domain code. GateShared/core keeps ONLY the shared infrastructure base
# library, GateInfraCore (persistence + cache + config + gRPC clients + runtime),
# which every domain depends on but no single domain owns.
#
# The per-domain support cores were relocated into their owning service dirs and
# are defined there (target names unchanged so existing references stay valid):
#   GateAccountCore -> AccountShared/   (AuthCache, AuthLoginSupport, Http2ProfileSupport, async)
#   GateMediaCore   -> MediaService/    (MediaStorage, S3MediaStorage, Http2MediaSupport)
#   GateCallCore    -> CallService/     (CallService support)
#   GateR18Core     -> R18Service/      (domain/services/r18 source service)
#
# GateServerCore stays here as an INTERFACE aggregate that links GateInfraCore +
# all relocated domain cores by target name, so the legacy transports (H1/H2/H3)
# and the GateAppCore compat aggregate keep linking one "GateServerCore" name
# unchanged. CMake resolves these target names globally at generate time, so the
# forward references to domain cores defined in later-processed service dirs are
# fine.
#
# NOTE on persistence: PostgresMgr/MongoMgr each hold a PostgresDao/MongoDao
# member and forward to ALL their methods, and PostgresDao is a single class
# whose definitions are split across PostgresDao.cpp (broad) and
# PostgresDaoAccount.cpp (account). The Dao+Mgr persistence layer is therefore a
# single shared infrastructure unit and lives wholly in GateInfraCore; it cannot
# be split per domain without splitting the Dao/Mgr classes themselves.

# GlazeCompat.h (glaze) requires C++23; enforce it regardless of inherited
# CMAKE_CXX_STANDARD in subdirectory builds.
set(CMAKE_CXX_STANDARD ${MEMOCHAT_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- GateInfraCore: shared base. Includes the whole persistence layer (broad +
# account DAO, Postgres/Mongo managers) because the Dao/Mgr classes are a single
# shared unit (see NOTE above), plus redis/config/runtime/gRPC clients. Every
# domain core depends on this. The configure helper + include roots are defined
# at the server/core root (memochat_configure_gate_core_lib). ------------------
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
    persistence/CallSessionTypes.h
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
    support/UserTokenValidator.cpp        # Shared utoken_<uid> Redis token validator (finding ④)
    support/UserTokenValidator.h
)
memochat_configure_gate_core_lib(GateInfraCore)

# --- GateServerCore: INTERFACE aggregate of the full monolith-era surface:
# GateInfraCore + every relocated per-domain support core + every relocated
# domain route library. The legacy transports (H1/H2/H3) + the GateAppCore
# compat aggregate link this one name unchanged, and through it transitively
# pick up the (now scattered) domain/core include roots they #include from
# (e.g. h3's GateHttp3ServiceRoutes.cpp pulls services/auth/AuthService.h,
# Http2MediaSupport.h, CallService.h, ...). The domain/core targets are defined
# in their owning service directories; CMake resolves them globally at generate.
add_library(GateServerCore INTERFACE)
target_link_libraries(GateServerCore INTERFACE
    GateInfraCore
    GateAccountCore
    GateMediaCore
    GateCallCore
    GateR18Core
    GateAiDomain
    GateAuthDomain
    GateAccountDomain
    GateMediaDomain
    GateMomentsDomain
    GateCallDomain
    GateR18Domain
)
