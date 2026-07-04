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
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(aigateway_public_dtos_gtest
        MODULES
            memochat.ai.public_dto_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain/services/ai/cxx_modules/AIPublicDto.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain/services/ai/AIPublicDtos.cpp
    )
endif()

gtest_discover_tests(aigateway_public_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    add_executable(aigateway_route_module_algorithms_gtest
        main.cpp
        AIRouteModuleAlgorithmsConsumer.cpp
        AIRouteModuleAlgorithmsTest.cpp
    )

    target_link_libraries(aigateway_route_module_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    target_include_directories(aigateway_route_module_algorithms_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain
    )

    memochat_enable_gnu_modules(aigateway_route_module_algorithms_gtest
        MODULES
            memochat.ai.route_module_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain/modules/cxx_modules/AIRouteModule.cppm
        CONSUMER_SOURCES
            AIRouteModuleAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests aigateway_route_module_algorithms_gtest)

    gtest_discover_tests(aigateway_route_module_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(aigateway_route_registration_algorithms_gtest
        main.cpp
        AIRouteRegistrationAlgorithmsConsumer.cpp
        AIRouteRegistrationAlgorithmsTest.cpp
    )

    target_link_libraries(aigateway_route_registration_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    target_include_directories(aigateway_route_registration_algorithms_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain
    )

    memochat_enable_gnu_modules(aigateway_route_registration_algorithms_gtest
        MODULES
            memochat.ai.route_registration_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain/modules/cxx_modules/AIRouteRegistration.cppm
        CONSUMER_SOURCES
            AIRouteRegistrationAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests aigateway_route_registration_algorithms_gtest)

    gtest_discover_tests(aigateway_route_registration_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(aigateway_service_algorithms_gtest
        main.cpp
        AIServiceAlgorithmsConsumer.cpp
        AIServiceAlgorithmsTest.cpp
    )

    target_link_libraries(aigateway_service_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    target_include_directories(aigateway_service_algorithms_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain
    )

    memochat_enable_gnu_modules(aigateway_service_algorithms_gtest
        MODULES
            memochat.ai.gateway_service_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain/services/ai/cxx_modules/AIService.cppm
        CONSUMER_SOURCES
            AIServiceAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests aigateway_service_algorithms_gtest)

    gtest_discover_tests(aigateway_service_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(aigateway_runtime_algorithms_gtest
        ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/app/AIGatewayRuntime.cpp
        main.cpp
        AIGatewayRuntimeAlgorithmsTest.cpp
    )

    target_link_libraries(aigateway_runtime_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    target_include_directories(aigateway_runtime_algorithms_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/app
    )

    memochat_enable_gnu_modules(aigateway_runtime_algorithms_gtest
        MODULES
            memochat.ai.gateway_runtime_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/app/cxx_modules/AIGatewayRuntime.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/app/AIGatewayRuntime.cpp
    )

    add_dependencies(gtest_tests aigateway_runtime_algorithms_gtest)

    gtest_discover_tests(aigateway_runtime_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(aigateway_client_algorithms_gtest
        main.cpp
        AIGatewayClientAlgorithmsConsumer.cpp
        AIGatewayClientAlgorithmsTest.cpp
    )

    target_link_libraries(aigateway_client_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    target_include_directories(aigateway_client_algorithms_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain
    )

    memochat_enable_gnu_modules(aigateway_client_algorithms_gtest
        MODULES
            memochat.ai.gateway_client_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain/modules/cxx_modules/AIGatewayClient.cppm
        CONSUMER_SOURCES
            AIGatewayClientAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests aigateway_client_algorithms_gtest)

    gtest_discover_tests(aigateway_client_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
endif()

add_executable(aigateway_route_schema_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain/modules/ai/AIRouteSchemas.cpp
    main.cpp
    AIRouteSchemaAlgorithmsConsumer.cpp
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

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(aigateway_route_schema_gtest
        MODULES
            memochat.ai.route_schema_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain/modules/cxx_modules/AIRouteSchema.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain/modules/ai/AIRouteSchemas.cpp
            AIRouteSchemaAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(aigateway_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)
