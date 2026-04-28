cmake_minimum_required(VERSION 3.29)

project(MemoChat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(CMAKE_RC_COMPILER NAMES rc.exe
    HINTS "$ENV{WINSDK_BIN}"
          "$ENV{VC_TOOLS_BIN}"
    NO_DEFAULT_PATH)

if(MSVC)
    # MSVC-specific settings
    if(DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES "static")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
    list(PREPEND CMAKE_LIBRARY_PATH "$ENV{VC_TOOLS_LIB}")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Options to control what gets built
option(BUILD_CLIENT "Build MemoChat QML client" ON)
option(BUILD_SERVER "Build all backend servers (GateServer, GateServerHttp2, StatusServer, ChatServer, VarifyServer)" ON)
option(BUILD_OPS "Build MemoOps monitoring platform" ON)
option(BUILD_LOADTEST "Build load test tool" ON)
option(BUILD_TESTS "Build unit tests with Google Test" ON)

# Determine what was already configured via preset (used by start_all_services.bat)
# If the user used a preset that configured only certain things, respect those settings
if(NOT DEFINED CMAKE_PROJECT_INVOKED_WITH_PRESET)
    # Invoked directly without preset
endif()

if(BUILD_TESTS AND NOT BUILD_SERVER)
    set(BUILD_SERVER_SERVICES OFF CACHE BOOL "Build backend service executables" FORCE)
else()
    set(BUILD_SERVER_SERVICES ${BUILD_SERVER} CACHE BOOL "Build backend service executables" FORCE)
endif()

if(BUILD_SERVER OR BUILD_TESTS)
    add_subdirectory(apps/server/core)
endif()

if(BUILD_CLIENT)
    add_subdirectory(apps/client/desktop)
endif()

if(BUILD_OPS)
    add_subdirectory(infra/Memo_ops)
endif()

if(BUILD_LOADTEST)
    add_subdirectory(tools/loadtest/local-loadtest-cpp)
endif()

if(BUILD_TESTS)
    enable_testing()
    # GTest package must be found here because tests/ subdirectories reference
    # GTest_TestLib before they are processed.
    find_package(GTest CONFIG REQUIRED)
    add_library(GTest_TestLib INTERFACE)
    target_link_libraries(GTest_TestLib INTERFACE GTest::gtest GTest::gmock GTest::gtest_main)
    add_subdirectory(tests)
endif()

# Custom target to build everything at once
# Usage: cmake --build <build_dir> --target build-all
# Or: cmake --build <build_dir> --config Release
add_custom_target(build-all
    COMMENT "Building all MemoChat components: servers, client, and MemoOps"
)

if(BUILD_SERVER)
    add_dependencies(build-all
        GateServer
        StatusServer
        ChatServer
        VarifyServer
    )
    if(TARGET GateServerHttp2)
        add_dependencies(build-all GateServerHttp2)
    endif()
endif()

if(BUILD_CLIENT)
    add_dependencies(build-all MemoChatQml)
endif()

if(BUILD_OPS)
    add_dependencies(build-all MemoOpsQml)
endif()

if(BUILD_LOADTEST)
    add_dependencies(build-all memochat_loadtest)
endif()

if(BUILD_TESTS)
    add_dependencies(build-all gtest_tests)
endif()
