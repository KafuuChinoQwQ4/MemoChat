import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
DESKTOP_CMAKE = REPO_ROOT / "apps/client/desktop/CMakeLists.txt"
QML_CMAKE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/CMakeLists.txt"
CLIENT_DIALOG_TEST_CMAKE = REPO_ROOT / "tests/client/dialog/CMakeLists.txt"
CLIENT_CACHE_TEST_CMAKE = REPO_ROOT / "tests/client/cache/CMakeLists.txt"
CLIENT_MOMENTS_TEST_CMAKE = REPO_ROOT / "tests/client/moments/CMakeLists.txt"
CLIENT_NETWORK_TEST_CMAKE = REPO_ROOT / "tests/client/network/CMakeLists.txt"
QML_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
CORE_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/core"
CORE_CMAKE = CORE_DIR / "CMakeLists.txt"
OLD_SHARED_DIR = REPO_ROOT / "apps/client/desktop/MemoChatShared"
CORE_QRC = CORE_DIR / "rc.qrc"
MOMENTS_CONTROLLER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/moments/MomentsController.h"
QML_QRC = QML_DIR / "qml.qrc"


class MemoChatQmlCoreLayoutTests(unittest.TestCase):
    def test_core_lives_under_memochat_qml_and_legacy_shared_module_is_removed(self):
        self.assertTrue(CORE_DIR.is_dir())
        self.assertTrue((CORE_DIR / "CMakeLists.txt").is_file())
        self.assertTrue((CORE_DIR / "network/ChatFrameCodec.cpp").is_file())
        self.assertTrue((CORE_DIR / "network/ChatMessageDispatcher.cpp").is_file())
        self.assertTrue((CORE_DIR / "network/ChatMessageDispatcherAuth.cpp").is_file())
        self.assertTrue((CORE_DIR / "network/ChatMessageDispatcherContacts.cpp").is_file())
        self.assertTrue((CORE_DIR / "network/ChatMessageDispatcherPrivate.cpp").is_file())
        self.assertTrue((CORE_DIR / "network/ChatMessageDispatcherGroup.cpp").is_file())
        self.assertTrue((CORE_DIR / "network/ChatMessageDispatcherSystem.cpp").is_file())
        self.assertTrue((CORE_DIR / "network/HttpMgrRequestUtils.cpp").is_file())
        self.assertTrue((CORE_DIR / "network/httpmgr.cpp").is_file())
        self.assertTrue((CORE_DIR / "network/QuicChatTransportMsquic.cpp").is_file())
        self.assertTrue((CORE_DIR / "network/IChatTransport.h").is_file())
        self.assertTrue((CORE_DIR / "session/usermgr.cpp").is_file())
        self.assertTrue((CORE_DIR / "session/UserContactData.h").is_file())
        self.assertTrue((CORE_DIR / "session/UserGroupData.h").is_file())
        self.assertTrue((CORE_DIR / "session/UserMessageData.h").is_file())
        self.assertTrue((CORE_DIR / "session/UserMgrApply.cpp").is_file())
        self.assertTrue((CORE_DIR / "session/UserMgrFriends.cpp").is_file())
        self.assertTrue((CORE_DIR / "session/UserMgrGroupList.cpp").is_file())
        self.assertTrue((CORE_DIR / "session/UserMgrGroupMessages.cpp").is_file())
        self.assertTrue((CORE_DIR / "session/UserMgrPrivateMessages.cpp").is_file())
        self.assertTrue((CORE_DIR / "common/global.h").is_file())
        self.assertTrue((CORE_DIR / "media/imagecropperdialog.h").is_file())
        self.assertFalse(OLD_SHARED_DIR.exists())

    def test_qml_cpp_sources_are_grouped_by_responsibility(self):
        expected_files = (
            "app/main.cpp",
            "app/MainPlatformBootstrap.cpp",
            "app/MainQmlBootstrap.cpp",
            "app/MainQmlTypeRegistry.cpp",
            "app/MainQmlEngineSetup.cpp",
            "app/MainQmlWindowLoader.cpp",
            "app/MainRuntimeConfig.cpp",
            "app/MainWindowHooks.cpp",
            "app/AppController.cpp",
            "app/AppControllerConnectionState.h",
            "app/AppControllerDialogStateData.h",
            "app/AppControllerGroupState.h",
            "app/AppControllerPendingSendState.h",
            "app/AppControllerRuntimeState.h",
            "app/AppControllerUserState.h",
            "app/AppSessionCoordinator.cpp",
            "app/SessionAuthCoordinator.cpp",
            "app/SessionAuthCoordinatorCommands.cpp",
            "app/SessionAuthCoordinatorLoginResponse.cpp",
            "app/SessionAuthCoordinatorAuthResponses.cpp",
            "app/SessionChatEntryCoordinator.cpp",
            "app/SessionRelationBootstrap.cpp",
            "app/RegisterCountdownController.cpp",
            "app/CallCoordinator.cpp",
            "app/ContactCoordinatorShell.cpp",
            "app/GroupCoordinator.cpp",
            "app/MediaCoordinator.cpp",
            "app/ProfileCoordinator.cpp",
            "app/AppControllerGroupEvents.cpp",
            "app/AppControllerGroupManagement.cpp",
            "app/AppControllerGroupResponses.cpp",
            "app/AppControllerGroupResponseErrors.cpp",
            "app/AppControllerGroupManagementResponses.cpp",
            "app/AppControllerGroupHistoryResponses.cpp",
            "app/AppControllerGroupMessageResponses.cpp",
            "app/AppControllerPrivateMessageResponses.cpp",
            "app/AppControllerDialogMetaResponses.cpp",
            "app/AppControllerDialogModels.cpp",
            "app/AppControllerPrivateHistory.cpp",
            "app/AppControllerReadAcks.cpp",
            "app/AppControllerPrivateSelection.cpp",
            "app/AppControllerContactSelection.cpp",
            "app/AppControllerGroupSelection.cpp",
            "app/AppControllerDialogSelection.cpp",
            "app/AppControllerPagination.cpp",
            "app/AppControllerUiProperties.cpp",
            "app/AppControllerUserProperties.cpp",
            "app/AppControllerGroupProperties.cpp",
            "app/AppControllerModelProperties.cpp",
            "app/AppControllerContactState.cpp",
            "app/AppControllerStatusState.cpp",
            "app/AppControllerLoadingState.cpp",
            "app/AppControllerDialogRuntimeState.cpp",
            "app/AppControllerProfileState.cpp",
            "app/AppControllerBootstrapState.cpp",
            "app/AppControllerConnectionLogin.cpp",
            "app/AppControllerHeartbeat.cpp",
            "app/AppControllerReconnect.cpp",
            "app/AppControllerLoginSession.cpp",
            "app/AppControllerChatBootstrap.cpp",
            "app/AppControllerRelationBootstrap.cpp",
            "app/AppControllerRegisterCountdown.cpp",
            "app/AppControllerPendingAttachments.cpp",
            "app/AppControllerMediaUploadQueue.cpp",
            "app/AppControllerMessageDispatch.cpp",
            "app/AppControllerCallTarget.cpp",
            "app/AppControllerMessageStatus.cpp",
            "app/AppControllerPrivateHistoryResponses.cpp",
            "app/AppControllerPrivateMessageEvents.cpp",
            "app/AppControllerPrivateReadEvents.cpp",
            "features/agent/AgentControllerChat.cpp",
            "features/agent/AgentController.cpp",
            "features/agent/AgentControllerGameNetwork.cpp",
            "features/agent/AgentControllerGameResponses.cpp",
            "features/agent/AgentControllerGameRooms.cpp",
            "features/agent/AgentControllerGameTemplates.cpp",
            "features/agent/AgentControllerAgentTasks.cpp",
            "features/agent/AgentControllerKnowledge.cpp",
            "features/agent/AgentControllerMemory.cpp",
            "features/auth/AuthController.cpp",
            "features/chat/ChatCacheMessageCodec.cpp",
            "features/chat/ChatMessageModelContent.cpp",
            "features/chat/ChatMessageModel.cpp",
            "features/chat/ChatMessageModelMutations.cpp",
            "features/chat/ChatMessageModelPresentation.cpp",
            "features/chat/ChatMessageModelQueries.cpp",
            "features/chat/DialogListEntryBuilder.cpp",
            "features/chat/PrivateChatCacheStore.cpp",
            "features/contact/FriendListModel.cpp",
            "features/contact/FriendListModelMutations.cpp",
            "features/contact/FriendListModelState.cpp",
            "features/contact/FriendListModelQueries.cpp",
            "features/moments/MomentsEntryParser.cpp",
            "features/moments/MomentsControllerParsing.cpp",
            "features/moments/MomentsControllerPublish.cpp",
            "features/moments/MomentsControllerRequests.cpp",
            "features/moments/MomentsControllerResponses.cpp",
            "features/r18/R18Controller.cpp",
            "features/moments/MomentsModel.cpp",
            "features/moments/MomentsModelMutations.cpp",
            "features/moments/MomentsModelQueries.cpp",
            "features/r18/R18ControllerNetwork.cpp",
            "features/r18/R18ControllerResponses.cpp",
            "features/r18/R18ControllerSources.cpp",
            "features/r18/R18ControllerState.cpp",
            "features/pet/PetController.cpp",
            "features/pet/PetControllerSession.cpp",
            "features/pet/PetControllerState.cpp",
            "features/pet/PetControllerVoiceRuntime.cpp",
            "features/pet/PetControllerVoiceTraining.cpp",
            "shared/gateway/ClientGateway.cpp",
            "shared/gateway/TransportEndpointPolicy.cpp",
            "shared/media/LocalFilePickerService.cpp",
            "shared/media/LocalFilePickerServicePrivate.h",
            "shared/media/LocalFilePickerServiceAttachments.cpp",
            "shared/media/LocalFilePickerServiceAvatar.cpp",
            "shared/media/MediaUploadService.cpp",
            "shared/media/MediaUploadServicePrivate.h",
            "shared/media/MediaUploadServiceHelpers.cpp",
            "shared/media/MediaUploadServiceNetwork.cpp",
            "shared/media/MediaUploadServiceUploads.cpp",
            "shared/utils/IconPathUtils.h",
            "platform/windows/ClientWinCompat.h",
            "live2d/Live2DRenderItem.cpp",
        )

        for rel_path in expected_files:
            with self.subTest(path=rel_path):
                self.assertTrue((QML_DIR / rel_path).is_file())

        root_cpp_or_h = [path for path in QML_DIR.iterdir() if path.is_file() and path.suffix in {".cpp", ".h"}]
        self.assertEqual([], root_cpp_or_h)

        old_horizontal_dirs = ("controllers", "models", "services", "storage", "utils")
        for rel_dir in old_horizontal_dirs:
            with self.subTest(old_dir=rel_dir):
                self.assertFalse((QML_DIR / rel_dir).exists())

    def test_cmake_adds_core_from_qml_module_without_desktop_shared_entry(self):
        desktop_cmake = DESKTOP_CMAKE.read_text(encoding="utf-8")
        qml_cmake = QML_CMAKE.read_text(encoding="utf-8")
        core_cmake = CORE_CMAKE.read_text(encoding="utf-8")
        client_cache_test_cmake = CLIENT_CACHE_TEST_CMAKE.read_text(encoding="utf-8")
        client_dialog_test_cmake = CLIENT_DIALOG_TEST_CMAKE.read_text(encoding="utf-8")
        client_moments_test_cmake = CLIENT_MOMENTS_TEST_CMAKE.read_text(encoding="utf-8")
        client_network_test_cmake = CLIENT_NETWORK_TEST_CMAKE.read_text(encoding="utf-8")

        self.assertNotIn("add_subdirectory(MemoChatShared)", desktop_cmake)
        self.assertIn("add_subdirectory(MemoChat-qml)", desktop_cmake)
        self.assertIn("add_subdirectory(core)", qml_cmake)
        self.assertIn("session/UserMgrApply.cpp", core_cmake)
        self.assertIn("session/UserMgrFriends.cpp", core_cmake)
        self.assertIn("session/UserMgrGroupList.cpp", core_cmake)
        self.assertIn("session/UserMgrGroupMessages.cpp", core_cmake)
        self.assertIn("session/UserMgrPrivateMessages.cpp", core_cmake)
        self.assertIn("session/UserContactData.h", core_cmake)
        self.assertIn("session/UserGroupData.h", core_cmake)
        self.assertIn("session/UserMessageData.h", core_cmake)
        self.assertIn("network/ChatMessageDispatcherAuth.cpp", core_cmake)
        self.assertIn("network/ChatFrameCodec.cpp", core_cmake)
        self.assertIn("network/ChatFrameCodec.h", core_cmake)
        self.assertIn("network/ChatMessageDispatcherContacts.cpp", core_cmake)
        self.assertIn("network/ChatMessageDispatcherPrivate.cpp", core_cmake)
        self.assertIn("network/ChatMessageDispatcherGroup.cpp", core_cmake)
        self.assertIn("network/ChatMessageDispatcherSystem.cpp", core_cmake)
        self.assertIn("network/HttpMgrRequestUtils.cpp", core_cmake)
        self.assertIn("network/HttpMgrRequestUtils.h", core_cmake)
        self.assertIn("network/QuicChatTransportMsquic.cpp", core_cmake)
        self.assertIn("ChatFrameCodec.cpp", client_network_test_cmake)
        self.assertIn("chat_frame_codec_gtest", client_network_test_cmake)
        self.assertIn("TransportEndpointPolicy.cpp", client_network_test_cmake)
        self.assertIn("transport_endpoint_policy_gtest", client_network_test_cmake)
        self.assertIn("set(MEMOCHAT_QML_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/core)", qml_cmake)
        self.assertIn("set(MEMOCHAT_QML_FEATURE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/features)", qml_cmake)
        self.assertIn("set(MEMOCHAT_QML_SHARED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shared)", qml_cmake)
        self.assertIn("app/AppController.cpp", qml_cmake)
        self.assertIn("app/MainQmlBootstrap.cpp", qml_cmake)
        self.assertIn("app/MainQmlTypeRegistry.cpp", qml_cmake)
        self.assertIn("app/MainQmlEngineSetup.cpp", qml_cmake)
        self.assertIn("app/MainQmlWindowLoader.cpp", qml_cmake)
        self.assertIn("app/MainRuntimeConfig.cpp", qml_cmake)
        self.assertIn("app/AppControllerConnectionState.h", qml_cmake)
        self.assertIn("app/AppControllerDialogStateData.h", qml_cmake)
        self.assertIn("app/AppControllerGroupState.h", qml_cmake)
        self.assertIn("app/AppControllerPendingSendState.h", qml_cmake)
        self.assertIn("app/AppControllerRuntimeState.h", qml_cmake)
        self.assertIn("app/AppControllerUserState.h", qml_cmake)
        self.assertIn("app/AppSessionCoordinator.cpp", qml_cmake)
        self.assertIn("app/SessionAuthCoordinator.cpp", qml_cmake)
        self.assertIn("app/SessionAuthCoordinatorCommands.cpp", qml_cmake)
        self.assertIn("app/SessionAuthCoordinatorLoginResponse.cpp", qml_cmake)
        self.assertIn("app/SessionAuthCoordinatorAuthResponses.cpp", qml_cmake)
        self.assertIn("app/SessionChatEntryCoordinator.cpp", qml_cmake)
        self.assertIn("app/SessionRelationBootstrap.cpp", qml_cmake)
        self.assertIn("app/RegisterCountdownController.cpp", qml_cmake)
        self.assertIn("app/CallCoordinator.cpp", qml_cmake)
        self.assertIn("app/ContactCoordinatorShell.cpp", qml_cmake)
        self.assertIn("app/GroupCoordinator.cpp", qml_cmake)
        self.assertIn("app/MediaCoordinator.cpp", qml_cmake)
        self.assertIn("app/ProfileCoordinator.cpp", qml_cmake)
        self.assertIn("app/AppControllerGroupEvents.cpp", qml_cmake)
        self.assertIn("app/AppControllerGroupManagement.cpp", qml_cmake)
        self.assertIn("app/AppControllerGroupResponses.cpp", qml_cmake)
        self.assertIn("app/AppControllerGroupResponseErrors.cpp", qml_cmake)
        self.assertIn("app/AppControllerGroupManagementResponses.cpp", qml_cmake)
        self.assertIn("app/AppControllerGroupHistoryResponses.cpp", qml_cmake)
        self.assertIn("app/AppControllerGroupMessageResponses.cpp", qml_cmake)
        self.assertIn("app/AppControllerPrivateMessageResponses.cpp", qml_cmake)
        self.assertIn("app/AppControllerDialogMetaResponses.cpp", qml_cmake)
        self.assertIn("app/AppControllerDialogModels.cpp", qml_cmake)
        self.assertIn("app/AppControllerPrivateHistory.cpp", qml_cmake)
        self.assertIn("app/AppControllerReadAcks.cpp", qml_cmake)
        self.assertIn("app/AppControllerPrivateSelection.cpp", qml_cmake)
        self.assertIn("app/AppControllerContactSelection.cpp", qml_cmake)
        self.assertIn("app/AppControllerGroupSelection.cpp", qml_cmake)
        self.assertIn("app/AppControllerDialogSelection.cpp", qml_cmake)
        self.assertIn("app/AppControllerPagination.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerSelection.cpp", qml_cmake)
        self.assertIn("app/AppControllerUiProperties.cpp", qml_cmake)
        self.assertIn("app/AppControllerUserProperties.cpp", qml_cmake)
        self.assertIn("app/AppControllerGroupProperties.cpp", qml_cmake)
        self.assertIn("app/AppControllerModelProperties.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerProperties.cpp", qml_cmake)
        self.assertIn("app/AppControllerContactState.cpp", qml_cmake)
        self.assertIn("app/AppControllerStatusState.cpp", qml_cmake)
        self.assertIn("app/AppControllerLoadingState.cpp", qml_cmake)
        self.assertIn("app/AppControllerDialogRuntimeState.cpp", qml_cmake)
        self.assertIn("app/AppControllerProfileState.cpp", qml_cmake)
        self.assertIn("app/AppControllerBootstrapState.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerState.cpp", qml_cmake)
        self.assertIn("app/AppControllerConnectionLogin.cpp", qml_cmake)
        self.assertIn("app/AppControllerHeartbeat.cpp", qml_cmake)
        self.assertIn("app/AppControllerReconnect.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerConnection.cpp", qml_cmake)
        self.assertIn("app/AppControllerLoginSession.cpp", qml_cmake)
        self.assertIn("app/AppControllerChatBootstrap.cpp", qml_cmake)
        self.assertIn("app/AppControllerRelationBootstrap.cpp", qml_cmake)
        self.assertIn("app/AppControllerRegisterCountdown.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerSession.cpp", qml_cmake)
        self.assertIn("app/AppControllerPendingAttachments.cpp", qml_cmake)
        self.assertIn("app/AppControllerMediaUploadQueue.cpp", qml_cmake)
        self.assertIn("app/AppControllerMessageDispatch.cpp", qml_cmake)
        self.assertIn("app/AppControllerCallTarget.cpp", qml_cmake)
        self.assertIn("app/AppControllerMessageStatus.cpp", qml_cmake)
        self.assertIn("app/AppControllerPrivateHistoryResponses.cpp", qml_cmake)
        self.assertIn("app/AppControllerPrivateMessageEvents.cpp", qml_cmake)
        self.assertIn("app/AppControllerPrivateReadEvents.cpp", qml_cmake)
        self.assertIn("features/agent/AgentControllerChat.cpp", qml_cmake)
        self.assertIn("features/agent/AgentController.cpp", qml_cmake)
        self.assertIn("features/agent/AgentControllerGameNetwork.cpp", qml_cmake)
        self.assertIn("features/agent/AgentControllerGameResponses.cpp", qml_cmake)
        self.assertIn("features/agent/AgentControllerGameRooms.cpp", qml_cmake)
        self.assertIn("features/agent/AgentControllerGameTemplates.cpp", qml_cmake)
        self.assertIn("features/agent/AgentControllerAgentTasks.cpp", qml_cmake)
        self.assertIn("features/agent/AgentControllerKnowledge.cpp", qml_cmake)
        self.assertIn("features/agent/AgentControllerMemory.cpp", qml_cmake)
        self.assertNotIn("features/agent/AgentControllerGame.cpp", qml_cmake)
        self.assertIn("features/agent/AgentControllerModels.cpp", qml_cmake)
        self.assertIn("features/agent/AgentControllerSessions.cpp", qml_cmake)
        self.assertIn("features/agent/AgentControllerState.cpp", qml_cmake)
        self.assertIn("features/chat/ChatMessageModelContent.cpp", qml_cmake)
        self.assertIn("features/chat/ChatMessageModel.cpp", qml_cmake)
        self.assertIn("features/chat/ChatCacheMessageCodec.cpp", qml_cmake)
        self.assertIn("features/chat/ChatCacheMessageCodec.h", qml_cmake)
        self.assertIn("ChatCacheMessageCodec.cpp", client_cache_test_cmake)
        self.assertIn("chat_cache_message_codec_gtest", client_cache_test_cmake)
        self.assertIn("features/chat/ChatMessageModelMutations.cpp", qml_cmake)
        self.assertIn("features/chat/ChatMessageModelPresentation.cpp", qml_cmake)
        self.assertIn("features/chat/ChatMessageModelQueries.cpp", qml_cmake)
        self.assertIn("features/chat/DialogListEntryBuilder.cpp", qml_cmake)
        self.assertIn("features/chat/DialogListEntryBuilder.h", qml_cmake)
        self.assertIn("features/chat/DialogListTypes.h", qml_cmake)
        self.assertIn("DialogListEntryBuilder.cpp", client_dialog_test_cmake)
        self.assertIn("features/chat/PrivateChatCacheStore.cpp", qml_cmake)
        self.assertIn("features/contact/FriendListModelMutations.cpp", qml_cmake)
        self.assertIn("features/contact/FriendListModelState.cpp", qml_cmake)
        self.assertIn("features/contact/FriendListModelQueries.cpp", qml_cmake)
        self.assertIn("FriendListModelMutations.cpp", client_dialog_test_cmake)
        self.assertIn("FriendListModelState.cpp", client_dialog_test_cmake)
        self.assertIn("FriendListModelQueries.cpp", client_dialog_test_cmake)
        self.assertIn("features/moments/MomentsEntryParser.cpp", qml_cmake)
        self.assertIn("features/moments/MomentsEntryParser.h", qml_cmake)
        self.assertIn("MomentsEntryParser.cpp", client_moments_test_cmake)
        self.assertIn("moments_entry_parser_gtest", client_moments_test_cmake)
        self.assertIn("features/moments/MomentsControllerParsing.cpp", qml_cmake)
        self.assertIn("features/moments/MomentsControllerPublish.cpp", qml_cmake)
        self.assertIn("features/moments/MomentsControllerRequests.cpp", qml_cmake)
        self.assertIn("features/moments/MomentsControllerResponses.cpp", qml_cmake)
        self.assertIn("features/moments/MomentsModelMutations.cpp", qml_cmake)
        self.assertIn("features/moments/MomentsModelQueries.cpp", qml_cmake)
        self.assertIn("features/r18/R18ControllerNetwork.cpp", qml_cmake)
        self.assertIn("features/r18/R18ControllerResponses.cpp", qml_cmake)
        self.assertIn("features/r18/R18ControllerSources.cpp", qml_cmake)
        self.assertIn("features/r18/R18ControllerState.cpp", qml_cmake)
        self.assertIn("features/pet/PetControllerSession.cpp", qml_cmake)
        self.assertIn("features/pet/PetControllerState.cpp", qml_cmake)
        self.assertIn("features/pet/PetControllerVoiceRuntime.cpp", qml_cmake)
        self.assertIn("features/pet/PetControllerVoiceTraining.cpp", qml_cmake)
        self.assertIn("shared/gateway/ClientGateway.cpp", qml_cmake)
        self.assertIn("shared/gateway/TransportEndpointPolicy.cpp", qml_cmake)
        self.assertIn("shared/gateway/TransportEndpointPolicy.h", qml_cmake)
        self.assertIn("shared/media/LocalFilePickerServiceAttachments.cpp", qml_cmake)
        self.assertIn("shared/media/LocalFilePickerServiceAvatar.cpp", qml_cmake)
        self.assertIn("shared/media/MediaUploadServiceHelpers.cpp", qml_cmake)
        self.assertIn("shared/media/MediaUploadServiceNetwork.cpp", qml_cmake)
        self.assertIn("shared/media/MediaUploadServiceUploads.cpp", qml_cmake)
        self.assertIn("live2d/Live2DRenderItem.cpp", qml_cmake)
        self.assertIn("${MEMOCHAT_QML_FEATURE_DIR}/agent", qml_cmake)
        self.assertIn("${MEMOCHAT_QML_FEATURE_DIR}/chat", qml_cmake)
        self.assertIn("${MEMOCHAT_QML_SHARED_DIR}/gateway", qml_cmake)
        self.assertIn("${MEMOCHAT_QML_SHARED_DIR}/media", qml_cmake)
        self.assertIn("${MEMOCHAT_QML_CORE_DIR}/rc.qrc", qml_cmake)
        self.assertIn("${MEMOCHAT_QML_CORE_DIR}/config.ini", qml_cmake)
        self.assertNotIn("${CMAKE_CURRENT_SOURCE_DIR}/controllers", qml_cmake)
        self.assertNotIn("${CMAKE_CURRENT_SOURCE_DIR}/models", qml_cmake)
        self.assertNotIn("${CMAKE_CURRENT_SOURCE_DIR}/services", qml_cmake)
        self.assertNotIn("${CMAKE_CURRENT_SOURCE_DIR}/storage", qml_cmake)
        self.assertNotIn("${CMAKE_CURRENT_SOURCE_DIR}/utils", qml_cmake)
        self.assertNotIn("SHARED_CLIENT_DIR", qml_cmake)
        self.assertNotIn("../MemoChatShared", qml_cmake)

    def test_startup_qml_bootstrap_concerns_are_split(self):
        bootstrap = (QML_DIR / "app/MainQmlBootstrap.cpp").read_text(encoding="utf-8")
        registry = (QML_DIR / "app/MainQmlTypeRegistry.cpp").read_text(encoding="utf-8")
        engine = (QML_DIR / "app/MainQmlEngineSetup.cpp").read_text(encoding="utf-8")
        loader = (QML_DIR / "app/MainQmlWindowLoader.cpp").read_text(encoding="utf-8")

        self.assertIn("void registerMemoChatQmlTypes()", registry)
        self.assertIn("qmlRegisterUncreatableType<AppController>", registry)
        self.assertIn("void configureMemoChatEngine", engine)
        self.assertIn('setContextProperty("controller"', engine)
        self.assertIn("QUrl memoChatMainQmlUrl()", loader)
        self.assertIn("bool loadMemoChatMainWindow", loader)
        self.assertIn("ensureInitialCenteringHook(window)", loader)
        self.assertIn("scheduleTopLevelQuickWindowHookRetries(&app)", loader)
        self.assertNotIn("void registerMemoChatQmlTypes()", bootstrap)
        self.assertNotIn("void configureMemoChatEngine", bootstrap)
        self.assertNotIn("bool loadMemoChatMainWindow", bootstrap)
        self.assertLessEqual(len(bootstrap.splitlines()), 12)

    def test_qml_shell_runtime_helpers_are_registered(self):
        qrc = QML_QRC.read_text(encoding="utf-8")
        r18_shell = (QML_DIR / "qml/r18/R18ShellPane.qml").read_text(encoding="utf-8")
        r18_runtime = (QML_DIR / "qml/r18/R18ShellRuntime.js").read_text(encoding="utf-8")
        agent_pane = (QML_DIR / "qml/agent/AgentPane.qml").read_text(encoding="utf-8")
        agent_runtime = (QML_DIR / "qml/agent/AgentPaneRuntime.js").read_text(encoding="utf-8")

        self.assertIn("qml/r18/R18ShellRuntime.js", qrc)
        self.assertIn("qml/agent/AgentPaneRuntime.js", qrc)
        self.assertIn('import "R18ShellRuntime.js" as R18ShellRuntime', r18_shell)
        self.assertIn("R18ShellRuntime.buildSourceTagBuckets", r18_shell)
        self.assertIn("function buildSourceTagBuckets", r18_runtime)
        self.assertIn("function isGridAtBottom", r18_runtime)
        self.assertIn('import "AgentPaneRuntime.js" as AgentPaneRuntime', agent_pane)
        self.assertIn("AgentPaneRuntime.currentSessionTitle", agent_pane)
        self.assertIn("function currentSessionTitle", agent_runtime)
        self.assertIn("function sessionSummary", agent_runtime)
        self.assertIn("function skillModeHint", agent_runtime)

    def test_heavy_moments_controller_concerns_are_split(self):
        controller = (QML_DIR / "features/moments/MomentsController.cpp").read_text(encoding="utf-8")
        parsing = (QML_DIR / "features/moments/MomentsControllerParsing.cpp").read_text(encoding="utf-8")
        publish = (QML_DIR / "features/moments/MomentsControllerPublish.cpp").read_text(encoding="utf-8")
        requests = (QML_DIR / "features/moments/MomentsControllerRequests.cpp").read_text(encoding="utf-8")
        responses = (QML_DIR / "features/moments/MomentsControllerResponses.cpp").read_text(encoding="utf-8")
        parser = (QML_DIR / "features/moments/MomentsEntryParser.cpp").read_text(encoding="utf-8")

        self.assertIn("MomentEntry MomentsController::parseMomentEntry", parsing)
        self.assertIn("parseMomentEntryFromJson", parsing)
        self.assertIn("MomentEntry parseMomentEntryFromJson", parser)
        self.assertIn("QVector<MomentComment> parseMomentComments", parser)
        self.assertIn("void MomentsController::publishDraftMoment", publish)
        self.assertIn("void MomentsController::toggleLike", requests)
        self.assertIn("void MomentsController::onLoadFeedRsp", responses)
        self.assertNotIn("void MomentsController::publishDraftMoment", controller)
        self.assertNotIn("void MomentsController::onLoadFeedRsp", controller)
        self.assertLess(len(controller.splitlines()), 180)

    def test_heavy_agent_controller_concerns_are_split(self):
        controller = (QML_DIR / "features/agent/AgentController.cpp").read_text(encoding="utf-8")
        chat = (QML_DIR / "features/agent/AgentControllerChat.cpp").read_text(encoding="utf-8")
        models = (QML_DIR / "features/agent/AgentControllerModels.cpp").read_text(encoding="utf-8")
        sessions = (QML_DIR / "features/agent/AgentControllerSessions.cpp").read_text(encoding="utf-8")
        state = (QML_DIR / "features/agent/AgentControllerState.cpp").read_text(encoding="utf-8")

        self.assertIn("void AgentController::onHttpFinish", controller)
        self.assertIn("void AgentController::sendMessage", chat)
        self.assertIn("void AgentController::handleChatRsp", chat)
        self.assertIn("void AgentController::refreshModelList", models)
        self.assertIn("void AgentController::handleModelListRsp", models)
        self.assertIn("void AgentController::loadSessions", sessions)
        self.assertIn("void AgentController::handleSessionRsp", sessions)
        self.assertIn("AgentMessageModel* AgentController::model", state)
        self.assertNotIn("void AgentController::sendMessage", controller)
        self.assertNotIn("void AgentController::handleModelListRsp", controller)
        self.assertNotIn("void AgentController::handleSessionRsp", controller)
        self.assertLess(len(controller.splitlines()), 220)

    def test_heavy_agent_knowledge_concerns_are_split(self):
        knowledge = (QML_DIR / "features/agent/AgentControllerKnowledge.cpp").read_text(encoding="utf-8")
        memory = (QML_DIR / "features/agent/AgentControllerMemory.cpp").read_text(encoding="utf-8")
        tasks = (QML_DIR / "features/agent/AgentControllerAgentTasks.cpp").read_text(encoding="utf-8")

        self.assertIn("void AgentController::uploadDocument", knowledge)
        self.assertIn("void AgentController::handleKbRsp", knowledge)
        self.assertIn("void AgentController::listMemories", memory)
        self.assertIn("void AgentController::handleMemoryRsp", memory)
        self.assertIn("void AgentController::listAgentTasks", tasks)
        self.assertIn("void AgentController::handleAgentTaskRsp", tasks)
        self.assertNotIn("void AgentController::listMemories", knowledge)
        self.assertNotIn("void AgentController::listAgentTasks", knowledge)
        self.assertLess(len(knowledge.splitlines()), 260)

    def test_heavy_agent_game_controller_concerns_are_split(self):
        network = (QML_DIR / "features/agent/AgentControllerGameNetwork.cpp").read_text(encoding="utf-8")
        responses = (QML_DIR / "features/agent/AgentControllerGameResponses.cpp").read_text(encoding="utf-8")
        rooms = (QML_DIR / "features/agent/AgentControllerGameRooms.cpp").read_text(encoding="utf-8")
        templates = (QML_DIR / "features/agent/AgentControllerGameTemplates.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "features/agent/AgentControllerGame.cpp").exists())
        self.assertIn("void AgentController::sendGameGet", network)
        self.assertIn("void AgentController::sendGamePost", network)
        self.assertIn("void AgentController::handleGameResponse", responses)
        self.assertIn("void AgentController::createGameRoom", rooms)
        self.assertIn("void AgentController::submitGameAction", rooms)
        self.assertIn("void AgentController::saveGameTemplate", templates)
        self.assertIn("bool AgentController::importGameTemplate", templates)
        self.assertLess(len(network.splitlines()), 220)
        self.assertLess(len(responses.splitlines()), 270)
        self.assertLess(len(rooms.splitlines()), 260)

    def test_heavy_chat_message_model_concerns_are_split(self):
        model = (QML_DIR / "features/chat/ChatMessageModel.cpp").read_text(encoding="utf-8")
        content = (QML_DIR / "features/chat/ChatMessageModelContent.cpp").read_text(encoding="utf-8")
        mutations = (QML_DIR / "features/chat/ChatMessageModelMutations.cpp").read_text(encoding="utf-8")
        presentation = (QML_DIR / "features/chat/ChatMessageModelPresentation.cpp").read_text(encoding="utf-8")
        queries = (QML_DIR / "features/chat/ChatMessageModelQueries.cpp").read_text(encoding="utf-8")

        self.assertIn("QVariant ChatMessageModel::data", model)
        self.assertIn("ChatMessageModel::MessageEntry ChatMessageModel::toEntry", content)
        self.assertIn("void ChatMessageModel::setDownloadAuthContext", content)
        self.assertIn("void ChatMessageModel::setMessagesAtomic", mutations)
        self.assertIn("bool ChatMessageModel::patchMessageContent", mutations)
        self.assertIn("QString ChatMessageModel::timeDividerText", presentation)
        self.assertIn("void ChatMessageModel::recomputeAvatarFlags", presentation)
        self.assertIn("QString ChatMessageModel::exportRecentText", queries)
        self.assertNotIn("void ChatMessageModel::setMessagesAtomic", model)
        self.assertNotIn("ChatMessageModel::MessageEntry ChatMessageModel::toEntry", model)
        self.assertLess(len(model.splitlines()), 190)

    def test_heavy_group_event_response_concerns_are_split(self):
        events = (QML_DIR / "app/AppControllerGroupEvents.cpp").read_text(encoding="utf-8")
        responses = (QML_DIR / "app/AppControllerGroupResponses.cpp").read_text(encoding="utf-8")
        errors = (QML_DIR / "app/AppControllerGroupResponseErrors.cpp").read_text(encoding="utf-8")
        management = (QML_DIR / "app/AppControllerGroupManagementResponses.cpp").read_text(encoding="utf-8")
        history = (QML_DIR / "app/AppControllerGroupHistoryResponses.cpp").read_text(encoding="utf-8")
        group_messages = (QML_DIR / "app/AppControllerGroupMessageResponses.cpp").read_text(encoding="utf-8")
        private_messages = (QML_DIR / "app/AppControllerPrivateMessageResponses.cpp").read_text(encoding="utf-8")
        dialog_meta = (QML_DIR / "app/AppControllerDialogMetaResponses.cpp").read_text(encoding="utf-8")

        self.assertIn("void AppController::onGroupChatMsg", events)
        self.assertIn("void AppController::onGroupRsp", responses)
        self.assertIn("handleGroupRspError(reqId, error, payload)", responses)
        self.assertIn("bool AppController::handleGroupRspError", errors)
        self.assertIn("void AppController::handleGroupManagementRsp", management)
        self.assertIn("void AppController::handleGroupHistoryRsp", history)
        self.assertIn("void AppController::handleGroupMessageAckRsp", group_messages)
        self.assertIn("void AppController::handleGroupMessageMutationRsp", group_messages)
        self.assertIn("void AppController::handlePrivateMessageMutationRsp", private_messages)
        self.assertIn("void AppController::handlePrivateForwardRsp", private_messages)
        self.assertIn("void AppController::handleDialogMetaRsp", dialog_meta)
        self.assertIn("case ID_GROUP_HISTORY_RSP", responses)
        self.assertIn("handleGroupHistoryRsp(payload)", responses)
        self.assertNotIn("MessagePayloadService::parseMessageUpdateFields", responses)
        self.assertNotIn("MessagePayloadService::buildGroupAckMessage", responses)
        self.assertNotIn("saveDraftStore(ownerUid)", responses)
        self.assertNotIn("void AppController::onGroupRsp", events)
        self.assertLess(len(events.splitlines()), 260)
        self.assertLess(len(responses.splitlines()), 180)

    def test_heavy_group_command_management_concerns_are_split(self):
        commands = (QML_DIR / "app/AppControllerGroupCommands.cpp").read_text(encoding="utf-8")
        management = (QML_DIR / "app/AppControllerGroupManagement.cpp").read_text(encoding="utf-8")

        self.assertIn("void AppController::loadGroupHistory", commands)
        self.assertIn("void AppController::requestGroupHistoryForBootstrap", commands)
        self.assertIn("void AppController::updateGroupAnnouncement", management)
        self.assertIn("void AppController::updateGroupIcon", management)
        self.assertIn("void AppController::setGroupAdmin", management)
        self.assertIn("void AppController::muteGroupMember", management)
        self.assertIn("void AppController::kickGroupMember", management)
        self.assertIn("void AppController::quitCurrentGroup", management)
        self.assertIn("void AppController::dissolveCurrentGroup", management)
        self.assertNotIn("void AppController::updateGroupIcon", commands)
        self.assertLess(len(commands.splitlines()), 360)

    def test_heavy_app_coordinator_concerns_are_split(self):
        shell = (QML_DIR / "app/AppCoordinators.cpp").read_text(encoding="utf-8")
        session = (QML_DIR / "app/AppSessionCoordinator.cpp").read_text(encoding="utf-8")
        session_auth = (QML_DIR / "app/SessionAuthCoordinator.cpp").read_text(encoding="utf-8")
        session_auth_commands = (QML_DIR / "app/SessionAuthCoordinatorCommands.cpp").read_text(encoding="utf-8")
        session_auth_login = (QML_DIR / "app/SessionAuthCoordinatorLoginResponse.cpp").read_text(encoding="utf-8")
        session_auth_responses = (QML_DIR / "app/SessionAuthCoordinatorAuthResponses.cpp").read_text(encoding="utf-8")
        session_chat = (QML_DIR / "app/SessionChatEntryCoordinator.cpp").read_text(encoding="utf-8")
        session_relation = (QML_DIR / "app/SessionRelationBootstrap.cpp").read_text(encoding="utf-8")
        session_countdown = (QML_DIR / "app/RegisterCountdownController.cpp").read_text(encoding="utf-8")
        contact = (QML_DIR / "app/ContactCoordinatorShell.cpp").read_text(encoding="utf-8")
        call = (QML_DIR / "app/CallCoordinator.cpp").read_text(encoding="utf-8")
        profile = (QML_DIR / "app/ProfileCoordinator.cpp").read_text(encoding="utf-8")
        group = (QML_DIR / "app/GroupCoordinator.cpp").read_text(encoding="utf-8")
        media = (QML_DIR / "app/MediaCoordinator.cpp").read_text(encoding="utf-8")

        self.assertIn("AppSessionCoordinator::AppSessionCoordinator", session)
        self.assertIn("void AppSessionCoordinator::login", session)
        self.assertIn("SessionAuthCoordinator::SessionAuthCoordinator", session_auth)
        self.assertIn("void SessionAuthCoordinator::login", session_auth_commands)
        self.assertIn("void SessionAuthCoordinator::onLoginHttpFinished", session_auth_login)
        self.assertIn("void SessionAuthCoordinator::onRegisterHttpFinished", session_auth_responses)
        self.assertNotIn("void SessionAuthCoordinator::login", session_auth)
        self.assertNotIn("void SessionAuthCoordinator::onLoginHttpFinished", session_auth)
        self.assertNotIn("void SessionAuthCoordinator::onRegisterHttpFinished", session_auth)
        self.assertIn("SessionChatEntryCoordinator::SessionChatEntryCoordinator", session_chat)
        self.assertIn("void SessionChatEntryCoordinator::runPostLoginBootstrap", session_chat)
        self.assertIn("SessionRelationBootstrap::SessionRelationBootstrap", session_relation)
        self.assertIn("void SessionRelationBootstrap::onRelationBootstrapUpdated", session_relation)
        self.assertIn("RegisterCountdownController::RegisterCountdownController", session_countdown)
        self.assertIn("void RegisterCountdownController::onRegisterCountdownTimeout", session_countdown)
        self.assertNotIn("QTimer::singleShot", session)
        self.assertNotIn("GetFriendListSnapshot", session)
        self.assertNotIn("gateAuthBusinessErrorTip", session)
        self.assertIn("ContactCoordinatorShell::ContactCoordinatorShell", contact)
        self.assertIn("void ContactCoordinatorShell::approveFriend", contact)
        self.assertIn("CallCoordinator::CallCoordinator", call)
        self.assertIn("void CallCoordinator::endCurrentCall", call)
        self.assertIn("ProfileCoordinator::ProfileCoordinator", profile)
        self.assertIn("void ProfileCoordinator::chooseAvatar", profile)
        self.assertIn("GroupCoordinator::GroupCoordinator", group)
        self.assertIn("void GroupCoordinator::createGroup", group)
        self.assertIn("MediaCoordinator::MediaCoordinator", media)
        self.assertIn("void MediaCoordinator::sendCurrentComposerPayload", media)
        self.assertNotIn("void MediaCoordinator::sendTextMessage", shell)
        self.assertNotIn("void GroupCoordinator::createGroup", shell)
        self.assertLess(len(session.splitlines()), 90)
        self.assertLess(len(shell.splitlines()), 40)

    def test_heavy_app_controller_model_concerns_are_split(self):
        models = (QML_DIR / "app/AppControllerModels.cpp").read_text(encoding="utf-8")
        dialogs = (QML_DIR / "app/AppControllerDialogModels.cpp").read_text(encoding="utf-8")
        history = (QML_DIR / "app/AppControllerPrivateHistory.cpp").read_text(encoding="utf-8")
        read_acks = (QML_DIR / "app/AppControllerReadAcks.cpp").read_text(encoding="utf-8")
        private_selection = (QML_DIR / "app/AppControllerPrivateSelection.cpp").read_text(encoding="utf-8")

        self.assertIn("void AppController::bootstrapDialogs", models)
        self.assertIn("void AppController::refreshGroupModel", models)
        self.assertIn("void AppController::refreshDialogModel", dialogs)
        self.assertIn("void AppController::refreshDialogModelIncremental", dialogs)
        self.assertIn("void AppController::loadCurrentChatMessages", history)
        self.assertIn("void AppController::requestPrivateHistory", history)
        self.assertIn("void AppController::sendGroupReadAck", read_acks)
        self.assertIn("void AppController::sendPrivateReadAck", read_acks)
        self.assertIn("void AppController::selectChatByUid", private_selection)
        self.assertNotIn("void AppController::loadCurrentChatMessages", models)
        self.assertNotIn("void AppController::refreshDialogModelIncremental", models)
        self.assertNotIn("void AppController::selectChatByUid", models)
        self.assertLess(len(models.splitlines()), 180)

    def test_heavy_app_controller_selection_concerns_are_split(self):
        private_selection = (QML_DIR / "app/AppControllerPrivateSelection.cpp").read_text(encoding="utf-8")
        contact_selection = (QML_DIR / "app/AppControllerContactSelection.cpp").read_text(encoding="utf-8")
        group_selection = (QML_DIR / "app/AppControllerGroupSelection.cpp").read_text(encoding="utf-8")
        dialog_selection = (QML_DIR / "app/AppControllerDialogSelection.cpp").read_text(encoding="utf-8")
        pagination = (QML_DIR / "app/AppControllerPagination.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/AppControllerSelection.cpp").exists())
        self.assertIn("void AppController::selectChatIndex", private_selection)
        self.assertIn("void AppController::selectChatByUid", private_selection)
        self.assertIn("void AppController::selectContactIndex", contact_selection)
        self.assertIn("QVariantMap AppController::contactProfileByUid", contact_selection)
        self.assertIn("void AppController::deleteFriend", contact_selection)
        self.assertIn("void AppController::selectGroupIndex", group_selection)
        self.assertIn("void AppController::selectGroupByDialogUid", group_selection)
        self.assertIn("void AppController::selectDialogByUid", dialog_selection)
        self.assertIn("void AppController::showApplyRequests", dialog_selection)
        self.assertIn("void AppController::jumpChatWithCurrentContact", dialog_selection)
        self.assertIn("void AppController::loadMoreChats", pagination)
        self.assertIn("void AppController::loadMorePrivateHistory", pagination)
        self.assertIn("void AppController::loadMoreContacts", pagination)
        self.assertNotIn("selectGroupIndex", private_selection)
        self.assertNotIn("selectChatIndex", group_selection)
        self.assertLess(len(private_selection.splitlines()), 170)
        self.assertLess(len(group_selection.splitlines()), 260)

    def test_heavy_app_controller_property_concerns_are_split(self):
        ui_props = (QML_DIR / "app/AppControllerUiProperties.cpp").read_text(encoding="utf-8")
        user_props = (QML_DIR / "app/AppControllerUserProperties.cpp").read_text(encoding="utf-8")
        group_props = (QML_DIR / "app/AppControllerGroupProperties.cpp").read_text(encoding="utf-8")
        model_props = (QML_DIR / "app/AppControllerModelProperties.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/AppControllerProperties.cpp").exists())
        self.assertIn("QString AppController::tipText", ui_props)
        self.assertIn("bool AppController::chatShellBusy", ui_props)
        self.assertIn("QString AppController::currentUserName", user_props)
        self.assertIn("QString AppController::currentContactName", user_props)
        self.assertIn("QString AppController::currentChatPeerName", user_props)
        self.assertIn("int AppController::currentGroupRole", group_props)
        self.assertIn("bool AppController::currentGroupCanChangeInfo", group_props)
        self.assertRegex(model_props, r"FriendListModel\s*\*\s*AppController::dialogListModel")
        self.assertRegex(model_props, r"PetController\s*\*\s*AppController::petController")
        self.assertNotIn("currentGroupCanChangeInfo", ui_props)
        self.assertNotIn("dialogListModel", group_props)
        self.assertLess(len(ui_props.splitlines()), 220)
        self.assertLess(len(model_props.splitlines()), 130)

    def test_heavy_app_controller_state_concerns_are_split(self):
        header = (QML_DIR / "app/AppController.h").read_text(encoding="utf-8")
        connection_state_header = (QML_DIR / "app/AppControllerConnectionState.h").read_text(encoding="utf-8")
        runtime_state_header = (QML_DIR / "app/AppControllerRuntimeState.h").read_text(encoding="utf-8")
        user_state_header = (QML_DIR / "app/AppControllerUserState.h").read_text(encoding="utf-8")
        contact_state = (QML_DIR / "app/AppControllerContactState.cpp").read_text(encoding="utf-8")
        status_state = (QML_DIR / "app/AppControllerStatusState.cpp").read_text(encoding="utf-8")
        loading_state = (QML_DIR / "app/AppControllerLoadingState.cpp").read_text(encoding="utf-8")
        dialog_state = (QML_DIR / "app/AppControllerDialogRuntimeState.cpp").read_text(encoding="utf-8")
        profile_state = (QML_DIR / "app/AppControllerProfileState.cpp").read_text(encoding="utf-8")
        bootstrap_state = (QML_DIR / "app/AppControllerBootstrapState.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/AppControllerState.cpp").exists())
        self.assertIn("struct AppPendingLoginState", connection_state_header)
        self.assertIn("struct AppChatEndpointState", connection_state_header)
        self.assertIn("struct AppChatRecoveryState", connection_state_header)
        self.assertIn("AppPendingLoginState _pending_login_state", header)
        self.assertIn("AppChatEndpointState _chat_endpoint_state", header)
        self.assertIn("AppChatRecoveryState _chat_recovery_state", header)
        self.assertNotIn("int _pending_uid", header)
        self.assertNotIn("QString _chat_server_host", header)
        self.assertNotIn("bool _reconnecting_chat", header)
        self.assertNotIn("qint64 _last_heartbeat_sent_ms", header)
        self.assertIn("struct AppShellRuntimeState", runtime_state_header)
        self.assertIn("struct AppSearchRuntimeState", runtime_state_header)
        self.assertIn("struct AppFeatureStatusState", runtime_state_header)
        self.assertIn("struct AppLoadingRuntimeState", runtime_state_header)
        self.assertIn("struct AppLazyBootstrapState", runtime_state_header)
        self.assertIn("struct AppMediaUploadRuntimeState", runtime_state_header)
        self.assertIn("AppShellRuntimeState _shell_state", header)
        self.assertIn("AppSearchRuntimeState _search_state", header)
        self.assertIn("AppFeatureStatusState _feature_status_state", header)
        self.assertIn("AppLoadingRuntimeState _loading_state", header)
        self.assertIn("AppLazyBootstrapState _bootstrap_state", header)
        self.assertIn("AppMediaUploadRuntimeState _media_upload_state", header)
        self.assertNotIn("QString _tip_text", header)
        self.assertNotIn("bool _search_pending", header)
        self.assertNotIn("QString _auth_status_text", header)
        self.assertNotIn("bool _chat_loading_more", header)
        self.assertNotIn("bool _dialogs_ready", header)
        self.assertNotIn("bool _media_upload_in_progress", header)
        self.assertIn("struct AppCurrentUserState", user_state_header)
        self.assertIn("struct AppCurrentContactState", user_state_header)
        self.assertIn("struct AppCurrentChatState", user_state_header)
        self.assertIn("AppCurrentUserState _user_state", header)
        self.assertIn("AppCurrentContactState _contact_state", header)
        self.assertIn("AppCurrentChatState _chat_state", header)
        self.assertNotIn("QString _current_user_name", header)
        self.assertNotIn("QString _current_contact_name", header)
        self.assertNotIn("QString _current_chat_peer_name", header)
        self.assertIn("void AppController::setContactPane", contact_state)
        self.assertIn("void AppController::setCurrentContact", contact_state)
        self.assertIn("void AppController::setCurrentChatPeerName", contact_state)
        self.assertIn("void AppController::setAuthStatus", status_state)
        self.assertIn("void AppController::setGroupStatus", status_state)
        self.assertIn("void AppController::setPage", status_state)
        self.assertIn("void AppController::setChatLoadingMore", loading_state)
        self.assertIn("void AppController::refreshChatLoadMoreState", loading_state)
        self.assertIn("void AppController::setCurrentDraftText", dialog_state)
        self.assertIn("void AppController::setPendingReplyContext", dialog_state)
        self.assertIn("void AppController::applyCurrentUserProfile", profile_state)
        self.assertIn("QString AppController::normalizeIconPath", profile_state)
        self.assertIn("void AppController::setDialogsReady", bootstrap_state)
        self.assertIn("void AppController::setApplyReady", bootstrap_state)
        self.assertNotIn("applyCurrentUserProfile", contact_state)
        self.assertNotIn("setPendingReplyContext", status_state)
        self.assertLess(len(contact_state.splitlines()), 100)
        self.assertLess(len(profile_state.splitlines()), 110)

    def test_heavy_app_controller_connection_concerns_are_split(self):
        login = (QML_DIR / "app/AppControllerConnectionLogin.cpp").read_text(encoding="utf-8")
        heartbeat = (QML_DIR / "app/AppControllerHeartbeat.cpp").read_text(encoding="utf-8")
        reconnect = (QML_DIR / "app/AppControllerReconnect.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/AppControllerConnection.cpp").exists())
        self.assertIn("void AppController::onTcpConnectFinished", login)
        self.assertIn("void AppController::onChatLoginFailed", login)
        self.assertIn("void AppController::onHeartbeatTimeout", heartbeat)
        self.assertIn("void AppController::onHeartbeatAck", heartbeat)
        self.assertIn("void AppController::onNotifyOffline", heartbeat)
        self.assertIn("void AppController::onConnectionClosed", reconnect)
        self.assertIn("bool AppController::tryLoginFallbackToTcp", reconnect)
        self.assertIn("bool AppController::tryReconnectChat", reconnect)
        self.assertIn("void AppController::resetHeartbeatTracking", reconnect)
        self.assertNotIn("tryReconnectChat", login)
        self.assertNotIn("onTcpConnectFinished", heartbeat)
        self.assertLess(len(login.splitlines()), 120)
        self.assertLess(len(heartbeat.splitlines()), 110)

    def test_heavy_app_controller_session_concerns_are_split(self):
        coordinator = (QML_DIR / "app/AppSessionCoordinator.cpp").read_text(encoding="utf-8")
        auth_coordinator = (QML_DIR / "app/SessionAuthCoordinator.cpp").read_text(encoding="utf-8")
        auth_commands = (QML_DIR / "app/SessionAuthCoordinatorCommands.cpp").read_text(encoding="utf-8")
        auth_login = (QML_DIR / "app/SessionAuthCoordinatorLoginResponse.cpp").read_text(encoding="utf-8")
        auth_responses = (QML_DIR / "app/SessionAuthCoordinatorAuthResponses.cpp").read_text(encoding="utf-8")
        chat_entry = (QML_DIR / "app/SessionChatEntryCoordinator.cpp").read_text(encoding="utf-8")
        relation = (QML_DIR / "app/SessionRelationBootstrap.cpp").read_text(encoding="utf-8")
        countdown = (QML_DIR / "app/RegisterCountdownController.cpp").read_text(encoding="utf-8")
        login = (QML_DIR / "app/AppControllerLoginSession.cpp").read_text(encoding="utf-8")
        chat_bootstrap = (QML_DIR / "app/AppControllerChatBootstrap.cpp").read_text(encoding="utf-8")
        relation_bootstrap = (QML_DIR / "app/AppControllerRelationBootstrap.cpp").read_text(encoding="utf-8")
        register_countdown = (QML_DIR / "app/AppControllerRegisterCountdown.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/AppControllerSession.cpp").exists())
        self.assertIn("void AppSessionCoordinator::onLoginHttpFinished", coordinator)
        self.assertIn("SessionAuthCoordinator::SessionAuthCoordinator", auth_coordinator)
        self.assertIn("void SessionAuthCoordinator::login", auth_commands)
        self.assertIn("void SessionAuthCoordinator::onLoginHttpFinished", auth_login)
        self.assertIn("ChatTransportKind parseTransportKind", auth_login)
        self.assertIn("void SessionAuthCoordinator::onRegisterHttpFinished", auth_responses)
        self.assertIn("void SessionAuthCoordinator::onResetHttpFinished", auth_responses)
        self.assertIn("void SessionChatEntryCoordinator::onSwitchToChat", chat_entry)
        self.assertIn("void SessionChatEntryCoordinator::runPostLoginBootstrap", chat_entry)
        self.assertIn("void SessionRelationBootstrap::requestRelationBootstrap", relation)
        self.assertIn("void SessionRelationBootstrap::onRelationBootstrapUpdated", relation)
        self.assertIn("void RegisterCountdownController::onRegisterCountdownTimeout", countdown)
        self.assertIn("void AppController::onLoginHttpFinished", login)
        self.assertIn("void AppController::onSwitchToChat", chat_bootstrap)
        self.assertIn("void AppController::runPostLoginBootstrap", chat_bootstrap)
        self.assertIn("void AppController::requestRelationBootstrap", relation_bootstrap)
        self.assertIn("void AppController::onRelationBootstrapUpdated", relation_bootstrap)
        self.assertIn("void AppController::onRegisterCountdownTimeout", register_countdown)
        self.assertIn("_session_coordinator->onLoginHttpFinished", login)
        self.assertIn("_session_coordinator->onSwitchToChat", chat_bootstrap)
        self.assertIn("_session_coordinator->runPostLoginBootstrap", chat_bootstrap)
        self.assertIn("_session_coordinator->requestRelationBootstrap", relation_bootstrap)
        self.assertIn("_session_coordinator->onRegisterCountdownTimeout", register_countdown)
        self.assertIn("_auth->onLoginHttpFinished", coordinator)
        self.assertIn("_chat_entry->onSwitchToChat", coordinator)
        self.assertIn("_relation_bootstrap->requestRelationBootstrap", coordinator)
        self.assertIn("_register_countdown->onRegisterCountdownTimeout", coordinator)
        self.assertNotIn("parseTransportKind", coordinator)
        self.assertNotIn("parseTransportKind", login)
        self.assertNotIn("parseTransportKind", auth_coordinator)
        self.assertNotIn("gateAuthBusinessErrorTip", auth_coordinator)
        self.assertNotIn("QTimer::singleShot", chat_bootstrap)
        self.assertNotIn("GetFriendListSnapshot", relation_bootstrap)
        self.assertNotIn("onRelationBootstrapUpdated", login)
        self.assertNotIn("onLoginHttpFinished", chat_bootstrap)
        self.assertLess(len(login.splitlines()), 12)
        self.assertLess(len(chat_bootstrap.splitlines()), 20)
        self.assertLess(len(register_countdown.splitlines()), 40)

    def test_session_auth_workflow_is_owned_by_session_coordinator(self):
        coordinator = (QML_DIR / "app/AppSessionCoordinator.cpp").read_text(encoding="utf-8")
        auth_coordinator = (QML_DIR / "app/SessionAuthCoordinator.cpp").read_text(encoding="utf-8")
        auth_commands = (QML_DIR / "app/SessionAuthCoordinatorCommands.cpp").read_text(encoding="utf-8")
        auth_login = (QML_DIR / "app/SessionAuthCoordinatorLoginResponse.cpp").read_text(encoding="utf-8")
        auth_responses = (QML_DIR / "app/SessionAuthCoordinatorAuthResponses.cpp").read_text(encoding="utf-8")
        chat_entry = (QML_DIR / "app/SessionChatEntryCoordinator.cpp").read_text(encoding="utf-8")
        relation = (QML_DIR / "app/SessionRelationBootstrap.cpp").read_text(encoding="utf-8")
        countdown = (QML_DIR / "app/RegisterCountdownController.cpp").read_text(encoding="utf-8")
        login = (QML_DIR / "app/AppControllerLoginSession.cpp").read_text(encoding="utf-8")
        app_auth_responses = (QML_DIR / "app/AppControllerAuthResponses.cpp").read_text(encoding="utf-8")
        chat_bootstrap = (QML_DIR / "app/AppControllerChatBootstrap.cpp").read_text(encoding="utf-8")
        relation_bootstrap = (QML_DIR / "app/AppControllerRelationBootstrap.cpp").read_text(encoding="utf-8")
        register_countdown = (QML_DIR / "app/AppControllerRegisterCountdown.cpp").read_text(encoding="utf-8")

        self.assertIn("void AppSessionCoordinator::onLoginHttpFinished", coordinator)
        self.assertIn("SessionAuthCoordinator::SessionAuthCoordinator", auth_coordinator)
        self.assertIn("void SessionAuthCoordinator::onRegisterHttpFinished", auth_responses)
        self.assertIn("void SessionAuthCoordinator::onResetHttpFinished", auth_responses)
        self.assertIn("void SessionChatEntryCoordinator::onSwitchToChat", chat_entry)
        self.assertIn("void SessionRelationBootstrap::requestRelationBootstrap", relation)
        self.assertIn("void SessionRelationBootstrap::onRelationBootstrapUpdated", relation)
        self.assertIn("void RegisterCountdownController::onRegisterCountdownTimeout", countdown)
        self.assertIn("_app._auth_controller.sendLogin", auth_commands)
        self.assertIn("parseTransportKind", auth_login)
        self.assertIn("gateAuthBusinessErrorTip", auth_responses)
        self.assertIn("_app._register_countdown_timer.start(1000)", auth_responses)
        self.assertNotIn("_app._auth_controller.sendLogin", auth_coordinator)
        self.assertNotIn("parseTransportKind", auth_coordinator)
        self.assertNotIn("gateAuthBusinessErrorTip", auth_coordinator)

        self.assertIn("_session_coordinator->onLoginHttpFinished", login)
        self.assertIn("_session_coordinator->onRegisterHttpFinished", app_auth_responses)
        self.assertIn("_session_coordinator->onResetHttpFinished", app_auth_responses)
        self.assertIn("_session_coordinator->onSwitchToChat", chat_bootstrap)
        self.assertIn("_session_coordinator->requestRelationBootstrap", relation_bootstrap)
        self.assertIn("_session_coordinator->onRelationBootstrapUpdated", relation_bootstrap)
        self.assertIn("_session_coordinator->onRegisterCountdownTimeout", register_countdown)

        self.assertNotIn("_auth_controller.sendLogin", login)
        self.assertNotIn("_register_countdown_timer.start(1000)", app_auth_responses)
        self.assertNotIn("_auth_controller.sendLogin", coordinator)
        self.assertNotIn("gateAuthBusinessErrorTip", coordinator)
        self.assertNotIn("QTimer::singleShot", chat_bootstrap)
        self.assertNotIn("GetFriendListSnapshot", relation_bootstrap)
        self.assertLess(len(register_countdown.splitlines()), 20)

    def test_heavy_friend_list_model_concerns_are_split(self):
        model = (QML_DIR / "features/contact/FriendListModel.cpp").read_text(encoding="utf-8")
        mutations = (QML_DIR / "features/contact/FriendListModelMutations.cpp").read_text(encoding="utf-8")
        state = (QML_DIR / "features/contact/FriendListModelState.cpp").read_text(encoding="utf-8")
        queries = (QML_DIR / "features/contact/FriendListModelQueries.cpp").read_text(encoding="utf-8")

        self.assertIn("QVariant FriendListModel::data", model)
        self.assertIn("QHash<int, QByteArray> FriendListModel::roleNames", model)
        self.assertIn("void FriendListModel::setFriends", mutations)
        self.assertIn("void FriendListModel::upsertFriend", mutations)
        self.assertIn("void FriendListModel::upsertBatch", mutations)
        self.assertIn("void FriendListModel::updateLastMessage", state)
        self.assertIn("void FriendListModel::incrementUnread", state)
        self.assertIn("void FriendListModel::setDialogMeta", state)
        self.assertIn("QVariantMap FriendListModel::get", queries)
        self.assertIn("int FriendListModel::indexOfUid", queries)
        self.assertNotIn("void FriendListModel::setFriends", model)
        self.assertNotIn("void FriendListModel::setDialogMeta", model)
        self.assertNotIn("QVariantMap FriendListModel::get", model)
        self.assertLess(len(model.splitlines()), 150)

    def test_heavy_app_controller_media_concerns_are_split(self):
        media = (QML_DIR / "app/AppControllerMedia.cpp").read_text(encoding="utf-8")
        attachments = (QML_DIR / "app/AppControllerPendingAttachments.cpp").read_text(encoding="utf-8")
        upload_queue = (QML_DIR / "app/AppControllerMediaUploadQueue.cpp").read_text(encoding="utf-8")
        dispatch = (QML_DIR / "app/AppControllerMessageDispatch.cpp").read_text(encoding="utf-8")
        call_target = (QML_DIR / "app/AppControllerCallTarget.cpp").read_text(encoding="utf-8")

        self.assertIn("void AppController::sendTextMessage", media)
        self.assertIn("void AppController::sendCurrentComposerPayload", media)
        self.assertIn("void AppController::sendImageMessage", media)
        self.assertIn("void AppController::startMediaUploadAndSend", attachments)
        self.assertIn("void AppController::addPendingAttachments", attachments)
        self.assertIn("void AppController::removePendingAttachmentById", attachments)
        self.assertIn("void AppController::processPendingAttachmentQueue", upload_queue)
        self.assertIn("bool AppController::sendUploadedAttachmentToDialog", upload_queue)
        self.assertIn("bool AppController::dispatchChatContent", dispatch)
        self.assertIn("bool AppController::dispatchGroupChatContent", dispatch)
        self.assertIn("bool AppController::ensureCallTargetFromCurrentChat", call_target)
        self.assertIn("void AppController::sendCallInvite", call_target)
        self.assertNotIn("void AppController::processPendingAttachmentQueue", media)
        self.assertNotIn("bool AppController::dispatchGroupChatContent", media)
        self.assertNotIn("bool AppController::ensureCallTargetFromCurrentChat", media)
        self.assertLess(len(media.splitlines()), 90)

    def test_heavy_private_event_concerns_are_split(self):
        events = (QML_DIR / "app/AppControllerPrivateEvents.cpp").read_text(encoding="utf-8")
        status = (QML_DIR / "app/AppControllerMessageStatus.cpp").read_text(encoding="utf-8")
        history = (QML_DIR / "app/AppControllerPrivateHistoryResponses.cpp").read_text(encoding="utf-8")
        message_events = (QML_DIR / "app/AppControllerPrivateMessageEvents.cpp").read_text(encoding="utf-8")
        read_events = (QML_DIR / "app/AppControllerPrivateReadEvents.cpp").read_text(encoding="utf-8")

        self.assertIn("void AppController::onTextChatMsg", events)
        self.assertIn("void AppController::onMessageStatus", status)
        self.assertIn("void AppController::onPrivateHistoryRsp", history)
        self.assertIn("void AppController::onPrivateMsgChanged", message_events)
        self.assertIn("void AppController::onPrivateReadAck", read_events)
        self.assertNotIn("void AppController::onMessageStatus", events)
        self.assertNotIn("void AppController::onPrivateHistoryRsp", events)
        self.assertNotIn("void AppController::onPrivateMsgChanged", events)
        self.assertNotIn("void AppController::onPrivateReadAck", events)
        self.assertLess(len(events.splitlines()), 130)

    def test_heavy_r18_controller_concerns_are_split(self):
        controller = (QML_DIR / "features/r18/R18Controller.cpp").read_text(encoding="utf-8")
        network = (QML_DIR / "features/r18/R18ControllerNetwork.cpp").read_text(encoding="utf-8")
        responses = (QML_DIR / "features/r18/R18ControllerResponses.cpp").read_text(encoding="utf-8")
        sources = (QML_DIR / "features/r18/R18ControllerSources.cpp").read_text(encoding="utf-8")
        state = (QML_DIR / "features/r18/R18ControllerState.cpp").read_text(encoding="utf-8")

        self.assertIn("void R18Controller::refreshSources", controller)
        self.assertIn("void R18Controller::search", controller)
        self.assertIn("void R18Controller::postJson", network)
        self.assertIn("void R18Controller::getJson", network)
        self.assertIn("void R18Controller::handleResponse", responses)
        self.assertIn("void R18Controller::refreshOfficialSources", sources)
        self.assertIn("void R18Controller::downloadAndImportSource", sources)
        self.assertIn("void R18Controller::setLoading", state)
        self.assertIn("void R18Controller::setSearchState", state)
        self.assertNotIn("void R18Controller::postJson", controller)
        self.assertNotIn("void R18Controller::downloadAndImportSource", controller)
        self.assertNotIn("void R18Controller::handleResponse", controller)
        self.assertLess(len(controller.splitlines()), 260)

    def test_heavy_pet_controller_concerns_are_split(self):
        controller = (QML_DIR / "features/pet/PetController.cpp").read_text(encoding="utf-8")
        session = (QML_DIR / "features/pet/PetControllerSession.cpp").read_text(encoding="utf-8")
        state = (QML_DIR / "features/pet/PetControllerState.cpp").read_text(encoding="utf-8")
        runtime = (QML_DIR / "features/pet/PetControllerVoiceRuntime.cpp").read_text(encoding="utf-8")
        training = (QML_DIR / "features/pet/PetControllerVoiceTraining.cpp").read_text(encoding="utf-8")

        self.assertIn("QString PetController::audioUrl", controller)
        self.assertIn("void PetController::startSession", session)
        self.assertIn("void PetController::sendText", session)
        self.assertIn("void PetController::sendObservation", session)
        self.assertIn("void PetController::setBusy", state)
        self.assertIn("QJsonObject PetController::authPayload", state)
        self.assertIn("void PetController::setVoiceRuntimeSettings", runtime)
        self.assertIn("QJsonObject PetController::voiceRuntimeMetadata", runtime)
        self.assertIn("void PetController::startVoiceTraining", training)
        self.assertIn("void PetController::applyVoiceTrainingJob", training)
        self.assertNotIn("void PetController::startSession", controller)
        self.assertNotIn("void PetController::startVoiceTraining", controller)
        self.assertNotIn("void PetController::setVoiceRuntimeSettings", controller)
        self.assertNotIn("QJsonObject PetController::authPayload", controller)
        self.assertLess(len(controller.splitlines()), 180)

    def test_heavy_chat_message_dispatcher_concerns_are_split(self):
        dispatcher = (CORE_DIR / "network/ChatMessageDispatcher.cpp").read_text(encoding="utf-8")
        auth = (CORE_DIR / "network/ChatMessageDispatcherAuth.cpp").read_text(encoding="utf-8")
        contacts = (CORE_DIR / "network/ChatMessageDispatcherContacts.cpp").read_text(encoding="utf-8")
        private = (CORE_DIR / "network/ChatMessageDispatcherPrivate.cpp").read_text(encoding="utf-8")
        group = (CORE_DIR / "network/ChatMessageDispatcherGroup.cpp").read_text(encoding="utf-8")
        system = (CORE_DIR / "network/ChatMessageDispatcherSystem.cpp").read_text(encoding="utf-8")

        self.assertIn("void ChatMessageDispatcher::initHandlers", dispatcher)
        self.assertIn("void ChatMessageDispatcher::dispatchMessage", dispatcher)
        self.assertIn("void ChatMessageDispatcher::registerAuthHandlers", auth)
        self.assertIn("ID_CHAT_LOGIN_RSP", auth)
        self.assertIn("void ChatMessageDispatcher::registerContactHandlers", contacts)
        self.assertIn("ID_SEARCH_USER_RSP", contacts)
        self.assertIn("ID_GET_RELATION_BOOTSTRAP_RSP", contacts)
        self.assertIn("void ChatMessageDispatcher::registerPrivateMessageHandlers", private)
        self.assertIn("ID_PRIVATE_HISTORY_RSP", private)
        self.assertIn("ID_NOTIFY_PRIVATE_READ_ACK_REQ", private)
        self.assertIn("void ChatMessageDispatcher::registerGroupHandlers", group)
        self.assertIn("ID_GROUP_HISTORY_RSP", group)
        self.assertIn("ID_NOTIFY_GROUP_CHAT_MSG_REQ", group)
        self.assertIn("void ChatMessageDispatcher::registerSystemHandlers", system)
        self.assertIn("ID_GET_DIALOG_LIST_RSP", system)
        self.assertIn("ID_HEARTBEAT_RSP", system)
        self.assertNotIn("ID_CHAT_LOGIN_RSP", dispatcher)
        self.assertNotIn("ID_GROUP_HISTORY_RSP", dispatcher)
        self.assertLess(len(dispatcher.splitlines()), 90)

    def test_heavy_media_upload_service_concerns_are_split(self):
        service = (QML_DIR / "shared/media/MediaUploadService.cpp").read_text(encoding="utf-8")
        private_header = (QML_DIR / "shared/media/MediaUploadServicePrivate.h").read_text(encoding="utf-8")
        helpers = (QML_DIR / "shared/media/MediaUploadServiceHelpers.cpp").read_text(encoding="utf-8")
        network = (QML_DIR / "shared/media/MediaUploadServiceNetwork.cpp").read_text(encoding="utf-8")
        uploads = (QML_DIR / "shared/media/MediaUploadServiceUploads.cpp").read_text(encoding="utf-8")

        self.assertIn("bool MediaUploadService::uploadLocalFile", service)
        self.assertIn("struct ClientMediaConfig", private_header)
        self.assertIn("QString resolveLocalPath", helpers)
        self.assertIn("ClientMediaConfig loadMediaConfig", helpers)
        self.assertIn("bool postJson", network)
        self.assertIn("bool postBinary", network)
        self.assertIn("bool getJson", network)
        self.assertIn("bool uploadAvatarFile", uploads)
        self.assertIn("bool uploadChunkedFile", uploads)
        self.assertNotIn("bool postJson", service)
        self.assertNotIn("bool postBinary", service)
        self.assertNotIn("bool uploadChunkedFile", service)
        self.assertLess(len(service.splitlines()), 120)

    def test_heavy_moments_model_concerns_are_split(self):
        model = (QML_DIR / "features/moments/MomentsModel.cpp").read_text(encoding="utf-8")
        mutations = (QML_DIR / "features/moments/MomentsModelMutations.cpp").read_text(encoding="utf-8")
        queries = (QML_DIR / "features/moments/MomentsModelQueries.cpp").read_text(encoding="utf-8")

        self.assertIn("QVariant MomentsModel::data", model)
        self.assertIn("QHash<int, QByteArray> MomentsModel::roleNames", model)
        self.assertIn("void MomentsModel::setMoments", mutations)
        self.assertIn("void MomentsModel::prependOrUpdateMoment", mutations)
        self.assertIn("void MomentsModel::updateDetail", mutations)
        self.assertIn("QVariantMap MomentsModel::get", queries)
        self.assertIn("MomentEntry MomentsModel::getMomentById", queries)
        self.assertIn("QVariantMap MomentsModel::snapshotMoment", queries)
        self.assertNotIn("void MomentsModel::setMoments", model)
        self.assertNotIn("QVariantMap MomentsModel::snapshotMoment", model)
        self.assertLess(len(model.splitlines()), 185)

    def test_heavy_local_file_picker_service_concerns_are_split(self):
        service = (QML_DIR / "shared/media/LocalFilePickerService.cpp").read_text(encoding="utf-8")
        private_header = (QML_DIR / "shared/media/LocalFilePickerServicePrivate.h").read_text(encoding="utf-8")
        attachments = (QML_DIR / "shared/media/LocalFilePickerServiceAttachments.cpp").read_text(encoding="utf-8")
        avatar = (QML_DIR / "shared/media/LocalFilePickerServiceAvatar.cpp").read_text(encoding="utf-8")

        self.assertIn("bool LocalFilePickerService::openUrl", service)
        self.assertIn("QString inferMomentAttachmentType", private_header)
        self.assertIn("QVariantMap buildAttachmentMap", private_header)
        self.assertIn("bool LocalFilePickerService::pickImageUrl", attachments)
        self.assertIn("bool LocalFilePickerService::pickMomentMediaUrls", attachments)
        self.assertIn("bool LocalFilePickerService::pickFileUrls", attachments)
        self.assertIn("bool LocalFilePickerService::pickAvatarUrl", avatar)
        self.assertIn("bool LocalFilePickerService::pickAvatarFromScreen", avatar)
        self.assertIn("bool LocalFilePickerService::pickAvatarFromWebcam", avatar)
        self.assertNotIn("pickAvatarUrl", service)
        self.assertNotIn("pickMomentMediaUrls", service)
        self.assertLess(len(service.splitlines()), 60)

    def test_core_resource_aliases_reference_qml_app_assets(self):
        qrc = CORE_QRC.read_text(encoding="utf-8")

        self.assertIn('<file alias="app/icon.ico">icon.ico</file>', qrc)
        self.assertIn("<file>style/stylesheet.qss</file>", qrc)
        self.assertIn('<file alias="res/head_1.png">../src/head_1.png</file>', qrc)
        self.assertIn('<file alias="res/head_1.jpg">../src/head_1.png</file>', qrc)
        self.assertNotIn("../MemoChat-qml/src", qrc)

    def test_qml_controllers_include_core_headers_through_target_include_path(self):
        source = MOMENTS_CONTROLLER.read_text(encoding="utf-8")

        self.assertIn('#include "httpmgr.h"', source)
        self.assertIn('#include "usermgr.h"', source)
        self.assertNotIn("../MemoChatShared", source)


if __name__ == "__main__":
    unittest.main()
