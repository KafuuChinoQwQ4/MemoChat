cmake_minimum_required(VERSION 3.29)

# GTest_TestLib and find_package(GTest) are already defined in the parent
# CMakeLists.txt when BUILD_TESTS=ON. This file only:
#   1. Registers test-group subdirectories (each creates its own executable)
#   2. Hooks them into CTest via GoogleTest

# Aggregate custom target so "build-all" can depend on all tests at once.
add_custom_target(gtest_tests)

# --- Register test groups ---
# Each subdirectory creates an executable and calls add_dependencies(gtest_tests ...).
add_subdirectory(../apps/server/core/common/proto/tests   ${CMAKE_CURRENT_BINARY_DIR}/proto)
add_subdirectory(../apps/server/core/common/logging/tests ${CMAKE_CURRENT_BINARY_DIR}/logging)
add_subdirectory(../apps/server/core/AIServer/tests       ${CMAKE_CURRENT_BINARY_DIR}/aiserver)

# Hook into CTest so "ctest" / "RUN_TESTS" discovers everything.
# Discovery is handled in each subdirectory's CMakeLists.txt via
# gtest_discover_tests(TEST_DISCOVERY_MODE POST_BUILD), which defers
# test registration until the test binary is actually built.
