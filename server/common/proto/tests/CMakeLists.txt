cmake_minimum_required(VERSION 3.16)

# Test executable for memochat_proto round-trip serialization.
# Depends on the generated protobuf headers (MEMOCHAT_PROTO_GEN_DIR).
# The parent server/CMakeLists.txt already defined those variables,
# so we reference them directly here.

add_executable(proto_gtest_proto_test
    main.cpp
    ProtoMessageTest.cpp
)
target_link_libraries(proto_gtest_proto_test PRIVATE
    GTest_TestLib
    memochat_proto
    memochat_server_deps
)
target_include_directories(proto_gtest_proto_test PRIVATE
    ${MEMOCHAT_PROTO_GEN_DIR}          # generated protobuf headers
    ${CMAKE_CURRENT_SOURCE_DIR}/..    # server/common (for other includes)
)

# Make the aggregate gtest_tests target depend on this executable.
add_dependencies(gtest_tests proto_gtest_proto_test)

include(GoogleTest)
gtest_discover_tests(proto_gtest_proto_test TEST_DISCOVERY_MODE POST_BUILD)
