cmake_minimum_required(VERSION 3.16)

add_executable(aiserver_gtest_json_mapper_test
    main.cpp
    AIServiceJsonMapperTest.cpp
    ../AIServiceJsonMapper.cpp
)
target_link_libraries(aiserver_gtest_json_mapper_test PRIVATE
    GTest_TestLib
    memochat_proto
    memochat_server_deps
)
target_include_directories(aiserver_gtest_json_mapper_test PRIVATE
    ${MEMOCHAT_PROTO_GEN_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(gtest_tests aiserver_gtest_json_mapper_test)

include(GoogleTest)
gtest_discover_tests(aiserver_gtest_json_mapper_test TEST_DISCOVERY_MODE POST_BUILD)
