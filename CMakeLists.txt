cmake_minimum_required(VERSION 3.29)

project(MemoChat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /external:I \"D:/VS2026stable/VC/Tools/MSVC/14.50.35717/include\" /external:I \"C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/ucrt\" /external:I \"C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared\" /external:I \"C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um\"")
    list(PREPEND CMAKE_LIBRARY_PATH "D:/VS2026stable/VC/Tools/MSVC/14.50.35717/lib/x64")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Options to control what gets built
option(BUILD_CLIENT "Build MemoChat QML client" ON)
option(BUILD_SERVER "Build all backend servers (GateServer, GateServerHttp2, StatusServer, ChatServer, VarifyServer)" ON)
option(BUILD_OPS "Build MemoOps monitoring platform" ON)
option(BUILD_LOADTEST "Build load test tool" OFF)

# Determine what was already configured via preset (used by start_all_services.bat)
# If the user used a preset that configured only certain things, respect those settings
if(NOT DEFINED CMAKE_PROJECT_INVOKED_WITH_PRESET)
    # Invoked directly without preset
endif()

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(BUILD_OPS)
    add_subdirectory(Memo_ops)
endif()

if(BUILD_LOADTEST)
    add_subdirectory(local-loadtest-cpp)
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
