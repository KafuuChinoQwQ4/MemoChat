cmake_minimum_required(VERSION 3.16)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

add_executable(app_chat_connection_policy_gtest
    AppChatConnectionPolicyTest.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/app/connection/AppChatConnectionPolicy.cpp
)
set_target_properties(app_chat_connection_policy_gtest PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(app_chat_connection_policy_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

target_include_directories(app_chat_connection_policy_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/app
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/app/connection
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/common
)

add_dependencies(gtest_tests app_chat_connection_policy_gtest)

include(GoogleTest)
gtest_discover_tests(app_chat_connection_policy_gtest TEST_DISCOVERY_MODE POST_BUILD)
