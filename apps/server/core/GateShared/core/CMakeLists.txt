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

# GlazeCompat.hpp (glaze) requires C++23; enforce it regardless of inherited
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
    cache/RedisMgr.hpp
    cache/RedisPipelines.cpp
    cache/RedisPipelines.hpp
    clients/AuthVerifyClient.cpp
    clients/AuthVerifyClient.hpp
    clients/ChatGrpcClient.cpp
    clients/ChatGrpcClient.hpp
    clients/VerifyGrpcClient.cpp
    clients/VerifyGrpcClient.hpp
    config/ConfigMgr.cpp
    config/ConfigMgr.hpp
    config/const.hpp
    persistence/CallSessionTypes.hpp
    persistence/PostgresDao.cpp
    persistence/PostgresDaoAccount.cpp
    persistence/PostgresDao.hpp
    persistence/PostgresMgr.cpp
    persistence/PostgresMgr.hpp
    persistence/MongoDao.cpp
    persistence/MongoDao.hpp
    persistence/MongoMgr.cpp
    persistence/MongoMgr.hpp
    runtime/AsioIOServicePool.cpp
    runtime/AsioIOServicePool.hpp
    runtime/GateGlobals.cpp
    runtime/GateGlobals.hpp
    runtime/Singleton.hpp
    support/BearerAccessAuth.cpp       # Strict Authorization: Bearer access-token resolver
    support/BearerAccessAuth.hpp
    support/GateRouteModules.hpp           # Shared route registration interface (H1+H3 handlers)
    support/UserTokenValidator.cpp        # Shared utoken_<uid> Redis token validator (finding ④)
    support/UserTokenValidator.hpp
)
memochat_configure_gate_core_lib(GateInfraCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateInfraCore
        MODULES
            memochat.gate.chat_grpc_client_algorithms=clients/cxx_modules/ChatGrpcClient.cppm
            memochat.gate.mongo_dao_algorithms=persistence/cxx_modules/MongoDao.cppm
            memochat.gate.mongo_mgr_algorithms=persistence/cxx_modules/MongoMgr.cppm
            memochat.gate.postgres_dao_account_algorithms=persistence/cxx_modules/PostgresDaoAccount.cppm
            memochat.gate.postgres_dao_algorithms=persistence/cxx_modules/PostgresDao.cppm
            memochat.gate.postgres_mgr_algorithms=persistence/cxx_modules/PostgresMgr.cppm
            memochat.gate.redis_mgr_algorithms=cache/cxx_modules/RedisMgr.cppm
            memochat.gate.redis_pipeline_algorithms=cache/cxx_modules/RedisPipeline.cppm
            memochat.gate.bearer_access_auth_algorithms=support/cxx_modules/BearerAccessAuth.cppm
            memochat.gate.user_token_validator_algorithms=support/cxx_modules/UserTokenValidator.cppm
        CONSUMER_SOURCES
            clients/ChatGrpcClient.cpp
            persistence/MongoDao.cpp
            persistence/MongoMgr.cpp
            persistence/PostgresDaoAccount.cpp
            persistence/PostgresDao.cpp
            persistence/PostgresMgr.cpp
            cache/RedisMgr.cpp
            cache/RedisPipelines.cpp
            support/BearerAccessAuth.cpp
            support/UserTokenValidator.cpp
    )
endif()

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
