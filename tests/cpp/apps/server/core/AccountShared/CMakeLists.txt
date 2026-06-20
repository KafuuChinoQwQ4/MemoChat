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

add_dependencies(gtest_tests accountshared_auth_login_cache_profile_gtest)

include(GoogleTest)
gtest_discover_tests(accountshared_auth_login_cache_profile_gtest TEST_DISCOVERY_MODE POST_BUILD)

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

add_dependencies(gtest_tests accountshared_auth_public_dtos_gtest)

gtest_discover_tests(accountshared_auth_public_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

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

add_executable(accountshared_profile_route_schema_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/profile/ProfileRouteSchemas.cpp
    main.cpp
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

add_dependencies(gtest_tests accountshared_profile_route_schema_gtest)

gtest_discover_tests(accountshared_profile_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(accountshared_auth_route_schema_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/auth/AuthRouteSchemas.cpp
    main.cpp
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

add_dependencies(gtest_tests accountshared_auth_route_schema_gtest)

gtest_discover_tests(accountshared_auth_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)
