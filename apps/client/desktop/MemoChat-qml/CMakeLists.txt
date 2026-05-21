cmake_minimum_required(VERSION 3.16)

project(MemoChatQml LANGUAGES CXX)

get_filename_component(MEMOCHAT_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${MEMOCHAT_REPO_ROOT}/cmake")

# Restore Windows SDK library paths that the vcpkg toolchain stripped
# Environment variables: VC_TOOLS_LIB, WINSDK_LIB_UM, WINSDK_LIB_UCRT
if(WIN32 AND MSVC)
    link_directories(
        "$ENV{VC_TOOLS_LIB}"
        "$ENV{WINSDK_LIB_UM}"
        "$ENV{WINSDK_LIB_UCRT}"
    )
    # Also add as linker flags for robustness
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LIBPATH:\"$ENV{VC_TOOLS_LIB}\" /LIBPATH:\"$ENV{WINSDK_LIB_UM}\"")
endif()

if(NOT DEFINED MEMOCHAT_CXX_STANDARD)
    set(MEMOCHAT_CXX_STANDARD "23" CACHE STRING "C++ standard used by MemoChat targets")
endif()
set(CMAKE_CXX_STANDARD ${MEMOCHAT_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MEMOCHAT_ENABLE_LIVE2D_NATIVE "Enable opt-in Live2D Cubism Native integration for MemoChatQml" OFF)
set(MEMOCHAT_LIVE2D_SDK_ROOT "" CACHE PATH "Local Live2D Cubism SDK root used when MEMOCHAT_ENABLE_LIVE2D_NATIVE is ON")
file(TO_CMAKE_PATH "${MEMOCHAT_LIVE2D_SDK_ROOT}" MEMOCHAT_LIVE2D_SDK_ROOT_NORMALIZED)
set(MEMOCHAT_LIVE2D_NATIVE_AVAILABLE OFF)

if(MEMOCHAT_ENABLE_LIVE2D_NATIVE)
    if(NOT MEMOCHAT_LIVE2D_SDK_ROOT_NORMALIZED)
        message(FATAL_ERROR
            "MEMOCHAT_ENABLE_LIVE2D_NATIVE is ON, but MEMOCHAT_LIVE2D_SDK_ROOT is empty. "
            "Install/extract the Live2D Cubism SDK for Native outside the repo, for example "
            "/data/third_party/live2d/CubismSdkForNative-current, then configure with "
            "-DMEMOCHAT_LIVE2D_SDK_ROOT=/path/to/CubismSdkForNative.")
    endif()
    set(Live2DCubism_ROOT "${MEMOCHAT_LIVE2D_SDK_ROOT_NORMALIZED}" CACHE PATH
        "Local Live2D Cubism SDK for Native root" FORCE)
    find_package(Live2DCubism REQUIRED)
    set(MEMOCHAT_LIVE2D_NATIVE_AVAILABLE ON)
    message(STATUS "Live2D Cubism Native enabled from ${Live2DCubism_ROOT_DIR}")
else()
    message(STATUS "Live2D Cubism Native disabled; Live2DRenderItem will show model error diagnostics")
endif()

if(MSVC)
    add_compile_options(/wd4819 /utf-8)
    # Allow standard library headers through /external:W0 so C++23 features are visible
    add_compile_options(/wd4819 /utf-8 /wd4038)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Sql Qml Quick QuickControls2 Concurrent WebEngineQuick)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network Sql Qml Quick QuickControls2 Concurrent WebEngineQuick)
find_package(Qt${QT_VERSION_MAJOR} QUIET COMPONENTS Multimedia)
find_package(Qt${QT_VERSION_MAJOR} QUIET COMPONENTS VirtualKeyboard)

set(SHARED_CLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../MemoChatShared)

set(SOURCES
    main.cpp
    AppController.cpp
    AppControllerAuth.cpp
    AppControllerAuthResponses.cpp
    AppControllerConnection.cpp
    AppControllerContactEvents.cpp
    AppControllerDialogListEvents.cpp
    AppControllerDialogState.cpp
    AppControllerDialogCommands.cpp
    AppControllerGroupCommands.cpp
    AppControllerModels.cpp
    AppControllerNavigation.cpp
    AppControllerProperties.cpp
    AppControllerProfileCommands.cpp
    AppControllerSelection.cpp
    AppCoordinators.cpp
    AppControllerCall.cpp
    AppControllerGroupEvents.cpp
    AppControllerMedia.cpp
    AppControllerPrivateEvents.cpp
    AppControllerSession.cpp
    AppControllerState.cpp
    AuthController.cpp
    CallController.cpp
    CallSessionModel.cpp
    LivekitBridge.cpp
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
    TransportSelector.cpp
    MessageContentCodec.cpp
    PrivateChatCacheStore.cpp
    GroupChatCacheStore.cpp
    ProfileController.cpp
    LocalFilePickerService.cpp
    MediaUploadService.cpp
    MessagePayloadService.cpp
    WindowShapeService.cpp
    MomentsController.cpp
    MomentsModel.cpp
    AgentController.cpp
    AgentControllerGame.cpp
    AgentControllerKnowledge.cpp
    AgentControllerStream.cpp
    AgentMessageModel.cpp
    PetAssetSettings.cpp
    PetController.cpp
    PetModel.cpp
    PetSpeechSynthesizer.cpp
    Live2DAvatarOpenGLRenderer.cpp
    Live2DAsset.cpp
    Live2DCoreRenderer.cpp
    Live2DOfficialOpenGLRenderer.cpp
    Live2DPlaceholderRenderer.cpp
    Live2DRenderItem.cpp
    R18Controller.cpp
    R18ListModel.cpp
)

set(HEADERS
    ClientWinCompat.h
    AppController.h
    AppCoordinators.h
    AuthController.h
    CallController.h
    CallSessionModel.h
    LivekitBridge.h
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
    TransportSelector.h
    MessageContentCodec.h
    MessagePayloadService.h
    PrivateChatCacheStore.h
    GroupChatCacheStore.h
    ProfileController.h
    LocalFilePickerService.h
    MediaUploadService.h
    WindowShapeService.h
    IconPathUtils.h
    MomentsController.h
    MomentsModel.h
    AgentController.h
    AgentMessageModel.h
    PetAssetSettings.h
    PetController.h
    PetModel.h
    PetSpeechSynthesizer.h
    Live2DAvatarOpenGLRenderer.h
    Live2DAsset.h
    Live2DRenderer.h
    Live2DCoreRenderer.h
    Live2DOfficialOpenGLRenderer.h
    Live2DPlaceholderRenderer.h
    Live2DRenderItem.h
    R18Controller.h
    R18ListModel.h
    ${SHARED_CLIENT_DIR}/imagecropperdialog.h
)

set_source_files_properties(
    ${SHARED_CLIENT_DIR}/imagecropperlabel.h
    PROPERTIES SKIP_AUTOMOC ON
)

set(RESOURCES
    qml.qrc
    ${SHARED_CLIENT_DIR}/rc.qrc
)

add_executable(MemoChatQml WIN32 MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${RESOURCES})
set_target_properties(MemoChatQml PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(MemoChatQml PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SHARED_CLIENT_DIR}
)

target_compile_definitions(MemoChatQml PRIVATE
    MEMOCHAT_ENABLE_LIVE2D_NATIVE=$<BOOL:${MEMOCHAT_ENABLE_LIVE2D_NATIVE}>
    MEMOCHAT_LIVE2D_SDK_ROOT="${MEMOCHAT_LIVE2D_SDK_ROOT_NORMALIZED}"
    MEMOCHAT_LIVE2D_CUBISM_FOUND=$<BOOL:${MEMOCHAT_LIVE2D_NATIVE_AVAILABLE}>
    MEMOCHAT_LIVE2D_CORE_INCLUDE_DIR="${Live2DCubism_CORE_INCLUDE_DIR}"
    MEMOCHAT_LIVE2D_FRAMEWORK_INCLUDE_DIR="${Live2DCubism_FRAMEWORK_INCLUDE_DIR}"
    MEMOCHAT_QML_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

if(MEMOCHAT_ENABLE_LIVE2D_NATIVE AND Live2DCubism_FOUND)
    set(MEMOCHAT_LIVE2D_FRAMEWORK_ROOT "${Live2DCubism_ROOT_DIR}/Framework")
    set(MEMOCHAT_LIVE2D_FRAMEWORK_SOURCE OpenGL)
    set(FRAMEWORK_SOURCE "${MEMOCHAT_LIVE2D_FRAMEWORK_SOURCE}")
    set(LIB_NAME MemoChatLive2DCubismFramework)
    add_library(${LIB_NAME} STATIC)
    add_subdirectory("${MEMOCHAT_LIVE2D_FRAMEWORK_ROOT}/src" "${CMAKE_CURRENT_BINARY_DIR}/live2d-framework")
    unset(LIB_NAME)
    unset(FRAMEWORK_SOURCE)

    target_include_directories(MemoChatLive2DCubismFramework
        PUBLIC
            "${Live2DCubism_FRAMEWORK_INCLUDE_DIR}"
            "${Live2DCubism_CORE_INCLUDE_DIR}"
    )
    target_link_libraries(MemoChatLive2DCubismFramework PUBLIC Live2DCubism::Core)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        find_package(OpenGL REQUIRED)
        find_package(GLEW REQUIRED)
        target_compile_definitions(MemoChatLive2DCubismFramework PUBLIC CSM_TARGET_LINUX_GL)
        target_link_libraries(MemoChatLive2DCubismFramework PUBLIC OpenGL::GL GLEW::GLEW)
        target_link_libraries(MemoChatQml PRIVATE OpenGL::GL GLEW::GLEW)
    endif()

    target_include_directories(MemoChatQml PRIVATE ${Live2DCubism_INCLUDE_DIRS})
    target_link_libraries(MemoChatQml PRIVATE Live2DCubism::Native MemoChatLive2DCubismFramework)
endif()

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
    Qt${QT_VERSION_MAJOR}::WebEngineQuick
)

if(TARGET Qt${QT_VERSION_MAJOR}::Multimedia)
    target_link_libraries(MemoChatQml PRIVATE Qt${QT_VERSION_MAJOR}::Multimedia)
    message(STATUS "Qt Multimedia found; desktop pet generated speech playback is available")
else()
    message(STATUS "Qt Multimedia not found; desktop pet generated speech playback stays disabled until the QML module is available")
endif()

if(TARGET Qt${QT_VERSION_MAJOR}::VirtualKeyboard)
    target_link_libraries(MemoChatQml PRIVATE Qt${QT_VERSION_MAJOR}::VirtualKeyboard)
    message(STATUS "Qt VirtualKeyboard found; desktop pet pinyin input fallback is available")
else()
    message(STATUS "Qt VirtualKeyboard not found; desktop pet uses the platform IME fallback")
endif()

if(WIN32)
    find_library(DWMAPI_LIB dwmapi)
    target_link_libraries(MemoChatQml PRIVATE "${DWMAPI_LIB}")
endif()

add_custom_command(TARGET MemoChatQml POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${SHARED_CLIENT_DIR}/config.ini
    $<TARGET_FILE_DIR:MemoChatQml>/config.ini
    COMMENT "Copying config.ini for MemoChatQml"
)
