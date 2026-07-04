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

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(mediaservice_upload_session_dto_gtest
        MODULES
            memochat.media.upload_session_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/services/media/cxx_modules/MediaUploadSession.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/services/media/MediaUploadSessionDto.cpp
    )
endif()

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

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(mediaservice_public_dtos_gtest
        MODULES
            memochat.media.public_dto_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/services/media/cxx_modules/MediaPublicDto.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/services/media/MediaPublicDtos.cpp
    )
endif()

add_dependencies(gtest_tests mediaservice_public_dtos_gtest)

gtest_discover_tests(mediaservice_public_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(mediaservice_route_schema_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/modules/media/MediaRouteSchemas.cpp
    main.cpp
    MediaRouteSchemaAlgorithmsConsumer.cpp
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

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(mediaservice_route_schema_gtest
        MODULES
            memochat.media.route_schema_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/modules/media/cxx_modules/MediaRouteSchema.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/modules/media/MediaRouteSchemas.cpp
            MediaRouteSchemaAlgorithmsConsumer.cpp
    )
endif()

add_dependencies(gtest_tests mediaservice_route_schema_gtest)

gtest_discover_tests(mediaservice_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    add_executable(mediaservice_route_registration_algorithms_gtest
        main.cpp
        MediaRouteRegistrationAlgorithmsConsumer.cpp
        MediaRouteRegistrationAlgorithmsTest.cpp
    )

    target_link_libraries(mediaservice_route_registration_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    target_include_directories(mediaservice_route_registration_algorithms_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain
    )

    memochat_enable_gnu_modules(mediaservice_route_registration_algorithms_gtest
        MODULES
            memochat.media.route_registration_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/modules/media/cxx_modules/MediaRouteRegistration.cppm
        CONSUMER_SOURCES
            MediaRouteRegistrationAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests mediaservice_route_registration_algorithms_gtest)

    gtest_discover_tests(mediaservice_route_registration_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(mediaservice_service_algorithms_gtest
        main.cpp
        MediaServiceAlgorithmsConsumer.cpp
        MediaServiceAlgorithmsTest.cpp
    )

    target_link_libraries(mediaservice_service_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(mediaservice_service_algorithms_gtest
        MODULES
            memochat.media.service_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/services/media/cxx_modules/MediaService.cppm
        CONSUMER_SOURCES
            MediaServiceAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests mediaservice_service_algorithms_gtest)

    gtest_discover_tests(mediaservice_service_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(mediaservice_s3_storage_algorithms_gtest
        main.cpp
        S3MediaStorageAlgorithmsConsumer.cpp
        S3MediaStorageAlgorithmsTest.cpp
    )

    target_link_libraries(mediaservice_s3_storage_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(mediaservice_s3_storage_algorithms_gtest
        MODULES
            memochat.media.s3_storage_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/core/storage/cxx_modules/S3MediaStorage.cppm
        CONSUMER_SOURCES
            S3MediaStorageAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests mediaservice_s3_storage_algorithms_gtest)

    gtest_discover_tests(mediaservice_s3_storage_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(mediaservice_local_storage_algorithms_gtest
        main.cpp
        LocalMediaStorageAlgorithmsConsumer.cpp
        LocalMediaStorageAlgorithmsTest.cpp
    )

    target_link_libraries(mediaservice_local_storage_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(mediaservice_local_storage_algorithms_gtest
        MODULES
            memochat.media.local_storage_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/core/storage/cxx_modules/LocalMediaStorage.cppm
        CONSUMER_SOURCES
            LocalMediaStorageAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests mediaservice_local_storage_algorithms_gtest)

    gtest_discover_tests(mediaservice_local_storage_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
endif()

add_executable(mediaservice_storage_gtest
    main.cpp
    MediaStorageTest.cpp
)

target_link_libraries(mediaservice_storage_gtest PRIVATE
    GTest_TestLib
    GateMediaCore
    memochat_server_deps
)

target_include_directories(mediaservice_storage_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/core/storage
    ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain
)

add_dependencies(gtest_tests mediaservice_storage_gtest)

gtest_discover_tests(mediaservice_storage_gtest TEST_DISCOVERY_MODE POST_BUILD)
