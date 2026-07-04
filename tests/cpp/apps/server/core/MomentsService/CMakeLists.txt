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

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(momentsservice_output_dtos_gtest
        MODULES
            memochat.moments.output_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/services/moments/cxx_modules/MomentsOutput.cppm
            memochat.moments.public_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/services/moments/cxx_modules/MomentsPublic.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/services/moments/MomentsOutputDtos.cpp
            ${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/services/moments/MomentsPublicDtos.cpp
    )
endif()

add_dependencies(gtest_tests momentsservice_output_dtos_gtest)

include(GoogleTest)
gtest_discover_tests(momentsservice_output_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(momentsservice_route_schema_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/modules/moments/MomentsRouteSchemas.cpp
    main.cpp
    MomentsRouteSchemaAlgorithmsConsumer.cpp
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

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(momentsservice_route_schema_gtest
        MODULES
            memochat.moments.route_schema_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/modules/moments/cxx_modules/MomentsRouteSchema.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/modules/moments/MomentsRouteSchemas.cpp
            MomentsRouteSchemaAlgorithmsConsumer.cpp
    )
endif()

add_dependencies(gtest_tests momentsservice_route_schema_gtest)

gtest_discover_tests(momentsservice_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    add_executable(momentsservice_route_registration_algorithms_gtest
        main.cpp
        MomentsRouteRegistrationAlgorithmsConsumer.cpp
        MomentsRouteRegistrationAlgorithmsTest.cpp
    )

    target_link_libraries(momentsservice_route_registration_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    target_include_directories(momentsservice_route_registration_algorithms_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain
    )

    memochat_enable_gnu_modules(momentsservice_route_registration_algorithms_gtest
        MODULES
            memochat.moments.route_registration_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/modules/moments/cxx_modules/MomentsRouteRegistration.cppm
        CONSUMER_SOURCES
            MomentsRouteRegistrationAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests momentsservice_route_registration_algorithms_gtest)

    gtest_discover_tests(momentsservice_route_registration_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(momentsservice_service_algorithms_gtest
        main.cpp
        MomentsServiceAlgorithmsConsumer.cpp
        MomentsServiceAlgorithmsTest.cpp
    )

    target_link_libraries(momentsservice_service_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(momentsservice_service_algorithms_gtest
        MODULES
            memochat.moments.service_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/services/moments/cxx_modules/MomentsService.cppm
        CONSUMER_SOURCES
            MomentsServiceAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests momentsservice_service_algorithms_gtest)

    gtest_discover_tests(momentsservice_service_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(momentsservice_relation_client_algorithms_gtest
        main.cpp
        MomentsRelationClientAlgorithmsConsumer.cpp
        MomentsRelationClientAlgorithmsTest.cpp
    )

    target_link_libraries(momentsservice_relation_client_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(momentsservice_relation_client_algorithms_gtest
        MODULES
            memochat.moments.relation_client_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/clients/cxx_modules/MomentsRelationClient.cppm
        CONSUMER_SOURCES
            MomentsRelationClientAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests momentsservice_relation_client_algorithms_gtest)

    gtest_discover_tests(momentsservice_relation_client_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(momentsservice_gateway_runtime_algorithms_gtest
        main.cpp
        MomentsGatewayRuntimeAlgorithmsConsumer.cpp
        MomentsGatewayRuntimeAlgorithmsTest.cpp
    )

    target_link_libraries(momentsservice_gateway_runtime_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(momentsservice_gateway_runtime_algorithms_gtest
        MODULES
            memochat.moments.gateway_runtime_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/app/cxx_modules/MomentsGatewayRuntime.cppm
        CONSUMER_SOURCES
            MomentsGatewayRuntimeAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests momentsservice_gateway_runtime_algorithms_gtest)

    gtest_discover_tests(momentsservice_gateway_runtime_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
endif()
