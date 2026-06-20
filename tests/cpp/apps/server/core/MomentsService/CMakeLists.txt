cmake_minimum_required(VERSION 3.16)

add_executable(momentsservice_output_dtos_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/services/moments/MomentsOutputDtos.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/services/moments/MomentsPublicDtos.cpp
    main.cpp
    MomentsOutputDtosTest.cpp
)

target_link_libraries(momentsservice_output_dtos_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(momentsservice_output_dtos_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core
)

add_dependencies(gtest_tests momentsservice_output_dtos_gtest)

include(GoogleTest)
gtest_discover_tests(momentsservice_output_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(momentsservice_route_schema_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/modules/moments/MomentsRouteSchemas.cpp
    main.cpp
    MomentsRouteSchemaTest.cpp
)

target_link_libraries(momentsservice_route_schema_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(momentsservice_route_schema_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core
    ${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain
)

add_dependencies(gtest_tests momentsservice_route_schema_gtest)

gtest_discover_tests(momentsservice_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)
