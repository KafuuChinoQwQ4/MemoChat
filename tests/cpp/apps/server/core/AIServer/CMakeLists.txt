cmake_minimum_required(VERSION 3.16)

add_executable(aiserver_gtest_json_mapper_test
    main.cpp
    AIServiceJsonMapperTest.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIServer/AIServiceJsonDtos.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIServer/AIServiceJsonMapper.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIServer/ConversationContext.cpp
)
target_link_libraries(aiserver_gtest_json_mapper_test PRIVATE
    GTest_TestLib
    memochat_proto
    memochat_server_deps
    memochat_boost_stubs
)
target_include_directories(aiserver_gtest_json_mapper_test PRIVATE
    ${MEMOCHAT_PROTO_GEN_DIR}
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIServer
)

add_dependencies(gtest_tests aiserver_gtest_json_mapper_test)

include(GoogleTest)
gtest_discover_tests(aiserver_gtest_json_mapper_test TEST_DISCOVERY_MODE POST_BUILD)
