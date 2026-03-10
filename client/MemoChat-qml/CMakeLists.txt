cmake_minimum_required(VERSION 3.16)

project(MemoChatQml LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/wd4819 /utf-8)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Sql Qml Quick QuickControls2 Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network Sql Qml Quick QuickControls2 Concurrent)

set(SHARED_CLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../MemoChatShared)

set(SOURCES
    main.cpp
    AppController.cpp
    AppControllerGroupEvents.cpp
    AppControllerMedia.cpp
    AppControllerPrivateEvents.cpp
    AppControllerSession.cpp
    AppControllerState.cpp
    AuthController.cpp
    ChatController.cpp
    ChatDispatchService.cpp
    ClientGateway.cpp
    ConversationSyncService.cpp
    ContactController.cpp
    DialogListService.cpp
    FriendListModel.cpp
    ChatMessageModel.cpp
    SearchResultModel.cpp
    ApplyRequestModel.cpp
    MessageContentCodec.cpp
    PrivateChatCacheStore.cpp
    GroupChatCacheStore.cpp
    ProfileController.cpp
    LocalFilePickerService.cpp
    MediaUploadService.cpp
    MessagePayloadService.cpp
    WindowShapeService.cpp
    ${SHARED_CLIENT_DIR}/imagecropperlabel.cpp
)

set(HEADERS
    AppController.h
    AuthController.h
    ChatController.h
    ChatDispatchService.h
    ClientGateway.h
    ConversationSyncService.h
    ContactController.h
    DialogListService.h
    FriendListModel.h
    ChatMessageModel.h
    SearchResultModel.h
    ApplyRequestModel.h
    MessageContentCodec.h
    MessagePayloadService.h
    PrivateChatCacheStore.h
    GroupChatCacheStore.h
    ProfileController.h
    LocalFilePickerService.h
    MediaUploadService.h
    WindowShapeService.h
    IconPathUtils.h
    ${SHARED_CLIENT_DIR}/imagecropperdialog.h
    ${SHARED_CLIENT_DIR}/imagecropperlabel.h
)

set(RESOURCES
    qml.qrc
    ${SHARED_CLIENT_DIR}/rc.qrc
)

add_executable(MemoChatQml WIN32 MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(MemoChatQml PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SHARED_CLIENT_DIR}
)

target_link_libraries(MemoChatQml PRIVATE
    MemoChatCore
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::QuickControls2
    Qt${QT_VERSION_MAJOR}::Concurrent
)

if(WIN32)
    target_link_libraries(MemoChatQml PRIVATE dwmapi)
endif()

add_custom_command(TARGET MemoChatQml POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${SHARED_CLIENT_DIR}/config.ini
    $<TARGET_FILE_DIR:MemoChatQml>/config.ini
    COMMENT "Copying config.ini for MemoChatQml"
)
