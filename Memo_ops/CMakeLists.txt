cmake_minimum_required(VERSION 3.16)

project(MemoOps LANGUAGES CXX)

if(NOT MSVC OR NOT CMAKE_CXX_COMPILER MATCHES [[(^|[\\/])cl(\.exe)?$]])
    message(FATAL_ERROR "Memo_ops requires the MSVC2022 toolchain with cl.exe. Use the 'msvc2022-memo-ops' CMake preset with Qt 6.8.3 msvc2022_64.")
endif()

add_subdirectory(client/MemoOps-qml)
