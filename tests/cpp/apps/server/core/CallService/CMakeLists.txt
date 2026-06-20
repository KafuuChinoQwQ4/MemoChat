cmake_minimum_required(VERSION 3.16)

add_executable(callservice_session_cache_dto_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/core/support/CallSessionCacheDto.cpp
    main.cpp
    CallSessionCacheDtoTest.cpp
)

target_link_libraries(callservice_session_cache_dto_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(callservice_session_cache_dto_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/core/support
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core/persistence
)

add_dependencies(gtest_tests callservice_session_cache_dto_gtest)

include(GoogleTest)
gtest_discover_tests(callservice_session_cache_dto_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(callservice_public_dtos_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/core/support/CallPublicDtos.cpp
    main.cpp
    CallPublicDtosTest.cpp
)

target_link_libraries(callservice_public_dtos_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(callservice_public_dtos_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/core/support
)

add_dependencies(gtest_tests callservice_public_dtos_gtest)

gtest_discover_tests(callservice_public_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(callservice_route_schema_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/domain/modules/call/CallRouteSchemas.cpp
    main.cpp
    CallRouteSchemaTest.cpp
)

target_link_libraries(callservice_route_schema_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(callservice_route_schema_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared
    ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/core/support
)

add_dependencies(gtest_tests callservice_route_schema_gtest)

gtest_discover_tests(callservice_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)
