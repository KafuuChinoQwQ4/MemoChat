cmake_minimum_required(VERSION 3.29)

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
add_subdirectory(cpp/apps/server/core/AIServer                             ${CMAKE_CURRENT_BINARY_DIR}/aiserver)
add_subdirectory(cpp/apps/server/core/VarifyServer                         ${CMAKE_CURRENT_BINARY_DIR}/varifyserver)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/features/chat/cache   ${CMAKE_CURRENT_BINARY_DIR}/client_cache)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/live2d                ${CMAKE_CURRENT_BINARY_DIR}/client_live2d)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/core/media            ${CMAKE_CURRENT_BINARY_DIR}/client_media)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/features/chat/services ${CMAKE_CURRENT_BINARY_DIR}/client_dialog)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/features/moments/parsing ${CMAKE_CURRENT_BINARY_DIR}/client_moments)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/core/network          ${CMAKE_CURRENT_BINARY_DIR}/client_network)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/shared/gateway        ${CMAKE_CURRENT_BINARY_DIR}/client_gateway)
add_subdirectory(cpp/apps/client/desktop/MemoChat-qml/app/connection        ${CMAKE_CURRENT_BINARY_DIR}/client_connection)

# Hook into CTest so "ctest" / "RUN_TESTS" discovers everything.
# Discovery is handled in each subdirectory's CMakeLists.txt via
# gtest_discover_tests(TEST_DISCOVERY_MODE POST_BUILD), which defers
# test registration until the test binary is actually built.
