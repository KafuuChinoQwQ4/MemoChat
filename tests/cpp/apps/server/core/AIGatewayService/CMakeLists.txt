cmake_minimum_required(VERSION 3.16)

add_executable(aigateway_public_dtos_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain/services/ai/AIPublicDtos.cpp
    main.cpp
    AIPublicDtosTest.cpp
)

target_link_libraries(aigateway_public_dtos_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(aigateway_public_dtos_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain
)

add_dependencies(gtest_tests aigateway_public_dtos_gtest)

include(GoogleTest)
gtest_discover_tests(aigateway_public_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(aigateway_route_schema_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain/modules/ai/AIRouteSchemas.cpp
    main.cpp
    AIRouteSchemaTest.cpp
)

target_link_libraries(aigateway_route_schema_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(aigateway_route_schema_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain
)

add_dependencies(gtest_tests aigateway_route_schema_gtest)

gtest_discover_tests(aigateway_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)
