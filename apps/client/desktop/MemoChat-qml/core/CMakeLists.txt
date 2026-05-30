cmake_minimum_required(VERSION 3.14)
project(MemoChatCore LANGUAGES CXX)

if(NOT DEFINED MEMOCHAT_CXX_STANDARD)
    set(MEMOCHAT_CXX_STANDARD "23" CACHE STRING "C++ standard used by MemoChat targets")
endif()
set(CMAKE_CXX_STANDARD ${MEMOCHAT_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
    common/global.cpp
    media/ImageCropperGeometry.cpp
    media/ImageCropperInteraction.cpp
    media/ImageCropperPaint.cpp
    media/imagecropperlabel.cpp
    media/ScreenCaptureWidget.cpp
    media/ScreenCaptureService.cpp
    media/WebcamCaptureDialog.cpp
    network/ChatFrameCodec.cpp
    network/ChatMessageDispatcher.cpp
    network/ChatMessageDispatcherAuth.cpp
    network/ChatMessageDispatcherContacts.cpp
    network/ChatMessageDispatcherPrivate.cpp
    network/ChatMessageDispatcherGroup.cpp
    network/ChatMessageDispatcherSystem.cpp
    network/HttpMgrRequestUtils.cpp
    network/httpmgr.cpp
    network/QuicChatTransport.cpp
    network/QuicChatTransportMsquic.cpp
    network/TelemetryUtils.cpp
    network/tcpmgr.cpp
    session/userdata.cpp
    session/usermgr.cpp
    session/UserMgrApply.cpp
    session/UserMgrFriends.cpp
    session/UserMgrGroupList.cpp
    session/UserMgrGroupMessages.cpp
    session/UserMgrPrivateMessages.cpp
)

set(CORE_HEADERS
    common/global.h
    common/singleton.h
    media/ImageCropperGeometry.h
    media/imagecropperdialog.h
    media/imagecropperlabel.h
    media/ScreenCaptureWidget.h
    media/ScreenCaptureService.h
    media/WebcamCaptureDialog.h
    network/ChatFrameCodec.h
    network/ChatMessageDispatcher.h
    network/HttpMgrRequestUtils.h
    network/httpmgr.h
    network/IChatTransport.h
    network/QuicChatTransport.h
    network/TelemetryUtils.h
    network/tcpmgr.h
    session/UserContactData.h
    session/UserGroupData.h
    session/UserMessageData.h
    session/userdata.h
    session/usermgr.h
)

add_library(MemoChatCore STATIC ${CORE_SOURCES} ${CORE_HEADERS})
set_target_properties(MemoChatCore PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
target_include_directories(MemoChatCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/media
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/session
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
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
