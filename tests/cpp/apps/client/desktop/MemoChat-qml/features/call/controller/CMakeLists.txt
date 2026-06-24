cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(CallControllerTests LANGUAGES CXX)
    enable_testing()
endif()

set(MEMOCHAT_REPO_ROOT "${CMAKE_SOURCE_DIR}")
if(NOT EXISTS "${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/call/controller/CallController.cpp")
    get_filename_component(MEMOCHAT_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../../.." ABSOLUTE)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Test Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Test Widgets)

if(NOT TARGET GTest_TestLib)
    find_package(GTest CONFIG REQUIRED)
    add_library(GTest_TestLib INTERFACE)
    target_link_libraries(GTest_TestLib INTERFACE GTest::gtest GTest::gmock GTest::gtest_main)
endif()

if(NOT TARGET gtest_tests)
    add_custom_target(gtest_tests)
endif()

add_executable(call_controller_gtest
    CallControllerTest.cpp
    CallRequestPayloadsTest.cpp
    ClientGatewayStub.cpp
    GlobalUrlPrefixStub.cpp
    CallControllerOpaqueTypes.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/call/controller/CallController.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/call/controller/CallController.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/call/controller/CallRequestPayloads.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/call/controller/CallRequestPayloads.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/call/model/CallSessionModel.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/call/model/CallSessionModel.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/call/bridge/LivekitBridge.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/call/bridge/LivekitBridge.h
)
set_target_properties(call_controller_gtest PROPERTIES AUTOMOC ON)
set_target_properties(call_controller_gtest PROPERTIES
    CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(call_controller_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(call_controller_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/network
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/call/bridge
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/call/controller
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/call/model
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/gateway
)

add_dependencies(gtest_tests call_controller_gtest)

include(GoogleTest)
gtest_discover_tests(call_controller_gtest TEST_DISCOVERY_MODE POST_BUILD)
