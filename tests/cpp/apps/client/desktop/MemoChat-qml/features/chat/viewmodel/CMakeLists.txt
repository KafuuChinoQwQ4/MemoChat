cmake_minimum_required(VERSION 3.16)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_executable(chat_view_model_gtest
    ChatViewModelTest.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModel.h
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModel.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelContent.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelMutations.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelPresentation.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelQueries.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services/MessageContentCodec.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/viewmodel/ChatViewModel.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.h
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelMutations.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelQueries.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelState.cpp
    GlobalUrlPrefixStub.cpp
)
set_target_properties(chat_view_model_gtest PROPERTIES AUTOMOC ON)
set_target_properties(chat_view_model_gtest PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(chat_view_model_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
)

target_include_directories(chat_view_model_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/model
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/services
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/viewmodel
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/contact/model
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/session
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/shared/utils
)

add_dependencies(gtest_tests chat_view_model_gtest)

include(GoogleTest)
gtest_discover_tests(chat_view_model_gtest TEST_DISCOVERY_MODE POST_BUILD)
