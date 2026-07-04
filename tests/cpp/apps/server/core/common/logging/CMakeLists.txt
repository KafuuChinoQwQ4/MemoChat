cmake_minimum_required(VERSION 3.16)

# Test executable for memochat_logging JSON output and structured fields.
# Does NOT initialize a real file logger — captures log output via a
# temporary spdlog sink registered in-memory for the duration of each test.

add_executable(logging_gtest_json_logger_test
    main.cpp
    GrpcTraceAlgorithmsConsumer.cpp
    JsonLoggerTest.cpp
    LoggerAlgorithmsConsumer.cpp
    RedactionAlgorithmsConsumer.cpp
    TelemetryAlgorithmsConsumer.cpp
)
target_link_libraries(logging_gtest_json_logger_test PRIVATE
    GTest_TestLib
    memochat_logging
    memochat_server_deps
)
target_include_directories(logging_gtest_json_logger_test PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/common    # server/common (for logging/*)
)

add_dependencies(gtest_tests logging_gtest_json_logger_test)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(logging_gtest_json_logger_test
        MODULES
            memochat.logging.grpc_trace_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/common/logging/cxx_modules/GrpcTrace.cppm
            memochat.logging.logger_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/common/logging/cxx_modules/Logger.cppm
            memochat.logging.redaction_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/common/logging/cxx_modules/Redaction.cppm
            memochat.logging.telemetry_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/common/logging/cxx_modules/Telemetry.cppm
        CONSUMER_SOURCES
            GrpcTraceAlgorithmsConsumer.cpp
            LoggerAlgorithmsConsumer.cpp
            RedactionAlgorithmsConsumer.cpp
            TelemetryAlgorithmsConsumer.cpp
    )
endif()

add_executable(logging_config_gtest
    main.cpp
    LoggingConfigTest.cpp
)
target_link_libraries(logging_config_gtest PRIVATE
    GTest_TestLib
    memochat_logging
    memochat_server_deps
)
target_include_directories(logging_config_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
)

add_dependencies(gtest_tests logging_config_gtest)

add_executable(logging_log_config_algorithms_gtest
    main.cpp
    LogConfigAlgorithmsConsumer.cpp
    LogConfigAlgorithmsTest.cpp
)
target_link_libraries(logging_log_config_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)
target_include_directories(logging_log_config_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(logging_log_config_algorithms_gtest
        MODULES
            memochat.logging.log_config_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/common/logging/cxx_modules/LogConfig.cppm
        CONSUMER_SOURCES
            LogConfigAlgorithmsConsumer.cpp
    )
endif()
add_dependencies(gtest_tests logging_log_config_algorithms_gtest)

add_executable(logging_telemetry_config_algorithms_gtest
    main.cpp
    TelemetryConfigAlgorithmsConsumer.cpp
    TelemetryConfigAlgorithmsTest.cpp
)
target_link_libraries(logging_telemetry_config_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)
target_include_directories(logging_telemetry_config_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(logging_telemetry_config_algorithms_gtest
        MODULES
            memochat.logging.telemetry_config_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/common/logging/cxx_modules/TelemetryConfig.cppm
        CONSUMER_SOURCES
            TelemetryConfigAlgorithmsConsumer.cpp
    )
endif()
add_dependencies(gtest_tests logging_telemetry_config_algorithms_gtest)

include(GoogleTest)
gtest_discover_tests(logging_gtest_json_logger_test TEST_DISCOVERY_MODE POST_BUILD)
gtest_discover_tests(logging_config_gtest TEST_DISCOVERY_MODE POST_BUILD)
gtest_discover_tests(logging_log_config_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
gtest_discover_tests(logging_telemetry_config_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
