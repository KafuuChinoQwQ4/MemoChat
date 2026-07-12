cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ChatServiceTests LANGUAGES CXX)
    enable_testing()
endif()

set(MEMOCHAT_REPO_ROOT "${CMAKE_SOURCE_DIR}")
if(NOT EXISTS "${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/DialogListService.cpp")
    get_filename_component(MEMOCHAT_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../../.." ABSOLUTE)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Widgets)

if(NOT TARGET GTest_TestLib)
    find_package(GTest CONFIG REQUIRED)
    add_library(GTest_TestLib INTERFACE)
    target_link_libraries(GTest_TestLib INTERFACE GTest::gtest GTest::gmock GTest::gtest_main)
endif()

if(NOT TARGET gtest_tests)
    add_custom_target(gtest_tests)
endif()

set(AUTHENTICATED_MEDIA_CACHE_SOURCE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/media/AuthenticatedMediaCache.cpp
)

add_executable(dialog_list_service_gtest
    DialogListServiceTest.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/DialogListEntryBuilder.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/DialogListService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelOrdering.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelMutations.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelState.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelQueries.cpp
    ${AUTHENTICATED_MEDIA_CACHE_SOURCE}
)
set_target_properties(dialog_list_service_gtest PROPERTIES AUTOMOC ON)
set_target_properties(dialog_list_service_gtest PROPERTIES
    CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(dialog_list_service_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)

target_include_directories(dialog_list_service_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/utils
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session
)

add_dependencies(gtest_tests dialog_list_service_gtest)

include(GoogleTest)
gtest_discover_tests(dialog_list_service_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chat_dialog_list_response_service_gtest
    ChatDialogListResponseServiceTest.cpp
    GlobalUrlPrefixStub.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/ChatDialogListResponseService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/DialogListEntryBuilder.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/DialogListService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/ConversationSyncService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/MessageContentCodec.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModel.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelContent.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelMutations.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelPresentation.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelQueries.cpp
    ${AUTHENTICATED_MEDIA_CACHE_SOURCE}
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelOrdering.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelMutations.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelState.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelQueries.cpp
)
set_target_properties(chat_dialog_list_response_service_gtest PROPERTIES AUTOMOC ON)
set_target_properties(chat_dialog_list_response_service_gtest PROPERTIES
    CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(chat_dialog_list_response_service_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)

target_include_directories(chat_dialog_list_response_service_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/utils
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session
)

add_dependencies(gtest_tests chat_dialog_list_response_service_gtest)
gtest_discover_tests(chat_dialog_list_response_service_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chat_protocol_router_gtest
    ChatProtocolRouterTest.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/ChatProtocolRouter.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/ChatProtocolRouter.h
)
set_target_properties(chat_protocol_router_gtest PROPERTIES AUTOMOC ON)
set_target_properties(chat_protocol_router_gtest PROPERTIES
    CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(chat_protocol_router_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(chat_protocol_router_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services
)

add_dependencies(gtest_tests chat_protocol_router_gtest)
gtest_discover_tests(chat_protocol_router_gtest TEST_DISCOVERY_MODE POST_BUILD)

set(UPLOADED_ATTACHMENT_DISPATCH_SERVICE_SOURCE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/UploadedAttachmentDispatchService.cpp
)
set(UPLOADED_ATTACHMENT_DISPATCH_SERVICE_HEADER
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/UploadedAttachmentDispatchService.h
)

if(EXISTS "${UPLOADED_ATTACHMENT_DISPATCH_SERVICE_SOURCE}" AND EXISTS "${UPLOADED_ATTACHMENT_DISPATCH_SERVICE_HEADER}")
    add_executable(uploaded_attachment_dispatch_service_gtest
        UploadedAttachmentDispatchServiceTest.cpp
        ${UPLOADED_ATTACHMENT_DISPATCH_SERVICE_SOURCE}
        ${UPLOADED_ATTACHMENT_DISPATCH_SERVICE_HEADER}
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/ChatDispatchService.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/ChatDispatchService.h
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/MessageContentCodec.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/MessageContentCodec.h
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/media/MediaUploadResult.h
    )
    set_target_properties(uploaded_attachment_dispatch_service_gtest PROPERTIES AUTOMOC ON)
    set_target_properties(uploaded_attachment_dispatch_service_gtest PROPERTIES
        CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_link_libraries(uploaded_attachment_dispatch_service_gtest PRIVATE
        GTest_TestLib
        Qt${QT_VERSION_MAJOR}::Core
    )

    target_include_directories(uploaded_attachment_dispatch_service_gtest PRIVATE
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/media
    )

    add_dependencies(gtest_tests uploaded_attachment_dispatch_service_gtest)
    gtest_discover_tests(uploaded_attachment_dispatch_service_gtest TEST_DISCOVERY_MODE POST_BUILD)
else()
    message(STATUS "uploaded_attachment_dispatch_service_gtest skipped until UploadedAttachmentDispatchService lands")
endif()

set(MESSAGE_MUTATION_COMMAND_SERVICE_SOURCE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/MessageMutationCommandService.cpp
)
set(MESSAGE_MUTATION_COMMAND_SERVICE_HEADER
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/MessageMutationCommandService.h
)

if(EXISTS "${MESSAGE_MUTATION_COMMAND_SERVICE_SOURCE}" AND EXISTS "${MESSAGE_MUTATION_COMMAND_SERVICE_HEADER}")
    add_executable(message_mutation_command_service_gtest
        MessageMutationCommandServiceTest.cpp
        ${MESSAGE_MUTATION_COMMAND_SERVICE_SOURCE}
        ${MESSAGE_MUTATION_COMMAND_SERVICE_HEADER}
    )
    set_target_properties(message_mutation_command_service_gtest PROPERTIES AUTOMOC ON)
    set_target_properties(message_mutation_command_service_gtest PROPERTIES
        CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_link_libraries(message_mutation_command_service_gtest PRIVATE
        GTest_TestLib
        Qt${QT_VERSION_MAJOR}::Core
    )

    target_include_directories(message_mutation_command_service_gtest PRIVATE
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services
    )

    add_dependencies(gtest_tests message_mutation_command_service_gtest)
    gtest_discover_tests(message_mutation_command_service_gtest TEST_DISCOVERY_MODE POST_BUILD)
else()
    message(STATUS "message_mutation_command_service_gtest skipped until MessageMutationCommandService lands")
endif()

set(INCOMING_MESSAGE_ROUTER_SOURCE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/IncomingMessageRouter.cpp
)
set(INCOMING_MESSAGE_ROUTER_HEADER
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/IncomingMessageRouter.h
)

if(EXISTS "${INCOMING_MESSAGE_ROUTER_SOURCE}" AND EXISTS "${INCOMING_MESSAGE_ROUTER_HEADER}")
    add_executable(incoming_message_router_gtest
        IncomingMessageRouterTest.cpp
        ${INCOMING_MESSAGE_ROUTER_SOURCE}
        ${INCOMING_MESSAGE_ROUTER_HEADER}
    )
    set_target_properties(incoming_message_router_gtest PROPERTIES AUTOMOC ON)
    set_target_properties(incoming_message_router_gtest PROPERTIES
        CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_link_libraries(incoming_message_router_gtest PRIVATE
        GTest_TestLib
        Qt${QT_VERSION_MAJOR}::Core
    )

    target_include_directories(incoming_message_router_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/session
    )

    add_dependencies(gtest_tests incoming_message_router_gtest)
    gtest_discover_tests(incoming_message_router_gtest TEST_DISCOVERY_MODE POST_BUILD)
else()
    message(STATUS "incoming_message_router_gtest skipped until IncomingMessageRouter lands")
endif()

set(PRIVATE_CHAT_SEND_SERVICE_SOURCE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatSendService.cpp
)
set(PRIVATE_CHAT_SEND_SERVICE_HEADER
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatSendService.h
)

if(EXISTS "${PRIVATE_CHAT_SEND_SERVICE_SOURCE}" AND EXISTS "${PRIVATE_CHAT_SEND_SERVICE_HEADER}")
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Sql)

    add_executable(private_chat_send_service_gtest
        PrivateChatSendServiceTest.cpp
        ${PRIVATE_CHAT_SEND_SERVICE_SOURCE}
        ${PRIVATE_CHAT_SEND_SERVICE_HEADER}
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModel.h
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModel.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelContent.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelMutations.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelPresentation.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelQueries.cpp
        ${AUTHENTICATED_MEDIA_CACHE_SOURCE}
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/ConversationSyncService.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/MessageContentCodec.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelOrdering.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.h
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelMutations.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelQueries.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelState.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/cache/ChatCacheMessageCodec.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/cache/PrivateChatCacheStore.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelOrdering.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.h
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelMutations.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelQueries.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelState.cpp
    )
    set_target_properties(private_chat_send_service_gtest PROPERTIES AUTOMOC ON)
    set_target_properties(private_chat_send_service_gtest PROPERTIES
        CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_link_libraries(private_chat_send_service_gtest PRIVATE
        GTest_TestLib
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Sql
    )

    target_include_directories(private_chat_send_service_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/cache
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/common
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/session
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/shared/utils
    )

    add_dependencies(gtest_tests private_chat_send_service_gtest)
    gtest_discover_tests(private_chat_send_service_gtest TEST_DISCOVERY_MODE POST_BUILD)
else()
    message(STATUS "private_chat_send_service_gtest skipped until PrivateChatSendService lands")
endif()

set(PRIVATE_CHAT_HISTORY_SERVICE_SOURCE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatHistoryService.cpp
)
set(PRIVATE_CHAT_HISTORY_SERVICE_HEADER
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatHistoryService.h
)

if(EXISTS "${PRIVATE_CHAT_HISTORY_SERVICE_SOURCE}" AND EXISTS "${PRIVATE_CHAT_HISTORY_SERVICE_HEADER}")
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Sql)

    add_executable(private_chat_history_service_gtest
        PrivateChatHistoryServiceTest.cpp
        ${PRIVATE_CHAT_HISTORY_SERVICE_SOURCE}
        ${PRIVATE_CHAT_HISTORY_SERVICE_HEADER}
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModel.h
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModel.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelContent.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelMutations.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelPresentation.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelQueries.cpp
        ${AUTHENTICATED_MEDIA_CACHE_SOURCE}
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/ConversationSyncService.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/MessageContentCodec.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/MessagePayloadService.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/cache/ChatCacheMessageCodec.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/cache/PrivateChatCacheStore.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelOrdering.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.h
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelMutations.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelQueries.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelState.cpp
    )
    set_target_properties(private_chat_history_service_gtest PROPERTIES AUTOMOC ON)
    set_target_properties(private_chat_history_service_gtest PROPERTIES
        CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_link_libraries(private_chat_history_service_gtest PRIVATE
        GTest_TestLib
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Sql
    )

    target_include_directories(private_chat_history_service_gtest PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/cache
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/common
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/session
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/shared/utils
    )

    add_dependencies(gtest_tests private_chat_history_service_gtest)
    gtest_discover_tests(private_chat_history_service_gtest TEST_DISCOVERY_MODE POST_BUILD)
else()
    message(STATUS "private_chat_history_service_gtest skipped until PrivateChatHistoryService lands")
endif()

set(PRIVATE_CHAT_HISTORY_REQUEST_SERVICE_SOURCE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatHistoryRequestService.cpp
)
set(PRIVATE_CHAT_HISTORY_REQUEST_SERVICE_HEADER
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatHistoryRequestService.h
)

if(EXISTS "${PRIVATE_CHAT_HISTORY_REQUEST_SERVICE_SOURCE}" AND EXISTS "${PRIVATE_CHAT_HISTORY_REQUEST_SERVICE_HEADER}")
    add_executable(private_chat_history_request_service_gtest
        PrivateChatHistoryRequestServiceTest.cpp
        ${PRIVATE_CHAT_HISTORY_REQUEST_SERVICE_SOURCE}
        ${PRIVATE_CHAT_HISTORY_REQUEST_SERVICE_HEADER}
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModel.h
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModel.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelContent.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelMutations.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelPresentation.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelQueries.cpp
        ${AUTHENTICATED_MEDIA_CACHE_SOURCE}
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/ConversationSyncService.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/MessageContentCodec.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelOrdering.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.h
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelMutations.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelQueries.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelState.cpp
    )
    set_target_properties(private_chat_history_request_service_gtest PROPERTIES AUTOMOC ON)
    set_target_properties(private_chat_history_request_service_gtest PROPERTIES
        CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_link_libraries(private_chat_history_request_service_gtest PRIVATE
        GTest_TestLib
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
    )

    target_include_directories(private_chat_history_request_service_gtest PRIVATE
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/utils
    )

    add_dependencies(gtest_tests private_chat_history_request_service_gtest)
    gtest_discover_tests(private_chat_history_request_service_gtest TEST_DISCOVERY_MODE POST_BUILD)
else()
    message(STATUS "private_chat_history_request_service_gtest skipped until PrivateChatHistoryRequestService lands")
endif()

set(PRIVATE_CHAT_EVENT_SERVICE_SOURCE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatEventService.cpp
)
set(PRIVATE_CHAT_EVENT_SERVICE_HEADER
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatEventService.h
)

if(EXISTS "${PRIVATE_CHAT_EVENT_SERVICE_SOURCE}" AND EXISTS "${PRIVATE_CHAT_EVENT_SERVICE_HEADER}")
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Sql)

    add_executable(private_chat_event_service_gtest
        PrivateChatEventServiceTest.cpp
        ${PRIVATE_CHAT_EVENT_SERVICE_SOURCE}
        ${PRIVATE_CHAT_EVENT_SERVICE_HEADER}
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatReadStatusService.cpp
        ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatResponseService.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatEventServiceInternal.h
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModel.h
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModel.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelContent.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelMutations.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelPresentation.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelQueries.cpp
        ${AUTHENTICATED_MEDIA_CACHE_SOURCE}
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/ConversationSyncService.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/MessageContentCodec.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/MessagePayloadService.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/cache/ChatCacheMessageCodec.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/cache/PrivateChatCacheStore.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/userdata.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelOrdering.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.h
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelMutations.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelQueries.cpp
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelState.cpp
    )
    set_target_properties(private_chat_event_service_gtest PROPERTIES AUTOMOC ON)
    set_target_properties(private_chat_event_service_gtest PROPERTIES
        CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_link_libraries(private_chat_event_service_gtest PRIVATE
        GTest::gmock
        GTest::gtest
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Sql
    )

    target_include_directories(private_chat_event_service_gtest PRIVATE
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/cache
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session
        ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/utils
    )

    add_dependencies(gtest_tests private_chat_event_service_gtest)
    gtest_discover_tests(private_chat_event_service_gtest
        TEST_DISCOVERY_MODE POST_BUILD
        PROPERTIES RESOURCE_LOCK "private_chat_event_service_cache"
    )
else()
    message(STATUS "private_chat_event_service_gtest skipped until PrivateChatEventService lands")
endif()
