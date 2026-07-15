cmake_minimum_required(VERSION 3.16)

project(GateInfraCore LANGUAGES CXX)

# Slice j of the gateserver microservice split relocated each domain's code into
# its owning service dir. This file owns what remains: the SHARED INFRASTRUCTURE
# base that every domain depends on but no single domain owns.
#
# ── Decomposition (finding: "GateShared 仍是高风险共享层") ────────────────────
# The infra base used to be ONE fused static archive (GateInfraCore) bundling six
# unrelated concerns: runtime + config + cache + gRPC clients + persistence +
# cross-cutting auth support. That fusion is the coupling the architecture review
# flagged: touching any one .cpp rebuilt the whole archive and relinked every
# service, and every consumer was force-linked against libpq + mongoc + bson even
# when it used no database at all (the AI proxy, R18).
#
# It is now split into six LEAF slices along their real dependency DAG, so a
# change to persistence no longer recompiles cache TUs, and only the persistence
# slice pulls the heavy DB externals:
#
#   GateInfraRuntime      (base)          AsioIOServicePool, GateGlobals, Singleton
#   GateInfraConfig    -> Runtime         ConfigMgr, const
#   GateInfraCache     -> Config          RedisMgr, RedisPipelines            [hiredis]
#   GateInfraClients   -> Config          Auth/Chat/Verify gRPC clients       [grpc]
#   GateInfraPersistence -> Config        Postgres/Mongo Dao+Mgr        [pq/mongoc/bson]
#   GateInfraSupport   -> Cache, Config   BearerAccessAuth, UserTokenValidator
#
# GateInfraCore stays as the target name every existing consumer references, but
# it is now an INTERFACE aggregate that links all six slices — so the fat domain
# cores (GateR18Core/GateMediaCore/GateCallCore/GateAccountCore) that link
# "GateInfraCore" keep their full transitive closure unchanged, while new/leaner
# consumers can depend on just the slice they need.
#
# NOTE on persistence: PostgresMgr/MongoMgr each hold a PostgresDao/MongoDao
# member and forward to all their methods, and PostgresDao is one class split
# across PostgresDao.cpp (broad) + PostgresDaoAccount.cpp (account). The Dao+Mgr
# persistence layer is therefore a single shared unit and lives wholly in
# GateInfraPersistence; it cannot be split per domain without splitting the
# Dao/Mgr classes themselves. This slice isolates persistence as a WHOLE from the
# other concerns — which is the split that removes the cross-service coupling.

# GlazeCompat.hpp (glaze) requires C++23; enforce it regardless of inherited
# CMAKE_CXX_STANDARD in subdirectory builds.
set(CMAKE_CXX_STANDARD ${MEMOCHAT_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Lean base wiring shared by every infra slice: C++ standard, the shared infra
# include roots (persistence/cache/config/clients/runtime/support + common), the
# common server dependency bundle (proto/grpc/hiredis/boost/spdlog/sodium/...),
# and the Windows SDK bcrypt/libpath fixups. Deliberately does NOT link the heavy
# database externals (Postgres/mongoc/bson) or AWS/Kafka/RabbitMQ — only the one
# slice that needs each attaches it, so the dependency stays where it is used.
function(memochat_configure_gate_infra_slice target_name)
    target_compile_features(${target_name} PUBLIC cxx_std_23)
    target_include_directories(${target_name} PUBLIC ${MEMOCHAT_GATE_CORE_INCLUDE_DIRS})
    target_link_libraries(${target_name} PUBLIC memochat_server_deps)
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

# --- GateInfraRuntime: the leaf base. IO service pool, process globals, the
# Singleton template, and the transport-agnostic readiness-probe contract the
# domain-server bootstrap consumes. Depends on nothing but the server bundle. ---
add_library(GateInfraRuntime STATIC
    runtime/AsioIOServicePool.cpp
    runtime/AsioIOServicePool.hpp
    runtime/GateGlobals.cpp
    runtime/GateGlobals.hpp
    runtime/GateReadinessProbe.hpp
    runtime/Singleton.hpp
)
memochat_configure_gate_infra_slice(GateInfraRuntime)

# --- GateInfraConfig: config manager + shared constants. ----------------------
add_library(GateInfraConfig STATIC
    config/ConfigMgr.cpp
    config/ConfigMgr.hpp
    config/const.hpp
)
memochat_configure_gate_infra_slice(GateInfraConfig)
target_link_libraries(GateInfraConfig PUBLIC GateInfraRuntime)

# --- GateInfraCache: Redis manager + pipelines, and the Redis readiness probe
# factory. hiredis comes from the shared server bundle. -----------------------
add_library(GateInfraCache STATIC
    cache/RedisMgr.cpp
    cache/RedisMgr.hpp
    cache/RedisPipelines.cpp
    cache/RedisPipelines.hpp
    cache/CacheReadinessProbes.cpp
    cache/CacheReadinessProbes.hpp
)
memochat_configure_gate_infra_slice(GateInfraCache)
target_link_libraries(GateInfraCache PUBLIC GateInfraConfig)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateInfraCache
        MODULES
            memochat.gate.redis_mgr_algorithms=cache/cxx_modules/RedisMgr.cppm
            memochat.gate.redis_pipeline_algorithms=cache/cxx_modules/RedisPipeline.cppm
        CONSUMER_SOURCES
            cache/RedisMgr.cpp
            cache/RedisPipelines.cpp
    )
endif()

# --- GateInfraClients: internal gRPC clients (Auth/Chat/Verify). grpc + proto
# come from the shared server bundle. -----------------------------------------
add_library(GateInfraClients STATIC
    clients/AuthVerifyClient.cpp
    clients/AuthVerifyClient.hpp
    clients/ChatGrpcClient.cpp
    clients/ChatGrpcClient.hpp
    clients/VerifyGrpcClient.cpp
    clients/VerifyGrpcClient.hpp
)
memochat_configure_gate_infra_slice(GateInfraClients)
target_link_libraries(GateInfraClients PUBLIC GateInfraConfig)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateInfraClients
        MODULES
            memochat.gate.chat_grpc_client_algorithms=clients/cxx_modules/ChatGrpcClient.cppm
        CONSUMER_SOURCES
            clients/ChatGrpcClient.cpp
    )
endif()

# --- GateInfraPersistence: the whole Postgres/Mongo Dao+Mgr unit (see NOTE) plus
# the persistence readiness probe factories. THIS is the only slice that links
# the heavy database externals, so services that touch no database no longer drag
# libpq/mongoc/bson into their link. -----------------------------------------
add_library(GateInfraPersistence STATIC
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
    persistence/PersistenceReadinessProbes.cpp
    persistence/PersistenceReadinessProbes.hpp
)
memochat_configure_gate_infra_slice(GateInfraPersistence)
target_link_libraries(GateInfraPersistence PUBLIC GateInfraConfig)
target_link_libraries(GateInfraPersistence PRIVATE ${MEMOCHAT_POSTGRES_TARGET})
target_link_libraries(GateInfraPersistence PUBLIC ${MEMOCHAT_MONGOC_TARGET})
target_link_libraries(GateInfraPersistence PRIVATE ${MEMOCHAT_BSON_TARGET})
target_link_libraries(GateInfraPersistence PRIVATE OpenSSL::SSL OpenSSL::Crypto)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateInfraPersistence
        MODULES
            memochat.gate.mongo_dao_algorithms=persistence/cxx_modules/MongoDao.cppm
            memochat.gate.mongo_mgr_algorithms=persistence/cxx_modules/MongoMgr.cppm
            memochat.gate.postgres_dao_account_algorithms=persistence/cxx_modules/PostgresDaoAccount.cppm
            memochat.gate.postgres_dao_algorithms=persistence/cxx_modules/PostgresDao.cppm
            memochat.gate.postgres_mgr_algorithms=persistence/cxx_modules/PostgresMgr.cppm
        CONSUMER_SOURCES
            persistence/MongoDao.cpp
            persistence/MongoMgr.cpp
            persistence/PostgresDaoAccount.cpp
            persistence/PostgresDao.cpp
            persistence/PostgresMgr.cpp
    )
endif()

# --- GateInfraSupport: cross-cutting auth support shared by domains (strict
# Bearer access-token resolver + the one shared utoken_<uid> Redis validator +
# the shared route registration interface). Sits above cache/config. ----------
add_library(GateInfraSupport STATIC
    support/BearerAccessAuth.cpp       # Strict Authorization: Bearer access-token resolver
    support/BearerAccessAuth.hpp
    support/GateRouteModules.hpp           # Shared route registration interface (H1+H3 handlers)
    support/UserTokenValidator.cpp        # Shared utoken_<uid> Redis token validator (finding ④)
    support/UserTokenValidator.hpp
)
memochat_configure_gate_infra_slice(GateInfraSupport)
target_link_libraries(GateInfraSupport PUBLIC GateInfraCache GateInfraConfig)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateInfraSupport
        MODULES
            memochat.gate.bearer_access_auth_algorithms=support/cxx_modules/BearerAccessAuth.cppm
            memochat.gate.user_token_validator_algorithms=support/cxx_modules/UserTokenValidator.cppm
        CONSUMER_SOURCES
            support/BearerAccessAuth.cpp
            support/UserTokenValidator.cpp
    )
endif()

# --- GateInfraCore: the compatibility facade. Every existing consumer links this
# target name and, through it, the full infra closure it always had. New/leaner
# consumers should depend on the specific slice(s) they use instead. Keeping this
# an INTERFACE library (no sources of its own) is enforced by the layering
# guardrail test: business/domain code must never accrete here. ---------------
add_library(GateInfraCore INTERFACE)
target_link_libraries(GateInfraCore INTERFACE
    GateInfraRuntime
    GateInfraConfig
    GateInfraCache
    GateInfraClients
    GateInfraPersistence
    GateInfraSupport
)

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
