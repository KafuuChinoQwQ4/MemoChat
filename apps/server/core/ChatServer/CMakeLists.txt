set(CHATSERVER_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/clients
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/domain
    ${CMAKE_CURRENT_SOURCE_DIR}/domain/delivery
    ${CMAKE_CURRENT_SOURCE_DIR}/domain/message
    ${CMAKE_CURRENT_SOURCE_DIR}/domain/orchestration
    ${CMAKE_CURRENT_SOURCE_DIR}/domain/relation
    ${CMAKE_CURRENT_SOURCE_DIR}/domain/session
    ${CMAKE_CURRENT_SOURCE_DIR}/domain/users
    ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure
    ${CMAKE_CURRENT_SOURCE_DIR}/messaging
    ${CMAKE_CURRENT_SOURCE_DIR}/persistence
    ${CMAKE_CURRENT_SOURCE_DIR}/transport
)

set(CHATSERVER_APP_SOURCES
    app/ChatRuntime.cpp
)

set(CHATSERVER_CLIENT_SOURCES
    clients/ChatGrpcClient.cpp
    clients/MessageGrpcClient.cpp
    clients/MessageGrpcServiceAdapter.cpp
    clients/RelationGrpcClient.cpp
    clients/RelationGrpcServiceAdapter.cpp
    clients/RelationQueryGrpcClient.cpp
    clients/RelationQueryServiceFactory.cpp
)

set(CHATSERVER_CONFIG_SOURCES
    config/ChatSessionConfig.cpp
    config/ConfigMgr.cpp
    config/LogicSystemConfig.cpp
    config/MessageServiceConfig.cpp
    config/RelationQueryServiceConfig.cpp
    config/RelationServiceConfig.cpp
)

set(CHATSERVER_INFRASTRUCTURE_SOURCES
    infrastructure/AsioIOServicePool.cpp
)

set(CHATSERVER_MESSAGING_SOURCES
    messaging/AsyncEventDispatcher.cpp
    messaging/ChatAsyncEvent.cpp
    messaging/ChatTaskEnvelope.cpp
    messaging/InlineTaskBus.cpp
    messaging/KafkaAsyncEventBus.cpp
    messaging/KafkaConfig.cpp
    messaging/RabbitMqConfig.cpp
    messaging/RabbitMqTaskBus.cpp
    messaging/RedisAsyncEventBus.cpp
    messaging/SessionSendSupport.cpp
)

set(CHATSERVER_PERSISTENCE_SOURCES
    persistence/ChatMessageRepository.cpp
    persistence/ChatOutboxRepairScheduler.cpp
    persistence/ChatRelationRepository.cpp
    persistence/ChatOutboxService.cpp
    persistence/ChatSessionRepository.cpp
    persistence/DistLock.cpp
    persistence/MongoDao.cpp
    persistence/MongoMgr.cpp
    persistence/PostgresDao.cpp
    persistence/PostgresDaoDialogs.cpp
    persistence/PostgresDaoGroupMessages.cpp
    persistence/PostgresDaoGroups.cpp
    persistence/PostgresDaoPrivateMessages.cpp
    persistence/PostgresDaoUsers.cpp
    persistence/PostgresMgr.cpp
    persistence/OnlineRouteSessionSupport.cpp
    persistence/RedisRelationBootstrapCache.cpp
    persistence/RedisOnlineRouteStore.cpp
    persistence/RedisMgr.cpp
)

set(CHATSERVER_TRANSPORT_SOURCES
    transport/CServer.cpp
    transport/ChatSessionCleanupSupport.cpp
    transport/CSession.cpp
    transport/ChatFrameCodec.cpp
    transport/ChatFrameDispatch.cpp
    transport/ChatIngressCoordinator.cpp
    transport/ChatServiceImpl.cpp
    transport/MsgNode.cpp
    transport/QuicChatServer.cpp
    transport/QuicSession.cpp
    transport/TcpSession.cpp
    transport/UnavailableWebTransportProvider.cpp
    transport/WebSocketChatServer.cpp
    transport/WebSocketSession.cpp
    transport/WebTransportChatServer.cpp
    transport/WebTransportProviderFactory.cpp
    transport/WebTransportSession.cpp
)

set(CHATSERVER_ORCHESTRATION_SOURCES
    domain/orchestration/ChatHandlerRegistrars.cpp
    domain/orchestration/ChatRuntimeComposition.cpp
    domain/orchestration/LogicSystem.cpp
)

set(CHATSERVER_SESSION_SOURCES
    domain/session/ChatSessionService.cpp
    domain/users/UserMgr.cpp
)

set(CHATSERVER_USER_SUPPORT_SOURCES
    domain/users/ChatUserProfileDto.cpp
    domain/users/ChatUserSupport.cpp
)

set(CHATSERVER_RELATION_SOURCES
    domain/relation/ChatRelationInternalGrpcService.cpp
    domain/relation/ChatRelationService.cpp
    domain/relation/RelationServiceFactory.cpp
)

set(CHATSERVER_RELATION_SESSION_ADAPTER_SOURCES
    domain/relation/ChatRelationSessionAdapter.cpp
)

set(CHATSERVER_DELIVERY_PAYLOAD_SOURCES
    domain/delivery/MessageDeliveryTaskPayload.cpp
)

set(CHATSERVER_MESSAGE_SOURCES
    domain/message/ChatMessageInternalGrpcService.cpp
    domain/message/GroupManagementService.cpp
    domain/message/GroupMembershipService.cpp
    domain/message/GroupMessageHistoryService.cpp
    domain/message/GroupMessageService.cpp
    domain/message/GroupMessageWorkflow.cpp
    domain/message/GroupResponseFormatter.cpp
    domain/message/MessageDeliveryPayload.cpp
    domain/message/MessageServiceFactory.cpp
    domain/message/MessageDeliveryService.cpp
    domain/message/PrivateMessageService.cpp
)

set(CHATSERVER_DELIVERY_SOURCES
    domain/delivery/ChatDeliveryRuntime.cpp
    domain/delivery/TaskDispatcher.cpp
)

function(chatserver_core_target target_name)
    target_include_directories(${target_name} PUBLIC
        ${CHATSERVER_INCLUDE_DIRS}
    )
    target_link_libraries(${target_name} PUBLIC
        memochat_server_deps
        memochat_boost_stubs
    )
    set_target_properties(${target_name} PROPERTIES FOLDER "Server/ChatServer/Core")
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /bigobj /Zm200)
    endif()
endfunction()

add_library(ChatConfigCore STATIC
    ${CHATSERVER_CONFIG_SOURCES}
)
chatserver_core_target(ChatConfigCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatConfigCore
        MODULES
            memochat.chat.config_algorithms=config/cxx_modules/ChatConfig.cppm
        CONSUMER_SOURCES
            config/ChatSessionConfig.cpp
            config/MessageServiceConfig.cpp
            config/RelationQueryServiceConfig.cpp
            config/RelationServiceConfig.cpp
    )
endif()

add_library(ChatAppCore STATIC
    ${CHATSERVER_APP_SOURCES}
)
chatserver_core_target(ChatAppCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatAppCore
        MODULES
            memochat.chat.runtime_algorithms=app/cxx_modules/ChatRuntime.cppm
        CONSUMER_SOURCES
            app/ChatRuntime.cpp
    )
endif()
target_link_libraries(ChatAppCore PUBLIC
    ChatConfigCore
)

add_library(ChatClientCore STATIC
    ${CHATSERVER_CLIENT_SOURCES}
)
chatserver_core_target(ChatClientCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatClientCore
        MODULES
            memochat.chat.chat_grpc_client_algorithms=clients/cxx_modules/ChatGrpcClient.cppm
            memochat.chat.message_grpc_client_algorithms=clients/cxx_modules/MessageGrpcClient.cppm
            memochat.chat.relation_grpc_client_algorithms=clients/cxx_modules/RelationGrpcClient.cppm
            memochat.chat.relation_grpc_service_adapter_algorithms=clients/cxx_modules/RelationGrpcServiceAdapter.cppm
            memochat.chat.relation_query_grpc_client_algorithms=clients/cxx_modules/RelationQueryGrpcClient.cppm
            memochat.chat.service_factory_algorithms=domain/cxx_modules/ServiceFactory.cppm
        CONSUMER_SOURCES
            clients/ChatGrpcClient.cpp
            clients/MessageGrpcClient.cpp
            clients/RelationGrpcClient.cpp
            clients/RelationGrpcServiceAdapter.cpp
            clients/RelationQueryGrpcClient.cpp
            clients/RelationQueryServiceFactory.cpp
    )
endif()
target_link_libraries(ChatClientCore PUBLIC
    ChatConfigCore
)

add_library(ChatInfrastructureCore STATIC
    ${CHATSERVER_INFRASTRUCTURE_SOURCES}
)
chatserver_core_target(ChatInfrastructureCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatInfrastructureCore
        MODULES
            memochat.runtime.io_context_pool_algorithms=../common/runtime/cxx_modules/IoContextPool.cppm
        CONSUMER_SOURCES
            infrastructure/AsioIOServicePool.cpp
    )
endif()

add_library(ChatPersistenceCore STATIC
    ${CHATSERVER_PERSISTENCE_SOURCES}
)
chatserver_core_target(ChatPersistenceCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatPersistenceCore
        MODULES
            memochat.chat.session_repository_algorithms=persistence/cxx_modules/ChatSessionRepository.cppm
            memochat.chat.message_repository_algorithms=persistence/cxx_modules/MessageRepository.cppm
            memochat.chat.mongo_dao_algorithms=persistence/cxx_modules/MongoDao.cppm
            memochat.chat.mongo_mgr_algorithms=persistence/cxx_modules/MongoMgr.cppm
            memochat.chat.postgres_mgr_algorithms=persistence/cxx_modules/PostgresMgr.cppm
            memochat.chat.postgres_dao_algorithms=persistence/cxx_modules/PostgresDao.cppm
            memochat.chat.postgres_dao_users_algorithms=persistence/cxx_modules/PostgresDaoUsers.cppm
            memochat.chat.postgres_dao_private_messages_algorithms=persistence/cxx_modules/PostgresDaoPrivateMessages.cppm
            memochat.chat.postgres_dao_group_messages_algorithms=persistence/cxx_modules/PostgresDaoGroupMessages.cppm
            memochat.chat.postgres_dao_dialogs_algorithms=persistence/cxx_modules/PostgresDaoDialogs.cppm
            memochat.chat.postgres_dao_groups_algorithms=persistence/cxx_modules/PostgresDaoGroups.cppm
            memochat.chat.dist_lock_algorithms=persistence/cxx_modules/DistLock.cppm
            memochat.chat.redis_mgr_algorithms=persistence/cxx_modules/RedisMgr.cppm
            memochat.chat.outbox_algorithms=persistence/cxx_modules/Outbox.cppm
            memochat.chat.relation_bootstrap_cache_algorithms=persistence/cxx_modules/RelationBootstrapCache.cppm
            memochat.chat.online_route_store_algorithms=persistence/cxx_modules/OnlineRouteStore.cppm
            memochat.chat.relation_repository_algorithms=persistence/cxx_modules/RelationRepository.cppm
        CONSUMER_SOURCES
            persistence/ChatSessionRepository.cpp
            persistence/ChatRelationRepository.cpp
            persistence/ChatMessageRepository.cpp
            persistence/MongoDao.cpp
            persistence/MongoMgr.cpp
            persistence/PostgresMgr.cpp
            persistence/PostgresDao.cpp
            persistence/PostgresDaoUsers.cpp
            persistence/PostgresDaoPrivateMessages.cpp
            persistence/PostgresDaoGroupMessages.cpp
            persistence/PostgresDaoDialogs.cpp
            persistence/PostgresDaoGroups.cpp
            persistence/DistLock.cpp
            persistence/RedisMgr.cpp
            persistence/ChatOutboxService.cpp
            persistence/RedisRelationBootstrapCache.cpp
            persistence/RedisOnlineRouteStore.cpp
    )
endif()
target_link_libraries(ChatPersistenceCore PUBLIC
    ChatConfigCore
    ${MEMOCHAT_MONGOC_TARGET}
)
target_link_libraries(ChatPersistenceCore PRIVATE
    ${MEMOCHAT_BSON_TARGET}
    ${MEMOCHAT_POSTGRES_TARGET}
    OpenSSL::Crypto
)

add_library(ChatMessagingCore STATIC
    ${CHATSERVER_MESSAGING_SOURCES}
)
chatserver_core_target(ChatMessagingCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatMessagingCore
        MODULES
            memochat.chat.async_event_dispatcher_algorithms=messaging/cxx_modules/AsyncEventDispatcher.cppm
            memochat.chat.kafka_async_event_bus_algorithms=messaging/cxx_modules/KafkaAsyncEventBus.cppm
            memochat.chat.messaging_envelope_algorithms=messaging/cxx_modules/MessagingEnvelope.cppm
            memochat.chat.messaging_config_algorithms=messaging/cxx_modules/MessagingConfig.cppm
            memochat.chat.inline_task_bus_algorithms=messaging/cxx_modules/InlineTaskBus.cppm
            memochat.chat.rabbitmq_task_bus_algorithms=messaging/cxx_modules/RabbitMqTaskBus.cppm
            memochat.chat.redis_async_event_bus_algorithms=messaging/cxx_modules/RedisAsyncEventBus.cppm
        CONSUMER_SOURCES
            messaging/AsyncEventDispatcher.cpp
            messaging/ChatAsyncEvent.cpp
            messaging/ChatTaskEnvelope.cpp
            messaging/InlineTaskBus.cpp
            messaging/KafkaAsyncEventBus.cpp
            messaging/KafkaConfig.cpp
            messaging/RabbitMqConfig.cpp
            messaging/RabbitMqTaskBus.cpp
            messaging/RedisAsyncEventBus.cpp
    )
endif()
target_link_libraries(ChatMessagingCore PUBLIC
    ChatAppCore
    ChatConfigCore
    ChatPersistenceCore
)
target_link_libraries(ChatMessagingCore PRIVATE
    ChatClientCore
)
if(MEMOCHAT_ENABLE_KAFKA)
    target_link_libraries(ChatMessagingCore PRIVATE ${MEMOCHAT_RDKAFKA_TARGET})
endif()

add_library(ChatTransportCore STATIC
    ${CHATSERVER_TRANSPORT_SOURCES}
)
chatserver_core_target(ChatTransportCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatTransportCore
        MODULES
            memochat.chat.msg_node_algorithms=transport/cxx_modules/MsgNode.cppm
        CONSUMER_SOURCES
            transport/MsgNode.cpp
    )
endif()
target_link_libraries(ChatTransportCore PUBLIC
    ChatAppCore
    ChatConfigCore
    ChatInfrastructureCore
)
target_compile_definitions(ChatTransportCore PUBLIC
    MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER=${MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER_COMPILED}
)
if(MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER_COMPILED)
    target_sources(ChatTransportCore PRIVATE
        transport/LibwebsocketsWebTransportProvider.cpp
    )
    target_link_libraries(ChatTransportCore PRIVATE
        ${MEMOCHAT_LWS_WEBTRANSPORT_TARGET}
    )
endif()

add_library(ChatOrchestrationCore STATIC
    ${CHATSERVER_ORCHESTRATION_SOURCES}
)
chatserver_core_target(ChatOrchestrationCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatOrchestrationCore
        MODULES
            memochat.chat.handler_registrar_algorithms=domain/orchestration/cxx_modules/ChatHandlerRegistrar.cppm
            memochat.chat.runtime_composition_algorithms=domain/orchestration/cxx_modules/ChatRuntimeComposition.cppm
        CONSUMER_SOURCES
            domain/orchestration/ChatHandlerRegistrars.cpp
            domain/orchestration/ChatRuntimeComposition.cpp
    )
endif()

add_library(ChatUserSupportCore STATIC
    ${CHATSERVER_USER_SUPPORT_SOURCES}
)
chatserver_core_target(ChatUserSupportCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatUserSupportCore
        MODULES
            memochat.chat.user_profile_algorithms=domain/users/cxx_modules/UserProfile.cppm
            memochat.chat.user_support_algorithms=domain/users/cxx_modules/UserSupport.cppm
        CONSUMER_SOURCES
            domain/users/ChatUserProfileDto.cpp
            domain/users/ChatUserSupport.cpp
    )
endif()
target_link_libraries(ChatUserSupportCore PUBLIC
    ChatPersistenceCore
)

add_library(ChatSessionCore STATIC
    ${CHATSERVER_SESSION_SOURCES}
)
chatserver_core_target(ChatSessionCore)
target_link_libraries(ChatSessionCore PUBLIC
    ChatAppCore
    ChatConfigCore
    ChatMessagingCore
    ChatPersistenceCore
    ChatTransportCore
    ChatUserSupportCore
)

add_library(ChatDeliveryPayloadCore STATIC
    ${CHATSERVER_DELIVERY_PAYLOAD_SOURCES}
)
chatserver_core_target(ChatDeliveryPayloadCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatDeliveryPayloadCore
        MODULES
            memochat.chat.delivery_task_payload_algorithms=domain/delivery/cxx_modules/DeliveryTaskPayload.cppm
        CONSUMER_SOURCES
            domain/delivery/MessageDeliveryTaskPayload.cpp
    )
endif()

add_library(ChatRelationCore STATIC
    ${CHATSERVER_RELATION_SOURCES}
)
chatserver_core_target(ChatRelationCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatRelationCore
        MODULES
            memochat.chat.relation_internal_grpc_service_algorithms=domain/relation/cxx_modules/ChatRelationInternalGrpcService.cppm
            memochat.chat.relation_service_algorithms=domain/relation/cxx_modules/ChatRelationService.cppm
            memochat.chat.service_factory_algorithms=domain/cxx_modules/ServiceFactory.cppm
        CONSUMER_SOURCES
            domain/relation/ChatRelationInternalGrpcService.cpp
            domain/relation/ChatRelationService.cpp
            domain/relation/RelationServiceFactory.cpp
    )
endif()
target_link_libraries(ChatRelationCore PUBLIC
    ChatAppCore
    ChatDeliveryPayloadCore
    ChatPersistenceCore
    ChatUserSupportCore
)

add_library(ChatRelationSessionAdapterCore STATIC
    ${CHATSERVER_RELATION_SESSION_ADAPTER_SOURCES}
)
chatserver_core_target(ChatRelationSessionAdapterCore)
target_link_libraries(ChatRelationSessionAdapterCore PUBLIC
    ChatRelationCore
    ChatTransportCore
)

add_library(ChatMessageCore STATIC
    ${CHATSERVER_MESSAGE_SOURCES}
)
chatserver_core_target(ChatMessageCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatMessageCore
        MODULES
            memochat.chat.message_internal_grpc_service_algorithms=domain/message/cxx_modules/ChatMessageInternalGrpcService.cppm
            memochat.chat.group_response_algorithms=domain/message/cxx_modules/GroupResponse.cppm
            memochat.chat.message_delivery_algorithms=domain/message/cxx_modules/MessageDelivery.cppm
            memochat.chat.service_factory_algorithms=domain/cxx_modules/ServiceFactory.cppm
        CONSUMER_SOURCES
            domain/message/ChatMessageInternalGrpcService.cpp
            domain/message/GroupResponseFormatter.cpp
            domain/message/MessageDeliveryPayload.cpp
            domain/message/MessageServiceFactory.cpp
    )
endif()
target_link_libraries(ChatMessageCore PUBLIC
    ChatAppCore
    ChatClientCore
    ChatConfigCore
    ChatDeliveryPayloadCore
    ChatMessagingCore
    ChatPersistenceCore
    ChatRelationCore
    ChatTransportCore
)

add_library(ChatDeliveryCore STATIC
    ${CHATSERVER_DELIVERY_SOURCES}
)
chatserver_core_target(ChatDeliveryCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatDeliveryCore
        MODULES
            memochat.chat.delivery_runtime_algorithms=domain/delivery/cxx_modules/DeliveryRuntime.cppm
            memochat.chat.task_dispatcher_algorithms=domain/delivery/cxx_modules/TaskDispatcher.cppm
        CONSUMER_SOURCES
            domain/delivery/ChatDeliveryRuntime.cpp
            domain/delivery/TaskDispatcher.cpp
    )
endif()
target_link_libraries(ChatDeliveryCore PUBLIC
    ChatAppCore
    ChatDeliveryPayloadCore
    ChatMessagingCore
    ChatPersistenceCore
)

add_library(ChatServerCore STATIC
    app/ChatServerCore.cpp
)
chatserver_core_target(ChatServerCore)
set(CHATSERVER_INTERNAL_CORE_TARGETS
    ChatAppCore
    ChatClientCore
    ChatConfigCore
    ChatDeliveryCore
    ChatDeliveryPayloadCore
    ChatInfrastructureCore
    ChatMessageCore
    ChatMessagingCore
    ChatOrchestrationCore
    ChatPersistenceCore
    ChatRelationCore
    ChatRelationSessionAdapterCore
    ChatSessionCore
    ChatTransportCore
    ChatUserSupportCore
)
if(CMAKE_LINK_GROUP_USING_RESCAN_SUPPORTED)
    string(JOIN "," CHATSERVER_INTERNAL_CORE_LINK_GROUP ${CHATSERVER_INTERNAL_CORE_TARGETS})
    target_link_libraries(ChatServerCore PUBLIC
        "$<LINK_GROUP:RESCAN,${CHATSERVER_INTERNAL_CORE_LINK_GROUP}>"
    )
else()
    target_link_libraries(ChatServerCore PUBLIC
        ${CHATSERVER_INTERNAL_CORE_TARGETS}
    )
endif()

if(WIN32)
    # Environment variables: WINSDK_LIB_UM, WINSDK_LIB_UCRT, VC_TOOLS_LIB
    target_link_directories(ChatServerCore PRIVATE
        "$ENV{WINSDK_LIB_UM}"
        "$ENV{WINSDK_LIB_UCRT}"
        "$ENV{VC_TOOLS_LIB}")
    target_link_libraries(ChatServerCore PRIVATE bcrypt)
endif()

set_target_properties(ChatServerCore PROPERTIES FOLDER "Server/ChatServer")

function(chatserver_link_microservice_cores target_name)
    set(core_targets ${ARGN})
    if(CMAKE_LINK_GROUP_USING_RESCAN_SUPPORTED)
        string(JOIN "," core_link_group ${core_targets})
        target_link_libraries(${target_name} PRIVATE
            "$<LINK_GROUP:RESCAN,${core_link_group}>"
        )
    else()
        target_link_libraries(${target_name} PRIVATE ${core_targets})
    endif()
endfunction()

add_executable(ChatServer
    app/ChatServer.cpp
)

target_include_directories(ChatServer PRIVATE
    ${CHATSERVER_INCLUDE_DIRS}
)

set_target_properties(ChatServer PROPERTIES FOLDER "Server")
memochat_configure_server_target(ChatServer)
target_link_libraries(ChatServer PRIVATE ChatServerCore)
if(MSVC)
    target_compile_options(ChatServer PRIVATE /bigobj /Zm200)
endif()

add_executable(ChatDeliveryWorker
    app/ChatDeliveryWorker.cpp
)

target_include_directories(ChatDeliveryWorker PRIVATE
    ${CHATSERVER_INCLUDE_DIRS}
)

set_target_properties(ChatDeliveryWorker PROPERTIES FOLDER "Server")
memochat_configure_server_target(ChatDeliveryWorker)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatDeliveryWorker
        MODULES
            memochat.chat.delivery_worker_runtime_algorithms=app/cxx_modules/ChatDeliveryWorkerRuntime.cppm
        CONSUMER_SOURCES
            app/ChatDeliveryWorker.cpp
    )
endif()
chatserver_link_microservice_cores(ChatDeliveryWorker
    ChatAppCore
    ChatClientCore
    ChatConfigCore
    ChatDeliveryCore
    ChatDeliveryPayloadCore
    ChatInfrastructureCore
    ChatMessageCore
    ChatMessagingCore
    ChatOrchestrationCore
    ChatPersistenceCore
    ChatRelationCore
    ChatRelationSessionAdapterCore
    ChatSessionCore
    ChatTransportCore
    ChatUserSupportCore
)
if(MSVC)
    target_compile_options(ChatDeliveryWorker PRIVATE /bigobj /Zm200)
endif()

add_executable(ChatRelationQueryService
    app/ChatRelationQueryService.cpp
)

target_include_directories(ChatRelationQueryService PRIVATE
    ${CHATSERVER_INCLUDE_DIRS}
)

set_target_properties(ChatRelationQueryService PROPERTIES FOLDER "Server")
memochat_configure_server_target(ChatRelationQueryService)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatRelationQueryService
        MODULES
            memochat.chat.relation_query_service_runtime_algorithms=app/cxx_modules/ChatRelationQueryServiceRuntime.cppm
        CONSUMER_SOURCES
            app/ChatRelationQueryService.cpp
    )
endif()
chatserver_link_microservice_cores(ChatRelationQueryService
    ChatAppCore
    ChatClientCore
    ChatConfigCore
    ChatDeliveryPayloadCore
    ChatInfrastructureCore
    ChatMessagingCore
    ChatPersistenceCore
    ChatRelationCore
    ChatUserSupportCore
)
if(MSVC)
    target_compile_options(ChatRelationQueryService PRIVATE /bigobj /Zm200)
endif()

add_executable(ChatRelationServiceWorker
    app/ChatRelationServiceWorker.cpp
)

target_include_directories(ChatRelationServiceWorker PRIVATE
    ${CHATSERVER_INCLUDE_DIRS}
)

set_target_properties(ChatRelationServiceWorker PROPERTIES FOLDER "Server")
memochat_configure_server_target(ChatRelationServiceWorker)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatRelationServiceWorker
        MODULES
            memochat.chat.relation_service_worker_runtime_algorithms=app/cxx_modules/ChatRelationServiceWorkerRuntime.cppm
        CONSUMER_SOURCES
            app/ChatRelationServiceWorker.cpp
    )
endif()
chatserver_link_microservice_cores(ChatRelationServiceWorker
    ChatAppCore
    ChatClientCore
    ChatConfigCore
    ChatDeliveryCore
    ChatDeliveryPayloadCore
    ChatInfrastructureCore
    ChatMessagingCore
    ChatPersistenceCore
    ChatRelationCore
    ChatUserSupportCore
)
if(MSVC)
    target_compile_options(ChatRelationServiceWorker PRIVATE /bigobj /Zm200)
endif()

add_executable(ChatMessageService
    app/ChatMessageService.cpp
)

target_include_directories(ChatMessageService PRIVATE
    ${CHATSERVER_INCLUDE_DIRS}
)

set_target_properties(ChatMessageService PROPERTIES FOLDER "Server")
memochat_configure_server_target(ChatMessageService)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(ChatMessageService
        MODULES
            memochat.chat.message_service_runtime_algorithms=app/cxx_modules/ChatMessageServiceRuntime.cppm
        CONSUMER_SOURCES
            app/ChatMessageService.cpp
    )
endif()
chatserver_link_microservice_cores(ChatMessageService
    ChatAppCore
    ChatClientCore
    ChatConfigCore
    ChatDeliveryCore
    ChatDeliveryPayloadCore
    ChatInfrastructureCore
    ChatMessageCore
    ChatMessagingCore
    ChatOrchestrationCore
    ChatPersistenceCore
    ChatRelationCore
    ChatRelationSessionAdapterCore
    ChatSessionCore
    ChatTransportCore
    ChatUserSupportCore
)
if(MSVC)
    target_compile_options(ChatMessageService PRIVATE /bigobj /Zm200)
endif()

# ── Embed Lua scripts for Redis operations ───────────────────────────────────
include(EmbedLuaScripts)
memochat_embed_lua_scripts(
    SCRIPTS
        "${CMAKE_CURRENT_SOURCE_DIR}/persistence/lua/dist_lock_release.lua"
        "${CMAKE_CURRENT_SOURCE_DIR}/persistence/lua/hash_decrement_floor.lua"
        "${CMAKE_CURRENT_SOURCE_DIR}/persistence/lua/repair_online_route.lua"
    OUTPUT_DIR  "${CMAKE_BINARY_DIR}/generated/lua/chat"
    HEADER_NAME "chat_lua_scripts.hpp"
    NAMESPACE   "memochat::chat::lua_scripts"
)
# Make the generated header visible to all ChatServer sources
get_directory_property(_chat_targets BUILDSYSTEM_TARGETS)
foreach(_t ${_chat_targets})
    if(_t MATCHES "^Chat")
        get_target_property(_chat_target_type ${_t} TYPE)
        if(_chat_target_type MATCHES "^(STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY|EXECUTABLE)$")
            target_include_directories(${_t} PRIVATE "${CMAKE_BINARY_DIR}/generated/lua/chat")
        endif()
    endif()
endforeach()
