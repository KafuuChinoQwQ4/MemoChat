cmake_minimum_required(VERSION 3.16)

add_executable(aiserver_gtest_json_mapper_test
    main.cpp
    AIServiceAlgorithmsTest.cpp
    AIServiceJsonMapperTest.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIServer/AIServiceAlgorithms.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIServer/AIServiceJsonDtos.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIServer/AIServiceJsonMapper.cpp
)
target_link_libraries(aiserver_gtest_json_mapper_test PRIVATE
    GTest_TestLib
    memochat_proto
    memochat_server_deps
    memochat_boost_stubs
)
target_include_directories(aiserver_gtest_json_mapper_test PRIVATE
    ${MEMOCHAT_PROTO_GEN_DIR}
    ${CMAKE_SOURCE_DIR}/apps/server/core/AIServer
)

add_dependencies(gtest_tests aiserver_gtest_json_mapper_test)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(aiserver_gtest_json_mapper_test
        MODULES
            memochat.ai.json_dto_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AIServer/cxx_modules/AIServiceJsonDto.cppm
            memochat.ai.service_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AIServer/cxx_modules/AIService.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/AIServer/AIServiceAlgorithms.cpp
            ${CMAKE_SOURCE_DIR}/apps/server/core/AIServer/AIServiceJsonDtos.cpp
    )
endif()

include(GoogleTest)
gtest_discover_tests(aiserver_gtest_json_mapper_test TEST_DISCOVERY_MODE POST_BUILD)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    add_executable(aiserver_client_algorithms_gtest
        main.cpp
        AIServiceClientAlgorithmsConsumer.cpp
        AIServiceClientAlgorithmsTest.cpp
    )

    target_link_libraries(aiserver_client_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(aiserver_client_algorithms_gtest
        MODULES
            memochat.ai.client_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AIServer/cxx_modules/AIServiceClient.cppm
        CONSUMER_SOURCES
            AIServiceClientAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests aiserver_client_algorithms_gtest)

    gtest_discover_tests(aiserver_client_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(aiserver_impl_algorithms_gtest
        main.cpp
        AIServiceImplAlgorithmsConsumer.cpp
        AIServiceImplAlgorithmsTest.cpp
    )

    target_link_libraries(aiserver_impl_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(aiserver_impl_algorithms_gtest
        MODULES
            memochat.ai.impl_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AIServer/cxx_modules/AIServiceImpl.cppm
        CONSUMER_SOURCES
            AIServiceImplAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests aiserver_impl_algorithms_gtest)

    gtest_discover_tests(aiserver_impl_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(aiserver_core_algorithms_gtest
        main.cpp
        AIServiceCoreAlgorithmsConsumer.cpp
        AIServiceCoreAlgorithmsTest.cpp
    )

    target_link_libraries(aiserver_core_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(aiserver_core_algorithms_gtest
        MODULES
            memochat.ai.core_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AIServer/cxx_modules/AIServiceCore.cppm
        CONSUMER_SOURCES
            AIServiceCoreAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests aiserver_core_algorithms_gtest)

    gtest_discover_tests(aiserver_core_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

    add_executable(aiserver_repository_algorithms_gtest
        main.cpp
        AIServerRepositoryAlgorithmsConsumer.cpp
        AIServerRepositoryAlgorithmsTest.cpp
    )

    target_link_libraries(aiserver_repository_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(aiserver_repository_algorithms_gtest
        MODULES
            memochat.ai.repository_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AIServer/cxx_modules/AIServerRepository.cppm
        CONSUMER_SOURCES
            AIServerRepositoryAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests aiserver_repository_algorithms_gtest)

    gtest_discover_tests(aiserver_repository_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
endif()
