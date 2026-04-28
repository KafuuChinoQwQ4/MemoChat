cmake_minimum_required(VERSION 3.16)

project(MemoOpsQml LANGUAGES CXX)

if(NOT MSVC OR NOT CMAKE_CXX_COMPILER MATCHES [[(^|[\\/])cl(\.exe)?$]])
    message(FATAL_ERROR "MemoOpsQml supports only the MSVC toolchain. MinGW/g++ is not supported with the current Qt msvc2022_64 kit.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(MSVC)
    add_compile_options(/wd4819 /utf-8)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Network Qml Quick QuickControls2 Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Network Qml Quick QuickControls2 Charts)

set(SOURCES
    main.cpp
    OpsApiClient.cpp
)

set(HEADERS
    OpsApiClient.h
)

set(RESOURCES
    qml.qrc
)

add_executable(MemoOpsQml WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

target_link_libraries(MemoOpsQml PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::QuickControls2
    Qt${QT_VERSION_MAJOR}::Charts
)

target_include_directories(MemoOpsQml PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(TARGET MemoOpsQml POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/infra/Memo_ops/config/memoops-qml.ini
    $<TARGET_FILE_DIR:MemoOpsQml>/memoops-qml.ini
    COMMENT "Copying memoops-qml.ini"
)
