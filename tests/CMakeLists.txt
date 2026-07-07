cmake_minimum_required(VERSION 3.29)

if(NOT DEFINED MEMOCHAT_CXX_STANDARD)
    set(MEMOCHAT_CXX_STANDARD "26" CACHE STRING "C++ standard used by MemoChat targets")
endif()
if(NOT DEFINED MEMOCHAT_QT_CXX_STANDARD)
    set(MEMOCHAT_QT_CXX_STANDARD "23" CACHE STRING "C++ standard used by Qt/QML targets")
endif()

# GTest_TestLib and find_package(GTest) are already defined in the parent
# CMakeLists.txt when BUILD_TESTS=ON. This file only:
#   1. Registers test-group subdirectories (each creates its own executable)
#   2. Hooks them into CTest via GoogleTest

# Aggregate custom target so "build-all" can depend on all tests at once.
add_custom_target(gtest_tests)

# --- Register test groups ---
# Each subdirectory creates an executable and calls add_dependencies(gtest_tests ...).
add_subdirectory(cpp/apps/server/core/common/proto                         ${CMAKE_CURRENT_BINARY_DIR}/proto)
add_subdirectory(cpp/apps/server/core/common/logging                       ${CMAKE_CURRENT_BINARY_DIR}/logging)
add_subdirectory(cpp/apps/server/core/common/json                          ${CMAKE_CURRENT_BINARY_DIR}/json)
add_subdirectory(cpp/apps/server/core/common/auth                          ${CMAKE_CURRENT_BINARY_DIR}/auth)
add_subdirectory(cpp/apps/server/core/common/reflection                    ${CMAKE_CURRENT_BINARY_DIR}/reflection)
add_subdirectory(cpp/apps/server/core/common/runtime                       ${CMAKE_CURRENT_BINARY_DIR}/runtime)
add_subdirectory(cpp/apps/server/core/AccountShared                        ${CMAKE_CURRENT_BINARY_DIR}/accountshared)
add_subdirectory(cpp/apps/server/core/CallService                          ${CMAKE_CURRENT_BINARY_DIR}/callservice)
add_subdirectory(cpp/apps/server/core/AIGatewayService                     ${CMAKE_CURRENT_BINARY_DIR}/aigatewayservice)
add_subdirectory(cpp/apps/server/core/AIServer                             ${CMAKE_CURRENT_BINARY_DIR}/aiserver)
add_subdirectory(cpp/apps/server/core/GateShared                           ${CMAKE_CURRENT_BINARY_DIR}/gateshared)
add_subdirectory(cpp/apps/server/core/VarifyServer                         ${CMAKE_CURRENT_BINARY_DIR}/varifyserver)
add_subdirectory(cpp/apps/server/core/R18Service                           ${CMAKE_CURRENT_BINARY_DIR}/r18service)
add_subdirectory(cpp/apps/server/core/MediaService                         ${CMAKE_CURRENT_BINARY_DIR}/mediaservice)
add_subdirectory(cpp/apps/server/core/MomentsService                       ${CMAKE_CURRENT_BINARY_DIR}/momentsservice)
add_subdirectory(cpp/apps/server/core/ChatServer                           ${CMAKE_CURRENT_BINARY_DIR}/chatserver)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/features/chat/cache   ${CMAKE_CURRENT_BINARY_DIR}/client_cache)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/features/chat/viewmodel ${CMAKE_CURRENT_BINARY_DIR}/client_chat_viewmodel)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/features/chat/controller ${CMAKE_CURRENT_BINARY_DIR}/client_chat_controller)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/features/contact/controller ${CMAKE_CURRENT_BINARY_DIR}/client_contact_controller)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/features/group/controller ${CMAKE_CURRENT_BINARY_DIR}/client_group_controller)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/features/call/controller ${CMAKE_CURRENT_BINARY_DIR}/client_call_controller)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/features/profile/controller ${CMAKE_CURRENT_BINARY_DIR}/client_profile_controller)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/features/auth/viewmodel ${CMAKE_CURRENT_BINARY_DIR}/client_auth_viewmodel)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/features/agent/model   ${CMAKE_CURRENT_BINARY_DIR}/client_agent_model)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/live2d                ${CMAKE_CURRENT_BINARY_DIR}/client_live2d)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/core/media            ${CMAKE_CURRENT_BINARY_DIR}/client_media)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/features/chat/services ${CMAKE_CURRENT_BINARY_DIR}/client_dialog)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/features/moments/parsing ${CMAKE_CURRENT_BINARY_DIR}/client_moments)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/core/network          ${CMAKE_CURRENT_BINARY_DIR}/client_network)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/shared/gateway        ${CMAKE_CURRENT_BINARY_DIR}/client_gateway)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/app/connection        ${CMAKE_CURRENT_BINARY_DIR}/client_connection)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/app/coordinators      ${CMAKE_CURRENT_BINARY_DIR}/client_app_coordinators)

# Hook into CTest so "ctest" / "RUN_TESTS" discovers everything.
# Discovery is handled in each subdirectory's CMakeLists.txt via
# gtest_discover_tests(TEST_DISCOVERY_MODE POST_BUILD), which defers
# test registration until the test binary is actually built.
