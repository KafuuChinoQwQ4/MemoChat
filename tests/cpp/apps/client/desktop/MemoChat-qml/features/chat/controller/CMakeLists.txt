cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ChatFeatureControllerTests LANGUAGES CXX)
    enable_testing()
endif()

set(MEMOCHAT_REPO_ROOT "${CMAKE_SOURCE_DIR}")
if(NOT EXISTS "${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureController.cpp")
    get_filename_component(MEMOCHAT_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../../.." ABSOLUTE)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Sql Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql Widgets)

if(NOT TARGET GTest_TestLib)
    find_package(GTest CONFIG REQUIRED)
    add_library(GTest_TestLib INTERFACE)
    target_link_libraries(GTest_TestLib INTERFACE GTest::gtest GTest::gmock GTest::gtest_main)
endif()

if(NOT TARGET gtest_tests)
    add_custom_target(gtest_tests)
endif()

add_executable(chat_feature_controller_gtest
    ChatFeatureControllerTest.cpp
    GlobalUrlPrefixStub.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureController.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerDialogDecorations.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerDialogDraftCommands.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerDialogList.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerDialogMeta.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerDialogReadCommands.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerDialogRuntime.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerGroupConversation.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerIncoming.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerPendingSend.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerPrivateConversation.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerPrivateHistory.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerPrivateSend.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureController.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatDialogRuntimeState.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerDialogRuntimeInternal.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatPendingSendQueueState.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/cache/ChatCacheMessageCodec.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/cache/GroupChatCacheStore.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/cache/PrivateChatCacheStore.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModel.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelContent.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelMutations.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelPresentation.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelQueries.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/ChatDialogListResponseService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/ChatDialogSelectionService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/ConversationSyncService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/DialogListEntryBuilder.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/DialogListService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/GroupConversationAckService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/GroupConversationHistoryService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/GroupConversationIncomingService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/GroupConversationMutationService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/GroupConversationService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/GroupConversationServiceInternal.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/IncomingMessageRouter.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/MessageContentCodec.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/MessageMutationCommandService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/MessagePayloadService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatEventService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatReadStatusService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatResponseService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatEventServiceInternal.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatHistoryRequestService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatHistoryService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatSendService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/UploadedAttachmentDispatchService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/viewmodel/ChatViewModel.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelOrdering.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelMutations.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelQueries.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelState.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/UserMgrFriends.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/UserMgrGroupList.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/UserMgrGroupMessages.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/UserMgrPrivateMessages.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/userdata.cpp
)
set_target_properties(chat_feature_controller_gtest PROPERTIES AUTOMOC ON)
set_target_properties(chat_feature_controller_gtest PROPERTIES
    CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(chat_feature_controller_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(chat_feature_controller_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/cache
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/viewmodel
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/media
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/utils
)

add_dependencies(gtest_tests chat_feature_controller_gtest)

include(GoogleTest)
gtest_discover_tests(chat_feature_controller_gtest TEST_DISCOVERY_MODE POST_BUILD)
