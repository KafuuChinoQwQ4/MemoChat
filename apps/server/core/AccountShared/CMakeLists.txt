# AccountShared — the auth + account aggregate (D-ACCOUNT). Register, Login, and
# Account are separate processes but share one account aggregate: the same
# user-table access, auth cache, login support and route logic. Per the split's
# data-ownership decision they are ONE aggregate, so their shared domain code
# lives here and all three account services link the libraries defined below.
#
# Defines (target names unchanged from the pre-relocation GateShared layout):
#   GateAccountCore    - account support core (was GateShared/core GateAccountCore)
#   GateAuthDomain     - register/login route + service + account persistence
#   GateAccountDomain  - profile route module (account self-service)
#
# Include roots: the shared helpers (memochat_configure_gate_core_lib /
# memochat_configure_gate_app_lib) at the server/core root supply the GateShared
# framework + GateInfraCore infra roots. Each library here adds its own local
# roots PUBLIC so that (a) sibling libraries compile and (b) the legacy transports
# that link the GateServerCore aggregate can still resolve these now-relocated
# headers (e.g. h3 includes services/auth/AuthService.h, Http2ProfileSupport.h,
# AuthLoginSupport.hpp).

# --- GateAccountCore: memo_account aggregate logic above persistence (auth
# cache, login support, profile support, async side-effects). ------------------
add_library(GateAccountCore STATIC
    core/cache/AuthCache.cpp
    core/cache/AuthCache.hpp
    core/support/AuthLoginCacheProfileDto.cpp
    core/support/AuthLoginCacheProfileDto.hpp
    core/support/AuthLoginRateLimiter.cpp
    core/support/AuthLoginRateLimiter.hpp
    core/support/AuthLoginSupport.cpp
    core/support/AuthLoginSupport.hpp
    core/support/AuthPublicDtos.cpp
    core/support/AuthPublicDtos.hpp
    core/support/AuthUserInfo.hpp
    core/support/Http2ProfileSupport.cpp
    core/support/Http2ProfileSupport.hpp
    core/async/GateAsyncSideEffectDtos.cpp
    core/async/GateAsyncSideEffectDtos.hpp
    core/async/GateAsyncSideEffects.cpp
    core/async/GateAsyncSideEffects.hpp
)
memochat_configure_gate_core_lib(GateAccountCore)
target_include_directories(GateAccountCore PUBLIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared
    ${CMAKE_BINARY_DIR}/generated/lua/common
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cache
    ${CMAKE_CURRENT_SOURCE_DIR}/core/support
    ${CMAKE_CURRENT_SOURCE_DIR}/core/async
)
target_link_libraries(GateAccountCore PUBLIC GateInfraCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateAccountCore
        MODULES
            memochat.account.async_side_effect_dto_algorithms=core/async/cxx_modules/GateAsyncSideEffectDto.cppm
            memochat.account.async_side_effect_algorithms=core/async/cxx_modules/GateAsyncSideEffect.cppm
            memochat.account.auth_cache_algorithms=core/cache/cxx_modules/AuthCache.cppm
            memochat.account.auth_login_cache_profile_algorithms=core/support/cxx_modules/AuthLoginCacheProfile.cppm
            memochat.account.auth_login_algorithms=core/support/cxx_modules/AuthLogin.cppm
            memochat.account.auth_public_dto_algorithms=core/support/cxx_modules/AuthPublicDto.cppm
            memochat.account.profile_support_algorithms=core/support/cxx_modules/ProfileSupport.cppm
        CONSUMER_SOURCES
            core/async/GateAsyncSideEffectDtos.cpp
            core/async/GateAsyncSideEffects.cpp
            core/cache/AuthCache.cpp
            core/support/AuthLoginCacheProfileDto.cpp
            core/support/AuthLoginSupport.cpp
            core/support/AuthPublicDtos.cpp
            core/support/Http2ProfileSupport.cpp
    )
endif()

# --- GateAuthDomain: register/login routes + auth service + account persistence.
add_library(GateAuthDomain STATIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/GateRouteProfileRegistrar.hpp
    domain/profiles/GateAuthProfile.cpp
    domain/modules/auth/AuthRouteModule.cpp
    domain/modules/auth/AuthRouteSchemas.cpp
    domain/services/account/AccountPersistence.cpp
    domain/services/auth/AuthService.cpp
)
memochat_configure_gate_app_lib(GateAuthDomain)
target_include_directories(GateAuthDomain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain)
target_link_libraries(GateAuthDomain PUBLIC GateRuntimeCore GateAccountCore)

# ── Embed shared Lua scripts (incr_with_expire used by AuthLoginRateLimiter) ──
include(EmbedLuaScripts)
memochat_embed_lua_scripts(
    SCRIPTS
        "${CMAKE_SOURCE_DIR}/apps/server/core/common/lua/incr_with_expire.lua"
    OUTPUT_DIR  "${CMAKE_BINARY_DIR}/generated/lua/common"
    HEADER_NAME "common_lua_scripts.hpp"
    NAMESPACE   "memochat::common::lua_scripts"
)
target_include_directories(GateAuthDomain PRIVATE "${CMAKE_BINARY_DIR}/generated/lua/common")
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateAuthDomain
        MODULES
            memochat.account.auth_route_registration_algorithms=domain/modules/auth/cxx_modules/AuthRouteRegistration.cppm
            memochat.account.auth_route_schema_algorithms=domain/modules/auth/cxx_modules/AuthRouteSchema.cppm
            memochat.account.auth_service_algorithms=domain/services/auth/cxx_modules/AuthService.cppm
        CONSUMER_SOURCES
            domain/modules/auth/AuthRouteModule.cpp
            domain/modules/auth/AuthRouteSchemas.cpp
            domain/services/auth/AuthService.cpp
    )
endif()

# --- GateAccountDomain: account self-service profile routes. ------------------
add_library(GateAccountDomain STATIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/GateRouteProfileRegistrar.hpp
    domain/profiles/GateAccountProfile.cpp
    domain/modules/profile/ProfileRouteModule.cpp
    domain/modules/profile/ProfileRouteSchemas.cpp
)
memochat_configure_gate_app_lib(GateAccountDomain)
target_include_directories(GateAccountDomain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain)
target_link_libraries(GateAccountDomain PUBLIC GateRuntimeCore GateAccountCore GateAuthDomain)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateAccountDomain
        MODULES
            memochat.account.profile_route_registration_algorithms=domain/modules/profile/cxx_modules/ProfileRouteRegistration.cppm
            memochat.account.profile_route_schema_algorithms=domain/modules/profile/cxx_modules/ProfileRouteSchema.cppm
        CONSUMER_SOURCES
            domain/modules/profile/ProfileRouteModule.cpp
            domain/modules/profile/ProfileRouteSchemas.cpp
    )
endif()
