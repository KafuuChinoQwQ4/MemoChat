cmake_minimum_required(VERSION 3.16)
project(MemoChatLoadTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /utf-8 /await)
    add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00 MEMOCHAT_ENABLE_MSQUIC)
    # Suppress winsock warnings from msquic.h
    add_compile_options(/wd4005 /wd4090)
    # Ensure standard library headers are found (order matters: standard lib BEFORE vcpkg)
    # Environment variables: VC_TOOLS_INCLUDE, WINSDK_INCLUDE
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /external:I \"$ENV{VC_TOOLS_INCLUDE}\" /external:I \"$ENV{WINSDK_INCLUDE}/ucrt\" /external:I \"$ENV{WINSDK_INCLUDE}/shared\" /external:I \"$ENV{WINSDK_INCLUDE}/um\"")
else()
    add_compile_options(-Wall -Wextra -Wpedantic -DMEMOCHAT_ENABLE_MSQUIC)
endif()

# ---- Dependencies ----
# spdlog (header-only, find the installed copy from vcpkg)
find_package(spdlog REQUIRED)

# msquic (system-installed msquic.h / msquic.lib from vcpkg)
find_path(MSQUIC_H msquic.h)
find_library(MSQUIC_LIB msquic)
if(NOT MSQUIC_H)
    # Fallback: look in vcpkg-installed locations (relative to project root)
    set(MSQUIC_H "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg_installed/x64-windows/include")
    set(MSQUIC_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg_installed/x64-windows/lib/msquic.lib")
endif()

if(NOT EXISTS "${MSQUIC_H}/msquic.h")
    message(WARNING "msquic.h not found. Set MSQUIC_H to the msquic include directory.")
endif()

# ---- Source files ----
set(CORE_SRCS
    src/core/config.cpp
    src/core/logger.cpp
    src/core/metrics.cpp
)

set(CLIENT_SRCS
    src/clients/client.cpp
    src/clients/quic_client.cpp
    src/clients/quic_lib.cpp
)

set(SCENARIO_SRCS
    src/scenarios/http_scenario.cpp
    src/scenarios/http2_scenario.cpp
    src/scenarios/http3_scenario.cpp
    src/scenarios/tcp_scenario.cpp
    src/scenarios/quic_scenario.cpp
)

set(MONITOR_SRCS
    src/monitor/monitor.cpp
)

set(RUNNER_SRCS
    src/runner.cpp
)

set(MAIN_SRC
    src/main.cpp
)

# ---- Executable ----
add_executable(memochat_loadtest
    ${CORE_SRCS}
    ${CLIENT_SRCS}
    ${SCENARIO_SRCS}
    ${MONITOR_SRCS}
    ${RUNNER_SRCS}
    ${MAIN_SRC}
)

target_include_directories(memochat_loadtest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${MSQUIC_H}
)

if(MSQUIC_LIB AND EXISTS "${MSQUIC_LIB}")
    target_link_libraries(memochat_loadtest PRIVATE
        "${MSQUIC_LIB}"
        ws2_32 winhttp winhttp
    )
else()
    target_link_libraries(memochat_loadtest PRIVATE
        ws2_32 winhttp
        optimized ws2_32
        debug ws2_32
    )
endif()

target_link_libraries(memochat_loadtest PRIVATE
    spdlog::spdlog
    ntdll # for GetSystemTimes on Windows
)

# ---- Copy default config ----
add_custom_command(TARGET memochat_loadtest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/config.example.json
        $<TARGET_FILE_DIR:memochat_loadtest>/config.json
    COMMENT "Copy config.example.json to output directory"
)

# ---- Install ----
install(TARGETS memochat_loadtest DESTINATION bin)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/config.example.json
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    DESTINATION share/memochat-loadtest
)
