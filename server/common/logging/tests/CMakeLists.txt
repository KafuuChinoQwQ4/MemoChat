cmake_minimum_required(VERSION 3.16)

# Test executable for memochat_logging JSON output and structured fields.
# Does NOT initialize a real file logger — captures log output via a
# temporary spdlog sink registered in-memory for the duration of each test.

add_executable(logging_gtest_json_logger_test
    main.cpp
    JsonLoggerTest.cpp
)
target_link_libraries(logging_gtest_json_logger_test PRIVATE
    GTest_TestLib
    memochat_logging
    memochat_server_deps
)
target_include_directories(logging_gtest_json_logger_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..    # server/common (for logging/*)
)

add_dependencies(gtest_tests logging_gtest_json_logger_test)

include(GoogleTest)
gtest_discover_tests(logging_gtest_json_logger_test TEST_DISCOVERY_MODE POST_BUILD)
