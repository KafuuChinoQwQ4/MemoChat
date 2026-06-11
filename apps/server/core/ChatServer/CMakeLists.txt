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
    clients/StatusGrpcClient.cpp
)

set(CHATSERVER_CONFIG_SOURCES
    config/ChatSessionConfig.cpp
    config/ConfigMgr.cpp
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
)

set(CHATSERVER_PERSISTENCE_SOURCES
    persistence/ChatMessageRepository.cpp
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
    persistence/PostgresPool.cpp
    persistence/RedisRelationBootstrapCache.cpp
    persistence/RedisOnlineRouteStore.cpp
    persistence/RedisMgr.cpp
)

set(CHATSERVER_TRANSPORT_SOURCES
    transport/CServer.cpp
    transport/CSession.cpp
    transport/ChatIngressCoordinator.cpp
    transport/ChatServiceImpl.cpp
    transport/MsgNode.cpp
    transport/QuicChatServer.cpp
    transport/QuicSession.cpp
)

set(CHATSERVER_ORCHESTRATION_SOURCES
    domain/orchestration/ChatHandlerRegistrars.cpp
    domain/orchestration/LogicSystem.cpp
)

set(CHATSERVER_SESSION_SOURCES
    domain/session/ChatSessionService.cpp
    domain/users/ChatUserSupport.cpp
    domain/users/UserMgr.cpp
)

set(CHATSERVER_RELATION_SOURCES
    domain/relation/ChatRelationInternalGrpcService.cpp
    domain/relation/ChatRelationService.cpp
    domain/relation/RelationServiceFactory.cpp
)

set(CHATSERVER_MESSAGE_SOURCES
    domain/message/ChatMessageInternalGrpcService.cpp
    domain/message/GroupMessageService.cpp
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

add_library(ChatAppCore STATIC
    ${CHATSERVER_APP_SOURCES}
)
chatserver_core_target(ChatAppCore)
target_link_libraries(ChatAppCore PUBLIC
    ChatConfigCore
)

add_library(ChatClientCore STATIC
    ${CHATSERVER_CLIENT_SOURCES}
)
chatserver_core_target(ChatClientCore)
target_link_libraries(ChatClientCore PUBLIC
    ChatConfigCore
)

add_library(ChatInfrastructureCore STATIC
    ${CHATSERVER_INFRASTRUCTURE_SOURCES}
)
chatserver_core_target(ChatInfrastructureCore)

add_library(ChatPersistenceCore STATIC
    ${CHATSERVER_PERSISTENCE_SOURCES}
)
chatserver_core_target(ChatPersistenceCore)
target_link_libraries(ChatPersistenceCore PRIVATE
    ChatConfigCore
    ${MEMOCHAT_MONGOCXX_TARGET}
    ${MEMOCHAT_BSONCXX_TARGET}
    ${MEMOCHAT_PQXX_TARGET}
    OpenSSL::Crypto
)

add_library(ChatMessagingCore STATIC
    ${CHATSERVER_MESSAGING_SOURCES}
)
chatserver_core_target(ChatMessagingCore)
target_link_libraries(ChatMessagingCore PUBLIC
    ChatAppCore
    ChatConfigCore
    ChatPersistenceCore
)
if(MEMOCHAT_ENABLE_KAFKA)
    target_link_libraries(ChatMessagingCore PRIVATE ${MEMOCHAT_CPPKAFKA_TARGET})
endif()

add_library(ChatTransportCore STATIC
    ${CHATSERVER_TRANSPORT_SOURCES}
)
chatserver_core_target(ChatTransportCore)
target_link_libraries(ChatTransportCore PUBLIC
    ChatAppCore
    ChatConfigCore
    ChatInfrastructureCore
)

add_library(ChatOrchestrationCore STATIC
    ${CHATSERVER_ORCHESTRATION_SOURCES}
)
chatserver_core_target(ChatOrchestrationCore)

add_library(ChatSessionCore STATIC
    ${CHATSERVER_SESSION_SOURCES}
)
chatserver_core_target(ChatSessionCore)
target_link_libraries(ChatSessionCore PUBLIC
    ChatAppCore
    ChatConfigCore
    ChatPersistenceCore
    ChatMessagingCore
    ChatTransportCore
)

add_library(ChatRelationCore STATIC
    ${CHATSERVER_RELATION_SOURCES}
)
chatserver_core_target(ChatRelationCore)
target_link_libraries(ChatRelationCore PUBLIC
    ChatAppCore
    ChatPersistenceCore
    ChatTransportCore
)

add_library(ChatMessageCore STATIC
    ${CHATSERVER_MESSAGE_SOURCES}
)
chatserver_core_target(ChatMessageCore)
target_link_libraries(ChatMessageCore PUBLIC
    ChatAppCore
    ChatClientCore
    ChatConfigCore
    ChatMessagingCore
    ChatPersistenceCore
    ChatRelationCore
    ChatTransportCore
)

add_library(ChatDeliveryCore STATIC
    ${CHATSERVER_DELIVERY_SOURCES}
)
chatserver_core_target(ChatDeliveryCore)
target_link_libraries(ChatDeliveryCore PUBLIC
    ChatAppCore
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
    ChatInfrastructureCore
    ChatMessageCore
    ChatMessagingCore
    ChatOrchestrationCore
    ChatPersistenceCore
    ChatRelationCore
    ChatSessionCore
    ChatTransportCore
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
target_link_libraries(ChatDeliveryWorker PRIVATE ChatServerCore)
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
target_link_libraries(ChatRelationQueryService PRIVATE ChatServerCore)
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
target_link_libraries(ChatRelationServiceWorker PRIVATE ChatServerCore)
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
target_link_libraries(ChatMessageService PRIVATE ChatServerCore)
if(MSVC)
    target_compile_options(ChatMessageService PRIVATE /bigobj /Zm200)
endif()
