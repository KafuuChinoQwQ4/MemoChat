cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(MediaPendingAttachmentRunnerTests LANGUAGES CXX)
    enable_testing()
endif()

set(MEMOCHAT_REPO_ROOT "${CMAKE_SOURCE_DIR}")
if(NOT EXISTS "${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/app/coordinators/MediaPendingAttachmentRunner.cpp")
    get_filename_component(MEMOCHAT_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../.." ABSOLUTE)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Concurrent Network Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent Network Widgets)

if(NOT TARGET GTest_TestLib)
    find_package(GTest CONFIG REQUIRED)
    add_library(GTest_TestLib INTERFACE)
    target_link_libraries(GTest_TestLib INTERFACE GTest::gtest GTest::gmock GTest::gtest_main)
endif()

if(NOT TARGET gtest_tests)
    add_custom_target(gtest_tests)
endif()

add_executable(media_pending_attachment_runner_gtest
    MediaPendingAttachmentRunnerTest.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/app/coordinators/MediaPendingAttachmentRunner.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/app/coordinators/MediaPendingAttachmentRunner.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller/ChatPendingSendQueueState.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/UploadedAttachmentDispatchService.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/media/MediaUploadRequest.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/media/MediaUploadResult.h
)
set_target_properties(media_pending_attachment_runner_gtest PROPERTIES AUTOMOC ON)
set_target_properties(media_pending_attachment_runner_gtest PROPERTIES
    CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(media_pending_attachment_runner_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
)

target_include_directories(media_pending_attachment_runner_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/app/connection
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/app/coordinators
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/controller
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/media
)

add_dependencies(gtest_tests media_pending_attachment_runner_gtest)

include(GoogleTest)
gtest_discover_tests(media_pending_attachment_runner_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(media_coordinator_gtest
    MediaCoordinatorTest.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/app/coordinators/MediaCoordinator.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/app/coordinators/AppCoordinators.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/MessageContentCodec.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/MessageContentCodec.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/media/LocalFilePickerService.h
)
set_target_properties(media_coordinator_gtest PROPERTIES AUTOMOC ON)
set_target_properties(media_coordinator_gtest PROPERTIES
    CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(media_coordinator_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(media_coordinator_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/app/connection
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/app/coordinators
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/shared/media
)

add_dependencies(gtest_tests media_coordinator_gtest)
gtest_discover_tests(media_coordinator_gtest TEST_DISCOVERY_MODE POST_BUILD)

add_executable(session_relation_bootstrap_gtest
    SessionRelationBootstrapTest.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/app/session/SessionRelationBootstrap.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/app/coordinators/AppCoordinators.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/DialogListService.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/DialogListService.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/DialogListEntryBuilder.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/DialogListEntryBuilder.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services/DialogListTypes.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session/userdata.cpp
)
set_target_properties(session_relation_bootstrap_gtest PROPERTIES AUTOMOC ON)
set_target_properties(session_relation_bootstrap_gtest PROPERTIES
    CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(session_relation_bootstrap_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(session_relation_bootstrap_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/app/connection
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/app/coordinators
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/common
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/core/session
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/chat/services
)

add_dependencies(gtest_tests session_relation_bootstrap_gtest)
gtest_discover_tests(session_relation_bootstrap_gtest TEST_DISCOVERY_MODE POST_BUILD)
