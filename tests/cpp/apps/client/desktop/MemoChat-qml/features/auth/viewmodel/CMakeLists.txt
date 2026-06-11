cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(AuthViewModelTests LANGUAGES CXX)
    enable_testing()
endif()

set(MEMOCHAT_REPO_ROOT "${CMAKE_SOURCE_DIR}")
if(NOT EXISTS "${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/auth/viewmodel/AuthViewModel.cpp")
    get_filename_component(MEMOCHAT_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../../.." ABSOLUTE)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

if(NOT TARGET GTest_TestLib)
    find_package(GTest CONFIG REQUIRED)
    add_library(GTest_TestLib INTERFACE)
    target_link_libraries(GTest_TestLib INTERFACE GTest::gtest GTest::gmock GTest::gtest_main)
endif()

if(NOT TARGET gtest_tests)
    add_custom_target(gtest_tests)
endif()

add_executable(auth_view_model_gtest
    AuthViewModelTest.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/auth/viewmodel/AuthViewModel.cpp
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/auth/viewmodel/AuthViewModel.h
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/auth/model/AuthState.h
)
set_target_properties(auth_view_model_gtest PROPERTIES AUTOMOC ON)
set_target_properties(auth_view_model_gtest PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(auth_view_model_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
)

target_include_directories(auth_view_model_gtest PRIVATE
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/auth
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/auth/model
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/auth/service
    ${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/features/auth/viewmodel
)

add_dependencies(gtest_tests auth_view_model_gtest)

include(GoogleTest)
gtest_discover_tests(auth_view_model_gtest TEST_DISCOVERY_MODE POST_BUILD)
