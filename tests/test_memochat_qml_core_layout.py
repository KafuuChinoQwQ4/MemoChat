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
QML_CMAKE_DIR = QML_DIR / "cmake"
RESOURCE_DIR = QML_DIR / "resources"
RESOURCE_QRC_DIR = RESOURCE_DIR / "qrc"
OLD_QML_CMAKE_FRAGMENTS = (
    QML_CMAKE_DIR / "AppSources.cmake",
    QML_CMAKE_DIR / "FeatureSources.cmake",
    QML_CMAKE_DIR / "SharedSources.cmake",
    QML_CMAKE_DIR / "QmlResources.cmake",
)
QML_CMAKE_MANIFESTS = (
    QML_DIR / "app/sources.cmake",
    QML_DIR / "features/sources.cmake",
    QML_DIR / "features/auth/sources.cmake",
    QML_DIR / "features/call/sources.cmake",
    QML_DIR / "features/chat/sources.cmake",
    QML_DIR / "features/contact/sources.cmake",
    QML_DIR / "features/profile/sources.cmake",
    QML_DIR / "features/moments/sources.cmake",
    QML_DIR / "features/agent/sources.cmake",
    QML_DIR / "features/pet/sources.cmake",
    QML_DIR / "features/r18/sources.cmake",
    QML_DIR / "shared/sources.cmake",
    QML_DIR / "live2d/sources.cmake",
    QML_DIR / "resources/resources.cmake",
)
CORE_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/core"
CORE_CMAKE = CORE_DIR / "CMakeLists.txt"
OLD_SHARED_DIR = REPO_ROOT / "apps/client/desktop/MemoChatShared"
APP_CORE_QRC = RESOURCE_QRC_DIR / "app-core.qrc"
MOMENTS_CONTROLLER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/moments/controller/MomentsController.h"
MOMENTS_RESPONSE_FAMILIES = {
    "feed": QML_DIR / "features/moments/controller/MomentsControllerFeedResponses.cpp",
    "post": QML_DIR / "features/moments/controller/MomentsControllerPostResponses.cpp",
    "comment": QML_DIR / "features/moments/controller/MomentsControllerCommentResponses.cpp",
}
QML_QRC = QML_DIR / "qml.qrc"
LEGACY_QRC_FILES = (
    QML_QRC,
    CORE_DIR / "rc.qrc",
)
DIRECTORY_MANIFESTS = (
    "app/sources.cmake",
    "features/sources.cmake",
    "shared/sources.cmake",
    "live2d/sources.cmake",
    "resources/resources.cmake",
)
RESOURCE_QRC_FILES = (
    "app-core.qrc",
    "qml-shell.qrc",
    "qml-chat.qrc",
    "qml-moments.qrc",
    "qml-agent.qrc",
    "qml-pet.qrc",
    "qml-r18.qrc",
    "icons.qrc",
    "web.qrc",
)


def qml_cmake_text() -> str:
    return (
        QML_CMAKE.read_text(encoding="utf-8")
        + "\n"
        + "\n".join(manifest.read_text(encoding="utf-8") for manifest in QML_CMAKE_MANIFESTS if manifest.exists())
    )


class MemoChatQmlCoreLayoutTests(unittest.TestCase):
    def test_client_cmake_uses_directory_owned_source_manifests(self):
        root = QML_CMAKE.read_text(encoding="utf-8")

        for rel_path in DIRECTORY_MANIFESTS:
            with self.subTest(manifest=rel_path):
                self.assertTrue((QML_DIR / rel_path).is_file())
                self.assertIn(f"include({rel_path})", root)

        for old_fragment in OLD_QML_CMAKE_FRAGMENTS:
            with self.subTest(old_fragment=old_fragment.name):
                self.assertNotIn(f"include(cmake/{old_fragment.name})", root)
                self.assertFalse(old_fragment.exists())

    def test_resource_qrc_files_are_split_by_domain_with_stable_aliases(self):
        resources = QML_DIR / "resources/resources.cmake"
        self.assertTrue(resources.is_file())

        qrc_text = ""
        for name in RESOURCE_QRC_FILES:
            path = RESOURCE_QRC_DIR / name
            with self.subTest(qrc=name):
                self.assertTrue(path.is_file())
            qrc_text += path.read_text(encoding="utf-8") if path.exists() else ""

        for token in (
            'alias="qml/Main.qml"',
            'alias="qml/linux/Main.qml"',
            'alias="qml/LoginPage.qml"',
            'alias="icons/dropdown.png"',
            'alias="res/head_1.png"',
            'alias="res/head_1.jpg"',
            'alias="web/livekit/index.html"',
        ):
            with self.subTest(alias=token):
                self.assertIn(token, qrc_text)

        for legacy_qrc in LEGACY_QRC_FILES:
            with self.subTest(legacy_qrc=legacy_qrc.name):
                self.assertFalse(legacy_qrc.exists())

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
            "app/bootstrap/main.cpp",
            "app/bootstrap/MainPlatformBootstrap.cpp",
            "app/bootstrap/MainQmlTypeRegistry.cpp",
            "app/bootstrap/MainQmlEngineSetup.cpp",
            "app/bootstrap/MainQmlWindowLoader.cpp",
            "app/bootstrap/MainRuntimeConfig.cpp",
            "app/window/MainWindowHooks.cpp",
            "app/controller/AppController.cpp",
            "app/connection/AppControllerConnectionState.h",
            "app/controller/AppControllerDialogStateData.h",
            "app/controller/AppControllerGroupState.h",
            "app/controller/AppControllerPendingSendState.h",
            "app/controller/AppControllerRuntimeState.h",
            "app/controller/AppControllerUserState.h",
            "app/session/AppSessionCoordinator.cpp",
            "app/session/SessionAuthCoordinator.cpp",
            "app/session/SessionAuthCoordinatorCommands.cpp",
            "app/session/SessionAuthCoordinatorLoginResponse.cpp",
            "app/session/SessionAuthCoordinatorAuthResponses.cpp",
            "app/session/SessionChatEntryCoordinator.cpp",
            "app/session/SessionRelationBootstrap.cpp",
            "app/coordinators/RegisterCountdownController.cpp",
            "app/coordinators/CallCoordinator.cpp",
            "app/coordinators/ContactCoordinatorShell.cpp",
            "app/coordinators/GroupCoordinator.cpp",
            "app/coordinators/MediaCoordinator.cpp",
            "app/coordinators/ProfileCoordinator.cpp",
            "app/controller/AppControllerGroupEvents.cpp",
            "app/controller/AppControllerGroupManagement.cpp",
            "app/controller/AppControllerGroupResponses.cpp",
            "app/controller/AppControllerGroupResponseErrors.cpp",
            "app/controller/AppControllerGroupManagementResponses.cpp",
            "app/controller/AppControllerGroupHistoryResponses.cpp",
            "app/controller/AppControllerGroupMessageResponses.cpp",
            "app/controller/AppControllerPrivateMessageResponses.cpp",
            "app/controller/AppControllerDialogMetaResponses.cpp",
            "app/controller/AppControllerDialogModels.cpp",
            "app/controller/AppControllerPrivateHistory.cpp",
            "app/controller/AppControllerReadAcks.cpp",
            "app/controller/AppControllerPrivateSelection.cpp",
            "app/controller/AppControllerContactSelection.cpp",
            "app/controller/AppControllerGroupSelection.cpp",
            "app/controller/AppControllerDialogSelection.cpp",
            "app/controller/AppControllerPagination.cpp",
            "app/controller/AppControllerUiProperties.cpp",
            "app/controller/AppControllerUserProperties.cpp",
            "app/controller/AppControllerGroupProperties.cpp",
            "app/controller/AppControllerModelProperties.cpp",
            "app/controller/AppControllerContactState.cpp",
            "app/controller/AppControllerStatusState.cpp",
            "app/controller/AppControllerLoadingState.cpp",
            "app/controller/AppControllerDialogRuntimeState.cpp",
            "app/controller/AppControllerProfileState.cpp",
            "app/controller/AppControllerBootstrapState.cpp",
            "app/connection/AppChatConnectionCoordinator.cpp",
            "app/connection/AppChatConnectionCoordinator.h",
            "app/connection/AppChatConnectionPolicy.cpp",
            "app/connection/AppChatConnectionPolicy.h",
            "app/connection/AppControllerChatBootstrap.cpp",
            "app/controller/AppControllerRelationBootstrap.cpp",
            "app/controller/AppControllerPendingAttachments.cpp",
            "app/controller/AppControllerMediaUploadQueue.cpp",
            "app/controller/AppControllerMessageDispatch.cpp",
            "app/controller/AppControllerMessageStatus.cpp",
            "app/controller/AppControllerPrivateHistoryResponses.cpp",
            "app/controller/AppControllerPrivateMessageEvents.cpp",
            "app/controller/AppControllerPrivateReadEvents.cpp",
            "features/agent/controller/AgentControllerChat.cpp",
            "features/agent/controller/AgentController.cpp",
            "features/agent/game/AgentControllerGameNetwork.cpp",
            "features/agent/game/AgentControllerGameResponses.cpp",
            "features/agent/game/AgentControllerGameRooms.cpp",
            "features/agent/game/AgentControllerGameTemplates.cpp",
            "features/agent/controller/AgentControllerAgentTasks.cpp",
            "features/agent/controller/AgentControllerKnowledge.cpp",
            "features/agent/controller/AgentControllerMemory.cpp",
            "features/auth/AuthController.cpp",
            "features/chat/cache/ChatCacheMessageCodec.cpp",
            "features/chat/model/ChatMessageModelContent.cpp",
            "features/chat/model/ChatMessageModel.cpp",
            "features/chat/model/ChatMessageModelMutations.cpp",
            "features/chat/model/ChatMessageModelPresentation.cpp",
            "features/chat/model/ChatMessageModelQueries.cpp",
            "features/chat/services/DialogListEntryBuilder.cpp",
            "features/chat/cache/PrivateChatCacheStore.cpp",
            "features/contact/model/FriendListModel.cpp",
            "features/contact/model/FriendListModelMutations.cpp",
            "features/contact/model/FriendListModelState.cpp",
            "features/contact/model/FriendListModelQueries.cpp",
            "features/moments/parsing/MomentsEntryParser.cpp",
            "features/moments/parsing/MomentsControllerParsing.cpp",
            "features/moments/controller/MomentsControllerPublish.cpp",
            "features/moments/controller/MomentsControllerRequests.cpp",
            "features/moments/controller/MomentsControllerFeedResponses.cpp",
            "features/moments/controller/MomentsControllerPostResponses.cpp",
            "features/moments/controller/MomentsControllerCommentResponses.cpp",
            "features/r18/controller/R18Controller.cpp",
            "features/moments/model/MomentsModel.cpp",
            "features/moments/model/MomentsModelMutations.cpp",
            "features/moments/model/MomentsModelQueries.cpp",
            "features/r18/controller/R18ControllerNetwork.cpp",
            "features/r18/controller/R18ControllerResponses.cpp",
            "features/r18/controller/R18ControllerSources.cpp",
            "features/r18/controller/R18ControllerState.cpp",
            "features/pet/controller/PetController.cpp",
            "features/pet/controller/PetControllerSession.cpp",
            "features/pet/controller/PetControllerState.cpp",
            "features/pet/speech/PetControllerVoiceRuntime.cpp",
            "features/pet/speech/PetControllerVoiceTraining.cpp",
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
            "live2d/rendering/Live2DRenderItem.cpp",
        )

        for rel_path in expected_files:
            with self.subTest(path=rel_path):
                self.assertTrue((QML_DIR / rel_path).is_file())

        over_split_cpp_files = (
            "app/AppCoordinators.cpp",
            "app/MainQmlBootstrap.cpp",
            "app/AppControllerLoginSession.cpp",
            "app/AppControllerRegisterCountdown.cpp",
        )
        for rel_path in over_split_cpp_files:
            with self.subTest(over_split=rel_path):
                self.assertFalse((QML_DIR / rel_path).exists())

        root_cpp_or_h = [path for path in QML_DIR.iterdir() if path.is_file() and path.suffix in {".cpp", ".h"}]
        self.assertEqual([], root_cpp_or_h)

        old_horizontal_dirs = ("controllers", "models", "services", "storage", "utils")
        for rel_dir in old_horizontal_dirs:
            with self.subTest(old_dir=rel_dir):
                self.assertFalse((QML_DIR / rel_dir).exists())

    def test_cmake_adds_core_from_qml_module_without_desktop_shared_entry(self):
        desktop_cmake = DESKTOP_CMAKE.read_text(encoding="utf-8")
        qml_cmake_root = QML_CMAKE.read_text(encoding="utf-8")
        qml_cmake = qml_cmake_text()
        core_cmake = CORE_CMAKE.read_text(encoding="utf-8")
        client_cache_test_cmake = CLIENT_CACHE_TEST_CMAKE.read_text(encoding="utf-8")
        client_dialog_test_cmake = CLIENT_DIALOG_TEST_CMAKE.read_text(encoding="utf-8")
        client_moments_test_cmake = CLIENT_MOMENTS_TEST_CMAKE.read_text(encoding="utf-8")
        client_network_test_cmake = CLIENT_NETWORK_TEST_CMAKE.read_text(encoding="utf-8")

        self.assertNotIn("add_subdirectory(MemoChatShared)", desktop_cmake)
        self.assertIn("add_subdirectory(MemoChat-qml)", desktop_cmake)
        self.assertIn("add_subdirectory(core)", qml_cmake_root)
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
        self.assertIn("app/controller/AppController.cpp", qml_cmake)
        self.assertNotIn("app/MainQmlBootstrap.cpp", qml_cmake)
        self.assertIn("app/bootstrap/MainQmlTypeRegistry.cpp", qml_cmake)
        self.assertIn("app/bootstrap/MainQmlEngineSetup.cpp", qml_cmake)
        self.assertIn("app/bootstrap/MainQmlWindowLoader.cpp", qml_cmake)
        self.assertIn("app/bootstrap/MainRuntimeConfig.cpp", qml_cmake)
        self.assertIn("app/connection/AppControllerConnectionState.h", qml_cmake)
        self.assertIn("app/controller/AppControllerDialogStateData.h", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupState.h", qml_cmake)
        self.assertIn("app/controller/AppControllerPendingSendState.h", qml_cmake)
        self.assertIn("app/controller/AppControllerRuntimeState.h", qml_cmake)
        self.assertIn("app/controller/AppControllerUserState.h", qml_cmake)
        self.assertIn("app/session/AppSessionCoordinator.cpp", qml_cmake)
        self.assertIn("app/session/SessionAuthCoordinator.cpp", qml_cmake)
        self.assertIn("app/session/SessionAuthCoordinatorCommands.cpp", qml_cmake)
        self.assertIn("app/session/SessionAuthCoordinatorLoginResponse.cpp", qml_cmake)
        self.assertIn("app/session/SessionAuthCoordinatorAuthResponses.cpp", qml_cmake)
        self.assertIn("app/session/SessionChatEntryCoordinator.cpp", qml_cmake)
        self.assertIn("app/session/SessionRelationBootstrap.cpp", qml_cmake)
        self.assertIn("app/coordinators/RegisterCountdownController.cpp", qml_cmake)
        self.assertIn("app/coordinators/CallCoordinator.cpp", qml_cmake)
        self.assertIn("app/coordinators/ContactCoordinatorShell.cpp", qml_cmake)
        self.assertIn("app/coordinators/GroupCoordinator.cpp", qml_cmake)
        self.assertIn("app/coordinators/MediaCoordinator.cpp", qml_cmake)
        self.assertIn("app/coordinators/ProfileCoordinator.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupEvents.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupManagement.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupResponses.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupResponseErrors.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupManagementResponses.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupHistoryResponses.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupMessageResponses.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerPrivateMessageResponses.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerDialogMetaResponses.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerDialogModels.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerPrivateHistory.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerReadAcks.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerPrivateSelection.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerContactSelection.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupSelection.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerDialogSelection.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerPagination.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerSelection.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerUiProperties.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerUserProperties.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupProperties.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerModelProperties.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerProperties.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerContactState.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerStatusState.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerLoadingState.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerDialogRuntimeState.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerProfileState.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerBootstrapState.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerState.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerConnectionLogin.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerHeartbeat.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerReconnect.cpp", qml_cmake)
        self.assertIn("app/connection/AppChatConnectionCoordinator.cpp", qml_cmake)
        self.assertIn("app/connection/AppChatConnectionPolicy.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerConnection.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerLoginSession.cpp", qml_cmake)
        self.assertIn("app/connection/AppControllerChatBootstrap.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerRelationBootstrap.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerRegisterCountdown.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerSession.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerPendingAttachments.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerMediaUploadQueue.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerMessageDispatch.cpp", qml_cmake)
        self.assertNotIn("app/AppControllerCallTarget.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerMessageStatus.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerPrivateHistoryResponses.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerPrivateMessageEvents.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerPrivateReadEvents.cpp", qml_cmake)
        self.assertIn("features/agent/controller/AgentControllerChat.cpp", qml_cmake)
        self.assertIn("features/agent/controller/AgentController.cpp", qml_cmake)
        self.assertIn("features/agent/game/AgentControllerGameNetwork.cpp", qml_cmake)
        self.assertIn("features/agent/game/AgentControllerGameResponses.cpp", qml_cmake)
        self.assertIn("features/agent/game/AgentControllerGameRooms.cpp", qml_cmake)
        self.assertIn("features/agent/game/AgentControllerGameTemplates.cpp", qml_cmake)
        self.assertIn("features/agent/controller/AgentControllerAgentTasks.cpp", qml_cmake)
        self.assertIn("features/agent/controller/AgentControllerKnowledge.cpp", qml_cmake)
        self.assertIn("features/agent/controller/AgentControllerMemory.cpp", qml_cmake)
        self.assertNotIn("features/agent/AgentControllerGame.cpp", qml_cmake)
        self.assertIn("features/agent/controller/AgentControllerModels.cpp", qml_cmake)
        self.assertIn("features/agent/controller/AgentControllerSessions.cpp", qml_cmake)
        self.assertIn("features/agent/controller/AgentControllerState.cpp", qml_cmake)
        self.assertIn("features/chat/model/ChatMessageModelContent.cpp", qml_cmake)
        self.assertIn("features/chat/model/ChatMessageModel.cpp", qml_cmake)
        self.assertIn("features/chat/cache/ChatCacheMessageCodec.cpp", qml_cmake)
        self.assertIn("features/chat/cache/ChatCacheMessageCodec.h", qml_cmake)
        self.assertIn("ChatCacheMessageCodec.cpp", client_cache_test_cmake)
        self.assertIn("chat_cache_message_codec_gtest", client_cache_test_cmake)
        self.assertIn("features/chat/model/ChatMessageModelMutations.cpp", qml_cmake)
        self.assertIn("features/chat/model/ChatMessageModelPresentation.cpp", qml_cmake)
        self.assertIn("features/chat/model/ChatMessageModelQueries.cpp", qml_cmake)
        self.assertIn("features/chat/services/DialogListEntryBuilder.cpp", qml_cmake)
        self.assertIn("features/chat/services/DialogListEntryBuilder.h", qml_cmake)
        self.assertIn("features/chat/services/DialogListTypes.h", qml_cmake)
        self.assertIn("DialogListEntryBuilder.cpp", client_dialog_test_cmake)
        self.assertIn("features/chat/cache/PrivateChatCacheStore.cpp", qml_cmake)
        self.assertIn("features/contact/model/FriendListModelMutations.cpp", qml_cmake)
        self.assertIn("features/contact/model/FriendListModelState.cpp", qml_cmake)
        self.assertIn("features/contact/model/FriendListModelQueries.cpp", qml_cmake)
        self.assertIn("FriendListModelMutations.cpp", client_dialog_test_cmake)
        self.assertIn("FriendListModelState.cpp", client_dialog_test_cmake)
        self.assertIn("FriendListModelQueries.cpp", client_dialog_test_cmake)
        self.assertIn("features/moments/parsing/MomentsEntryParser.cpp", qml_cmake)
        self.assertIn("features/moments/parsing/MomentsEntryParser.h", qml_cmake)
        self.assertIn("MomentsEntryParser.cpp", client_moments_test_cmake)
        self.assertIn("moments_entry_parser_gtest", client_moments_test_cmake)
        self.assertIn("features/moments/parsing/MomentsControllerParsing.cpp", qml_cmake)
        self.assertIn("features/moments/controller/MomentsControllerPublish.cpp", qml_cmake)
        self.assertIn("features/moments/controller/MomentsControllerRequests.cpp", qml_cmake)
        self.assertIn("features/moments/controller/MomentsControllerFeedResponses.cpp", qml_cmake)
        self.assertIn("features/moments/controller/MomentsControllerPostResponses.cpp", qml_cmake)
        self.assertIn("features/moments/controller/MomentsControllerCommentResponses.cpp", qml_cmake)
        self.assertNotIn("features/moments/MomentsControllerResponses.cpp", qml_cmake)
        self.assertIn("features/moments/model/MomentsModelMutations.cpp", qml_cmake)
        self.assertIn("features/moments/model/MomentsModelQueries.cpp", qml_cmake)
        self.assertIn("features/r18/controller/R18ControllerNetwork.cpp", qml_cmake)
        self.assertIn("features/r18/controller/R18ControllerResponses.cpp", qml_cmake)
        self.assertIn("features/r18/controller/R18ControllerSources.cpp", qml_cmake)
        self.assertIn("features/r18/controller/R18ControllerState.cpp", qml_cmake)
        self.assertIn("features/pet/controller/PetControllerSession.cpp", qml_cmake)
        self.assertIn("features/pet/controller/PetControllerState.cpp", qml_cmake)
        self.assertIn("features/pet/speech/PetControllerVoiceRuntime.cpp", qml_cmake)
        self.assertIn("features/pet/speech/PetControllerVoiceTraining.cpp", qml_cmake)
        self.assertIn("shared/gateway/ClientGateway.cpp", qml_cmake)
        self.assertIn("shared/gateway/TransportEndpointPolicy.cpp", qml_cmake)
        self.assertIn("shared/gateway/TransportEndpointPolicy.h", qml_cmake)
        self.assertIn("shared/media/LocalFilePickerServiceAttachments.cpp", qml_cmake)
        self.assertIn("shared/media/LocalFilePickerServiceAvatar.cpp", qml_cmake)
        self.assertIn("shared/media/MediaUploadServiceHelpers.cpp", qml_cmake)
        self.assertIn("shared/media/MediaUploadServiceNetwork.cpp", qml_cmake)
        self.assertIn("shared/media/MediaUploadServiceUploads.cpp", qml_cmake)
        self.assertIn("live2d/rendering/Live2DRenderItem.cpp", qml_cmake)
        self.assertIn("${MEMOCHAT_QML_FEATURE_DIR}/agent", qml_cmake)
        self.assertIn("${MEMOCHAT_QML_FEATURE_DIR}/chat", qml_cmake)
        self.assertIn("${MEMOCHAT_QML_SHARED_DIR}/gateway", qml_cmake)
        self.assertIn("${MEMOCHAT_QML_SHARED_DIR}/media", qml_cmake)
        self.assertIn("resources/qrc/app-core.qrc", qml_cmake)
        self.assertIn("${MEMOCHAT_QML_CORE_DIR}/config.ini", qml_cmake)
        self.assertNotIn("${CMAKE_CURRENT_SOURCE_DIR}/controllers", qml_cmake)
        self.assertNotIn("${CMAKE_CURRENT_SOURCE_DIR}/models", qml_cmake)
        self.assertNotIn("${CMAKE_CURRENT_SOURCE_DIR}/services", qml_cmake)
        self.assertNotIn("${CMAKE_CURRENT_SOURCE_DIR}/storage", qml_cmake)
        self.assertNotIn("${CMAKE_CURRENT_SOURCE_DIR}/utils", qml_cmake)
        self.assertNotIn("SHARED_CLIENT_DIR", qml_cmake)
        self.assertNotIn("../MemoChatShared", qml_cmake)

    def test_qml_cmake_source_lists_are_split_into_grouped_fragments(self):
        root = QML_CMAKE.read_text(encoding="utf-8")
        for rel_path in DIRECTORY_MANIFESTS:
            with self.subTest(manifest=rel_path):
                self.assertIn(f"include({rel_path})", root)

        app = (QML_DIR / "app/sources.cmake").read_text(encoding="utf-8")
        features = (QML_DIR / "features/sources.cmake").read_text(encoding="utf-8")
        agent = (QML_DIR / "features/agent/sources.cmake").read_text(encoding="utf-8")
        pet = (QML_DIR / "features/pet/sources.cmake").read_text(encoding="utf-8")
        shared = (QML_DIR / "shared/sources.cmake").read_text(encoding="utf-8")
        live2d = (QML_DIR / "live2d/sources.cmake").read_text(encoding="utf-8")
        resources = (QML_DIR / "resources/resources.cmake").read_text(encoding="utf-8")
        combined = "\n".join((root, app, features, agent, pet, shared, live2d, resources))

        self.assertNotIn("file(GLOB", combined)
        self.assertIn("set(MEMOCHAT_QML_APP_SOURCES", app)
        self.assertIn("set(MEMOCHAT_QML_APP_HEADERS", app)
        self.assertIn("app/controller/AppController.cpp", app)
        self.assertIn("app/connection/AppChatConnectionCoordinator.cpp", app)
        self.assertIn("set(MEMOCHAT_QML_FEATURE_SOURCES", features)
        self.assertIn("set(MEMOCHAT_QML_FEATURE_HEADERS", features)
        self.assertIn("features/agent/controller/AgentController.cpp", agent)
        self.assertIn("features/pet/controller/PetController.cpp", pet)
        self.assertIn("set(MEMOCHAT_QML_SHARED_SOURCES", shared)
        self.assertIn("set(MEMOCHAT_QML_SHARED_HEADERS", shared)
        self.assertIn("shared/media/MediaUploadService.cpp", shared)
        self.assertIn("set(MEMOCHAT_QML_LIVE2D_SOURCES", live2d)
        self.assertIn("set(MEMOCHAT_QML_LIVE2D_HEADERS", live2d)
        self.assertIn("live2d/rendering/Live2DRenderItem.cpp", live2d)
        self.assertIn("platform/windows/ClientWinCompat.h", shared)
        self.assertIn("set(MEMOCHAT_QML_RESOURCES", resources)
        self.assertIn("resources/qrc/qml-shell.qrc", resources)
        self.assertIn("resources/qrc/app-core.qrc", resources)
        self.assertIn("${MEMOCHAT_QML_APP_SOURCES}", root)
        self.assertIn("${MEMOCHAT_QML_FEATURE_SOURCES}", root)
        self.assertIn("${MEMOCHAT_QML_SHARED_SOURCES}", root)
        self.assertIn("${MEMOCHAT_QML_LIVE2D_SOURCES}", root)
        self.assertIn("${MEMOCHAT_QML_RESOURCES}", root)

    def test_startup_qml_bootstrap_concerns_are_split(self):
        bootstrap = (QML_DIR / "app/bootstrap/MainQmlBootstrap.h").read_text(encoding="utf-8")
        registry = (QML_DIR / "app/bootstrap/MainQmlTypeRegistry.cpp").read_text(encoding="utf-8")
        engine = (QML_DIR / "app/bootstrap/MainQmlEngineSetup.cpp").read_text(encoding="utf-8")
        loader = (QML_DIR / "app/bootstrap/MainQmlWindowLoader.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/MainQmlBootstrap.cpp").exists())
        self.assertIn("void registerMemoChatQmlTypes();", bootstrap)
        self.assertIn("QUrl memoChatMainQmlUrl();", bootstrap)
        self.assertIn("void configureMemoChatEngine", bootstrap)
        self.assertIn("bool loadMemoChatMainWindow", bootstrap)
        self.assertIn("void registerMemoChatQmlTypes()", registry)
        self.assertIn("qmlRegisterUncreatableType<AppController>", registry)
        self.assertIn("void configureMemoChatEngine", engine)
        self.assertIn('setContextProperty("controller"', engine)
        self.assertIn("QUrl memoChatMainQmlUrl()", loader)
        self.assertIn("bool loadMemoChatMainWindow", loader)
        self.assertIn("ensureInitialCenteringHook(window)", loader)
        self.assertIn("scheduleTopLevelQuickWindowHookRetries(&app)", loader)
        self.assertNotIn("qmlRegisterUncreatableType", bootstrap)
        self.assertNotIn("setContextProperty", bootstrap)
        self.assertNotIn("engine.load", bootstrap)
        self.assertLessEqual(len(bootstrap.splitlines()), 16)

    def test_qml_shell_runtime_helpers_are_registered(self):
        qrc = (
            (RESOURCE_QRC_DIR / "qml-r18.qrc").read_text(encoding="utf-8")
            + "\n"
            + (RESOURCE_QRC_DIR / "qml-agent.qrc").read_text(encoding="utf-8")
        )
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
        controller = (QML_DIR / "features/moments/controller/MomentsController.cpp").read_text(encoding="utf-8")
        parsing = (QML_DIR / "features/moments/parsing/MomentsControllerParsing.cpp").read_text(encoding="utf-8")
        publish = (QML_DIR / "features/moments/controller/MomentsControllerPublish.cpp").read_text(encoding="utf-8")
        requests = (QML_DIR / "features/moments/controller/MomentsControllerRequests.cpp").read_text(encoding="utf-8")
        feed_responses = MOMENTS_RESPONSE_FAMILIES["feed"].read_text(encoding="utf-8")
        parser = (QML_DIR / "features/moments/parsing/MomentsEntryParser.cpp").read_text(encoding="utf-8")

        self.assertIn("MomentEntry MomentsController::parseMomentEntry", parsing)
        self.assertIn("parseMomentEntryFromJson", parsing)
        self.assertIn("MomentEntry parseMomentEntryFromJson", parser)
        self.assertIn("QVector<MomentComment> parseMomentComments", parser)
        self.assertIn("void MomentsController::publishDraftMoment", publish)
        self.assertIn("void MomentsController::toggleLike", requests)
        self.assertIn("void MomentsController::onLoadFeedRsp", feed_responses)
        self.assertNotIn("void MomentsController::publishDraftMoment", controller)
        self.assertNotIn("void MomentsController::onLoadFeedRsp", controller)
        self.assertLess(len(controller.splitlines()), 180)

    def test_moments_response_handlers_are_split_by_family(self):
        feature_cmake = qml_cmake_text()
        legacy = (QML_DIR / "features/moments/controller/MomentsControllerResponses.cpp").read_text(encoding="utf-8")

        for path in MOMENTS_RESPONSE_FAMILIES.values():
            with self.subTest(path=path.name):
                self.assertTrue(path.is_file())
                rel_path = path.relative_to(QML_DIR).as_posix()
                self.assertIn(rel_path, feature_cmake)

        feed = MOMENTS_RESPONSE_FAMILIES["feed"].read_text(encoding="utf-8")
        post = MOMENTS_RESPONSE_FAMILIES["post"].read_text(encoding="utf-8")
        comment = MOMENTS_RESPONSE_FAMILIES["comment"].read_text(encoding="utf-8")

        for token in (
            "void MomentsController::onLoadFeedRsp",
            "void MomentsController::onDetailRsp",
            "void MomentsController::onCommentListRsp",
        ):
            with self.subTest(feed=token):
                self.assertIn(token, feed)
                self.assertNotIn(token, legacy)

        for token in (
            "void MomentsController::onPublishRsp",
            "void MomentsController::onDeleteRsp",
            "void MomentsController::onLikeRsp",
        ):
            with self.subTest(post=token):
                self.assertIn(token, post)
                self.assertNotIn(token, legacy)

        for token in (
            "void MomentsController::onCommentRsp",
            "void MomentsController::onCommentLikeRsp",
        ):
            with self.subTest(comment=token):
                self.assertIn(token, comment)
                self.assertNotIn(token, legacy)

    def test_heavy_agent_controller_concerns_are_split(self):
        controller = (QML_DIR / "features/agent/controller/AgentController.cpp").read_text(encoding="utf-8")
        chat = (QML_DIR / "features/agent/controller/AgentControllerChat.cpp").read_text(encoding="utf-8")
        models = (QML_DIR / "features/agent/controller/AgentControllerModels.cpp").read_text(encoding="utf-8")
        sessions = (QML_DIR / "features/agent/controller/AgentControllerSessions.cpp").read_text(encoding="utf-8")
        state = (QML_DIR / "features/agent/controller/AgentControllerState.cpp").read_text(encoding="utf-8")

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
        knowledge = (QML_DIR / "features/agent/controller/AgentControllerKnowledge.cpp").read_text(encoding="utf-8")
        memory = (QML_DIR / "features/agent/controller/AgentControllerMemory.cpp").read_text(encoding="utf-8")
        tasks = (QML_DIR / "features/agent/controller/AgentControllerAgentTasks.cpp").read_text(encoding="utf-8")

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
        network = (QML_DIR / "features/agent/game/AgentControllerGameNetwork.cpp").read_text(encoding="utf-8")
        responses = (QML_DIR / "features/agent/game/AgentControllerGameResponses.cpp").read_text(encoding="utf-8")
        rooms = (QML_DIR / "features/agent/game/AgentControllerGameRooms.cpp").read_text(encoding="utf-8")
        templates = (QML_DIR / "features/agent/game/AgentControllerGameTemplates.cpp").read_text(encoding="utf-8")

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
        model = (QML_DIR / "features/chat/model/ChatMessageModel.cpp").read_text(encoding="utf-8")
        content = (QML_DIR / "features/chat/model/ChatMessageModelContent.cpp").read_text(encoding="utf-8")
        mutations = (QML_DIR / "features/chat/model/ChatMessageModelMutations.cpp").read_text(encoding="utf-8")
        presentation = (QML_DIR / "features/chat/model/ChatMessageModelPresentation.cpp").read_text(encoding="utf-8")
        queries = (QML_DIR / "features/chat/model/ChatMessageModelQueries.cpp").read_text(encoding="utf-8")

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
        events = (QML_DIR / "app/controller/AppControllerGroupEvents.cpp").read_text(encoding="utf-8")
        responses = (QML_DIR / "app/controller/AppControllerGroupResponses.cpp").read_text(encoding="utf-8")
        errors = (QML_DIR / "app/controller/AppControllerGroupResponseErrors.cpp").read_text(encoding="utf-8")
        management = (QML_DIR / "app/controller/AppControllerGroupManagementResponses.cpp").read_text(encoding="utf-8")
        history = (QML_DIR / "app/controller/AppControllerGroupHistoryResponses.cpp").read_text(encoding="utf-8")
        group_messages = (QML_DIR / "app/controller/AppControllerGroupMessageResponses.cpp").read_text(encoding="utf-8")
        private_messages = (QML_DIR / "app/controller/AppControllerPrivateMessageResponses.cpp").read_text(
            encoding="utf-8"
        )
        dialog_meta = (QML_DIR / "app/controller/AppControllerDialogMetaResponses.cpp").read_text(encoding="utf-8")

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
        commands = (QML_DIR / "app/controller/AppControllerGroupCommands.cpp").read_text(encoding="utf-8")
        management = (QML_DIR / "app/controller/AppControllerGroupManagement.cpp").read_text(encoding="utf-8")

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
        session = (QML_DIR / "app/session/AppSessionCoordinator.cpp").read_text(encoding="utf-8")
        session_auth = (QML_DIR / "app/session/SessionAuthCoordinator.cpp").read_text(encoding="utf-8")
        session_auth_commands = (QML_DIR / "app/session/SessionAuthCoordinatorCommands.cpp").read_text(encoding="utf-8")
        session_auth_login = (QML_DIR / "app/session/SessionAuthCoordinatorLoginResponse.cpp").read_text(
            encoding="utf-8"
        )
        session_auth_responses = (QML_DIR / "app/session/SessionAuthCoordinatorAuthResponses.cpp").read_text(
            encoding="utf-8"
        )
        session_chat = (QML_DIR / "app/session/SessionChatEntryCoordinator.cpp").read_text(encoding="utf-8")
        session_relation = (QML_DIR / "app/session/SessionRelationBootstrap.cpp").read_text(encoding="utf-8")
        session_countdown = (QML_DIR / "app/coordinators/RegisterCountdownController.cpp").read_text(encoding="utf-8")
        contact = (QML_DIR / "app/coordinators/ContactCoordinatorShell.cpp").read_text(encoding="utf-8")
        coordinators = (QML_DIR / "app/coordinators/AppCoordinators.h").read_text(encoding="utf-8")
        call = (QML_DIR / "app/coordinators/CallCoordinator.cpp").read_text(encoding="utf-8")
        app_controller = (QML_DIR / "app/controller/AppController.cpp").read_text(encoding="utf-8")
        app_header = (QML_DIR / "app/controller/AppController.h").read_text(encoding="utf-8")
        profile = (QML_DIR / "app/coordinators/ProfileCoordinator.cpp").read_text(encoding="utf-8")
        group = (QML_DIR / "app/coordinators/GroupCoordinator.cpp").read_text(encoding="utf-8")
        media = (QML_DIR / "app/coordinators/MediaCoordinator.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/AppCoordinators.cpp").exists())
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
        self.assertIn("void CallCoordinator::finalizeEndedCall", call)
        self.assertIn("bool CallCoordinator::ensureCallTargetFromCurrentChat", call)
        self.assertIn("void CallCoordinator::startCallFlow", call)
        self.assertIn("void CallCoordinator::onCallHttpFinished", call)
        self.assertIn("void CallCoordinator::onCallEvent", call)
        self.assertIn("void CallCoordinator::onLivekitRoomJoined", call)
        self.assertIn("void CallCoordinator::onLivekitRoomDisconnected", call)
        self.assertIn("_app._livekit_bridge.leaveRoom();", call)
        self.assertIn("_app._call_session_model.markEnded(statusText);", call)
        self.assertIn("_app.setAuthStatus(statusText, false);", call)
        self.assertIn("QTimer::singleShot(1800", call)
        self.assertIn("_app._call_controller.startCall", call)
        self.assertIn("void finalizeEndedCall(const QString& statusText);", coordinators)
        self.assertNotIn("_app.startCallFlow", call)
        self.assertIn("_call_coordinator->onCallHttpFinished", app_controller)
        self.assertIn("_call_coordinator->onCallEvent", app_controller)
        self.assertIn("_call_coordinator->onLivekitRoomDisconnected", app_controller)
        self.assertNotIn("&AppController::onCallHttpFinished", app_controller)
        self.assertNotIn("&AppController::onCallEvent", app_controller)
        self.assertNotIn("&AppController::onLivekitRoomDisconnected", app_controller)
        self.assertNotIn("void onCallHttpFinished(ReqId id, QString res, ErrorCodes err);", app_header)
        self.assertNotIn("void onCallEvent(QJsonObject payload);", app_header)
        self.assertNotIn("void onLivekitRoomDisconnected(const QString& reason, bool recoverable);", app_header)
        self.assertIn("ProfileCoordinator::ProfileCoordinator", profile)
        self.assertIn("void ProfileCoordinator::chooseAvatar", profile)
        self.assertIn("GroupCoordinator::GroupCoordinator", group)
        self.assertIn("void GroupCoordinator::createGroup", group)
        self.assertIn("MediaCoordinator::MediaCoordinator", media)
        self.assertIn("void MediaCoordinator::sendCurrentComposerPayload", media)
        self.assertLess(len(session.splitlines()), 90)

    def test_heavy_app_controller_model_concerns_are_split(self):
        models = (QML_DIR / "app/controller/AppControllerModels.cpp").read_text(encoding="utf-8")
        dialogs = (QML_DIR / "app/controller/AppControllerDialogModels.cpp").read_text(encoding="utf-8")
        history = (QML_DIR / "app/controller/AppControllerPrivateHistory.cpp").read_text(encoding="utf-8")
        read_acks = (QML_DIR / "app/controller/AppControllerReadAcks.cpp").read_text(encoding="utf-8")
        private_selection = (QML_DIR / "app/controller/AppControllerPrivateSelection.cpp").read_text(encoding="utf-8")

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
        private_selection = (QML_DIR / "app/controller/AppControllerPrivateSelection.cpp").read_text(encoding="utf-8")
        contact_selection = (QML_DIR / "app/controller/AppControllerContactSelection.cpp").read_text(encoding="utf-8")
        group_selection = (QML_DIR / "app/controller/AppControllerGroupSelection.cpp").read_text(encoding="utf-8")
        dialog_selection = (QML_DIR / "app/controller/AppControllerDialogSelection.cpp").read_text(encoding="utf-8")
        pagination = (QML_DIR / "app/controller/AppControllerPagination.cpp").read_text(encoding="utf-8")

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
        ui_props = (QML_DIR / "app/controller/AppControllerUiProperties.cpp").read_text(encoding="utf-8")
        user_props = (QML_DIR / "app/controller/AppControllerUserProperties.cpp").read_text(encoding="utf-8")
        group_props = (QML_DIR / "app/controller/AppControllerGroupProperties.cpp").read_text(encoding="utf-8")
        model_props = (QML_DIR / "app/controller/AppControllerModelProperties.cpp").read_text(encoding="utf-8")

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
        header = (QML_DIR / "app/controller/AppController.h").read_text(encoding="utf-8")
        connection_state_header = (QML_DIR / "app/connection/AppControllerConnectionState.h").read_text(
            encoding="utf-8"
        )
        runtime_state_header = (QML_DIR / "app/controller/AppControllerRuntimeState.h").read_text(encoding="utf-8")
        user_state_header = (QML_DIR / "app/controller/AppControllerUserState.h").read_text(encoding="utf-8")
        contact_state = (QML_DIR / "app/controller/AppControllerContactState.cpp").read_text(encoding="utf-8")
        status_state = (QML_DIR / "app/controller/AppControllerStatusState.cpp").read_text(encoding="utf-8")
        loading_state = (QML_DIR / "app/controller/AppControllerLoadingState.cpp").read_text(encoding="utf-8")
        dialog_state = (QML_DIR / "app/controller/AppControllerDialogRuntimeState.cpp").read_text(encoding="utf-8")
        profile_state = (QML_DIR / "app/controller/AppControllerProfileState.cpp").read_text(encoding="utf-8")
        bootstrap_state = (QML_DIR / "app/controller/AppControllerBootstrapState.cpp").read_text(encoding="utf-8")

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
        app_controller = (QML_DIR / "app/controller/AppController.cpp").read_text(encoding="utf-8")
        app_header = (QML_DIR / "app/controller/AppController.h").read_text(encoding="utf-8")
        coordinator = (QML_DIR / "app/connection/AppChatConnectionCoordinator.cpp").read_text(encoding="utf-8")
        policy = (QML_DIR / "app/connection/AppChatConnectionPolicy.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/AppControllerConnection.cpp").exists())
        self.assertFalse((QML_DIR / "app/AppControllerConnectionLogin.cpp").exists())
        self.assertFalse((QML_DIR / "app/AppControllerHeartbeat.cpp").exists())
        self.assertFalse((QML_DIR / "app/AppControllerReconnect.cpp").exists())
        self.assertIn("_chat_connection_coordinator->onTcpConnectFinished(success);", app_controller)
        self.assertIn("_chat_connection_coordinator->onChatLoginFailed(err);", app_controller)
        self.assertIn("_chat_connection_coordinator->onHeartbeatTimeout();", app_controller)
        self.assertIn("_chat_connection_coordinator->onHeartbeatAck(ackAtMs);", app_controller)
        self.assertIn("_chat_connection_coordinator->onNotifyOffline();", app_controller)
        self.assertIn("_chat_connection_coordinator->onConnectionClosed();", app_controller)
        self.assertNotIn("&AppController::onTcpConnectFinished", app_controller)
        self.assertNotIn("&AppController::onChatLoginFailed", app_controller)
        self.assertNotIn("&AppController::onHeartbeatTimeout", app_controller)
        self.assertNotIn("&AppController::onHeartbeatAck", app_controller)
        self.assertNotIn("&AppController::onNotifyOffline", app_controller)
        self.assertNotIn("&AppController::onConnectionClosed", app_controller)
        self.assertNotIn("void onTcpConnectFinished(bool success);", app_header)
        self.assertNotIn("void onChatLoginFailed(int err);", app_header)
        self.assertNotIn("void onHeartbeatTimeout();", app_header)
        self.assertNotIn("void onHeartbeatAck(qint64 ackAtMs);", app_header)
        self.assertNotIn("void onNotifyOffline();", app_header)
        self.assertNotIn("void onConnectionClosed();", app_header)
        self.assertNotIn("bool tryLoginFallbackToTcp(const QString& reason);", app_header)
        self.assertNotIn("bool tryReconnectChat();", app_header)
        self.assertNotIn("void resetHeartbeatTracking();", app_header)
        self.assertIn("void AppChatConnectionCoordinator::onTcpConnectFinished", coordinator)
        self.assertIn("void AppChatConnectionCoordinator::onHeartbeatTimeout", coordinator)
        self.assertIn("void AppChatConnectionCoordinator::onHeartbeatAck", coordinator)
        self.assertIn("void AppChatConnectionCoordinator::onNotifyOffline", coordinator)
        self.assertIn("void AppChatConnectionCoordinator::onConnectionClosed", coordinator)
        self.assertIn("bool AppChatConnectionCoordinator::tryReconnectChat", coordinator)
        self.assertIn("AppChatConnectionPolicy::evaluateLoginTcpFallback", coordinator)
        self.assertIn("AppChatConnectionPolicy::evaluateReconnect", coordinator)
        self.assertIn("evaluateLoginTcpFallback", policy)
        self.assertIn("evaluateReconnect", policy)
        self.assertLess(len(coordinator.splitlines()), 380)

    def test_heavy_app_controller_session_concerns_are_split(self):
        coordinator = (QML_DIR / "app/session/AppSessionCoordinator.cpp").read_text(encoding="utf-8")
        auth_coordinator = (QML_DIR / "app/session/SessionAuthCoordinator.cpp").read_text(encoding="utf-8")
        auth_commands = (QML_DIR / "app/session/SessionAuthCoordinatorCommands.cpp").read_text(encoding="utf-8")
        auth_login = (QML_DIR / "app/session/SessionAuthCoordinatorLoginResponse.cpp").read_text(encoding="utf-8")
        auth_responses = (QML_DIR / "app/session/SessionAuthCoordinatorAuthResponses.cpp").read_text(encoding="utf-8")
        chat_entry = (QML_DIR / "app/session/SessionChatEntryCoordinator.cpp").read_text(encoding="utf-8")
        relation = (QML_DIR / "app/session/SessionRelationBootstrap.cpp").read_text(encoding="utf-8")
        countdown = (QML_DIR / "app/coordinators/RegisterCountdownController.cpp").read_text(encoding="utf-8")
        app_auth_responses = (QML_DIR / "app/controller/AppControllerAuthResponses.cpp").read_text(encoding="utf-8")
        chat_bootstrap = (QML_DIR / "app/connection/AppControllerChatBootstrap.cpp").read_text(encoding="utf-8")
        relation_bootstrap = (QML_DIR / "app/controller/AppControllerRelationBootstrap.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/AppControllerSession.cpp").exists())
        self.assertFalse((QML_DIR / "app/AppControllerLoginSession.cpp").exists())
        self.assertFalse((QML_DIR / "app/AppControllerRegisterCountdown.cpp").exists())
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
        self.assertIn("void AppController::onLoginHttpFinished", app_auth_responses)
        self.assertIn("void AppController::onSwitchToChat", chat_bootstrap)
        self.assertIn("void AppController::runPostLoginBootstrap", chat_bootstrap)
        self.assertIn("void AppController::requestRelationBootstrap", relation_bootstrap)
        self.assertIn("void AppController::onRelationBootstrapUpdated", relation_bootstrap)
        self.assertIn("void AppController::onRegisterCountdownTimeout", app_auth_responses)
        self.assertIn("_session_coordinator->onLoginHttpFinished", app_auth_responses)
        self.assertIn("_session_coordinator->onSwitchToChat", chat_bootstrap)
        self.assertIn("_session_coordinator->runPostLoginBootstrap", chat_bootstrap)
        self.assertIn("_session_coordinator->requestRelationBootstrap", relation_bootstrap)
        self.assertIn("_session_coordinator->onRegisterCountdownTimeout", app_auth_responses)
        self.assertIn("_auth->onLoginHttpFinished", coordinator)
        self.assertIn("_chat_entry->onSwitchToChat", coordinator)
        self.assertIn("_relation_bootstrap->requestRelationBootstrap", coordinator)
        self.assertIn("_register_countdown->onRegisterCountdownTimeout", coordinator)
        self.assertNotIn("parseTransportKind", coordinator)
        self.assertNotIn("parseTransportKind", app_auth_responses)
        self.assertNotIn("parseTransportKind", auth_coordinator)
        self.assertNotIn("gateAuthBusinessErrorTip", auth_coordinator)
        self.assertNotIn("QTimer::singleShot", chat_bootstrap)
        self.assertNotIn("GetFriendListSnapshot", relation_bootstrap)
        self.assertNotIn("onRelationBootstrapUpdated", app_auth_responses)
        self.assertNotIn("onLoginHttpFinished", chat_bootstrap)
        self.assertLess(len(chat_bootstrap.splitlines()), 20)
        self.assertLess(len(app_auth_responses.splitlines()), 90)

    def test_session_auth_workflow_is_owned_by_session_coordinator(self):
        coordinator = (QML_DIR / "app/session/AppSessionCoordinator.cpp").read_text(encoding="utf-8")
        auth_coordinator = (QML_DIR / "app/session/SessionAuthCoordinator.cpp").read_text(encoding="utf-8")
        auth_commands = (QML_DIR / "app/session/SessionAuthCoordinatorCommands.cpp").read_text(encoding="utf-8")
        auth_login = (QML_DIR / "app/session/SessionAuthCoordinatorLoginResponse.cpp").read_text(encoding="utf-8")
        auth_responses = (QML_DIR / "app/session/SessionAuthCoordinatorAuthResponses.cpp").read_text(encoding="utf-8")
        chat_entry = (QML_DIR / "app/session/SessionChatEntryCoordinator.cpp").read_text(encoding="utf-8")
        relation = (QML_DIR / "app/session/SessionRelationBootstrap.cpp").read_text(encoding="utf-8")
        countdown = (QML_DIR / "app/coordinators/RegisterCountdownController.cpp").read_text(encoding="utf-8")
        app_auth_responses = (QML_DIR / "app/controller/AppControllerAuthResponses.cpp").read_text(encoding="utf-8")
        chat_bootstrap = (QML_DIR / "app/connection/AppControllerChatBootstrap.cpp").read_text(encoding="utf-8")
        relation_bootstrap = (QML_DIR / "app/controller/AppControllerRelationBootstrap.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/AppControllerLoginSession.cpp").exists())
        self.assertFalse((QML_DIR / "app/AppControllerRegisterCountdown.cpp").exists())
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

        self.assertIn("_session_coordinator->onLoginHttpFinished", app_auth_responses)
        self.assertIn("_session_coordinator->onRegisterHttpFinished", app_auth_responses)
        self.assertIn("_session_coordinator->onResetHttpFinished", app_auth_responses)
        self.assertIn("_session_coordinator->onSwitchToChat", chat_bootstrap)
        self.assertIn("_session_coordinator->requestRelationBootstrap", relation_bootstrap)
        self.assertIn("_session_coordinator->onRelationBootstrapUpdated", relation_bootstrap)
        self.assertIn("_session_coordinator->onRegisterCountdownTimeout", app_auth_responses)

        self.assertNotIn("_auth_controller.sendLogin", app_auth_responses)
        self.assertNotIn("_register_countdown_timer.start(1000)", app_auth_responses)
        self.assertNotIn("_auth_controller.sendLogin", coordinator)
        self.assertNotIn("gateAuthBusinessErrorTip", coordinator)
        self.assertNotIn("QTimer::singleShot", chat_bootstrap)
        self.assertNotIn("GetFriendListSnapshot", relation_bootstrap)

    def test_heavy_friend_list_model_concerns_are_split(self):
        model = (QML_DIR / "features/contact/model/FriendListModel.cpp").read_text(encoding="utf-8")
        mutations = (QML_DIR / "features/contact/model/FriendListModelMutations.cpp").read_text(encoding="utf-8")
        state = (QML_DIR / "features/contact/model/FriendListModelState.cpp").read_text(encoding="utf-8")
        queries = (QML_DIR / "features/contact/model/FriendListModelQueries.cpp").read_text(encoding="utf-8")

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
        media = (QML_DIR / "app/controller/AppControllerMedia.cpp").read_text(encoding="utf-8")
        attachments = (QML_DIR / "app/controller/AppControllerPendingAttachments.cpp").read_text(encoding="utf-8")
        upload_queue = (QML_DIR / "app/controller/AppControllerMediaUploadQueue.cpp").read_text(encoding="utf-8")
        dispatch = (QML_DIR / "app/controller/AppControllerMessageDispatch.cpp").read_text(encoding="utf-8")
        call = (QML_DIR / "app/coordinators/CallCoordinator.cpp").read_text(encoding="utf-8")
        call_payload = (QML_DIR / "app/coordinators/CallCoordinatorPayloadPolicy.cpp").read_text(encoding="utf-8")
        call_payload_header = (QML_DIR / "app/coordinators/CallCoordinatorPayloadPolicy.h").read_text(encoding="utf-8")
        header = (QML_DIR / "app/controller/AppController.h").read_text(encoding="utf-8")
        qml_cmake = qml_cmake_text()

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
        self.assertFalse((QML_DIR / "app/AppControllerCallTarget.cpp").exists())
        self.assertIn("bool CallCoordinator::ensureCallTargetFromCurrentChat", call)
        self.assertIn("void CallCoordinator::startCallFlow", call)
        self.assertIn("void CallCoordinator::finalizeEndedCall", call)
        self.assertIn("void CallCoordinator::onCallHttpFinished", call)
        self.assertIn("void CallCoordinator::onLivekitMediaError", call)
        self.assertIn("_app._call_controller.startCall", call)
        self.assertIn("_app._auth_controller.parseJson(res, obj)", call)
        self.assertIn("app/coordinators/CallCoordinatorPayloadPolicy.cpp", qml_cmake)
        self.assertIn("QString mapCallError", call_payload_header)
        self.assertIn("QString callStateText", call_payload_header)
        self.assertIn("QJsonObject buildLivekitMetadata", call_payload_header)
        self.assertIn("QJsonObject buildLivekitLaunchPayload", call_payload_header)
        self.assertIn("CallEventPeer resolveCallEventPeer", call_payload_header)
        self.assertIn("QString mapCallError(int errorCode)", call_payload)
        self.assertIn("QString callStateText(const QString& eventType)", call_payload)
        self.assertIn("QJsonObject buildLivekitMetadata", call_payload)
        self.assertIn("QJsonObject buildLivekitLaunchPayload", call_payload)
        self.assertIn("CallEventPeer resolveCallEventPeer", call_payload)
        self.assertNotIn("QString mapCallError", call)
        self.assertNotIn("QString callStateText", call)
        self.assertNotIn("QJsonObject metadata;", call)
        self.assertNotIn('metadata["callId"]', call)
        self.assertFalse((QML_DIR / "app/AppControllerCall.cpp").exists())
        self.assertNotIn("app/AppControllerCall.cpp", qml_cmake)
        self.assertNotIn("_app.startCallFlow", call)
        self.assertNotIn("void sendCallInvite(const QString& callType);", header)
        self.assertNotIn("bool ensureCallTargetFromCurrentChat();", header)
        self.assertNotIn("void startCallFlow(const QString& callType);", header)
        self.assertNotIn("void finalizeEndedCall(const QString& statusText);", header)
        self.assertNotIn("void onLivekitMediaError(const QString& message);", header)
        self.assertNotIn("void AppController::processPendingAttachmentQueue", media)
        self.assertNotIn("bool AppController::dispatchGroupChatContent", media)
        self.assertNotIn("bool AppController::ensureCallTargetFromCurrentChat", media)
        self.assertLess(len(media.splitlines()), 90)

    def test_heavy_private_event_concerns_are_split(self):
        events = (QML_DIR / "app/controller/AppControllerPrivateEvents.cpp").read_text(encoding="utf-8")
        status = (QML_DIR / "app/controller/AppControllerMessageStatus.cpp").read_text(encoding="utf-8")
        history = (QML_DIR / "app/controller/AppControllerPrivateHistoryResponses.cpp").read_text(encoding="utf-8")
        message_events = (QML_DIR / "app/controller/AppControllerPrivateMessageEvents.cpp").read_text(encoding="utf-8")
        read_events = (QML_DIR / "app/controller/AppControllerPrivateReadEvents.cpp").read_text(encoding="utf-8")

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
        controller = (QML_DIR / "features/r18/controller/R18Controller.cpp").read_text(encoding="utf-8")
        network = (QML_DIR / "features/r18/controller/R18ControllerNetwork.cpp").read_text(encoding="utf-8")
        responses = (QML_DIR / "features/r18/controller/R18ControllerResponses.cpp").read_text(encoding="utf-8")
        sources = (QML_DIR / "features/r18/controller/R18ControllerSources.cpp").read_text(encoding="utf-8")
        state = (QML_DIR / "features/r18/controller/R18ControllerState.cpp").read_text(encoding="utf-8")

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
        controller = (QML_DIR / "features/pet/controller/PetController.cpp").read_text(encoding="utf-8")
        session = (QML_DIR / "features/pet/controller/PetControllerSession.cpp").read_text(encoding="utf-8")
        state = (QML_DIR / "features/pet/controller/PetControllerState.cpp").read_text(encoding="utf-8")
        runtime = (QML_DIR / "features/pet/speech/PetControllerVoiceRuntime.cpp").read_text(encoding="utf-8")
        training = (QML_DIR / "features/pet/speech/PetControllerVoiceTraining.cpp").read_text(encoding="utf-8")

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
        model = (QML_DIR / "features/moments/model/MomentsModel.cpp").read_text(encoding="utf-8")
        mutations = (QML_DIR / "features/moments/model/MomentsModelMutations.cpp").read_text(encoding="utf-8")
        queries = (QML_DIR / "features/moments/model/MomentsModelQueries.cpp").read_text(encoding="utf-8")

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
        qrc = APP_CORE_QRC.read_text(encoding="utf-8")

        self.assertIn('<file alias="app/icon.ico">../app/icon.ico</file>', qrc)
        self.assertIn('<file alias="style/stylesheet.qss">../app/style/stylesheet.qss</file>', qrc)
        self.assertIn('<file alias="res/head_1.png">../icons/head_1.png</file>', qrc)
        self.assertIn('<file alias="res/head_1.jpg">../icons/head_1.png</file>', qrc)
        self.assertNotIn("../MemoChat-qml/src", qrc)

    def test_qml_controllers_include_core_headers_through_target_include_path(self):
        source = MOMENTS_CONTROLLER.read_text(encoding="utf-8")

        self.assertIn('#include "httpmgr.h"', source)
        self.assertIn('#include "usermgr.h"', source)
        self.assertNotIn("../MemoChatShared", source)


if __name__ == "__main__":
    unittest.main()
