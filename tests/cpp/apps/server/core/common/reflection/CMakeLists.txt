cmake_minimum_required(VERSION 3.16)

add_executable(common_reflection_gtest
    main.cpp
    StdReflectionIntrospectionTest.cpp
)

target_link_libraries(common_reflection_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(common_reflection_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
    ${CMAKE_SOURCE_DIR}/apps/server/core/VarifyServer
)

add_dependencies(gtest_tests common_reflection_gtest)

include(GoogleTest)
gtest_discover_tests(common_reflection_gtest TEST_DISCOVERY_MODE POST_BUILD)

