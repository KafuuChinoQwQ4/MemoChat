cmake_minimum_required(VERSION 3.16)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

add_executable(chat_frame_codec_gtest
    ChatFrameCodecTest.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/network/ChatFrameCodec.cpp
)
set_target_properties(chat_frame_codec_gtest PROPERTIES
    CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(chat_frame_codec_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

target_include_directories(chat_frame_codec_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/network
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/common
)

add_dependencies(gtest_tests chat_frame_codec_gtest)

include(GoogleTest)
gtest_discover_tests(chat_frame_codec_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chat_message_dispatcher_group_payload_gtest
    ChatMessageDispatcherGroupPayloadTest.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/network/ChatMessageDispatcherGroupPayload.cpp
)
set_target_properties(chat_message_dispatcher_group_payload_gtest PROPERTIES
    CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(chat_message_dispatcher_group_payload_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

target_include_directories(chat_message_dispatcher_group_payload_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/network
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/common
)

add_dependencies(gtest_tests chat_message_dispatcher_group_payload_gtest)

gtest_discover_tests(chat_message_dispatcher_group_payload_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(chat_message_dispatcher_group_history_gtest
    ChatMessageDispatcherGroupHistoryTest.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/network/ChatMessageDispatcherGroupHistory.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/network/ChatMessageDispatcherGroupPayload.cpp
)
set_target_properties(chat_message_dispatcher_group_history_gtest PROPERTIES
    CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(chat_message_dispatcher_group_history_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

target_include_directories(chat_message_dispatcher_group_history_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/network
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/common
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/session
)

add_dependencies(gtest_tests chat_message_dispatcher_group_history_gtest)

gtest_discover_tests(chat_message_dispatcher_group_history_gtest TEST_DISCOVERY_MODE POST_BUILD)
