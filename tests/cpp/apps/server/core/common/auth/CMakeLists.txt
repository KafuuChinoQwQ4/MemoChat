cmake_minimum_required(VERSION 3.16)

add_executable(common_auth_jwt_access_token_gtest
    main.cpp
    JwtAccessTokenTest.cpp
)

target_link_libraries(common_auth_jwt_access_token_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(common_auth_jwt_access_token_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
)

add_dependencies(gtest_tests common_auth_jwt_access_token_gtest)

include(GoogleTest)
gtest_discover_tests(common_auth_jwt_access_token_gtest TEST_DISCOVERY_MODE POST_BUILD)
