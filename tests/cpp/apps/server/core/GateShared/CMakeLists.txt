cmake_minimum_required(VERSION 3.16)

add_executable(gateshared_route_schema_gtest
    main.cpp
    RouteSchemaTest.cpp
)

target_link_libraries(gateshared_route_schema_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(gateshared_route_schema_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
)

add_dependencies(gtest_tests gateshared_route_schema_gtest)

add_executable(gateshared_route_schema_catalog_gtest
    main.cpp
    RouteSchemaCatalogTest.cpp
)

target_link_libraries(gateshared_route_schema_catalog_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(gateshared_route_schema_catalog_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
)

add_dependencies(gtest_tests gateshared_route_schema_catalog_gtest)

add_executable(gateshared_cross_module_route_schema_catalog_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/auth/AuthRouteSchemas.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/profile/ProfileRouteSchemas.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/modules/media/MediaRouteSchemas.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/domain/modules/call/CallRouteSchemas.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/modules/r18/R18RouteSchemas.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain/modules/ai/AIRouteSchemas.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/modules/moments/MomentsRouteSchemas.cpp
    main.cpp
    CrossModuleRouteSchemaCatalogTest.cpp
)

target_link_libraries(gateshared_cross_module_route_schema_catalog_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(gateshared_cross_module_route_schema_catalog_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/core/support
    ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/core/support
    ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain
)

add_dependencies(gtest_tests gateshared_cross_module_route_schema_catalog_gtest)

include(GoogleTest)
gtest_discover_tests(gateshared_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)
gtest_discover_tests(gateshared_route_schema_catalog_gtest TEST_DISCOVERY_MODE POST_BUILD)
gtest_discover_tests(gateshared_cross_module_route_schema_catalog_gtest TEST_DISCOVERY_MODE POST_BUILD)
