cmake_minimum_required(VERSION 3.16)

add_executable(mediaservice_upload_session_dto_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/services/media/MediaUploadSessionDto.cpp
    main.cpp
    MediaUploadSessionDtoTest.cpp
)

target_link_libraries(mediaservice_upload_session_dto_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(mediaservice_upload_session_dto_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain
)

add_dependencies(gtest_tests mediaservice_upload_session_dto_gtest)

include(GoogleTest)
gtest_discover_tests(mediaservice_upload_session_dto_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(mediaservice_public_dtos_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/services/media/MediaPublicDtos.cpp
    main.cpp
    MediaPublicDtosTest.cpp
)

target_link_libraries(mediaservice_public_dtos_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(mediaservice_public_dtos_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain
)

add_dependencies(gtest_tests mediaservice_public_dtos_gtest)

gtest_discover_tests(mediaservice_public_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(mediaservice_route_schema_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/modules/media/MediaRouteSchemas.cpp
    main.cpp
    MediaRouteSchemaTest.cpp
)

target_link_libraries(mediaservice_route_schema_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(mediaservice_route_schema_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared
    ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain
)

add_dependencies(gtest_tests mediaservice_route_schema_gtest)

gtest_discover_tests(mediaservice_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)
