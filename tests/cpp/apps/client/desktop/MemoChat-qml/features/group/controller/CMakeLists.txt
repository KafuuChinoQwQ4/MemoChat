cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(GroupControllerTests LANGUAGES CXX)
    enable_testing()
endif()

set(MEMOCHAT_REPO_ROOT "${CMAKE_SOURCE_DIR}")
if(NOT EXISTS "${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupController.cpp")
    get_filename_component(MEMOCHAT_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../../.." ABSOLUTE)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Concurrent Network Test Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent Network Test Widgets)

if(NOT TARGET GTest_TestLib)
    find_package(GTest CONFIG REQUIRED)
    add_library(GTest_TestLib INTERFACE)
    target_link_libraries(GTest_TestLib INTERFACE GTest::gtest GTest::gmock GTest::gtest_main)
endif()

if(NOT TARGET gtest_tests)
    add_custom_target(gtest_tests)
endif()

add_executable(group_controller_gtest
    GroupControllerTest.cpp
    GroupRequestPayloadsTest.cpp
    ConversationSyncServiceStub.cpp
    GlobalUrlPrefixStub.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupController.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupController.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementEffectApplier.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementEffectApplier.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementEffectPorts.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementEventService.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementResponseService.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupRequestPayloads.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupRequestPayloads.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/ConversationSyncService.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModel.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelMutations.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelQueries.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model/FriendListModelState.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/userdata.cpp
)
set_target_properties(group_controller_gtest PROPERTIES AUTOMOC ON)
set_target_properties(group_controller_gtest PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(group_controller_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(group_controller_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/model
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/contact/model
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/media
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/utils
)

add_dependencies(gtest_tests group_controller_gtest)

include(GoogleTest)
gtest_discover_tests(group_controller_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(group_management_event_service_gtest
    GroupManagementEventServiceTest.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementEventService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementEventService.h
)
set_target_properties(group_management_event_service_gtest PROPERTIES AUTOMOC ON)
set_target_properties(group_management_event_service_gtest PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(group_management_event_service_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
)

target_include_directories(group_management_event_service_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller
)

add_dependencies(gtest_tests group_management_event_service_gtest)
gtest_discover_tests(group_management_event_service_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(group_management_effect_applier_gtest
    GroupManagementEffectApplierTest.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementEffectApplier.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementEffectApplier.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementEffectPorts.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementEventService.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementResponseService.h
)
set_target_properties(group_management_effect_applier_gtest PROPERTIES AUTOMOC ON)
set_target_properties(group_management_effect_applier_gtest PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(group_management_effect_applier_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(group_management_effect_applier_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller
)

add_dependencies(gtest_tests group_management_effect_applier_gtest)
gtest_discover_tests(group_management_effect_applier_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(group_management_response_service_gtest
    GroupManagementResponseServiceTest.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementResponseService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementResponseService.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupRequestPayloads.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupRequestPayloads.h
)
set_target_properties(group_management_response_service_gtest PROPERTIES AUTOMOC ON)
set_target_properties(group_management_response_service_gtest PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(group_management_response_service_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(group_management_response_service_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller
)

add_dependencies(gtest_tests group_management_response_service_gtest)
gtest_discover_tests(group_management_response_service_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(group_response_error_service_gtest
    GroupResponseErrorServiceTest.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupResponseErrorService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupResponseErrorService.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementResponseService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupManagementResponseService.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupRequestPayloads.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller/GroupRequestPayloads.h
)
set_target_properties(group_response_error_service_gtest PROPERTIES AUTOMOC ON)
set_target_properties(group_response_error_service_gtest PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(group_response_error_service_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(group_response_error_service_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/group/controller
)

add_dependencies(gtest_tests group_response_error_service_gtest)
gtest_discover_tests(group_response_error_service_gtest TEST_DISCOVERY_MODE POST_BUILD)
