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

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(callservice_session_cache_dto_gtest
        MODULES
            memochat.call.session_cache_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/CallService/core/support/cxx_modules/CallSessionCache.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/core/support/CallSessionCacheDto.cpp
    )
endif()

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

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(callservice_public_dtos_gtest
        MODULES
            memochat.call.public_dto_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/CallService/core/support/cxx_modules/CallPublicDto.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/core/support/CallPublicDtos.cpp
    )
endif()

add_dependencies(gtest_tests callservice_public_dtos_gtest)

gtest_discover_tests(callservice_public_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    add_executable(callservice_token_algorithms_gtest
        main.cpp
        CallTokenAlgorithmsConsumer.cpp
        CallTokenAlgorithmsTest.cpp
    )

    target_link_libraries(callservice_token_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(callservice_token_algorithms_gtest
        MODULES
            memochat.call.token_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/CallService/core/support/cxx_modules/CallToken.cppm
        CONSUMER_SOURCES
            CallTokenAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests callservice_token_algorithms_gtest)

    gtest_discover_tests(callservice_token_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(callservice_service_algorithms_gtest
        main.cpp
        CallServiceAlgorithmsConsumer.cpp
        CallServiceAlgorithmsTest.cpp
    )

    target_link_libraries(callservice_service_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(callservice_service_algorithms_gtest
        MODULES
            memochat.call.service_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/CallService/core/support/cxx_modules/CallService.cppm
        CONSUMER_SOURCES
            CallServiceAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests callservice_service_algorithms_gtest)

    gtest_discover_tests(callservice_service_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(callservice_route_module_algorithms_gtest
        main.cpp
        CallRouteModuleAlgorithmsConsumer.cpp
        CallRouteModuleAlgorithmsTest.cpp
    )

    target_link_libraries(callservice_route_module_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(callservice_route_module_algorithms_gtest
        MODULES
            memochat.call.route_module_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/CallService/domain/modules/call/cxx_modules/CallRouteModule.cppm
        CONSUMER_SOURCES
            CallRouteModuleAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests callservice_route_module_algorithms_gtest)

    gtest_discover_tests(callservice_route_module_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(callservice_session_math_algorithms_gtest
        main.cpp
        CallSessionMathAlgorithmsConsumer.cpp
        CallSessionMathAlgorithmsTest.cpp
    )

    target_link_libraries(callservice_session_math_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(callservice_session_math_algorithms_gtest
        MODULES
            memochat.call.session_math_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/CallService/core/support/cxx_modules/CallSessionMath.cppm
        CONSUMER_SOURCES
            CallSessionMathAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests callservice_session_math_algorithms_gtest)

    gtest_discover_tests(callservice_session_math_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(callservice_route_response_algorithms_gtest
        main.cpp
        CallRouteResponseAlgorithmsConsumer.cpp
        CallRouteResponseAlgorithmsTest.cpp
    )

    target_link_libraries(callservice_route_response_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(callservice_route_response_algorithms_gtest
        MODULES
            memochat.call.route_response_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/CallService/domain/modules/call/cxx_modules/CallRouteResponse.cppm
        CONSUMER_SOURCES
            CallRouteResponseAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests callservice_route_response_algorithms_gtest)

    gtest_discover_tests(callservice_route_response_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
endif()

add_executable(callservice_route_schema_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/domain/modules/call/CallRouteSchemas.cpp
    main.cpp
    CallRouteSchemaAlgorithmsConsumer.cpp
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

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(callservice_route_schema_gtest
        MODULES
            memochat.call.route_schema_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/CallService/domain/modules/call/cxx_modules/CallRouteSchema.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/domain/modules/call/CallRouteSchemas.cpp
            CallRouteSchemaAlgorithmsConsumer.cpp
    )
endif()

add_dependencies(gtest_tests callservice_route_schema_gtest)

gtest_discover_tests(callservice_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)
