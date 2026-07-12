cmake_minimum_required(VERSION 3.16)

add_executable(common_random_uuid_gtest
    main.cpp
    UuidTest.cpp
)

target_link_libraries(common_random_uuid_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(common_random_uuid_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
)

add_dependencies(gtest_tests common_random_uuid_gtest)

include(GoogleTest)
gtest_discover_tests(common_random_uuid_gtest TEST_DISCOVERY_MODE POST_BUILD)
