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

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(r18service_source_record_codec_gtest
        MODULES
            memochat.r18.source_record_codec_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services/r18/cxx_modules/R18SourceRecordCodec.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services/r18/R18SourceRecordCodec.cpp
    )
endif()

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

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(r18service_public_dtos_gtest
        MODULES
            memochat.r18.public_dto_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services/r18/cxx_modules/R18PublicDto.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services/r18/R18PublicDtos.cpp
    )
endif()

add_dependencies(gtest_tests r18service_public_dtos_gtest)

gtest_discover_tests(r18service_public_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(r18service_route_schema_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/modules/r18/R18RouteSchemas.cpp
    main.cpp
    R18RouteSchemaAlgorithmsConsumer.cpp
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

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(r18service_route_schema_gtest
        MODULES
            memochat.r18.route_schema_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/modules/r18/cxx_modules/R18RouteSchema.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/modules/r18/R18RouteSchemas.cpp
            R18RouteSchemaAlgorithmsConsumer.cpp
    )
endif()

add_dependencies(gtest_tests r18service_route_schema_gtest)

gtest_discover_tests(r18service_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    add_executable(r18service_adapter_utils_algorithms_gtest
        main.cpp
        R18AdapterUtilsAlgorithmsConsumer.cpp
        R18AdapterUtilsAlgorithmsTest.cpp
    )

    target_link_libraries(r18service_adapter_utils_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(r18service_adapter_utils_algorithms_gtest
        MODULES
            memochat.r18.adapter_utils_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services/r18/cxx_modules/R18AdapterUtils.cppm
        CONSUMER_SOURCES
            R18AdapterUtilsAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests r18service_adapter_utils_algorithms_gtest)

    gtest_discover_tests(r18service_adapter_utils_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(r18service_jm_adapter_algorithms_gtest
        main.cpp
        R18JmAdapterAlgorithmsConsumer.cpp
        R18JmAdapterAlgorithmsTest.cpp
    )

    target_link_libraries(r18service_jm_adapter_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(r18service_jm_adapter_algorithms_gtest
        MODULES
            memochat.r18.jm_adapter_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services/r18/cxx_modules/R18JmAdapter.cppm
        CONSUMER_SOURCES
            R18JmAdapterAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests r18service_jm_adapter_algorithms_gtest)

    gtest_discover_tests(r18service_jm_adapter_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(r18service_picacg_adapter_algorithms_gtest
        main.cpp
        R18PicacgAdapterAlgorithmsConsumer.cpp
        R18PicacgAdapterAlgorithmsTest.cpp
    )

    target_link_libraries(r18service_picacg_adapter_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(r18service_picacg_adapter_algorithms_gtest
        MODULES
            memochat.r18.picacg_adapter_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services/r18/cxx_modules/R18PicacgAdapter.cppm
        CONSUMER_SOURCES
            R18PicacgAdapterAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests r18service_picacg_adapter_algorithms_gtest)

    gtest_discover_tests(r18service_picacg_adapter_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(r18service_route_registration_algorithms_gtest
        main.cpp
        R18RouteRegistrationAlgorithmsConsumer.cpp
        R18RouteRegistrationAlgorithmsTest.cpp
    )

    target_link_libraries(r18service_route_registration_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    target_include_directories(r18service_route_registration_algorithms_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain
    )

    memochat_enable_gnu_modules(r18service_route_registration_algorithms_gtest
        MODULES
            memochat.r18.route_registration_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/modules/r18/cxx_modules/R18RouteRegistration.cppm
        CONSUMER_SOURCES
            R18RouteRegistrationAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests r18service_route_registration_algorithms_gtest)

    gtest_discover_tests(r18service_route_registration_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(r18service_service_algorithms_gtest
        main.cpp
        R18ServiceAlgorithmsConsumer.cpp
        R18ServiceAlgorithmsTest.cpp
    )

    target_link_libraries(r18service_service_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(r18service_service_algorithms_gtest
        MODULES
            memochat.r18.service_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services/r18/cxx_modules/R18Service.cppm
        CONSUMER_SOURCES
            R18ServiceAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests r18service_service_algorithms_gtest)

    gtest_discover_tests(r18service_service_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(r18service_source_service_algorithms_gtest
        main.cpp
        R18SourceServiceAlgorithmsConsumer.cpp
        R18SourceServiceAlgorithmsTest.cpp
    )

    target_link_libraries(r18service_source_service_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(r18service_source_service_algorithms_gtest
        MODULES
            memochat.r18.source_service_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services/r18/cxx_modules/R18SourceService.cppm
        CONSUMER_SOURCES
            R18SourceServiceAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests r18service_source_service_algorithms_gtest)

    gtest_discover_tests(r18service_source_service_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
endif()

add_executable(r18service_source_service_gtest
    main.cpp
    R18SourceServiceTest.cpp
)

target_link_libraries(r18service_source_service_gtest PRIVATE
    GTest_TestLib
    GateR18Core
    memochat_server_deps
)

target_include_directories(r18service_source_service_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/services
)

add_dependencies(gtest_tests r18service_source_service_gtest)

gtest_discover_tests(r18service_source_service_gtest TEST_DISCOVERY_MODE POST_BUILD)
