cmake_minimum_required(VERSION 3.14)
project(MemoChatShared LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/wd4819 /utf-8 /wd4038)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(msquic CONFIG QUIET)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Multimedia QUIET)
if(Qt${QT_VERSION_MAJOR}_Multimedia_FOUND)
    set(HAVE_QT_MULTIMEDIA 1)
else()
    set(HAVE_QT_MULTIMEDIA 0)
endif()

set(CORE_SOURCES
    ChatMessageDispatcher.cpp
    global.cpp
    httpmgr.cpp
    QuicChatTransport.cpp
    TelemetryUtils.cpp
    tcpmgr.cpp
    userdata.cpp
    usermgr.cpp
    imagecropperlabel.cpp
    ScreenCaptureWidget.cpp
    ScreenCaptureService.cpp
    WebcamCaptureDialog.cpp
)

set(CORE_HEADERS
    ChatMessageDispatcher.h
    global.h
    httpmgr.h
    IChatTransport.h
    QuicChatTransport.h
    TelemetryUtils.h
    tcpmgr.h
    userdata.h
    usermgr.h
    singleton.h
    imagecropperlabel.h
    ScreenCaptureWidget.h
    ScreenCaptureService.h
    WebcamCaptureDialog.h
)

add_library(MemoChatCore STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(MemoChatCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/server/common
)
target_link_libraries(MemoChatCore PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

if(Qt${QT_VERSION_MAJOR}_Multimedia_FOUND)
    target_link_libraries(MemoChatCore PUBLIC Qt${QT_VERSION_MAJOR}::Multimedia)
    target_compile_definitions(MemoChatCore PUBLIC HAVE_QT_MULTIMEDIA=1)
else()
    target_compile_definitions(MemoChatCore PUBLIC HAVE_QT_MULTIMEDIA=0)
endif()

if(TARGET msquic)
    target_link_libraries(MemoChatCore PUBLIC msquic)
    target_compile_definitions(MemoChatCore PUBLIC MEMOCHAT_HAS_MSQUIC=1)
else()
    target_compile_definitions(MemoChatCore PUBLIC MEMOCHAT_HAS_MSQUIC=0)
endif()
