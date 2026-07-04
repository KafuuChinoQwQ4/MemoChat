cmake_minimum_required(VERSION 3.16)

add_executable(varifyserver_gtest
    main.cpp
    VerifyCodePolicyTest.cpp
    EmailDeliveryTaskCodecTest.cpp
)

target_link_libraries(varifyserver_gtest PRIVATE
    GTest_TestLib
    VarifyServerCore
    memochat_server_deps
)

target_include_directories(varifyserver_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/VarifyServer
)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(varifyserver_gtest
        MODULES
            memochat.varify.email_delivery_task_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/VarifyServer/cxx_modules/EmailDeliveryTask.cppm
            memochat.varify.verify_code_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/VarifyServer/cxx_modules/VerifyCode.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/VarifyServer/EmailDeliveryTaskCodec.cpp
            ${CMAKE_SOURCE_DIR}/apps/server/core/VarifyServer/VerifyCodePolicy.cpp
    )
endif()

if(MEMOCHAT_ENABLE_GNU_MODULES)
    add_executable(varifyserver_email_sender_algorithms_gtest
        main.cpp
        EmailSenderAlgorithmsConsumer.cpp
        EmailSenderAlgorithmsTest.cpp
    )

    target_link_libraries(varifyserver_email_sender_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(varifyserver_email_sender_algorithms_gtest
        MODULES
            memochat.varify.email_sender_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/VarifyServer/cxx_modules/EmailSender.cppm
        CONSUMER_SOURCES
            EmailSenderAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests varifyserver_email_sender_algorithms_gtest)

    add_executable(varifyserver_email_task_bus_algorithms_gtest
        main.cpp
        EmailTaskBusAlgorithmsConsumer.cpp
        EmailTaskBusAlgorithmsTest.cpp
    )

    target_link_libraries(varifyserver_email_task_bus_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(varifyserver_email_task_bus_algorithms_gtest
        MODULES
            memochat.varify.email_task_bus_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/VarifyServer/cxx_modules/EmailTaskBus.cppm
        CONSUMER_SOURCES
            EmailTaskBusAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests varifyserver_email_task_bus_algorithms_gtest)

    add_executable(varifyserver_rate_limiter_algorithms_gtest
        main.cpp
        RateLimiterAlgorithmsConsumer.cpp
        RateLimiterAlgorithmsTest.cpp
    )

    target_link_libraries(varifyserver_rate_limiter_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(varifyserver_rate_limiter_algorithms_gtest
        MODULES
            memochat.varify.rate_limiter_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/VarifyServer/cxx_modules/RateLimiter.cppm
        CONSUMER_SOURCES
            RateLimiterAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests varifyserver_rate_limiter_algorithms_gtest)

    add_executable(varifyserver_redis_algorithms_gtest
        main.cpp
        VarifyRedisAlgorithmsConsumer.cpp
        VarifyRedisAlgorithmsTest.cpp
    )

    target_link_libraries(varifyserver_redis_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(varifyserver_redis_algorithms_gtest
        MODULES
            memochat.varify.redis_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/VarifyServer/cxx_modules/VarifyRedis.cppm
        CONSUMER_SOURCES
            VarifyRedisAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests varifyserver_redis_algorithms_gtest)

    add_executable(varifyserver_service_algorithms_gtest
        main.cpp
        VarifyServiceAlgorithmsConsumer.cpp
        VarifyServiceAlgorithmsTest.cpp
    )

    target_link_libraries(varifyserver_service_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(varifyserver_service_algorithms_gtest
        MODULES
            memochat.varify.service_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/VarifyServer/cxx_modules/VarifyService.cppm
        CONSUMER_SOURCES
            VarifyServiceAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests varifyserver_service_algorithms_gtest)

    add_executable(varifyserver_runtime_algorithms_gtest
        main.cpp
        VarifyServerRuntimeAlgorithmsConsumer.cpp
        VarifyServerRuntimeAlgorithmsTest.cpp
    )

    target_link_libraries(varifyserver_runtime_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(varifyserver_runtime_algorithms_gtest
        MODULES
            memochat.varify.server_runtime_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/VarifyServer/cxx_modules/VarifyServerRuntime.cppm
        CONSUMER_SOURCES
            VarifyServerRuntimeAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests varifyserver_runtime_algorithms_gtest)
endif()

add_dependencies(gtest_tests varifyserver_gtest)

include(GoogleTest)
gtest_discover_tests(varifyserver_gtest TEST_DISCOVERY_MODE POST_BUILD)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    gtest_discover_tests(varifyserver_email_sender_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(varifyserver_email_task_bus_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(varifyserver_rate_limiter_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(varifyserver_redis_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(varifyserver_service_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(varifyserver_runtime_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
endif()
