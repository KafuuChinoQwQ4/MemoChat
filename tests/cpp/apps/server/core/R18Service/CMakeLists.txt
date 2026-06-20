cmake_minimum_required(VERSION 3.16)

add_executable(r18service_source_record_codec_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services/r18/R18SourceRecordCodec.cpp
    main.cpp
    R18SourceRecordCodecTest.cpp
)

target_link_libraries(r18service_source_record_codec_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(r18service_source_record_codec_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services
)

add_dependencies(gtest_tests r18service_source_record_codec_gtest)

include(GoogleTest)
gtest_discover_tests(r18service_source_record_codec_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(r18service_public_dtos_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services/r18/R18PublicDtos.cpp
    main.cpp
    R18PublicDtosTest.cpp
)

target_link_libraries(r18service_public_dtos_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(r18service_public_dtos_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services
)

add_dependencies(gtest_tests r18service_public_dtos_gtest)

gtest_discover_tests(r18service_public_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(r18service_route_schema_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/modules/r18/R18RouteSchemas.cpp
    main.cpp
    R18RouteSchemaTest.cpp
)

target_link_libraries(r18service_route_schema_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(r18service_route_schema_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared
    ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services
)

add_dependencies(gtest_tests r18service_route_schema_gtest)

gtest_discover_tests(r18service_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)
