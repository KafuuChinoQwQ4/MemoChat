cmake_minimum_required(VERSION 3.16)

add_executable(common_db_no_exceptions_gtest
    main.cpp
    NoExceptionsDbAdaptersTest.cpp
)

target_link_libraries(common_db_no_exceptions_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
    PostgreSQL::PostgreSQL
    mongo::mongoc_static
)

target_include_directories(common_db_no_exceptions_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
)

add_dependencies(gtest_tests common_db_no_exceptions_gtest)

include(GoogleTest)
gtest_discover_tests(common_db_no_exceptions_gtest TEST_DISCOVERY_MODE POST_BUILD)
