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

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(gateshared_cross_module_route_schema_catalog_gtest
        MODULES
            memochat.account.auth_route_schema_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/auth/cxx_modules/AuthRouteSchema.cppm
            memochat.account.profile_route_schema_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/profile/cxx_modules/ProfileRouteSchema.cppm
            memochat.media.route_schema_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/modules/media/cxx_modules/MediaRouteSchema.cppm
            memochat.call.route_schema_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/CallService/domain/modules/call/cxx_modules/CallRouteSchema.cppm
            memochat.r18.route_schema_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/modules/r18/cxx_modules/R18RouteSchema.cppm
            memochat.ai.route_schema_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain/modules/cxx_modules/AIRouteSchema.cppm
            memochat.moments.route_schema_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/modules/moments/cxx_modules/MomentsRouteSchema.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/auth/AuthRouteSchemas.cpp
            ${CMAKE_SOURCE_DIR}/apps/server/core/AccountShared/domain/modules/profile/ProfileRouteSchemas.cpp
            ${CMAKE_SOURCE_DIR}/apps/server/core/MediaService/domain/modules/media/MediaRouteSchemas.cpp
            ${CMAKE_SOURCE_DIR}/apps/server/core/CallService/domain/modules/call/CallRouteSchemas.cpp
            ${CMAKE_SOURCE_DIR}/apps/server/core/R18Service/domain/modules/r18/R18RouteSchemas.cpp
            ${CMAKE_SOURCE_DIR}/apps/server/core/AIGatewayService/domain/modules/ai/AIRouteSchemas.cpp
            ${CMAKE_SOURCE_DIR}/apps/server/core/MomentsService/domain/modules/moments/MomentsRouteSchemas.cpp
    )
endif()

add_dependencies(gtest_tests gateshared_cross_module_route_schema_catalog_gtest)

# CServer accept 循环协程化(P3)启动/lifetime/不泄漏验收。链 GateRuntimeCore 拿到
# GateShared CServer + HttpConnection;测试用临时-owner 调用模式坐实传值 self 协程参数
# 覆盖启动窗口(不 bad_weak_ptr→terminate)+ accept 生效 + io_context 析构后不泄漏。
# 只用进程内 AsioIOServicePool + loopback socket,不触达 LogicSystem/Redis/gRPC,无需 Docker。
add_executable(gateshared_cserver_lifetime_gtest
    main.cpp
    CServerLifetimeTest.cpp
)

target_link_libraries(gateshared_cserver_lifetime_gtest PRIVATE
    GTest_TestLib
    GateRuntimeCore
    memochat_server_deps
)

target_include_directories(gateshared_cserver_lifetime_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
)

add_dependencies(gtest_tests gateshared_cserver_lifetime_gtest)

add_executable(gateshared_route_registry_gtest
    HealthRouteAlgorithmsConsumer.cpp
    HealthRouteModuleTest.cpp
    main.cpp
    RouteRegistryTest.cpp
)

target_link_libraries(gateshared_route_registry_gtest PRIVATE
    GTest_TestLib
    GateRuntimeCore
    memochat_server_deps
)

target_include_directories(gateshared_route_registry_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared
    ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
)

add_dependencies(gtest_tests gateshared_route_registry_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    add_executable(gateshared_bearer_access_auth_algorithms_gtest
        main.cpp
        BearerAccessAuthAlgorithmsConsumer.cpp
        BearerAccessAuthAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_bearer_access_auth_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_bearer_access_auth_algorithms_gtest
        MODULES
            memochat.gate.bearer_access_auth_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core/support/cxx_modules/BearerAccessAuth.cppm
        CONSUMER_SOURCES
            BearerAccessAuthAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_bearer_access_auth_algorithms_gtest)

    add_executable(gateshared_http_json_support_algorithms_gtest
        main.cpp
        GateHttpJsonSupportAlgorithmsConsumer.cpp
        GateHttpJsonSupportAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_http_json_support_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_http_json_support_algorithms_gtest
        MODULES
            memochat.gate.http_json_support_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/modules/cxx_modules/GateHttpJsonSupport.cppm
        CONSUMER_SOURCES
            GateHttpJsonSupportAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_http_json_support_algorithms_gtest)

    add_executable(gateshared_h1_route_adapter_algorithms_gtest
        main.cpp
        H1RouteAdapterAlgorithmsConsumer.cpp
        H1RouteAdapterAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_h1_route_adapter_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_h1_route_adapter_algorithms_gtest
        MODULES
            memochat.gate.h1_route_adapter_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/adapters/h1/cxx_modules/H1RouteAdapter.cppm
        CONSUMER_SOURCES
            H1RouteAdapterAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_h1_route_adapter_algorithms_gtest)

    add_executable(gateshared_worker_pool_algorithms_gtest
        main.cpp
        GateWorkerPoolAlgorithmsConsumer.cpp
        GateWorkerPoolAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_worker_pool_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_worker_pool_algorithms_gtest
        MODULES
            memochat.gate.worker_pool_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/modules/cxx_modules/GateWorkerPool.cppm
        CONSUMER_SOURCES
            GateWorkerPoolAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_worker_pool_algorithms_gtest)

    add_executable(gateshared_user_token_validator_algorithms_gtest
        main.cpp
        UserTokenValidatorAlgorithmsConsumer.cpp
        UserTokenValidatorAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_user_token_validator_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_user_token_validator_algorithms_gtest
        MODULES
            memochat.gate.user_token_validator_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core/support/cxx_modules/UserTokenValidator.cppm
        CONSUMER_SOURCES
            UserTokenValidatorAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_user_token_validator_algorithms_gtest)

    add_executable(gateshared_redis_pipeline_algorithms_gtest
        main.cpp
        RedisPipelineAlgorithmsConsumer.cpp
        RedisPipelineAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_redis_pipeline_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_redis_pipeline_algorithms_gtest
        MODULES
            memochat.gate.redis_pipeline_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core/cache/cxx_modules/RedisPipeline.cppm
        CONSUMER_SOURCES
            RedisPipelineAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_redis_pipeline_algorithms_gtest)

    add_executable(gateshared_redis_mgr_algorithms_gtest
        main.cpp
        RedisMgrAlgorithmsConsumer.cpp
        RedisMgrAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_redis_mgr_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_redis_mgr_algorithms_gtest
        MODULES
            memochat.gate.redis_mgr_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core/cache/cxx_modules/RedisMgr.cppm
        CONSUMER_SOURCES
            RedisMgrAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_redis_mgr_algorithms_gtest)

    add_executable(gateshared_postgres_dao_algorithms_gtest
        main.cpp
        PostgresDaoAlgorithmsConsumer.cpp
        PostgresDaoAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_postgres_dao_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_postgres_dao_algorithms_gtest
        MODULES
            memochat.gate.postgres_dao_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core/persistence/cxx_modules/PostgresDao.cppm
        CONSUMER_SOURCES
            PostgresDaoAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_postgres_dao_algorithms_gtest)

    add_executable(gateshared_postgres_dao_account_algorithms_gtest
        main.cpp
        PostgresDaoAccountAlgorithmsConsumer.cpp
        PostgresDaoAccountAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_postgres_dao_account_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_postgres_dao_account_algorithms_gtest
        MODULES
            memochat.gate.postgres_dao_account_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core/persistence/cxx_modules/PostgresDaoAccount.cppm
        CONSUMER_SOURCES
            PostgresDaoAccountAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_postgres_dao_account_algorithms_gtest)

    add_executable(gateshared_mongo_dao_algorithms_gtest
        main.cpp
        MongoDaoAlgorithmsConsumer.cpp
        MongoDaoAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_mongo_dao_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_mongo_dao_algorithms_gtest
        MODULES
            memochat.gate.mongo_dao_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core/persistence/cxx_modules/MongoDao.cppm
        CONSUMER_SOURCES
            MongoDaoAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_mongo_dao_algorithms_gtest)

    add_executable(gateshared_chat_grpc_client_algorithms_gtest
        main.cpp
        ChatGrpcClientAlgorithmsConsumer.cpp
        ChatGrpcClientAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_chat_grpc_client_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_chat_grpc_client_algorithms_gtest
        MODULES
            memochat.gate.chat_grpc_client_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core/clients/cxx_modules/ChatGrpcClient.cppm
        CONSUMER_SOURCES
            ChatGrpcClientAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_chat_grpc_client_algorithms_gtest)

    add_executable(gateshared_h3_legacy_route_algorithms_gtest
        main.cpp
        H3LegacyRouteAlgorithmsConsumer.cpp
        H3LegacyRouteAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_h3_legacy_route_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_h3_legacy_route_algorithms_gtest
        MODULES
            memochat.gate.h3_legacy_route_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/transports/h3/legacy_routes/cxx_modules/H3LegacyRoute.cppm
        CONSUMER_SOURCES
            H3LegacyRouteAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_h3_legacy_route_algorithms_gtest)

    add_executable(gateshared_postgres_mgr_algorithms_gtest
        main.cpp
        PostgresMgrAlgorithmsConsumer.cpp
        PostgresMgrAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_postgres_mgr_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_postgres_mgr_algorithms_gtest
        MODULES
            memochat.gate.postgres_mgr_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core/persistence/cxx_modules/PostgresMgr.cppm
        CONSUMER_SOURCES
            PostgresMgrAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_postgres_mgr_algorithms_gtest)

    add_executable(gateshared_mongo_mgr_algorithms_gtest
        main.cpp
        MongoMgrAlgorithmsConsumer.cpp
        MongoMgrAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_mongo_mgr_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_mongo_mgr_algorithms_gtest
        MODULES
            memochat.gate.mongo_mgr_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core/persistence/cxx_modules/MongoMgr.cppm
        CONSUMER_SOURCES
            MongoMgrAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_mongo_mgr_algorithms_gtest)

    add_executable(gateshared_h3_route_adapter_algorithms_gtest
        main.cpp
        H3RouteAdapterAlgorithmsConsumer.cpp
        H3RouteAdapterAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_h3_route_adapter_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_h3_route_adapter_algorithms_gtest
        MODULES
            memochat.gate.h3_route_adapter_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/transports/h3/adapters/h3/cxx_modules/H3RouteAdapter.cppm
        CONSUMER_SOURCES
            H3RouteAdapterAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_h3_route_adapter_algorithms_gtest)

    add_executable(gateshared_h3_json_support_algorithms_gtest
        main.cpp
        GateHttp3JsonSupportAlgorithmsConsumer.cpp
        GateHttp3JsonSupportAlgorithmsTest.cpp
    )

    target_link_libraries(gateshared_h3_json_support_algorithms_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    memochat_enable_gnu_modules(gateshared_h3_json_support_algorithms_gtest
        MODULES
            memochat.gate.h3_json_support_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/transports/h3/listener/cxx_modules/GateHttp3JsonSupport.cppm
        CONSUMER_SOURCES
            GateHttp3JsonSupportAlgorithmsConsumer.cpp
    )

    add_dependencies(gtest_tests gateshared_h3_json_support_algorithms_gtest)

    memochat_enable_gnu_modules(gateshared_route_registry_gtest
        MODULES
            memochat.gate.health_route_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/modules/cxx_modules/HealthRoute.cppm
        CONSUMER_SOURCES
            HealthRouteAlgorithmsConsumer.cpp
    )

    add_executable(gateshared_domain_runtime_gtest
        ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/app/GateDomainRuntime.cpp
        main.cpp
        GateDomainRuntimeTest.cpp
    )

    target_link_libraries(gateshared_domain_runtime_gtest PRIVATE
        GTest_TestLib
        memochat_server_deps
    )

    target_include_directories(gateshared_domain_runtime_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared
        ${CMAKE_SOURCE_DIR}/apps/server/core/common
    )

    memochat_enable_gnu_modules(gateshared_domain_runtime_gtest
        MODULES
            memochat.gate.domain_runtime_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/modules/cxx_modules/GateDomainRuntime.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/app/GateDomainRuntime.cpp
    )
    if(MEMOCHAT_ENABLE_GNU_STD_MODULES AND MEMOCHAT_ENABLE_CPP26_REFLECTION)
        set_property(SOURCE "${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/app/GateDomainRuntime.cpp" APPEND_STRING PROPERTY COMPILE_FLAGS " -fno-reflection")
    endif()

    add_dependencies(gtest_tests gateshared_domain_runtime_gtest)

    add_executable(gateshared_module_probe_gtest
        main.cpp
        ModuleProbeTest.cpp
    )

    target_link_libraries(gateshared_module_probe_gtest PRIVATE
        GTest_TestLib
        GateRuntimeCore
        memochat_server_deps
    )

    target_include_directories(gateshared_module_probe_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared
        ${CMAKE_SOURCE_DIR}/apps/server/core/GateShared/core
        ${CMAKE_SOURCE_DIR}/apps/server/core/common
    )

    add_dependencies(gtest_tests gateshared_module_probe_gtest)
endif()

include(GoogleTest)
gtest_discover_tests(gateshared_route_schema_gtest TEST_DISCOVERY_MODE POST_BUILD)
gtest_discover_tests(gateshared_route_schema_catalog_gtest TEST_DISCOVERY_MODE POST_BUILD)
gtest_discover_tests(gateshared_cross_module_route_schema_catalog_gtest TEST_DISCOVERY_MODE POST_BUILD)
gtest_discover_tests(gateshared_cserver_lifetime_gtest TEST_DISCOVERY_MODE POST_BUILD)
gtest_discover_tests(gateshared_route_registry_gtest TEST_DISCOVERY_MODE POST_BUILD)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    gtest_discover_tests(gateshared_bearer_access_auth_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_http_json_support_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_h1_route_adapter_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_worker_pool_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_user_token_validator_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_redis_pipeline_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_redis_mgr_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_postgres_dao_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_postgres_dao_account_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_mongo_dao_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_chat_grpc_client_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_h3_legacy_route_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_postgres_mgr_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_mongo_mgr_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_h3_route_adapter_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_h3_json_support_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_domain_runtime_gtest TEST_DISCOVERY_MODE POST_BUILD)
    gtest_discover_tests(gateshared_module_probe_gtest TEST_DISCOVERY_MODE POST_BUILD)
endif()
