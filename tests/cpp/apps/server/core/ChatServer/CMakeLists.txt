cmake_minimum_required(VERSION 3.16)

add_executable(chatserver_relation_internal_grpc_service_gtest
    main.cpp
    ChatRelationInternalGrpcServiceTest.cpp
)

target_link_libraries(chatserver_relation_internal_grpc_service_gtest PRIVATE
    GTest_TestLib
    ChatServerCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_relation_internal_grpc_service_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/transport
)

add_dependencies(gtest_tests chatserver_relation_internal_grpc_service_gtest)

include(GoogleTest)
gtest_discover_tests(chatserver_relation_internal_grpc_service_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_internal_grpc_service_algorithms_gtest
    main.cpp
    ChatRelationInternalGrpcServiceAlgorithmsConsumer.cpp
    ChatRelationInternalGrpcServiceAlgorithmsTest.cpp
)

target_link_libraries(chatserver_relation_internal_grpc_service_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_relation_internal_grpc_service_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation
)

add_dependencies(gtest_tests chatserver_relation_internal_grpc_service_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_relation_internal_grpc_service_algorithms_gtest
        MODULES
            memochat.chat.relation_internal_grpc_service_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation/cxx_modules/ChatRelationInternalGrpcService.cppm
        CONSUMER_SOURCES
            ChatRelationInternalGrpcServiceAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_relation_internal_grpc_service_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_service_algorithms_gtest
    main.cpp
    ChatRelationServiceAlgorithmsConsumer.cpp
    ChatRelationServiceAlgorithmsTest.cpp
)

target_link_libraries(chatserver_relation_service_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_relation_service_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation
)

add_dependencies(gtest_tests chatserver_relation_service_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_relation_service_algorithms_gtest
        MODULES
            memochat.chat.relation_service_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation/cxx_modules/ChatRelationService.cppm
        CONSUMER_SOURCES
            ChatRelationServiceAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_relation_service_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_message_internal_grpc_service_gtest
    main.cpp
    ChatMessageInternalGrpcServiceTest.cpp
)

target_link_libraries(chatserver_message_internal_grpc_service_gtest PRIVATE
    GTest_TestLib
    ChatMessageCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_message_internal_grpc_service_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
)

add_dependencies(gtest_tests chatserver_message_internal_grpc_service_gtest)

gtest_discover_tests(chatserver_message_internal_grpc_service_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_message_grpc_client_gtest
    main.cpp
    MessageGrpcClientTest.cpp
)

target_link_libraries(chatserver_message_grpc_client_gtest PRIVATE
    GTest_TestLib
    ChatClientCore
    ChatMessageCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_message_grpc_client_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
)

add_dependencies(gtest_tests chatserver_message_grpc_client_gtest)

gtest_discover_tests(chatserver_message_grpc_client_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_message_internal_grpc_service_algorithms_gtest
    main.cpp
    ChatMessageInternalGrpcServiceAlgorithmsConsumer.cpp
    ChatMessageInternalGrpcServiceAlgorithmsTest.cpp
)

target_link_libraries(chatserver_message_internal_grpc_service_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_message_internal_grpc_service_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message
)

add_dependencies(gtest_tests chatserver_message_internal_grpc_service_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_message_internal_grpc_service_algorithms_gtest
        MODULES
            memochat.chat.message_internal_grpc_service_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message/cxx_modules/ChatMessageInternalGrpcService.cppm
        CONSUMER_SOURCES
            ChatMessageInternalGrpcServiceAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_message_internal_grpc_service_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_chat_grpc_client_algorithms_gtest
    main.cpp
    ChatGrpcClientAlgorithmsConsumer.cpp
    ChatGrpcClientAlgorithmsTest.cpp
)

target_link_libraries(chatserver_chat_grpc_client_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_chat_grpc_client_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients
)

add_dependencies(gtest_tests chatserver_chat_grpc_client_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_chat_grpc_client_algorithms_gtest
        MODULES
            memochat.chat.chat_grpc_client_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients/cxx_modules/ChatGrpcClient.cppm
        CONSUMER_SOURCES
            ChatGrpcClientAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_chat_grpc_client_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_message_grpc_client_algorithms_gtest
    main.cpp
    MessageGrpcClientAlgorithmsConsumer.cpp
    MessageGrpcClientAlgorithmsTest.cpp
)

target_link_libraries(chatserver_message_grpc_client_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_message_grpc_client_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients
)

add_dependencies(gtest_tests chatserver_message_grpc_client_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_message_grpc_client_algorithms_gtest
        MODULES
            memochat.chat.message_grpc_client_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients/cxx_modules/MessageGrpcClient.cppm
        CONSUMER_SOURCES
            MessageGrpcClientAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_message_grpc_client_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_message_delivery_payload_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message/MessageDeliveryPayload.cpp
    main.cpp
    MessageDeliveryAlgorithmsTest.cpp
    MessageDeliveryPayloadTest.cpp
)

target_link_libraries(chatserver_message_delivery_payload_gtest PRIVATE
    GTest_TestLib
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_message_delivery_payload_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
)

add_dependencies(gtest_tests chatserver_message_delivery_payload_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_message_delivery_payload_gtest
        MODULES
            memochat.chat.message_delivery_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message/cxx_modules/MessageDelivery.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message/MessageDeliveryPayload.cpp
    )
endif()

gtest_discover_tests(chatserver_message_delivery_payload_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_message_delivery_task_payload_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/delivery/MessageDeliveryTaskPayload.cpp
    main.cpp
    MessageDeliveryTaskPayloadTest.cpp
)

target_link_libraries(chatserver_message_delivery_task_payload_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_message_delivery_task_payload_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/delivery
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message
)

add_dependencies(gtest_tests chatserver_message_delivery_task_payload_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_message_delivery_task_payload_gtest
        MODULES
            memochat.chat.delivery_task_payload_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/delivery/cxx_modules/DeliveryTaskPayload.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/delivery/MessageDeliveryTaskPayload.cpp
    )
endif()

gtest_discover_tests(chatserver_message_delivery_task_payload_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_delivery_runtime_algorithms_gtest
    main.cpp
    DeliveryRuntimeAlgorithmsConsumer.cpp
    DeliveryRuntimeAlgorithmsTest.cpp
)

target_link_libraries(chatserver_delivery_runtime_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_delivery_runtime_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/delivery
)

add_dependencies(gtest_tests chatserver_delivery_runtime_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_delivery_runtime_algorithms_gtest
        MODULES
            memochat.chat.delivery_runtime_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/delivery/cxx_modules/DeliveryRuntime.cppm
        CONSUMER_SOURCES
            DeliveryRuntimeAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_delivery_runtime_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_task_dispatcher_algorithms_gtest
    main.cpp
    TaskDispatcherAlgorithmsConsumer.cpp
    TaskDispatcherAlgorithmsTest.cpp
)

target_link_libraries(chatserver_task_dispatcher_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_task_dispatcher_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/delivery
)

add_dependencies(gtest_tests chatserver_task_dispatcher_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_task_dispatcher_algorithms_gtest
        MODULES
            memochat.chat.task_dispatcher_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/delivery/cxx_modules/TaskDispatcher.cppm
        CONSUMER_SOURCES
            TaskDispatcherAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_task_dispatcher_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_task_dispatcher_runtime_gtest
    main.cpp
    TaskDispatcherRuntimeTest.cpp
)

target_link_libraries(chatserver_task_dispatcher_runtime_gtest PRIVATE
    GTest_TestLib
    ChatDeliveryCore
    memochat_server_deps
)

target_include_directories(chatserver_task_dispatcher_runtime_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/delivery
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging
)

add_dependencies(gtest_tests chatserver_task_dispatcher_runtime_gtest)

gtest_discover_tests(chatserver_task_dispatcher_runtime_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_envelope_codec_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/app/ChatRuntime.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/config/ConfigMgr.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging/ChatAsyncEvent.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging/ChatTaskEnvelope.cpp
    main.cpp
    ChatRuntimeAlgorithmsConsumer.cpp
    ChatRuntimeAlgorithmsTest.cpp
    ChatEnvelopeCodecTest.cpp
)

target_link_libraries(chatserver_envelope_codec_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_envelope_codec_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/app
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/config
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging
)

add_dependencies(gtest_tests chatserver_envelope_codec_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_envelope_codec_gtest
        MODULES
            memochat.chat.messaging_envelope_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging/cxx_modules/MessagingEnvelope.cppm
            memochat.chat.runtime_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/app/cxx_modules/ChatRuntime.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/app/ChatRuntime.cpp
            ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging/ChatAsyncEvent.cpp
            ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging/ChatTaskEnvelope.cpp
            ChatRuntimeAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_envelope_codec_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_runtime_composition_algorithms_gtest
    main.cpp
    ChatRuntimeCompositionAlgorithmsConsumer.cpp
    ChatRuntimeCompositionAlgorithmsTest.cpp
)

target_link_libraries(chatserver_runtime_composition_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_runtime_composition_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/orchestration
)

add_dependencies(gtest_tests chatserver_runtime_composition_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_runtime_composition_algorithms_gtest
        MODULES
            memochat.chat.runtime_composition_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/orchestration/cxx_modules/ChatRuntimeComposition.cppm
        CONSUMER_SOURCES
            ChatRuntimeCompositionAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_runtime_composition_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_handler_registrar_algorithms_gtest
    main.cpp
    ChatHandlerRegistrarAlgorithmsConsumer.cpp
    ChatHandlerRegistrarAlgorithmsTest.cpp
)

target_link_libraries(chatserver_handler_registrar_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_handler_registrar_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/orchestration
)

add_dependencies(gtest_tests chatserver_handler_registrar_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_handler_registrar_algorithms_gtest
        MODULES
            memochat.chat.handler_registrar_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/orchestration/cxx_modules/ChatHandlerRegistrar.cppm
        CONSUMER_SOURCES
            ChatHandlerRegistrarAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_handler_registrar_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_message_service_runtime_algorithms_gtest
    main.cpp
    ChatMessageServiceRuntimeAlgorithmsConsumer.cpp
    ChatMessageServiceRuntimeAlgorithmsTest.cpp
)

target_link_libraries(chatserver_message_service_runtime_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_message_service_runtime_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/app
)

add_dependencies(gtest_tests chatserver_message_service_runtime_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_message_service_runtime_algorithms_gtest
        MODULES
            memochat.chat.message_service_runtime_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/app/cxx_modules/ChatMessageServiceRuntime.cppm
        CONSUMER_SOURCES
            ChatMessageServiceRuntimeAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_message_service_runtime_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_query_service_runtime_algorithms_gtest
    main.cpp
    ChatRelationQueryServiceRuntimeAlgorithmsConsumer.cpp
    ChatRelationQueryServiceRuntimeAlgorithmsTest.cpp
)

target_link_libraries(chatserver_relation_query_service_runtime_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_relation_query_service_runtime_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/app
)

add_dependencies(gtest_tests chatserver_relation_query_service_runtime_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_relation_query_service_runtime_algorithms_gtest
        MODULES
            memochat.chat.relation_query_service_runtime_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/app/cxx_modules/ChatRelationQueryServiceRuntime.cppm
        CONSUMER_SOURCES
            ChatRelationQueryServiceRuntimeAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_relation_query_service_runtime_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_delivery_worker_runtime_algorithms_gtest
    main.cpp
    ChatDeliveryWorkerRuntimeAlgorithmsConsumer.cpp
    ChatDeliveryWorkerRuntimeAlgorithmsTest.cpp
)

target_link_libraries(chatserver_delivery_worker_runtime_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_delivery_worker_runtime_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/app
)

add_dependencies(gtest_tests chatserver_delivery_worker_runtime_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_delivery_worker_runtime_algorithms_gtest
        MODULES
            memochat.chat.delivery_worker_runtime_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/app/cxx_modules/ChatDeliveryWorkerRuntime.cppm
        CONSUMER_SOURCES
            ChatDeliveryWorkerRuntimeAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_delivery_worker_runtime_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_service_worker_runtime_algorithms_gtest
    main.cpp
    ChatRelationServiceWorkerRuntimeAlgorithmsConsumer.cpp
    ChatRelationServiceWorkerRuntimeAlgorithmsTest.cpp
)

target_link_libraries(chatserver_relation_service_worker_runtime_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_relation_service_worker_runtime_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/app
)

add_dependencies(gtest_tests chatserver_relation_service_worker_runtime_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_relation_service_worker_runtime_algorithms_gtest
        MODULES
            memochat.chat.relation_service_worker_runtime_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/app/cxx_modules/ChatRelationServiceWorkerRuntime.cppm
        CONSUMER_SOURCES
            ChatRelationServiceWorkerRuntimeAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_relation_service_worker_runtime_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_messaging_config_algorithms_gtest
    main.cpp
    MessagingConfigAlgorithmsConsumer.cpp
    MessagingConfigAlgorithmsTest.cpp
)

target_link_libraries(chatserver_messaging_config_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_messaging_config_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging
)

add_dependencies(gtest_tests chatserver_messaging_config_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_messaging_config_algorithms_gtest
        MODULES
            memochat.chat.messaging_config_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging/cxx_modules/MessagingConfig.cppm
        CONSUMER_SOURCES
            MessagingConfigAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_messaging_config_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_inline_task_bus_algorithms_gtest
    main.cpp
    InlineTaskBusAlgorithmsConsumer.cpp
    InlineTaskBusAlgorithmsTest.cpp
)

target_link_libraries(chatserver_inline_task_bus_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_inline_task_bus_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging
)

add_dependencies(gtest_tests chatserver_inline_task_bus_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_inline_task_bus_algorithms_gtest
        MODULES
            memochat.chat.inline_task_bus_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging/cxx_modules/InlineTaskBus.cppm
        CONSUMER_SOURCES
            InlineTaskBusAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_inline_task_bus_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_async_event_dispatcher_algorithms_gtest
    main.cpp
    AsyncEventDispatcherAlgorithmsConsumer.cpp
    AsyncEventDispatcherAlgorithmsTest.cpp
)

target_link_libraries(chatserver_async_event_dispatcher_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_async_event_dispatcher_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging
)

add_dependencies(gtest_tests chatserver_async_event_dispatcher_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_async_event_dispatcher_algorithms_gtest
        MODULES
            memochat.chat.async_event_dispatcher_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging/cxx_modules/AsyncEventDispatcher.cppm
        CONSUMER_SOURCES
            AsyncEventDispatcherAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_async_event_dispatcher_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_kafka_async_event_bus_algorithms_gtest
    main.cpp
    KafkaAsyncEventBusAlgorithmsConsumer.cpp
    KafkaAsyncEventBusAlgorithmsTest.cpp
)

target_link_libraries(chatserver_kafka_async_event_bus_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_kafka_async_event_bus_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging
)

add_dependencies(gtest_tests chatserver_kafka_async_event_bus_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_kafka_async_event_bus_algorithms_gtest
        MODULES
            memochat.chat.kafka_async_event_bus_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging/cxx_modules/KafkaAsyncEventBus.cppm
        CONSUMER_SOURCES
            KafkaAsyncEventBusAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_kafka_async_event_bus_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_rabbitmq_task_bus_algorithms_gtest
    main.cpp
    RabbitMqTaskBusAlgorithmsConsumer.cpp
    RabbitMqTaskBusAlgorithmsTest.cpp
)

target_link_libraries(chatserver_rabbitmq_task_bus_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_rabbitmq_task_bus_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging
)

add_dependencies(gtest_tests chatserver_rabbitmq_task_bus_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_rabbitmq_task_bus_algorithms_gtest
        MODULES
            memochat.chat.rabbitmq_task_bus_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging/cxx_modules/RabbitMqTaskBus.cppm
        CONSUMER_SOURCES
            RabbitMqTaskBusAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_rabbitmq_task_bus_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_redis_async_event_bus_algorithms_gtest
    main.cpp
    RedisAsyncEventBusAlgorithmsConsumer.cpp
    RedisAsyncEventBusAlgorithmsTest.cpp
)

target_link_libraries(chatserver_redis_async_event_bus_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_redis_async_event_bus_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging
)

add_dependencies(gtest_tests chatserver_redis_async_event_bus_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_redis_async_event_bus_algorithms_gtest
        MODULES
            memochat.chat.redis_async_event_bus_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging/cxx_modules/RedisAsyncEventBus.cppm
        CONSUMER_SOURCES
            RedisAsyncEventBusAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_redis_async_event_bus_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_outbox_algorithms_gtest
    main.cpp
    OutboxAlgorithmsConsumer.cpp
    OutboxAlgorithmsTest.cpp
)

target_link_libraries(chatserver_outbox_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_outbox_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_outbox_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_outbox_algorithms_gtest
        MODULES
            memochat.chat.outbox_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/Outbox.cppm
        CONSUMER_SOURCES
            OutboxAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_outbox_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_bootstrap_cache_algorithms_gtest
    main.cpp
    RelationBootstrapCacheAlgorithmsConsumer.cpp
    RelationBootstrapCacheAlgorithmsTest.cpp
)

target_link_libraries(chatserver_relation_bootstrap_cache_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_relation_bootstrap_cache_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_relation_bootstrap_cache_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_relation_bootstrap_cache_algorithms_gtest
        MODULES
            memochat.chat.relation_bootstrap_cache_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/RelationBootstrapCache.cppm
        CONSUMER_SOURCES
            RelationBootstrapCacheAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_relation_bootstrap_cache_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_online_route_store_algorithms_gtest
    main.cpp
    OnlineRouteStoreAlgorithmsConsumer.cpp
    OnlineRouteStoreAlgorithmsTest.cpp
)

target_link_libraries(chatserver_online_route_store_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_online_route_store_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_online_route_store_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_online_route_store_algorithms_gtest
        MODULES
            memochat.chat.online_route_store_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/OnlineRouteStore.cppm
        CONSUMER_SOURCES
            OnlineRouteStoreAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_online_route_store_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_repository_algorithms_gtest
    main.cpp
    RelationRepositoryAlgorithmsConsumer.cpp
    RelationRepositoryAlgorithmsTest.cpp
)

target_link_libraries(chatserver_relation_repository_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_relation_repository_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_relation_repository_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_relation_repository_algorithms_gtest
        MODULES
            memochat.chat.relation_repository_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/RelationRepository.cppm
        CONSUMER_SOURCES
            RelationRepositoryAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_relation_repository_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_session_repository_algorithms_gtest
    main.cpp
    ChatSessionRepositoryAlgorithmsConsumer.cpp
    ChatSessionRepositoryAlgorithmsTest.cpp
)

target_link_libraries(chatserver_session_repository_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_session_repository_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_session_repository_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_session_repository_algorithms_gtest
        MODULES
            memochat.chat.session_repository_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/ChatSessionRepository.cppm
        CONSUMER_SOURCES
            ChatSessionRepositoryAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_session_repository_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_message_repository_algorithms_gtest
    main.cpp
    MessageRepositoryAlgorithmsConsumer.cpp
    MessageRepositoryAlgorithmsTest.cpp
)

target_link_libraries(chatserver_message_repository_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_message_repository_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_message_repository_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_message_repository_algorithms_gtest
        MODULES
            memochat.chat.message_repository_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/MessageRepository.cppm
        CONSUMER_SOURCES
            MessageRepositoryAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_message_repository_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_mongo_dao_algorithms_gtest
    main.cpp
    MongoDaoAlgorithmsConsumer.cpp
    MongoDaoAlgorithmsTest.cpp
)

target_link_libraries(chatserver_mongo_dao_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_mongo_dao_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_mongo_dao_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_mongo_dao_algorithms_gtest
        MODULES
            memochat.chat.mongo_dao_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/MongoDao.cppm
        CONSUMER_SOURCES
            MongoDaoAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_mongo_dao_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_mongo_mgr_algorithms_gtest
    main.cpp
    MongoMgrAlgorithmsConsumer.cpp
    MongoMgrAlgorithmsTest.cpp
)

target_link_libraries(chatserver_mongo_mgr_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_mongo_mgr_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_mongo_mgr_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_mongo_mgr_algorithms_gtest
        MODULES
            memochat.chat.mongo_mgr_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/MongoMgr.cppm
        CONSUMER_SOURCES
            MongoMgrAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_mongo_mgr_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_postgres_mgr_algorithms_gtest
    main.cpp
    PostgresMgrAlgorithmsConsumer.cpp
    PostgresMgrAlgorithmsTest.cpp
)

target_link_libraries(chatserver_postgres_mgr_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_postgres_mgr_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_postgres_mgr_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_postgres_mgr_algorithms_gtest
        MODULES
            memochat.chat.postgres_mgr_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/PostgresMgr.cppm
        CONSUMER_SOURCES
            PostgresMgrAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_postgres_mgr_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_postgres_dao_algorithms_gtest
    main.cpp
    PostgresDaoAlgorithmsConsumer.cpp
    PostgresDaoAlgorithmsTest.cpp
)

target_link_libraries(chatserver_postgres_dao_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_postgres_dao_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_postgres_dao_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_postgres_dao_algorithms_gtest
        MODULES
            memochat.chat.postgres_dao_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/PostgresDao.cppm
        CONSUMER_SOURCES
            PostgresDaoAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_postgres_dao_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_postgres_dao_users_algorithms_gtest
    main.cpp
    PostgresDaoUsersAlgorithmsConsumer.cpp
    PostgresDaoUsersAlgorithmsTest.cpp
)

target_link_libraries(chatserver_postgres_dao_users_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_postgres_dao_users_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_postgres_dao_users_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_postgres_dao_users_algorithms_gtest
        MODULES
            memochat.chat.postgres_dao_users_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/PostgresDaoUsers.cppm
        CONSUMER_SOURCES
            PostgresDaoUsersAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_postgres_dao_users_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_postgres_dao_private_messages_algorithms_gtest
    main.cpp
    PostgresDaoPrivateMessagesAlgorithmsConsumer.cpp
    PostgresDaoPrivateMessagesAlgorithmsTest.cpp
)

target_link_libraries(chatserver_postgres_dao_private_messages_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_postgres_dao_private_messages_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_postgres_dao_private_messages_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_postgres_dao_private_messages_algorithms_gtest
        MODULES
            memochat.chat.postgres_dao_private_messages_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/PostgresDaoPrivateMessages.cppm
        CONSUMER_SOURCES
            PostgresDaoPrivateMessagesAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_postgres_dao_private_messages_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_postgres_dao_group_messages_algorithms_gtest
    main.cpp
    PostgresDaoGroupMessagesAlgorithmsConsumer.cpp
    PostgresDaoGroupMessagesAlgorithmsTest.cpp
)

target_link_libraries(chatserver_postgres_dao_group_messages_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_postgres_dao_group_messages_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_postgres_dao_group_messages_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_postgres_dao_group_messages_algorithms_gtest
        MODULES
            memochat.chat.postgres_dao_group_messages_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/PostgresDaoGroupMessages.cppm
        CONSUMER_SOURCES
            PostgresDaoGroupMessagesAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_postgres_dao_group_messages_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_postgres_dao_dialogs_algorithms_gtest
    main.cpp
    PostgresDaoDialogsAlgorithmsConsumer.cpp
    PostgresDaoDialogsAlgorithmsTest.cpp
)

target_link_libraries(chatserver_postgres_dao_dialogs_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_postgres_dao_dialogs_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_postgres_dao_dialogs_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_postgres_dao_dialogs_algorithms_gtest
        MODULES
            memochat.chat.postgres_dao_dialogs_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/PostgresDaoDialogs.cppm
        CONSUMER_SOURCES
            PostgresDaoDialogsAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_postgres_dao_dialogs_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_postgres_dao_groups_algorithms_gtest
    main.cpp
    PostgresDaoGroupsAlgorithmsConsumer.cpp
    PostgresDaoGroupsAlgorithmsTest.cpp
)

target_link_libraries(chatserver_postgres_dao_groups_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_postgres_dao_groups_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_postgres_dao_groups_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_postgres_dao_groups_algorithms_gtest
        MODULES
            memochat.chat.postgres_dao_groups_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/PostgresDaoGroups.cppm
        CONSUMER_SOURCES
            PostgresDaoGroupsAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_postgres_dao_groups_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_dist_lock_algorithms_gtest
    main.cpp
    DistLockAlgorithmsConsumer.cpp
    DistLockAlgorithmsTest.cpp
)

target_link_libraries(chatserver_dist_lock_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_dist_lock_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_dist_lock_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_dist_lock_algorithms_gtest
        MODULES
            memochat.chat.dist_lock_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/DistLock.cppm
        CONSUMER_SOURCES
            DistLockAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_dist_lock_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_redis_mgr_algorithms_gtest
    main.cpp
    RedisMgrAlgorithmsConsumer.cpp
    RedisMgrAlgorithmsTest.cpp
)

target_link_libraries(chatserver_redis_mgr_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_redis_mgr_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence
)

add_dependencies(gtest_tests chatserver_redis_mgr_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_redis_mgr_algorithms_gtest
        MODULES
            memochat.chat.redis_mgr_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/persistence/cxx_modules/RedisMgr.cppm
        CONSUMER_SOURCES
            RedisMgrAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_redis_mgr_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_user_profile_dto_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/users/ChatUserProfileDto.cpp
    main.cpp
    ChatUserProfileDtoTest.cpp
)

target_link_libraries(chatserver_user_profile_dto_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_user_profile_dto_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/users
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
)

add_dependencies(gtest_tests chatserver_user_profile_dto_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_user_profile_dto_gtest
        MODULES
            memochat.chat.user_profile_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/users/cxx_modules/UserProfile.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/users/ChatUserProfileDto.cpp
    )
endif()

gtest_discover_tests(chatserver_user_profile_dto_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_user_support_algorithms_gtest
    main.cpp
    ChatUserSupportAlgorithmsConsumer.cpp
    ChatUserSupportAlgorithmsTest.cpp
)

target_link_libraries(chatserver_user_support_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_user_support_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/users
)

add_dependencies(gtest_tests chatserver_user_support_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_user_support_algorithms_gtest
        MODULES
            memochat.chat.user_support_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/users/cxx_modules/UserSupport.cppm
        CONSUMER_SOURCES
            ChatUserSupportAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_user_support_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_group_dtos_gtest
    main.cpp
    ChatRelationGroupDtosTest.cpp
)

target_link_libraries(chatserver_relation_group_dtos_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_relation_group_dtos_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message
)

add_dependencies(gtest_tests chatserver_relation_group_dtos_gtest)

gtest_discover_tests(chatserver_relation_group_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_group_command_dtos_gtest
    main.cpp
    ChatGroupCommandDtosTest.cpp
)

target_link_libraries(chatserver_group_command_dtos_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_group_command_dtos_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
)

add_dependencies(gtest_tests chatserver_group_command_dtos_gtest)

gtest_discover_tests(chatserver_group_command_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_history_command_dtos_gtest
    main.cpp
    ChatHistoryCommandDtosTest.cpp
)

target_link_libraries(chatserver_history_command_dtos_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_history_command_dtos_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
)

add_dependencies(gtest_tests chatserver_history_command_dtos_gtest)

gtest_discover_tests(chatserver_history_command_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_history_output_dtos_gtest
    main.cpp
    ChatHistoryOutputDtosTest.cpp
)

target_link_libraries(chatserver_history_output_dtos_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_history_output_dtos_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
)

add_dependencies(gtest_tests chatserver_history_output_dtos_gtest)

gtest_discover_tests(chatserver_history_output_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_message_command_dtos_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message/GroupResponseFormatter.cpp
    main.cpp
    ChatMessageCommandDtosTest.cpp
)

target_link_libraries(chatserver_message_command_dtos_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_message_command_dtos_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
)

add_dependencies(gtest_tests chatserver_message_command_dtos_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_message_command_dtos_gtest
        MODULES
            memochat.chat.group_response_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message/cxx_modules/GroupResponse.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message/GroupResponseFormatter.cpp
    )
endif()

gtest_discover_tests(chatserver_message_command_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_command_dtos_gtest
    main.cpp
    ChatRelationCommandDtosTest.cpp
)

target_link_libraries(chatserver_relation_command_dtos_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_relation_command_dtos_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
)

add_dependencies(gtest_tests chatserver_relation_command_dtos_gtest)

gtest_discover_tests(chatserver_relation_command_dtos_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_message_grpc_service_adapter_gtest
    main.cpp
    MessageGrpcServiceAdapterTest.cpp
)

target_link_libraries(chatserver_message_grpc_service_adapter_gtest PRIVATE
    GTest_TestLib
    ChatServerCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_message_grpc_service_adapter_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
)

add_dependencies(gtest_tests chatserver_message_grpc_service_adapter_gtest)

gtest_discover_tests(chatserver_message_grpc_service_adapter_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_message_service_factory_gtest
    main.cpp
    MessageServiceFactoryTest.cpp
)

target_link_libraries(chatserver_message_service_factory_gtest PRIVATE
    GTest_TestLib
    ChatServerCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_message_service_factory_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
)

add_dependencies(gtest_tests chatserver_message_service_factory_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_message_service_factory_gtest
        MODULES
            memochat.chat.service_factory_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/cxx_modules/ServiceFactory.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message/MessageServiceFactory.cpp
    )
endif()

gtest_discover_tests(chatserver_message_service_factory_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_message_remote_smoke_gtest
    main.cpp
    MessageRemoteSmokeTest.cpp
)

target_link_libraries(chatserver_message_remote_smoke_gtest PRIVATE
    GTest_TestLib
    ChatServerCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_message_remote_smoke_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/config
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
)

add_dependencies(gtest_tests chatserver_message_remote_smoke_gtest)

gtest_discover_tests(chatserver_message_remote_smoke_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_query_grpc_client_gtest
    main.cpp
    RelationQueryGrpcClientTest.cpp
)

target_link_libraries(chatserver_relation_query_grpc_client_gtest PRIVATE
    GTest_TestLib
    ChatServerCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_relation_query_grpc_client_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/transport
)

add_dependencies(gtest_tests chatserver_relation_query_grpc_client_gtest)

gtest_discover_tests(chatserver_relation_query_grpc_client_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_query_grpc_client_algorithms_gtest
    main.cpp
    RelationQueryGrpcClientAlgorithmsConsumer.cpp
    RelationQueryGrpcClientAlgorithmsTest.cpp
)

target_link_libraries(chatserver_relation_query_grpc_client_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_relation_query_grpc_client_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients
)

add_dependencies(gtest_tests chatserver_relation_query_grpc_client_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_relation_query_grpc_client_algorithms_gtest
        MODULES
            memochat.chat.relation_query_grpc_client_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients/cxx_modules/RelationQueryGrpcClient.cppm
        CONSUMER_SOURCES
            RelationQueryGrpcClientAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_relation_query_grpc_client_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_grpc_client_gtest
    main.cpp
    RelationGrpcClientTest.cpp
)

target_link_libraries(chatserver_relation_grpc_client_gtest PRIVATE
    GTest_TestLib
    ChatServerCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_relation_grpc_client_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
)

add_dependencies(gtest_tests chatserver_relation_grpc_client_gtest)

gtest_discover_tests(chatserver_relation_grpc_client_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_grpc_client_algorithms_gtest
    main.cpp
    RelationGrpcClientAlgorithmsConsumer.cpp
    RelationGrpcClientAlgorithmsTest.cpp
)

target_link_libraries(chatserver_relation_grpc_client_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_relation_grpc_client_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients
)

add_dependencies(gtest_tests chatserver_relation_grpc_client_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_relation_grpc_client_algorithms_gtest
        MODULES
            memochat.chat.relation_grpc_client_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients/cxx_modules/RelationGrpcClient.cppm
        CONSUMER_SOURCES
            RelationGrpcClientAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_relation_grpc_client_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_grpc_service_adapter_algorithms_gtest
    main.cpp
    RelationGrpcServiceAdapterAlgorithmsConsumer.cpp
    RelationGrpcServiceAdapterAlgorithmsTest.cpp
)

target_link_libraries(chatserver_relation_grpc_service_adapter_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_relation_grpc_service_adapter_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients
)

add_dependencies(gtest_tests chatserver_relation_grpc_service_adapter_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_relation_grpc_service_adapter_algorithms_gtest
        MODULES
            memochat.chat.relation_grpc_service_adapter_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients/cxx_modules/RelationGrpcServiceAdapter.cppm
        CONSUMER_SOURCES
            RelationGrpcServiceAdapterAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_relation_grpc_service_adapter_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_service_factory_gtest
    main.cpp
    RelationServiceFactoryTest.cpp
)

target_link_libraries(chatserver_relation_service_factory_gtest PRIVATE
    GTest_TestLib
    ChatServerCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_relation_service_factory_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
)

add_dependencies(gtest_tests chatserver_relation_service_factory_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_relation_service_factory_gtest
        MODULES
            memochat.chat.service_factory_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/cxx_modules/ServiceFactory.cppm
        CONSUMER_SOURCES
            ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation/RelationServiceFactory.cpp
    )
endif()

gtest_discover_tests(chatserver_relation_service_factory_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_query_service_factory_gtest
    main.cpp
    RelationQueryServiceFactoryTest.cpp
)

target_link_libraries(chatserver_relation_query_service_factory_gtest PRIVATE
    GTest_TestLib
    ChatClientCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_relation_query_service_factory_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
)

add_dependencies(gtest_tests chatserver_relation_query_service_factory_gtest)

gtest_discover_tests(chatserver_relation_query_service_factory_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_query_remote_smoke_gtest
    main.cpp
    RelationQueryRemoteSmokeTest.cpp
)

target_link_libraries(chatserver_relation_query_remote_smoke_gtest PRIVATE
    GTest_TestLib
    ChatClientCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_relation_query_remote_smoke_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/config
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
)

add_dependencies(gtest_tests chatserver_relation_query_remote_smoke_gtest)

gtest_discover_tests(chatserver_relation_query_remote_smoke_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_send_path_golden_gtest
    main.cpp
    SendPathGoldenTest.cpp
)

target_link_libraries(chatserver_send_path_golden_gtest PRIVATE
    GTest_TestLib
    ChatServerCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_send_path_golden_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
)

add_dependencies(gtest_tests chatserver_send_path_golden_gtest)

gtest_discover_tests(chatserver_send_path_golden_gtest TEST_DISCOVERY_MODE POST_BUILD)

# CSession 读循环协程化(P1b)生命周期/不泄漏验收。链 ChatServerCore 拿到 CSession
# 及其传递依赖;测试走 _server=nullptr 干净分支,不触达 Redis/gRPC 单例,无需 Docker。
add_executable(chatserver_csession_lifetime_gtest
    main.cpp
    CSessionLifetimeTest.cpp
)

target_link_libraries(chatserver_csession_lifetime_gtest PRIVATE
    GTest_TestLib
    ChatServerCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_csession_lifetime_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/transport
)

add_dependencies(gtest_tests chatserver_csession_lifetime_gtest)

gtest_discover_tests(chatserver_csession_lifetime_gtest TEST_DISCOVERY_MODE POST_BUILD)

# CServer accept/timer 循环协程化(P2)停机/不泄漏验收。聚焦 TimerLoop 路径
# (StartTimer/StopTimer),不触达 AsioIOServicePool 线程池,无需 Docker。
add_executable(chatserver_cserver_lifetime_gtest
    main.cpp
    CServerLifetimeTest.cpp
)

target_link_libraries(chatserver_cserver_lifetime_gtest PRIVATE
    GTest_TestLib
    ChatServerCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_cserver_lifetime_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/transport
)

add_dependencies(gtest_tests chatserver_cserver_lifetime_gtest)

gtest_discover_tests(chatserver_cserver_lifetime_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_frame_codec_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/transport/ChatFrameCodec.cpp
    main.cpp
    ChatFrameCodecTest.cpp
)

target_link_libraries(chatserver_frame_codec_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_frame_codec_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/transport
)

add_dependencies(gtest_tests chatserver_frame_codec_gtest)

gtest_discover_tests(chatserver_frame_codec_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_websocket_ingress_gtest
    main.cpp
    WebSocketIngressTest.cpp
)

target_link_libraries(chatserver_websocket_ingress_gtest PRIVATE
    GTest_TestLib
    ChatServerCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_websocket_ingress_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/transport
)

add_dependencies(gtest_tests chatserver_websocket_ingress_gtest)

gtest_discover_tests(chatserver_websocket_ingress_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_webtransport_session_gtest
    main.cpp
    WebTransportSessionTest.cpp
)

target_link_libraries(chatserver_webtransport_session_gtest PRIVATE
    GTest_TestLib
    ChatServerCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_webtransport_session_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/transport
)

add_dependencies(gtest_tests chatserver_webtransport_session_gtest)

gtest_discover_tests(chatserver_webtransport_session_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_webtransport_provider_gtest
    main.cpp
    WebTransportProviderTest.cpp
)

target_link_libraries(chatserver_webtransport_provider_gtest PRIVATE
    GTest_TestLib
    ChatServerCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_webtransport_provider_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/infrastructure
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/transport
)

add_dependencies(gtest_tests chatserver_webtransport_provider_gtest)

gtest_discover_tests(chatserver_webtransport_provider_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_msg_node_algorithms_gtest
    main.cpp
    MsgNodeAlgorithmsConsumer.cpp
    MsgNodeAlgorithmsTest.cpp
)

target_link_libraries(chatserver_msg_node_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_msg_node_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/transport
)

add_dependencies(gtest_tests chatserver_msg_node_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_msg_node_algorithms_gtest
        MODULES
            memochat.chat.msg_node_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/transport/cxx_modules/MsgNode.cppm
        CONSUMER_SOURCES
            MsgNodeAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_msg_node_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

# OnlineRouteResolver DecideOnlineRoute 四分支纯决策单测(无写操作断言)
add_executable(chatserver_online_route_resolver_gtest
    main.cpp
    OnlineRouteResolverTest.cpp
)

target_link_libraries(chatserver_online_route_resolver_gtest PRIVATE
    GTest_TestLib
    ChatServerCore
    memochat_proto
    memochat_server_deps
)

target_include_directories(chatserver_online_route_resolver_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/ports
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/transport
)

add_dependencies(gtest_tests chatserver_online_route_resolver_gtest)

gtest_discover_tests(chatserver_online_route_resolver_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_config_gtest
    main.cpp
    ChatConfigTest.cpp
)

target_link_libraries(chatserver_config_gtest PRIVATE
    GTest_TestLib
    ChatConfigCore
    memochat_server_deps
)

target_include_directories(chatserver_config_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/config
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
)

add_dependencies(gtest_tests chatserver_config_gtest)

gtest_discover_tests(chatserver_config_gtest
    TEST_DISCOVERY_MODE POST_BUILD
    PROPERTIES ENVIRONMENT "MEMOCHAT_CHATAUTH_HMACSECRET=test-chat-secret"
)

# MessageDeliveryService primitive fanout/retry decision module (gtest-only;
# production consumption remains disabled under GCC modules).
add_executable(chatserver_message_delivery_service_algorithms_gtest
    main.cpp
    MessageDeliveryServiceAlgorithmsConsumer.cpp
    MessageDeliveryServiceAlgorithmsTest.cpp
)

target_link_libraries(chatserver_message_delivery_service_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_message_delivery_service_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
)

add_dependencies(gtest_tests chatserver_message_delivery_service_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_message_delivery_service_algorithms_gtest
        MODULES
            memochat.chat.message_delivery_service_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message/cxx_modules/MessageDeliveryService.cppm
        CONSUMER_SOURCES
            MessageDeliveryServiceAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_message_delivery_service_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

# ChatServiceImpl primitive gRPC-notify decision module (gtest-only;
# production consumption remains disabled under GCC modules).
add_executable(chatserver_chat_service_impl_algorithms_gtest
    main.cpp
    ChatServiceImplAlgorithmsConsumer.cpp
    ChatServiceImplAlgorithmsTest.cpp
)

target_link_libraries(chatserver_chat_service_impl_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_chat_service_impl_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/transport
)

add_dependencies(gtest_tests chatserver_chat_service_impl_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_chat_service_impl_algorithms_gtest
        MODULES
            memochat.chat.chat_service_impl_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/transport/cxx_modules/ChatServiceImpl.cppm
        CONSUMER_SOURCES
            ChatServiceImplAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_chat_service_impl_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)

# ChatRelationSessionAdapter forwarding-surface + guard decision module
# (gtest-only; production consumption remains disabled under GCC modules).
add_executable(chatserver_relation_session_adapter_algorithms_gtest
    main.cpp
    ChatRelationSessionAdapterAlgorithmsConsumer.cpp
    ChatRelationSessionAdapterAlgorithmsTest.cpp
)

target_link_libraries(chatserver_relation_session_adapter_algorithms_gtest PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(chatserver_relation_session_adapter_algorithms_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain
)

add_dependencies(gtest_tests chatserver_relation_session_adapter_algorithms_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(chatserver_relation_session_adapter_algorithms_gtest
        MODULES
            memochat.chat.relation_session_adapter_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation/cxx_modules/ChatRelationSessionAdapter.cppm
        CONSUMER_SOURCES
            ChatRelationSessionAdapterAlgorithmsConsumer.cpp
    )
endif()

gtest_discover_tests(chatserver_relation_session_adapter_algorithms_gtest TEST_DISCOVERY_MODE POST_BUILD)
