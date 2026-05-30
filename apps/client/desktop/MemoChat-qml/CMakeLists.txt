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

set(MEMOCHAT_QML_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/core)
set(MEMOCHAT_QML_FEATURE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/features)
set(MEMOCHAT_QML_SHARED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shared)
add_subdirectory(core)

set(SOURCES
    app/main.cpp
    app/MainLogging.cpp
    app/MainPlatformBootstrap.cpp
    app/MainQmlBootstrap.cpp
    app/MainQmlTypeRegistry.cpp
    app/MainQmlEngineSetup.cpp
    app/MainQmlWindowLoader.cpp
    app/MainRuntimeConfig.cpp
    app/MainWindowEffects.cpp
    app/MainWindowHooks.cpp
    app/AppController.cpp
    app/AppSessionCoordinator.cpp
    app/SessionAuthCoordinator.cpp
    app/SessionAuthCoordinatorCommands.cpp
    app/SessionAuthCoordinatorLoginResponse.cpp
    app/SessionAuthCoordinatorAuthResponses.cpp
    app/SessionChatEntryCoordinator.cpp
    app/SessionRelationBootstrap.cpp
    app/RegisterCountdownController.cpp
    app/AppControllerAuth.cpp
    app/AppControllerAuthResponses.cpp
    app/CallCoordinator.cpp
    app/AppControllerConnectionLogin.cpp
    app/AppControllerHeartbeat.cpp
    app/AppControllerReconnect.cpp
    app/ContactCoordinatorShell.cpp
    app/AppControllerContactEvents.cpp
    app/AppControllerDialogListEvents.cpp
    app/AppControllerDialogState.cpp
    app/AppControllerDialogCommands.cpp
    app/GroupCoordinator.cpp
    app/MediaCoordinator.cpp
    app/ProfileCoordinator.cpp
    app/AppControllerGroupCommands.cpp
    app/AppControllerGroupPayloads.cpp
    app/AppControllerGroupManagement.cpp
    app/AppControllerModels.cpp
    app/AppControllerDialogModels.cpp
    app/AppControllerPrivateHistory.cpp
    app/AppControllerReadAcks.cpp
    app/AppControllerPrivateSelection.cpp
    app/AppControllerContactSelection.cpp
    app/AppControllerGroupSelection.cpp
    app/AppControllerDialogSelection.cpp
    app/AppControllerPagination.cpp
    app/AppControllerNavigation.cpp
    app/AppControllerUiProperties.cpp
    app/AppControllerUserProperties.cpp
    app/AppControllerGroupProperties.cpp
    app/AppControllerModelProperties.cpp
    app/AppControllerContactState.cpp
    app/AppControllerStatusState.cpp
    app/AppControllerLoadingState.cpp
    app/AppControllerDialogRuntimeState.cpp
    app/AppControllerProfileState.cpp
    app/AppControllerBootstrapState.cpp
    app/AppControllerProfileCommands.cpp
    app/AppCoordinators.cpp
    app/AppControllerCall.cpp
    app/AppControllerGroupEvents.cpp
    app/AppControllerGroupResponses.cpp
    app/AppControllerGroupResponseErrors.cpp
    app/AppControllerGroupManagementResponses.cpp
    app/AppControllerGroupHistoryResponses.cpp
    app/AppControllerGroupMessageResponses.cpp
    app/AppControllerPrivateMessageResponses.cpp
    app/AppControllerDialogMetaResponses.cpp
    app/AppControllerMedia.cpp
    app/AppControllerPendingAttachments.cpp
    app/AppControllerMediaUploadQueue.cpp
    app/AppControllerMessageDispatch.cpp
    app/AppControllerCallTarget.cpp
    app/AppControllerPrivateEvents.cpp
    app/AppControllerMessageStatus.cpp
    app/AppControllerPrivateHistoryResponses.cpp
    app/AppControllerPrivateMessageEvents.cpp
    app/AppControllerPrivateReadEvents.cpp
    app/AppControllerLoginSession.cpp
    app/AppControllerChatBootstrap.cpp
    app/AppControllerRelationBootstrap.cpp
    app/AppControllerRegisterCountdown.cpp
    features/auth/AuthController.cpp
    features/call/CallController.cpp
    features/call/CallSessionModel.cpp
    features/call/LivekitBridge.cpp
    features/chat/ChatController.cpp
    features/chat/ChatDispatchService.cpp
    features/chat/ChatCacheMessageCodec.cpp
    features/chat/ChatMessageModelContent.cpp
    features/chat/ChatMessageModel.cpp
    features/chat/ChatMessageModelMutations.cpp
    features/chat/ChatMessageModelPresentation.cpp
    features/chat/ChatMessageModelQueries.cpp
    features/chat/ConversationSyncService.cpp
    features/chat/DialogListEntryBuilder.cpp
    features/chat/DialogListService.cpp
    features/chat/GroupChatCacheStore.cpp
    features/chat/MessageContentCodec.cpp
    features/chat/MessagePayloadService.cpp
    features/chat/PrivateChatCacheStore.cpp
    features/contact/ApplyRequestModel.cpp
    features/contact/ContactController.cpp
    features/contact/FriendListModel.cpp
    features/contact/FriendListModelMutations.cpp
    features/contact/FriendListModelState.cpp
    features/contact/FriendListModelQueries.cpp
    features/contact/SearchResultModel.cpp
    features/profile/ProfileController.cpp
    features/moments/MomentsController.cpp
    features/moments/MomentsEntryParser.cpp
    features/moments/MomentsPublishPayload.cpp
    features/moments/MomentsControllerParsing.cpp
    features/moments/MomentsControllerPublish.cpp
    features/moments/MomentsControllerRequests.cpp
    features/moments/MomentsControllerResponses.cpp
    features/moments/MomentsResponseParser.cpp
    features/moments/MomentsModel.cpp
    features/moments/MomentsModelMutations.cpp
    features/moments/MomentsModelQueries.cpp
    features/agent/AgentControllerChat.cpp
    features/agent/AgentController.cpp
    features/agent/AgentControllerGameNetwork.cpp
    features/agent/AgentControllerGameResponses.cpp
    features/agent/AgentControllerGameRooms.cpp
    features/agent/AgentControllerGameTemplates.cpp
    features/agent/AgentControllerAgentTasks.cpp
    features/agent/AgentControllerKnowledge.cpp
    features/agent/AgentControllerMemory.cpp
    features/agent/AgentControllerModels.cpp
    features/agent/AgentControllerSessions.cpp
    features/agent/AgentControllerState.cpp
    features/agent/AgentControllerStream.cpp
    features/agent/AgentMessageModel.cpp
    features/pet/PetAssetSettings.cpp
    features/pet/PetAssetSettingsAvatar.cpp
    features/pet/PetAssetSettingsPersistence.cpp
    features/pet/PetAssetSettingsState.cpp
    features/pet/PetAvatarPathUtils.cpp
    features/pet/PetAvatarResolver.cpp
    features/pet/PetController.cpp
    features/pet/PetControllerNetwork.cpp
    features/pet/PetControllerSession.cpp
    features/pet/PetControllerState.cpp
    features/pet/PetControllerVoiceRuntime.cpp
    features/pet/PetControllerVoiceTraining.cpp
    features/pet/PetVisionFrameEncoder.cpp
    features/pet/PetControllerVision.cpp
    features/pet/PetControllerWindowsBridge.cpp
    features/pet/PetModel.cpp
    features/pet/PetSpeechSynthesizer.cpp
    features/r18/R18Controller.cpp
    features/r18/R18ControllerNetwork.cpp
    features/r18/R18ControllerResponses.cpp
    features/r18/R18ControllerSources.cpp
    features/r18/R18ControllerState.cpp
    features/r18/R18ListModel.cpp
    shared/gateway/ClientGateway.cpp
    shared/gateway/TransportEndpointPolicy.cpp
    shared/gateway/TransportSelector.cpp
    shared/media/LocalFilePickerService.cpp
    shared/media/LocalFilePickerServiceAttachments.cpp
    shared/media/LocalFilePickerServiceAvatar.cpp
    shared/media/MediaUploadService.cpp
    shared/media/MediaUploadServiceHelpers.cpp
    shared/media/MediaUploadServiceNetwork.cpp
    shared/media/MediaUploadServiceUploads.cpp
    shared/ui/WindowShapeService.cpp
    live2d/Live2DAvatarOpenGLRenderer.cpp
    live2d/Live2DAsset.cpp
    live2d/Live2DCoreRenderer.cpp
    live2d/Live2DOfficialOpenGLRenderer.cpp
    live2d/Live2DPlaceholderRenderer.cpp
    live2d/Live2DRenderItem.cpp
)

set(HEADERS
    platform/windows/ClientWinCompat.h
    app/MainLogging.h
    app/MainPlatformBootstrap.h
    app/MainQmlBootstrap.h
    app/MainRuntimeConfig.h
    app/MainWindowEffects.h
    app/MainWindowHooks.h
    app/AppController.h
    app/AppControllerConnectionState.h
    app/AppControllerDialogStateData.h
    app/AppControllerGroupState.h
    app/AppControllerGroupPayloads.h
    app/AppControllerPendingSendState.h
    app/AppControllerRuntimeState.h
    app/AppControllerUserState.h
    app/AppCoordinators.h
    features/auth/AuthController.h
    features/call/CallController.h
    features/call/CallSessionModel.h
    features/call/LivekitBridge.h
    features/chat/ChatController.h
    features/chat/ChatDispatchService.h
    features/chat/ChatCacheMessageCodec.h
    features/chat/ChatMessageModel.h
    features/chat/ConversationSyncService.h
    features/chat/DialogListEntryBuilder.h
    features/chat/DialogListService.h
    features/chat/DialogListTypes.h
    features/chat/GroupChatCacheStore.h
    features/chat/MessageContentCodec.h
    features/chat/MessagePayloadService.h
    features/chat/PrivateChatCacheStore.h
    features/contact/ApplyRequestModel.h
    features/contact/ContactController.h
    features/contact/FriendListModel.h
    features/contact/SearchResultModel.h
    features/profile/ProfileController.h
    features/moments/MomentsController.h
    features/moments/MomentsEntryParser.h
    features/moments/MomentsPublishPayload.h
    features/moments/MomentsResponseParser.h
    features/moments/MomentsModel.h
    features/agent/AgentController.h
    features/agent/AgentMessageModel.h
    features/pet/PetAssetSettings.h
    features/pet/PetAssetSettingsPrivate.h
    features/pet/PetAvatarPathUtils.h
    features/pet/PetAvatarResolver.h
    features/pet/PetController.h
    features/pet/PetNetworkRequestUtils.h
    features/pet/PetControllerPrivate.h
    features/pet/PetVisionFrameEncoder.h
    features/pet/PetVisionFrameUtils.h
    features/pet/PetWindowsBridgeUtils.h
    features/pet/PetModel.h
    features/pet/PetSpeechSynthesizer.h
    features/r18/R18Controller.h
    features/r18/R18ControllerPrivate.h
    features/r18/R18ListModel.h
    shared/gateway/ClientGateway.h
    shared/gateway/TransportEndpointPolicy.h
    shared/gateway/TransportSelector.h
    shared/media/LocalFilePickerService.h
    shared/media/LocalFilePickerServicePrivate.h
    shared/media/MediaUploadService.h
    shared/media/MediaUploadServicePrivate.h
    shared/ui/WindowShapeService.h
    shared/utils/IconPathUtils.h
    live2d/Live2DAvatarOpenGLRenderer.h
    live2d/Live2DAsset.h
    live2d/Live2DRenderer.h
    live2d/Live2DCoreRenderer.h
    live2d/Live2DOfficialOpenGLRenderer.h
    live2d/Live2DPlaceholderRenderer.h
    live2d/Live2DRenderItem.h
)

set_source_files_properties(
    ${MEMOCHAT_QML_CORE_DIR}/media/imagecropperlabel.h
    PROPERTIES SKIP_AUTOMOC ON
)

set(RESOURCES
    qml.qrc
    ${MEMOCHAT_QML_CORE_DIR}/rc.qrc
)

add_executable(MemoChatQml WIN32 MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${RESOURCES})
set_target_properties(MemoChatQml PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(MemoChatQml PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/windows
    ${MEMOCHAT_QML_FEATURE_DIR}
    ${MEMOCHAT_QML_FEATURE_DIR}/auth
    ${MEMOCHAT_QML_FEATURE_DIR}/call
    ${MEMOCHAT_QML_FEATURE_DIR}/chat
    ${MEMOCHAT_QML_FEATURE_DIR}/contact
    ${MEMOCHAT_QML_FEATURE_DIR}/profile
    ${MEMOCHAT_QML_FEATURE_DIR}/moments
    ${MEMOCHAT_QML_FEATURE_DIR}/agent
    ${MEMOCHAT_QML_FEATURE_DIR}/pet
    ${MEMOCHAT_QML_FEATURE_DIR}/r18
    ${MEMOCHAT_QML_SHARED_DIR}
    ${MEMOCHAT_QML_SHARED_DIR}/gateway
    ${MEMOCHAT_QML_SHARED_DIR}/media
    ${MEMOCHAT_QML_SHARED_DIR}/ui
    ${MEMOCHAT_QML_SHARED_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/live2d
    ${MEMOCHAT_QML_CORE_DIR}
    ${MEMOCHAT_QML_CORE_DIR}/common
    ${MEMOCHAT_QML_CORE_DIR}/media
    ${MEMOCHAT_QML_CORE_DIR}/network
    ${MEMOCHAT_QML_CORE_DIR}/session
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
    ${MEMOCHAT_QML_CORE_DIR}/config.ini
    $<TARGET_FILE_DIR:MemoChatQml>/config.ini
    COMMENT "Copying config.ini for MemoChatQml"
)
