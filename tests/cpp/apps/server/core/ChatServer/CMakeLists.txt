cmake_minimum_required(VERSION 3.16)

add_executable(chatserver_relation_internal_grpc_service_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation/ChatRelationInternalGrpcService.cpp
    main.cpp
    ChatRelationInternalGrpcServiceTest.cpp
)

target_link_libraries(chatserver_relation_internal_grpc_service_gtest PRIVATE
    GTest_TestLib
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

add_executable(chatserver_message_internal_grpc_service_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message/ChatMessageInternalGrpcService.cpp
    main.cpp
    ChatMessageInternalGrpcServiceTest.cpp
)

target_link_libraries(chatserver_message_internal_grpc_service_gtest PRIVATE
    GTest_TestLib
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
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients/MessageGrpcClient.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message/ChatMessageInternalGrpcService.cpp
    main.cpp
    MessageGrpcClientTest.cpp
)

target_link_libraries(chatserver_message_grpc_client_gtest PRIVATE
    GTest_TestLib
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

add_executable(chatserver_message_delivery_payload_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message/MessageDeliveryPayload.cpp
    main.cpp
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

gtest_discover_tests(chatserver_message_delivery_task_payload_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_envelope_codec_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/app/ChatRuntime.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/config/ConfigMgr.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging/ChatAsyncEvent.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/messaging/ChatTaskEnvelope.cpp
    main.cpp
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

gtest_discover_tests(chatserver_envelope_codec_gtest TEST_DISCOVERY_MODE POST_BUILD)

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

gtest_discover_tests(chatserver_user_profile_dto_gtest TEST_DISCOVERY_MODE POST_BUILD)

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
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients/RelationQueryGrpcClient.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation/ChatRelationInternalGrpcService.cpp
    main.cpp
    RelationQueryGrpcClientTest.cpp
)

target_link_libraries(chatserver_relation_query_grpc_client_gtest PRIVATE
    GTest_TestLib
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

add_executable(chatserver_relation_grpc_client_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients/RelationGrpcClient.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation/ChatRelationInternalGrpcService.cpp
    main.cpp
    RelationGrpcClientTest.cpp
)

target_link_libraries(chatserver_relation_grpc_client_gtest PRIVATE
    GTest_TestLib
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

gtest_discover_tests(chatserver_relation_service_factory_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chatserver_relation_query_service_factory_gtest
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients/RelationQueryGrpcClient.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients/RelationQueryServiceFactory.cpp
    main.cpp
    RelationQueryServiceFactoryTest.cpp
)

target_link_libraries(chatserver_relation_query_service_factory_gtest PRIVATE
    GTest_TestLib
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
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients/RelationQueryGrpcClient.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients/RelationQueryServiceFactory.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/config/ConfigMgr.cpp
    ${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/config/RelationQueryServiceConfig.cpp
    main.cpp
    RelationQueryRemoteSmokeTest.cpp
)

target_link_libraries(chatserver_relation_query_remote_smoke_gtest PRIVATE
    GTest_TestLib
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
