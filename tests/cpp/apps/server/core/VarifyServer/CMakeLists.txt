cmake_minimum_required(VERSION 3.16)

add_executable(varifyserver_gtest
    main.cpp
    VerifyCodePolicyTest.cpp
    EmailDeliveryTaskCodecTest.cpp
)

target_link_libraries(varifyserver_gtest PRIVATE
    GTest_TestLib
    VarifyServerCore
    memochat_server_deps
)

target_include_directories(varifyserver_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/VarifyServer
)

add_dependencies(gtest_tests varifyserver_gtest)

include(GoogleTest)
gtest_discover_tests(varifyserver_gtest TEST_DISCOVERY_MODE POST_BUILD)
