cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ProfileControllerTests LANGUAGES CXX)
    enable_testing()
endif()

set(MEMOCHAT_REPO_ROOT "${CMAKE_SOURCE_DIR}")
if(NOT EXISTS "${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/profile/controller/ProfileController.cpp")
    get_filename_component(MEMOCHAT_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../../.." ABSOLUTE)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Widgets Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Widgets Concurrent)

if(NOT TARGET GTest_TestLib)
    find_package(GTest CONFIG REQUIRED)
    add_library(GTest_TestLib INTERFACE)
    target_link_libraries(GTest_TestLib INTERFACE GTest::gtest GTest::gmock GTest::gtest_main)
endif()

if(NOT TARGET gtest_tests)
    add_custom_target(gtest_tests)
endif()

add_executable(profile_controller_gtest
    ProfileControllerTest.cpp
    ClientGatewayHttpMgrStub.cpp
    GlobalUrlPrefixStub.cpp
    LocalFilePickerServiceStub.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/profile/controller/ProfileController.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/profile/controller/ProfileController.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/profile/controller/ProfileRequestPayloads.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/profile/controller/ProfileRequestPayloads.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/usermgr.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/usermgr.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/UserMgrFriends.cpp
)
set_target_properties(profile_controller_gtest PROPERTIES AUTOMOC ON)
set_target_properties(profile_controller_gtest PROPERTIES
    CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(profile_controller_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Concurrent
)

target_include_directories(profile_controller_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/network
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/profile/controller
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/gateway
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/media
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/utils
)

add_dependencies(gtest_tests profile_controller_gtest)

include(GoogleTest)
gtest_discover_tests(profile_controller_gtest TEST_DISCOVERY_MODE POST_BUILD)
