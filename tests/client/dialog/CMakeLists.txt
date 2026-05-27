cmake_minimum_required(VERSION 3.16)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_executable(dialog_list_service_gtest
    DialogListServiceTest.cpp
    ../../../apps/client/desktop/MemoChat-qml/services/DialogListService.cpp
    ../../../apps/client/desktop/MemoChat-qml/models/FriendListModel.cpp
)
set_target_properties(dialog_list_service_gtest PROPERTIES AUTOMOC ON)

target_link_libraries(dialog_list_service_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
)

target_include_directories(dialog_list_service_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/models
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/services
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/utils
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/common
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/session
)

add_dependencies(gtest_tests dialog_list_service_gtest)

include(GoogleTest)
gtest_discover_tests(dialog_list_service_gtest TEST_DISCOVERY_MODE POST_BUILD)
