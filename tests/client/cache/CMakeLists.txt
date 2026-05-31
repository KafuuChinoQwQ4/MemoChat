cmake_minimum_required(VERSION 3.16)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_executable(chat_cache_message_codec_gtest
    ChatCacheMessageCodecTest.cpp
    ../../../apps/client/desktop/MemoChat-qml/features/chat/cache/ChatCacheMessageCodec.cpp
)
set_target_properties(chat_cache_message_codec_gtest PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(chat_cache_message_codec_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
)

target_include_directories(chat_cache_message_codec_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/features/chat/cache
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/common
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/session
)

add_dependencies(gtest_tests chat_cache_message_codec_gtest)

include(GoogleTest)
gtest_discover_tests(chat_cache_message_codec_gtest TEST_DISCOVERY_MODE POST_BUILD)
