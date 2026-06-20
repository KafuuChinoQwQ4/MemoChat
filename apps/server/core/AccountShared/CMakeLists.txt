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
# AuthLoginSupport.h).

# --- GateAccountCore: memo_account aggregate logic above persistence (auth
# cache, login support, profile support, async side-effects). ------------------
add_library(GateAccountCore STATIC
    core/cache/AuthCache.cpp
    core/cache/AuthCache.h
    core/support/AuthLoginCacheProfileDto.cpp
    core/support/AuthLoginCacheProfileDto.h
    core/support/AuthLoginSupport.cpp
    core/support/AuthLoginSupport.h
    core/support/AuthPublicDtos.cpp
    core/support/AuthPublicDtos.h
    core/support/AuthUserInfo.h
    core/support/Http2ProfileSupport.cpp
    core/support/Http2ProfileSupport.h
    core/async/GateAsyncSideEffectDtos.cpp
    core/async/GateAsyncSideEffectDtos.h
    core/async/GateAsyncSideEffects.cpp
    core/async/GateAsyncSideEffects.h
)
memochat_configure_gate_core_lib(GateAccountCore)
target_include_directories(GateAccountCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cache
    ${CMAKE_CURRENT_SOURCE_DIR}/core/support
    ${CMAKE_CURRENT_SOURCE_DIR}/core/async
)
target_link_libraries(GateAccountCore PUBLIC GateInfraCore)

# --- GateAuthDomain: register/login routes + auth service + account persistence.
add_library(GateAuthDomain STATIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/GateRouteProfileRegistrar.h
    domain/profiles/GateAuthProfile.cpp
    domain/modules/auth/AuthRouteModule.cpp
    domain/modules/auth/AuthRouteSchemas.cpp
    domain/services/account/AccountPersistence.cpp
    domain/services/auth/AuthService.cpp
)
memochat_configure_gate_app_lib(GateAuthDomain)
target_include_directories(GateAuthDomain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain)
target_link_libraries(GateAuthDomain PUBLIC GateRuntimeCore GateAccountCore)

# --- GateAccountDomain: account self-service profile routes. ------------------
add_library(GateAccountDomain STATIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/GateRouteProfileRegistrar.h
    domain/profiles/GateAccountProfile.cpp
    domain/modules/profile/ProfileRouteModule.cpp
    domain/modules/profile/ProfileRouteSchemas.cpp
)
memochat_configure_gate_app_lib(GateAccountDomain)
target_include_directories(GateAccountDomain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain)
target_link_libraries(GateAccountDomain PUBLIC GateRuntimeCore GateAccountCore GateAuthDomain)
