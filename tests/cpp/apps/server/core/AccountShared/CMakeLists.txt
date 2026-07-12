cmake_minimum_required(VERSION 3.16)

add_executable(accountshared_auth_login_cache_profile_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/support/AuthLoginCacheProfileDto.cpp
    main.cpp
    AuthLoginCacheProfileDtoTest.cpp
)

target_link_libraries(accountshared_auth_login_cache_profile_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(accountshared_auth_login_cache_profile_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/support
)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(accountshared_auth_login_cache_profile_gtest
        MODULES
            memochat.account.auth_login_cache_profile_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/support/cxx_modules/AuthLoginCacheProfile.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/support/AuthLoginCacheProfileDto.cpp
    )
endif()

add_dependencies(gtest_tests accountshared_auth_login_cache_profile_gtest)

include(GoogleTest)
gtest_discover_tests(accountshared_auth_login_cache_profile_gtest TEST_DISCOVERY_MODE POST_BUILD)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    add_executable(accountshared_auth_cache_algorithms_gtest
        main.cpp
        AuthCacheAlgorithmsConsumer.cpp
        AuthCacheAlgorithmsTest.cpp
    )

    target_link_libraries(accountshared_auth_cache_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(accountshared_auth_cache_algorithms_gtest
        MODULES
            memochat.account.auth_cache_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/cache/cxx_modules/AuthCache.cppm
        CONSUMER_SOURCES
            AuthCacheAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests accountshared_auth_cache_algorithms_gtest)

    gtest_discover_tests(accountshared_auth_cache_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
endif()

add_executable(accountshared_auth_public_dtos_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/support/AuthPublicDtos.cpp
    main.cpp
    AuthPublicDtosTest.cpp
)

target_link_libraries(accountshared_auth_public_dtos_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(accountshared_auth_public_dtos_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/support
)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(accountshared_auth_public_dtos_gtest
        MODULES
            memochat.account.auth_public_dto_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/support/cxx_modules/AuthPublicDto.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/support/AuthPublicDtos.cpp
    )
endif()

add_dependencies(gtest_tests accountshared_auth_public_dtos_gtest)

gtest_discover_tests(accountshared_auth_public_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(accountshared_refresh_token_security_gtest
    main.cpp
    RefreshTokenSecurityTest.cpp
)

target_link_libraries(accountshared_refresh_token_security_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

add_dependencies(gtest_tests accountshared_refresh_token_security_gtest)

gtest_discover_tests(accountshared_refresh_token_security_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(accountshared_gate_async_side_effect_dtos_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/async/GateAsyncSideEffectDtos.cpp
    main.cpp
    GateAsyncSideEffectDtosTest.cpp
)

target_link_libraries(accountshared_gate_async_side_effect_dtos_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(accountshared_gate_async_side_effect_dtos_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/async
)

add_dependencies(gtest_tests accountshared_gate_async_side_effect_dtos_gtest)

gtest_discover_tests(accountshared_gate_async_side_effect_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(accountshared_auth_login_support_gtest
    main.cpp
    AuthLoginSupportTest.cpp
)

target_link_libraries(accountshared_auth_login_support_gtest PRIVATE
    GTest_TestLib
    GateAccountCore
)

add_dependencies(gtest_tests accountshared_auth_login_support_gtest)

gtest_discover_tests(accountshared_auth_login_support_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(accountshared_auth_local_fallback_counter_store_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/support/AuthLocalFallbackCounterStore.cpp
    main.cpp
    AuthLocalFallbackCounterStoreTest.cpp
)

target_link_libraries(accountshared_auth_local_fallback_counter_store_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(accountshared_auth_local_fallback_counter_store_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/support
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
)

add_dependencies(gtest_tests accountshared_auth_local_fallback_counter_store_gtest)

gtest_discover_tests(accountshared_auth_local_fallback_counter_store_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(accountshared_profile_route_schema_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/profile/ProfileRouteSchemas.cpp
    main.cpp
    ProfileRouteSchemaAlgorithmsConsumer.cpp
    ProfileRouteSchemaTest.cpp
)

target_link_libraries(accountshared_profile_route_schema_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(accountshared_profile_route_schema_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/support
)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(accountshared_profile_route_schema_gtest
        MODULES
            memochat.account.profile_route_schema_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/profile/cxx_modules/ProfileRouteSchema.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/profile/ProfileRouteSchemas.cpp
            ProfileRouteSchemaAlgorithmsConsumer.cpp
    )
endif()

add_dependencies(gtest_tests accountshared_profile_route_schema_gtest)

gtest_discover_tests(accountshared_profile_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    add_executable(accountshared_profile_route_registration_algorithms_gtest
        main.cpp
        ProfileRouteRegistrationAlgorithmsConsumer.cpp
        ProfileRouteRegistrationAlgorithmsTest.cpp
    )

    target_link_libraries(accountshared_profile_route_registration_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    target_include_directories(accountshared_profile_route_registration_algorithms_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain
    )

    memochat_enable_gnu_modules(accountshared_profile_route_registration_algorithms_gtest
        MODULES
            memochat.account.profile_route_registration_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/profile/cxx_modules/ProfileRouteRegistration.cppm
        CONSUMER_SOURCES
            ProfileRouteRegistrationAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests accountshared_profile_route_registration_algorithms_gtest)

    gtest_discover_tests(accountshared_profile_route_registration_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
endif()

add_executable(accountshared_auth_route_schema_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/auth/AuthRouteSchemas.cpp
    main.cpp
    AuthRouteSchemaAlgorithmsConsumer.cpp
    AuthRouteSchemaTest.cpp
)

target_link_libraries(accountshared_auth_route_schema_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(accountshared_auth_route_schema_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/support
)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(accountshared_auth_route_schema_gtest
        MODULES
            memochat.account.auth_route_schema_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/auth/cxx_modules/AuthRouteSchema.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/auth/AuthRouteSchemas.cpp
            AuthRouteSchemaAlgorithmsConsumer.cpp
    )
endif()

add_dependencies(gtest_tests accountshared_auth_route_schema_gtest)

gtest_discover_tests(accountshared_auth_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    add_executable(accountshared_auth_route_registration_algorithms_gtest
        main.cpp
        AuthRouteRegistrationAlgorithmsConsumer.cpp
        AuthRouteRegistrationAlgorithmsTest.cpp
    )

    target_link_libraries(accountshared_auth_route_registration_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    target_include_directories(accountshared_auth_route_registration_algorithms_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain
    )

    memochat_enable_gnu_modules(accountshared_auth_route_registration_algorithms_gtest
        MODULES
            memochat.account.auth_route_registration_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/auth/cxx_modules/AuthRouteRegistration.cppm
        CONSUMER_SOURCES
            AuthRouteRegistrationAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests accountshared_auth_route_registration_algorithms_gtest)

    gtest_discover_tests(accountshared_auth_route_registration_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
endif()

if(MEMOCHAT_ENABLE_GNU_MODULES)
    add_executable(accountshared_auth_service_algorithms_gtest
        main.cpp
        AuthServiceAlgorithmsConsumer.cpp
        AuthServiceAlgorithmsTest.cpp
    )

    target_link_libraries(accountshared_auth_service_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(accountshared_auth_service_algorithms_gtest
        MODULES
            memochat.account.auth_service_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/services/auth/cxx_modules/AuthService.cppm
        CONSUMER_SOURCES
            AuthServiceAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests accountshared_auth_service_algorithms_gtest)

    gtest_discover_tests(accountshared_auth_service_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
endif()

if(MEMOCHAT_ENABLE_GNU_MODULES)
    add_executable(accountshared_profile_support_algorithms_gtest
        main.cpp
        ProfileSupportAlgorithmsConsumer.cpp
        ProfileSupportAlgorithmsTest.cpp
    )

    target_link_libraries(accountshared_profile_support_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(accountshared_profile_support_algorithms_gtest
        MODULES
            memochat.account.profile_support_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/support/cxx_modules/ProfileSupport.cppm
        CONSUMER_SOURCES
            ProfileSupportAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests accountshared_profile_support_algorithms_gtest)

    gtest_discover_tests(accountshared_profile_support_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
endif()

if(MEMOCHAT_ENABLE_GNU_MODULES)
    add_executable(accountshared_async_side_effect_algorithms_gtest
        main.cpp
        GateAsyncSideEffectAlgorithmsConsumer.cpp
        GateAsyncSideEffectAlgorithmsTest.cpp
    )

    target_link_libraries(accountshared_async_side_effect_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(accountshared_async_side_effect_algorithms_gtest
        MODULES
            memochat.account.async_side_effect_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/async/cxx_modules/GateAsyncSideEffect.cppm
        CONSUMER_SOURCES
            GateAsyncSideEffectAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests accountshared_async_side_effect_algorithms_gtest)

    gtest_discover_tests(accountshared_async_side_effect_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
endif()
