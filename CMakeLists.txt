cmake_minimum_required(VERSION 3.16)

project(MemoChat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # Prepend standard library include paths before Qt's /external:W0 so <utility> etc. are found
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /external:I \"D:/VS2026stable/VC/Tools/MSVC/14.50.35717/include\" /external:I \"C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/ucrt\" /external:I \"C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared\" /external:I \"C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um\"")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(BUILD_CLIENT "Build MemoChat client" ON)
option(BUILD_SERVER "Build MemoChat server" ON)

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(BUILD_SERVER)
    add_subdirectory(server)
endif()
