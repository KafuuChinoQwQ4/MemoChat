cmake_minimum_required(VERSION 3.16)

project(MemoChat LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(BUILD_CLIENT "Build MemoChat client" ON)
option(BUILD_SERVER "Build MemoChat server" ON)

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(BUILD_SERVER)
    add_subdirectory(server)
endif()
