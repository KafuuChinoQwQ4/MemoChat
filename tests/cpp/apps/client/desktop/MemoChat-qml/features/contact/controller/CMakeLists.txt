cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ContactControllerTests LANGUAGES CXX)
    enable_testing()
endif()

set(MEMOCHAT_REPO_ROOT "${CMAKE_SOURCE_DIR}")
if(NOT EXISTS "${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/controller/ContactController.cpp")
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

add_executable(contact_controller_gtest
    ContactControllerTest.cpp
    ContactEventServiceTest.cpp
    ContactRequestPayloadsTest.cpp
    ClientGatewayStub.cpp
    GlobalUrlPrefixStub.cpp
    ContactControllerOpaqueTypes.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/controller/ContactController.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/controller/ContactController.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/controller/ContactEventService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/controller/ContactEventService.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/controller/ContactRequestPayloads.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/controller/ContactRequestPayloads.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/ApplyRequestModel.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/ApplyRequestModel.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelOrdering.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelMutations.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelQueries.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelState.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/SearchResultModel.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/SearchResultModel.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/userdata.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/usermgr.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/usermgr.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/UserMgrFriends.cpp
)
set_target_properties(contact_controller_gtest PROPERTIES AUTOMOC ON)
set_target_properties(contact_controller_gtest PROPERTIES
    CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(contact_controller_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(contact_controller_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/network
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/controller
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/gateway
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/utils
)

add_dependencies(gtest_tests contact_controller_gtest)

include(GoogleTest)
gtest_discover_tests(contact_controller_gtest TEST_DISCOVERY_MODE POST_BUILD)
