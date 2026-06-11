import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
DESKTOP_CMAKE = REPO_ROOT / "apps/client/desktop/CMakeLists.txt"
QML_CMAKE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/CMakeLists.txt"
CPP_TEST_QML_ROOT = REPO_ROOT / "tests/cpp/apps/client/desktop/MemoChat-qml"
CLIENT_DIALOG_TEST_CMAKE = CPP_TEST_QML_ROOT / "features/chat/services/CMakeLists.txt"
CLIENT_CACHE_TEST_CMAKE = CPP_TEST_QML_ROOT / "features/chat/cache/CMakeLists.txt"
CLIENT_MOMENTS_TEST_CMAKE = CPP_TEST_QML_ROOT / "features/moments/parsing/CMakeLists.txt"
CLIENT_NETWORK_TEST_CMAKE = CPP_TEST_QML_ROOT / "core/network/CMakeLists.txt"
CLIENT_GATEWAY_TEST_CMAKE = CPP_TEST_QML_ROOT / "shared/gateway/CMakeLists.txt"
CLIENT_CONNECTION_TEST_CMAKE = CPP_TEST_QML_ROOT / "app/connection/CMakeLists.txt"
QML_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
QML_CMAKE_DIR = QML_DIR / "cmake"
APP_FEATURE_REGISTRY_H = QML_DIR / "app/composition/AppFeatureRegistry.h"
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
    QML_DIR / "features/group/sources.cmake",
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
    "icons.qrc",
    "web.qrc",
)
APP_PORT_BINDER_FILES = (
    "app/composition/AppPortBinder.cpp",
    "app/composition/AppSessionPortBinder.cpp",
    "app/composition/AppSessionAuthPortBinder.cpp",
    "app/composition/AppPostLoginBootstrapPortBinder.cpp",
    "app/composition/AppRelationBootstrapPortBinder.cpp",
    "app/composition/AppRegisterCountdownPortBinder.cpp",
    "app/composition/AppSessionLogoutPortBinder.cpp",
    "app/composition/AppMediaPortBinder.cpp",
    "app/composition/AppCallPortBinder.cpp",
    "app/composition/AppFeaturePortBinder.cpp",
    "app/composition/AppAuthFeaturePortBinder.cpp",
    "app/composition/AppContactFeaturePortBinder.cpp",
    "app/composition/AppGroupFeaturePortBinder.cpp",
    "app/composition/AppProfileFeaturePortBinder.cpp",
    "app/composition/AppConnectionPortBinder.cpp",
)
APP_SIGNAL_BINDER_FILES = (
    "app/composition/AppSignalBinder.cpp",
    "app/composition/AppHttpSignalBinder.cpp",
    "app/composition/AppChatTransportSignalBinder.cpp",
    "app/composition/AppChatDispatcherSignalBinder.cpp",
    "app/composition/AppCallSignalBinder.cpp",
    "app/composition/AppFeatureFacadeSignalBinder.cpp",
    "app/composition/AppShellSignalBinder.cpp",
    "app/composition/AppChatProjectionSignalBinder.cpp",
    "app/composition/AppTimerSignalBinder.cpp",
)
APP_CHAT_BINDING_FILES = (
    "app/controller/AppControllerChatFeatureBinding.cpp",
    "app/controller/AppChatProjectionBinding.cpp",
    "app/controller/AppChatSendBinding.cpp",
    "app/controller/AppChatDialogBinding.cpp",
    "app/controller/AppChatHistoryBinding.cpp",
    "app/controller/AppChatGroupBinding.cpp",
    "app/controller/AppChatMediaBinding.cpp",
    "app/controller/AppChatReadMutationBinding.cpp",
)
CALL_COORDINATOR_FILES = (
    "app/coordinators/CallCoordinator.cpp",
    "app/coordinators/CallCoordinatorCommands.cpp",
    "app/coordinators/CallCoordinatorEvents.cpp",
    "app/coordinators/CallCoordinatorHttp.cpp",
    "app/coordinators/CallCoordinatorLivekit.cpp",
)
LEGACY_BUSINESS_QRC_FILES = (
    "qml-chat.qrc",
    "qml-moments.qrc",
    "qml-agent.qrc",
    "qml-pet.qrc",
    "qml-r18.qrc",
)
FEATURE_QRC_FILES = (
    QML_DIR / "features/auth/resources/auth.qrc",
    QML_DIR / "features/call/resources/call.qrc",
    QML_DIR / "features/chat/resources/chat.qrc",
    QML_DIR / "features/contact/resources/contact.qrc",
    QML_DIR / "features/profile/resources/profile.qrc",
    QML_DIR / "features/settings/resources/settings.qrc",
    QML_DIR / "features/moments/resources/moments.qrc",
    QML_DIR / "features/agent/resources/agent.qrc",
    QML_DIR / "features/pet/resources/pet.qrc",
    QML_DIR / "features/r18/resources/r18.qrc",
)


def qml_cmake_text() -> str:
    return (
        QML_CMAKE.read_text(encoding="utf-8")
        + "\n"
        + "\n".join(manifest.read_text(encoding="utf-8") for manifest in QML_CMAKE_MANIFESTS if manifest.exists())
    )


def composition_port_binder_text() -> str:
    return "\n".join((QML_DIR / path).read_text(encoding="utf-8") for path in APP_PORT_BINDER_FILES)


def composition_signal_binder_text() -> str:
    return "\n".join((QML_DIR / path).read_text(encoding="utf-8") for path in APP_SIGNAL_BINDER_FILES)


def app_chat_binding_text() -> str:
    return "\n".join((QML_DIR / path).read_text(encoding="utf-8") for path in APP_CHAT_BINDING_FILES)


def call_coordinator_text() -> str:
    return "\n".join((QML_DIR / path).read_text(encoding="utf-8") for path in CALL_COORDINATOR_FILES)


def extract_class(source: str, class_name: str) -> str:
    start = source.index(f"class {class_name}")
    end = source.index("\n};", start)
    return source[start : end + 3]


def extract_function(source: str, signature: str) -> str:
    start = source.index(signature)
    open_brace = source.index("{", start)
    depth = 0
    for index in range(open_brace, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"Function body not found for {signature}")


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
        for path in FEATURE_QRC_FILES:
            with self.subTest(qrc=path.relative_to(QML_DIR)):
                self.assertTrue(path.is_file())
            qrc_text += path.read_text(encoding="utf-8") if path.exists() else ""

        for token in (
            'alias="qml/Main.qml"',
            'alias="qml/linux/Main.qml"',
            'alias="features/auth/view/LoginPage.qml"',
            'alias="features/call/view/CallOverlay.qml"',
            'alias="features/chat/view/ChatShellContent.qml"',
            'alias="features/moments/view/MomentsFeedPane.qml"',
            'alias="features/agent/view/AgentPane.qml"',
            'alias="features/pet/view/PetWindow.qml"',
            'alias="features/r18/view/R18ShellPane.qml"',
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

        resources_cmake = resources.read_text(encoding="utf-8")
        for legacy_qrc_name in LEGACY_BUSINESS_QRC_FILES:
            with self.subTest(legacy_business_qrc=legacy_qrc_name):
                self.assertFalse((RESOURCE_QRC_DIR / legacy_qrc_name).exists())
                self.assertNotIn(f"resources/qrc/{legacy_qrc_name}", resources_cmake)

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
            "app/bootstrap/MainLogging.cpp",
            "app/bootstrap/MainLogging.h",
            "app/bootstrap/MainPlatformBootstrap.cpp",
            "app/bootstrap/MainPlatformBootstrap.h",
            "app/bootstrap/MainQmlTypeRegistry.cpp",
            "app/bootstrap/MainQmlEngineSetup.cpp",
            "app/bootstrap/MainQmlWindowLoader.cpp",
            "app/bootstrap/MainRuntimeConfig.cpp",
            "app/bootstrap/MainRuntimeConfig.h",
            "app/composition/AppPortRegistry.cpp",
            "app/composition/AppPortRegistry.h",
            *APP_PORT_BINDER_FILES,
            "app/composition/AppPortBinder.h",
            "app/composition/AppComposition.cpp",
            "app/composition/AppComposition.h",
            "app/composition/AppChatDispatcherRouterFactory.cpp",
            "app/composition/AppChatDispatcherRouterFactory.h",
            "app/composition/AppSignalBinder.cpp",
            "app/composition/AppSignalBinder.h",
            "app/shell/AppShellStateController.cpp",
            "app/shell/AppShellStateController.h",
            "app/shell/ShellViewModel.cpp",
            "app/shell/ShellViewModel.h",
            "app/window/MainWindowEffects.cpp",
            "app/window/MainWindowEffects.h",
            "app/window/MainWindowHooks.cpp",
            "app/window/MainWindowHooks.h",
            "app/controller/AppController.cpp",
            *APP_CHAT_BINDING_FILES,
            "app/events/AppChatDispatcherEventRouter.cpp",
            "app/events/AppChatDispatcherEventRouter.h",
            "app/events/AppChatDispatcherGroupResponseHandlers.h",
            "app/events/AppHttpEventRouter.cpp",
            "app/events/AppHttpEventRouter.h",
            "app/connection/AppControllerConnectionState.h",
            "app/controller/AppControllerRuntimeState.h",
            "app/controller/AppControllerUserState.h",
            "app/session/AppSessionCoordinator.cpp",
            "app/session/AppSessionCoordinatorAuthTimers.cpp",
            "app/session/AppSessionCoordinatorConnectionState.cpp",
            "app/session/SessionAuthCoordinator.cpp",
            "app/session/SessionAuthCoordinatorCommands.cpp",
            "app/session/SessionAuthCoordinatorLoginResponse.cpp",
            "app/session/SessionAuthCoordinatorAuthResponses.cpp",
            "app/session/SessionChatEntryCoordinator.cpp",
            "app/session/SessionRelationBootstrap.cpp",
            "app/coordinators/RegisterCountdownController.cpp",
            *CALL_COORDINATOR_FILES,
            "app/coordinators/CallCoordinatorPayloadPolicy.cpp",
            "app/coordinators/CallCoordinatorPayloadPolicy.h",
            "app/coordinators/MediaCoordinator.cpp",
            "app/coordinators/MediaPendingAttachmentRunner.cpp",
            "app/coordinators/MediaPendingAttachmentRunner.h",
            "app/controller/AppControllerContactEvents.cpp",
            "app/controller/AppControllerDialogListPorts.cpp",
            "app/controller/AppControllerDialogState.cpp",
            "app/controller/AppControllerGroupCommands.cpp",
            "app/controller/AppControllerGroupEvents.cpp",
            "app/controller/AppControllerModels.cpp",
            "app/controller/AppControllerGroupResponses.cpp",
            "app/controller/AppControllerGroupResponseErrors.cpp",
            "app/controller/AppControllerGroupManagementResponses.cpp",
            "app/controller/AppControllerGroupHistoryResponses.cpp",
            "app/controller/AppControllerGroupMessageResponses.cpp",
            "app/controller/AppControllerPrivateMessageResponses.cpp",
            "app/controller/AppControllerDialogMetaResponses.cpp",
            "app/controller/AppControllerDialogModels.cpp",
            "app/controller/AppControllerPrivateHistory.cpp",
            "app/controller/AppControllerPrivateSelection.cpp",
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
            "app/controller/AppControllerIncomingBuffer.cpp",
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
            "app/controller/AppControllerPrivateEvents.cpp",
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
            "features/call/controller/CallRequestPayloads.cpp",
            "features/call/controller/CallRequestPayloads.h",
            "features/chat/cache/ChatCacheMessageCodec.cpp",
            "features/chat/controller/ChatDialogRuntimeState.h",
            "features/chat/controller/ChatFeatureController.cpp",
            "features/chat/controller/ChatFeatureController.h",
            "features/chat/controller/ChatPendingSendQueueState.h",
            "features/chat/model/ChatMessageModelContent.cpp",
            "features/chat/model/ChatMessageModel.cpp",
            "features/chat/model/ChatMessageModelMutations.cpp",
            "features/chat/model/ChatMessageModelPresentation.cpp",
            "features/chat/model/ChatMessageModelQueries.cpp",
            "features/chat/services/DialogListEntryBuilder.cpp",
            "features/chat/services/GroupConversationService.cpp",
            "features/chat/services/PrivateChatEventService.cpp",
            "features/chat/services/PrivateChatHistoryRequestService.cpp",
            "features/chat/services/PrivateChatHistoryService.cpp",
            "features/chat/services/PrivateChatSendService.cpp",
            "features/chat/cache/GroupChatCacheStore.cpp",
            "features/chat/cache/PrivateChatCacheStore.cpp",
            "features/contact/controller/ContactEventService.cpp",
            "features/contact/controller/ContactEventService.h",
            "features/contact/controller/ContactRequestPayloads.cpp",
            "features/contact/controller/ContactRequestPayloads.h",
            "features/contact/model/FriendListModel.cpp",
            "features/contact/model/FriendListModelMutations.cpp",
            "features/contact/model/FriendListModelState.cpp",
            "features/contact/model/FriendListModelQueries.cpp",
            "features/group/controller/GroupManagementEventService.cpp",
            "features/group/controller/GroupManagementEventService.h",
            "features/group/controller/GroupManagementResponseService.cpp",
            "features/group/controller/GroupManagementResponseService.h",
            "features/group/controller/GroupRequestPayloads.cpp",
            "features/group/controller/GroupRequestPayloads.h",
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
            "features/profile/controller/ProfileRequestPayloads.cpp",
            "features/profile/controller/ProfileRequestPayloads.h",
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
        client_gateway_test_cmake = CLIENT_GATEWAY_TEST_CMAKE.read_text(encoding="utf-8")
        client_connection_test_cmake = CLIENT_CONNECTION_TEST_CMAKE.read_text(encoding="utf-8")

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
        self.assertIn("TransportEndpointPolicy.cpp", client_gateway_test_cmake)
        self.assertIn("transport_endpoint_policy_gtest", client_gateway_test_cmake)
        self.assertIn("AppChatConnectionPolicy.cpp", client_connection_test_cmake)
        self.assertIn("app_chat_connection_policy_gtest", client_connection_test_cmake)
        self.assertIn("set(MEMOCHAT_QML_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/core)", qml_cmake)
        self.assertIn("set(MEMOCHAT_QML_FEATURE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/features)", qml_cmake)
        self.assertIn("set(MEMOCHAT_QML_SHARED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shared)", qml_cmake)
        for path in APP_PORT_BINDER_FILES:
            with self.subTest(port_binder_source=path):
                self.assertIn(path, qml_cmake)
        self.assertIn("app/composition/AppSignalBinder.cpp", qml_cmake)
        self.assertIn("app/controller/AppController.cpp", qml_cmake)
        for path in APP_CHAT_BINDING_FILES:
            with self.subTest(chat_binding_source=path):
                self.assertIn(path, qml_cmake)
        self.assertNotIn("app/MainQmlBootstrap.cpp", qml_cmake)
        self.assertIn("app/bootstrap/MainQmlTypeRegistry.cpp", qml_cmake)
        self.assertIn("app/bootstrap/MainQmlEngineSetup.cpp", qml_cmake)
        self.assertIn("app/bootstrap/MainQmlWindowLoader.cpp", qml_cmake)
        self.assertIn("app/bootstrap/MainRuntimeConfig.cpp", qml_cmake)
        self.assertIn("app/connection/AppControllerConnectionState.h", qml_cmake)
        self.assertNotIn("app/controller/AppControllerGroupState.h", qml_cmake)
        self.assertIn("features/chat/controller/ChatPendingSendQueueState.h", qml_cmake)
        self.assertIn("features/chat/controller/ChatFeatureControllerIncoming.cpp", qml_cmake)
        self.assertIn("features/chat/services/IncomingMessageRouter.cpp", qml_cmake)
        self.assertIn("features/chat/services/IncomingMessageRouter.h", qml_cmake)
        self.assertIn("app/controller/AppControllerRuntimeState.h", qml_cmake)
        self.assertIn("app/controller/AppControllerUserState.h", qml_cmake)
        self.assertIn("app/shell/AppShellStateController.cpp", qml_cmake)
        self.assertIn("app/shell/AppShellStateController.h", qml_cmake)
        self.assertIn("app/session/AppSessionCoordinator.cpp", qml_cmake)
        self.assertIn("app/session/SessionAuthCoordinator.cpp", qml_cmake)
        self.assertIn("app/session/SessionAuthCoordinatorCommands.cpp", qml_cmake)
        self.assertIn("app/session/SessionAuthCoordinatorLoginResponse.cpp", qml_cmake)
        self.assertIn("app/session/SessionAuthCoordinatorAuthResponses.cpp", qml_cmake)
        self.assertIn("app/session/SessionChatEntryCoordinator.cpp", qml_cmake)
        self.assertIn("app/session/SessionRelationBootstrap.cpp", qml_cmake)
        self.assertIn("app/coordinators/RegisterCountdownController.cpp", qml_cmake)
        for rel_path in CALL_COORDINATOR_FILES:
            with self.subTest(call_coordinator_source=rel_path):
                self.assertIn(rel_path, qml_cmake)
        self.assertNotIn("app/coordinators/ContactCoordinatorShell.cpp", qml_cmake)
        self.assertNotIn("app/coordinators/GroupCoordinator.cpp", qml_cmake)
        self.assertIn("app/coordinators/MediaCoordinator.cpp", qml_cmake)
        self.assertIn("app/coordinators/MediaPendingAttachmentRunner.cpp", qml_cmake)
        self.assertIn("app/coordinators/MediaPendingAttachmentRunner.h", qml_cmake)
        self.assertNotIn("app/coordinators/ProfileCoordinator.cpp", qml_cmake)
        self.assertNotIn("app/controller/AppControllerGroupPayloads.cpp", qml_cmake)
        self.assertNotIn("app/controller/AppControllerGroupPayloads.h", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupEvents.cpp", qml_cmake)
        self.assertNotIn("app/controller/AppControllerGroupManagement.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupResponses.cpp", qml_cmake)
        self.assertIn("app/events/AppChatDispatcherGroupResponseHandlers.h", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupResponseErrors.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupManagementResponses.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupHistoryResponses.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerGroupMessageResponses.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerPrivateMessageResponses.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerDialogMetaResponses.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerDialogModels.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerPrivateHistory.cpp", qml_cmake)
        self.assertNotIn("app/controller/AppControllerReadAcks.cpp", qml_cmake)
        self.assertIn("app/controller/AppControllerPrivateSelection.cpp", qml_cmake)
        self.assertNotIn("app/controller/AppControllerContactSelection.cpp", qml_cmake)
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
        self.assertNotIn("app/controller/AppControllerMessageStatus.cpp", qml_cmake)
        self.assertNotIn("app/controller/AppControllerPrivateHistoryResponses.cpp", qml_cmake)
        self.assertNotIn("app/controller/AppControllerPrivateMessageEvents.cpp", qml_cmake)
        self.assertNotIn("app/controller/AppControllerPrivateReadEvents.cpp", qml_cmake)
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
        self.assertIn("features/group/controller/GroupManagementEventService.cpp", qml_cmake)
        self.assertIn("features/group/controller/GroupManagementEventService.h", qml_cmake)
        self.assertIn("features/group/controller/GroupManagementResponseService.cpp", qml_cmake)
        self.assertIn("features/group/controller/GroupManagementResponseService.h", qml_cmake)
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
        for path in APP_PORT_BINDER_FILES:
            with self.subTest(port_binder_source=path):
                self.assertIn(path, app)
        self.assertIn("app/composition/AppPortBinder.h", app)
        self.assertIn("app/composition/AppSignalBinder.cpp", app)
        self.assertIn("app/composition/AppSignalBinder.h", app)
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
        composition = (QML_DIR / "app/composition/AppComposition.cpp").read_text(encoding="utf-8")
        loader = (QML_DIR / "app/bootstrap/MainQmlWindowLoader.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/MainQmlBootstrap.cpp").exists())
        self.assertIn("void registerMemoChatQmlTypes();", bootstrap)
        self.assertIn("QUrl memoChatMainQmlUrl();", bootstrap)
        self.assertIn("void configureMemoChatEngine", bootstrap)
        self.assertIn("bool loadMemoChatMainWindow", bootstrap)
        self.assertIn("void registerMemoChatQmlTypes()", registry)
        self.assertIn("qmlRegisterUncreatableType<ShellViewModel>", registry)
        self.assertNotIn("qmlRegisterUncreatableType<AppController>", registry)
        self.assertIn("void configureMemoChatEngine", engine)
        self.assertIn("composition.configureQmlContext(*engine.rootContext());", engine)
        self.assertIn('context.setContextProperty("shell", shell())', composition)
        self.assertNotIn('setContextProperty("controller"', engine)
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
            (QML_DIR / "features/r18/resources/r18.qrc").read_text(encoding="utf-8")
            + "\n"
            + (QML_DIR / "features/agent/resources/agent.qrc").read_text(encoding="utf-8")
        )
        r18_shell = (QML_DIR / "features/r18/view/R18ShellPane.qml").read_text(encoding="utf-8")
        r18_runtime = (QML_DIR / "features/r18/view/R18ShellRuntime.js").read_text(encoding="utf-8")
        agent_pane = (QML_DIR / "features/agent/view/AgentPane.qml").read_text(encoding="utf-8")
        agent_runtime = (QML_DIR / "features/agent/view/AgentPaneRuntime.js").read_text(encoding="utf-8")

        self.assertIn("features/r18/view/R18ShellRuntime.js", qrc)
        self.assertIn("features/agent/view/AgentPaneRuntime.js", qrc)
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
        chat_dispatcher_router = (QML_DIR / "app/events/AppChatDispatcherEventRouter.cpp").read_text(encoding="utf-8")
        chat_dispatcher_handlers = (QML_DIR / "app/events/AppChatDispatcherGroupResponseHandlers.h").read_text(
            encoding="utf-8"
        )
        responses = (QML_DIR / "app/controller/AppControllerGroupResponses.cpp").read_text(encoding="utf-8")
        errors = (QML_DIR / "app/controller/AppControllerGroupResponseErrors.cpp").read_text(encoding="utf-8")
        management = (QML_DIR / "app/controller/AppControllerGroupManagementResponses.cpp").read_text(encoding="utf-8")
        event_service = (QML_DIR / "features/group/controller/GroupManagementEventService.cpp").read_text(
            encoding="utf-8"
        )
        response_service = (QML_DIR / "features/group/controller/GroupManagementResponseService.cpp").read_text(
            encoding="utf-8"
        )
        error_service = (QML_DIR / "features/group/controller/GroupResponseErrorService.cpp").read_text(
            encoding="utf-8"
        )
        history = (QML_DIR / "app/controller/AppControllerGroupHistoryResponses.cpp").read_text(encoding="utf-8")
        group_messages = (QML_DIR / "app/controller/AppControllerGroupMessageResponses.cpp").read_text(encoding="utf-8")
        private_messages = (QML_DIR / "app/controller/AppControllerPrivateMessageResponses.cpp").read_text(
            encoding="utf-8"
        )
        dialog_meta = (QML_DIR / "app/controller/AppControllerDialogMetaResponses.cpp").read_text(encoding="utf-8")
        protocol_router = (QML_DIR / "features/chat/services/ChatProtocolRouter.cpp").read_text(encoding="utf-8")

        self.assertIn("void AppChatDispatcherEventRouter::onGroupChatMsg", chat_dispatcher_router)
        self.assertIn("void AppChatDispatcherEventRouter::onGroupRsp", chat_dispatcher_router)
        self.assertIn("AppChatDispatcherGroupResponseHandlers", chat_dispatcher_handlers + responses)
        self.assertIn("handlers.handleError", responses)
        self.assertIn("_group_response_handlers.handleError", chat_dispatcher_router)
        self.assertIn("bool AppController::handleGroupRspError", errors)
        self.assertIn("void AppController::handleGroupManagementRsp", management)
        self.assertIn("GroupManagementEventService::reduceGroupListUpdated", chat_dispatcher_router)
        self.assertIn("GroupManagementEventService::reduceGroupInvite", chat_dispatcher_router)
        self.assertIn("GroupManagementEventService::reduceGroupApply", chat_dispatcher_router)
        self.assertIn("GroupManagementEventService::reduceGroupMemberChanged", chat_dispatcher_router)
        self.assertIn("GroupManagementResponseService::reduceSuccess", management)
        self.assertIn("GroupResponseErrorService::reduce", errors)
        self.assertNotIn("GroupManagementResponseService::reduceError", errors)
        self.assertIn('QStringLiteral("收到群邀请', event_service)
        self.assertIn('QStringLiteral("收到入群申请', event_service)
        self.assertIn('QStringLiteral("群事件：%1")', event_service)
        self.assertIn('QStringLiteral("群聊创建成功', response_service)
        self.assertIn('QStringLiteral("群聊已解散")', response_service)
        self.assertIn("GroupResponseErrorService::reduce", error_service)
        self.assertNotIn("AppController", event_service + response_service + error_service)
        self.assertNotIn("QJsonDocument(", events + chat_dispatcher_router + management)
        self.assertIn("void AppController::handleGroupHistoryRsp", history)
        self.assertIn("void AppController::handleGroupMessageAckRsp", group_messages)
        self.assertIn("void AppController::handleGroupMessageMutationRsp", group_messages)
        self.assertIn("void AppController::handlePrivateMessageMutationRsp", private_messages)
        self.assertIn("void AppController::handlePrivateForwardRsp", private_messages)
        self.assertIn("void AppController::handleDialogMetaRsp", dialog_meta)
        self.assertIn("GroupResponseRoute::History", chat_dispatcher_router)
        self.assertIn("case ID_GROUP_HISTORY_RSP", protocol_router)
        self.assertIn("_group_response_handlers.handleHistory", chat_dispatcher_router)
        self.assertIn("handleGroupHistoryRsp(payload)", responses)
        self.assertNotIn("MessagePayloadService::parseMessageUpdateFields", responses + chat_dispatcher_router)
        self.assertNotIn("MessagePayloadService::buildGroupAckMessage", responses + chat_dispatcher_router)
        self.assertNotIn("saveDraftStore(ownerUid)", responses + chat_dispatcher_router)
        self.assertNotIn("收到群邀请", events + chat_dispatcher_router)
        self.assertNotIn("收到入群申请", events + chat_dispatcher_router)
        self.assertNotIn("群聊创建成功", management)
        self.assertNotIn("群聊已解散", management)
        self.assertNotIn("void AppController::onGroupRsp", events)
        self.assertNotIn("void AppController::onGroupRsp", responses)
        self.assertLess(len(events.splitlines()), 260)
        self.assertLess(len(responses.splitlines()), 180)

    def test_heavy_group_command_management_concerns_are_split(self):
        header = (QML_DIR / "app/controller/AppController.h").read_text(encoding="utf-8")
        controller = (
            (QML_DIR / "app/controller/AppController.cpp").read_text(encoding="utf-8")
            + "\n"
            + composition_port_binder_text()
            + "\n"
            + app_chat_binding_text()
        )
        commands = (QML_DIR / "app/controller/AppControllerGroupCommands.cpp").read_text(encoding="utf-8")
        group_controller = (QML_DIR / "features/group/controller/GroupController.cpp").read_text(encoding="utf-8")
        group_payloads = (QML_DIR / "features/group/controller/GroupRequestPayloads.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/controller/AppControllerGroupManagement.cpp").exists())
        self.assertNotIn("void loadGroupHistory();", header)
        self.assertNotIn("void AppController::loadGroupHistory", commands)
        self.assertIn("void AppController::bindChatFeatureGroupPorts()", controller)
        self.assertIn("ChatCurrentGroupHistoryPort currentGroupHistoryPort;", controller)
        self.assertIn(
            "_features.chatFeatureController.setCurrentGroupHistoryPort(std::move(currentGroupHistoryPort));",
            controller,
        )
        self.assertIn("_features.chatFeatureController.requestCurrentGroupHistory();", controller)
        self.assertNotIn(
            "_features.chatFeatureController.requestGroupHistory(request, groupConversationDependencies(), port);",
            controller,
        )
        self.assertIn("void AppController::requestGroupHistoryForBootstrap", commands)
        self.assertIn("void GroupController::updateGroupAnnouncement", group_controller)
        self.assertIn("void GroupController::updateGroupIcon", group_controller)
        self.assertIn("pickGroupIcon", group_controller)
        self.assertIn("uploadGroupIcon", group_controller)
        self.assertIn("buildUpdateGroupIconPayload", group_payloads)
        self.assertIn("ID_UPDATE_GROUP_ICON_REQ", group_controller)
        self.assertIn("QFutureWatcher<UploadedMediaInfo>", group_controller)
        self.assertIn("QtConcurrent::run", group_controller)
        self.assertIn("void GroupController::setGroupAdmin", group_controller)
        self.assertIn("void GroupController::muteGroupMember", group_controller)
        self.assertIn("void GroupController::kickGroupMember", group_controller)
        self.assertIn("void GroupController::quitCurrentGroup", group_controller)
        self.assertIn("void GroupController::dissolveCurrentGroup", group_controller)
        self.assertNotIn("void updateGroupIcon(int source = 0);", header)
        self.assertNotIn("void AppController::updateGroupIcon", commands)
        self.assertNotIn("QFutureWatcher<UploadedMediaInfo>", commands)
        self.assertNotIn("QtConcurrent::run", commands)
        self.assertLess(len(commands.splitlines()), 360)

    def test_heavy_app_coordinator_concerns_are_split(self):
        session = (QML_DIR / "app/session/AppSessionCoordinator.cpp").read_text(encoding="utf-8")
        session_state = (QML_DIR / "app/session/AppSessionCoordinatorConnectionState.cpp").read_text(encoding="utf-8")
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
        coordinators = (QML_DIR / "app/coordinators/AppCoordinators.h").read_text(encoding="utf-8")
        call = call_coordinator_text()
        app_controller = (QML_DIR / "app/controller/AppController.cpp").read_text(encoding="utf-8")
        session_binder = (QML_DIR / "app/composition/AppSessionPortBinder.cpp").read_text(encoding="utf-8")
        session_auth_binder = (QML_DIR / "app/composition/AppSessionAuthPortBinder.cpp").read_text(encoding="utf-8")
        post_login_binder = (QML_DIR / "app/composition/AppPostLoginBootstrapPortBinder.cpp").read_text(
            encoding="utf-8"
        )
        relation_binder = (QML_DIR / "app/composition/AppRelationBootstrapPortBinder.cpp").read_text(encoding="utf-8")
        countdown_binder = (QML_DIR / "app/composition/AppRegisterCountdownPortBinder.cpp").read_text(encoding="utf-8")
        logout_binder = (QML_DIR / "app/composition/AppSessionLogoutPortBinder.cpp").read_text(encoding="utf-8")
        feature_binder = (QML_DIR / "app/composition/AppFeaturePortBinder.cpp").read_text(encoding="utf-8")
        group_feature_binder = (QML_DIR / "app/composition/AppGroupFeaturePortBinder.cpp").read_text(encoding="utf-8")
        profile_feature_binder = (QML_DIR / "app/composition/AppProfileFeaturePortBinder.cpp").read_text(
            encoding="utf-8"
        )
        call_binder = (QML_DIR / "app/composition/AppCallPortBinder.cpp").read_text(encoding="utf-8")
        signal_binder = composition_signal_binder_text()
        chat_dispatcher_router = (QML_DIR / "app/events/AppChatDispatcherEventRouter.cpp").read_text(encoding="utf-8")
        app_header = (QML_DIR / "app/controller/AppController.h").read_text(encoding="utf-8")
        profile_controller = (QML_DIR / "features/profile/controller/ProfileController.cpp").read_text(encoding="utf-8")
        profile_header = (QML_DIR / "features/profile/controller/ProfileController.h").read_text(encoding="utf-8")
        media = (QML_DIR / "app/coordinators/MediaCoordinator.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/AppCoordinators.cpp").exists())
        self.assertFalse((QML_DIR / "app/controller/AppControllerAuth.cpp").exists())
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
        self.assertIn("std::make_unique<AppSessionCoordinator>", session_binder)
        self.assertIn("SessionAuthPort{", session_auth_binder)
        self.assertIn("PostLoginBootstrapPort port;", post_login_binder)
        self.assertIn("RelationBootstrapPort{", relation_binder)
        self.assertIn("RegisterCountdownPort{", countdown_binder)
        self.assertIn("SessionLogoutPort port;", logout_binder)
        self.assertNotIn("std::make_unique<AppSessionCoordinator>", app_controller)
        self.assertNotIn("SessionAuthPort{", app_controller)
        self.assertNotIn("PostLoginBootstrapPort{", app_controller)
        self.assertNotIn("RelationBootstrapPort{", app_controller)
        self.assertNotIn("RegisterCountdownPort{", app_controller)
        self.assertNotIn("SessionLogoutPort{", app_controller)
        self.assertNotIn("QTimer::singleShot", session)
        self.assertNotIn("GetFriendListSnapshot", session)
        self.assertNotIn("gateAuthBusinessErrorTip", session)
        self.assertNotIn("class ContactCoordinatorShell", coordinators)
        self.assertIn("CallCoordinator::CallCoordinator", call)
        self.assertIn("void CallCoordinator::endCurrentCall", call)
        self.assertIn("void CallCoordinator::finalizeEndedCall", call)
        self.assertIn("bool CallCoordinator::ensureCallTargetFromCurrentChat", call)
        self.assertIn("void CallCoordinator::startCallFlow", call)
        self.assertIn("void CallCoordinator::onCallHttpFinished", call)
        self.assertIn("void CallCoordinator::onCallEvent", call)
        self.assertIn("void CallCoordinator::onLivekitRoomJoined", call)
        self.assertIn("void CallCoordinator::onLivekitRoomDisconnected", call)
        self.assertIn("explicit CallCoordinator(CallShellPort port);", coordinators)
        self.assertIn("CallShellPort _port;", coordinators)
        self.assertIn("CallController* callController() const;", coordinators)
        self.assertIn("call->leaveRoom();", call)
        self.assertIn("call->markEnded(statusText);", call)
        self.assertIn("_port.setAuthStatus(statusText, false);", call)
        self.assertIn("_port.runDelayed(1800", call)
        self.assertIn("call->startCall", call)
        self.assertNotIn("_app.", call)
        self.assertNotIn("AppController&", call)
        self.assertNotIn('#include "AppController.h"', call)
        self.assertNotIn("friend class CallCoordinator", app_header)
        self.assertIn("void finalizeEndedCall(const QString& statusText);", coordinators)
        self.assertNotIn("_app.startCallFlow", call)
        self.assertIn("_call_coordinator->onCallHttpFinished", signal_binder)
        self.assertIn("_call_coordinator.onCallEvent", chat_dispatcher_router)
        self.assertIn("_call_coordinator->onLivekitRoomDisconnected", signal_binder)
        self.assertNotIn("_call_coordinator->onCallHttpFinished", app_controller)
        self.assertNotIn("_call_coordinator->onCallEvent", app_controller)
        self.assertNotIn("_call_coordinator->onLivekitRoomDisconnected", app_controller)
        self.assertNotIn("&AppController::onCallHttpFinished", app_controller + signal_binder)
        self.assertNotIn("&AppController::onCallEvent", app_controller + signal_binder)
        self.assertNotIn("&AppController::onLivekitRoomDisconnected", app_controller + signal_binder)
        self.assertNotIn("void onCallHttpFinished(ReqId id, QString res, ErrorCodes err);", app_header)
        self.assertNotIn("void onCallEvent(QJsonObject payload);", app_header)
        self.assertNotIn("void onLivekitRoomDisconnected(const QString& reason, bool recoverable);", app_header)
        self.assertFalse((QML_DIR / "app/coordinators/ProfileCoordinator.cpp").exists())
        self.assertNotIn("class ProfileCoordinator", coordinators)
        self.assertNotIn("friend class ProfileCoordinator", app_header)
        self.assertIn("bindProfileFeaturePorts();", feature_binder)
        normalized_feature_binder = " ".join(profile_feature_binder.split())
        self.assertIn("_features.profileController.setCommandPort(ProfileCommandPort{", normalized_feature_binder)
        self.assertIn("_features.profileController.setStatePort( ProfileStatePort{", normalized_feature_binder)
        self.assertNotIn("_features.profileController.setCommandPort(ProfileCommandPort{", app_controller)
        self.assertNotIn("_features.profileController.setStatePort(", app_controller)
        self.assertIn("struct ProfileCommandPort", profile_header)
        self.assertIn("struct ProfileStatePort", profile_header)
        self.assertIn("void ProfileController::chooseAvatar", profile_controller)
        self.assertIn("void ProfileController::saveProfile", profile_controller)
        self.assertIn("void ProfileController::handleSettingsHttpFinished", profile_controller)
        self.assertNotIn("class GroupCoordinator", coordinators)
        self.assertIn("bindGroupFeaturePorts();", feature_binder)
        self.assertIn("_features.groupController.setCommandPort(", group_feature_binder)
        self.assertIn("GroupCommandPort{", group_feature_binder)
        self.assertNotIn("_features.groupController.setCommandPort(", app_controller)
        self.assertIn("std::make_unique<CallCoordinator>", call_binder)
        self.assertIn("CallShellPort{", call_binder)
        self.assertNotIn("std::make_unique<CallCoordinator>", app_controller)
        self.assertNotIn("CallShellPort{", app_controller)
        self.assertIn("MediaCoordinator::MediaCoordinator", media)
        self.assertIn("void MediaCoordinator::sendCurrentComposerPayload", media)
        self.assertIn("const AppPendingLoginState& AppSessionCoordinator::pendingLoginState", session_state)
        self.assertIn("void AppSessionCoordinator::applyLoginSuccessState", session_state)
        self.assertIn("void AppSessionCoordinator::setIgnoreNextLoginDisconnect", session_state)
        self.assertLessEqual(len(session.splitlines()), 95)
        self.assertLessEqual(len(session_state.splitlines()), 180)

    def test_heavy_app_controller_model_concerns_are_split(self):
        models = (QML_DIR / "app/controller/AppControllerModels.cpp").read_text(encoding="utf-8")
        dialogs = (QML_DIR / "app/controller/AppControllerDialogModels.cpp").read_text(encoding="utf-8")
        history = (QML_DIR / "app/controller/AppControllerPrivateHistory.cpp").read_text(encoding="utf-8")
        private_selection = (QML_DIR / "app/controller/AppControllerPrivateSelection.cpp").read_text(encoding="utf-8")
        chat_feature = (QML_DIR / "features/chat/controller/ChatFeatureControllerDialogList.cpp").read_text(
            encoding="utf-8"
        )
        dialog_service = (QML_DIR / "features/chat/services/DialogListService.cpp").read_text(encoding="utf-8")

        self.assertIn("void AppController::bootstrapDialogs", models)
        self.assertIn("void AppController::refreshGroupModel", models)
        self.assertIn("_features.groupController.setGroupsFromSnapshots", models)
        self.assertNotIn("FriendInfo", models)
        self.assertNotIn("std::vector<std::shared_ptr<FriendInfo>>", models)
        self.assertNotIn("std::make_shared<FriendInfo>", models)
        self.assertNotIn("new FriendInfo", models)
        self.assertNotIn("ConversationSyncService", models)
        self.assertNotIn("resolveGroupDialogUid", models)
        self.assertNotIn("qrc:/res/chat_icon.png", models)
        self.assertNotIn("_group_id", models)
        self.assertNotIn("_icon", models)
        self.assertNotIn("_announcement", models)
        self.assertNotIn("_last_msg", models)
        self.assertNotIn("_features.groupController.setGroups(", models)
        self.assertIn("void AppController::refreshDialogModel", dialogs)
        self.assertIn("void AppController::refreshDialogModelIncremental", dialogs)
        self.assertIn("_features.chatFeatureController.replaceDialogListFromSnapshots", dialogs)
        self.assertIn("_features.chatFeatureController.upsertDialogListFromSnapshots", dialogs)
        self.assertNotIn("DialogEntrySeed", dialogs)
        self.assertNotIn("DialogListService::buildDialogEntry", dialogs)
        self.assertNotIn("DialogListService::sortDialogs", dialogs)
        self.assertNotIn("ConversationSyncService::resolveGroupDialogUid", dialogs)
        self.assertNotIn("_features.chatFeatureController.dialogListModel().upsertBatch", dialogs)
        self.assertNotIn("_features.chatFeatureController.chatListModel().upsertFriend", dialogs)
        self.assertIn("ChatFeatureController::replaceDialogListFromSnapshots", chat_feature)
        self.assertIn("ChatFeatureController::upsertDialogListFromSnapshots", chat_feature)
        self.assertIn("ChatFeatureController::resetGroupDialogIdentityMap", chat_feature)
        self.assertIn("ChatFeatureController::resolveGroupDialogUid", chat_feature)
        self.assertIn("DialogListService::buildSortedSnapshotDialogs", chat_feature)
        self.assertIn("DialogListService::buildSortedSnapshotDialogs", dialog_service)
        self.assertIn("void AppController::loadCurrentChatMessages", history)
        self.assertIn("void AppController::requestPrivateHistory", history)
        self.assertFalse((QML_DIR / "app/controller/AppControllerReadAcks.cpp").exists())
        self.assertIn("void AppController::selectChatByUid", private_selection)
        self.assertNotIn("void AppController::loadCurrentChatMessages", models)
        self.assertNotIn("void AppController::refreshDialogModelIncremental", models)
        self.assertNotIn("void AppController::selectChatByUid", models)
        self.assertLess(len(models.splitlines()), 180)

    def test_heavy_app_controller_selection_concerns_are_split(self):
        private_selection = (QML_DIR / "app/controller/AppControllerPrivateSelection.cpp").read_text(encoding="utf-8")
        group_selection = (QML_DIR / "app/controller/AppControllerGroupSelection.cpp").read_text(encoding="utf-8")
        dialog_selection = (QML_DIR / "app/controller/AppControllerDialogSelection.cpp").read_text(encoding="utf-8")
        chat_binding = app_chat_binding_text()
        selection_factory_h = (QML_DIR / "app/controller/ChatDialogSelectionPortFactory.h").read_text(encoding="utf-8")
        selection_factory_cpp = (QML_DIR / "app/controller/ChatDialogSelectionPortFactory.cpp").read_text(
            encoding="utf-8"
        )
        pagination = (QML_DIR / "app/controller/AppControllerPagination.cpp").read_text(encoding="utf-8")
        contact_controller = (QML_DIR / "features/contact/controller/ContactController.cpp").read_text(encoding="utf-8")
        selection_service = (QML_DIR / "features/chat/services/ChatDialogSelectionService.cpp").read_text(
            encoding="utf-8"
        )

        self.assertFalse((QML_DIR / "app/AppControllerSelection.cpp").exists())
        self.assertFalse((QML_DIR / "app/controller/AppControllerContactSelection.cpp").exists())
        self.assertIn("void AppController::selectChatIndex", private_selection)
        self.assertIn("void AppController::selectChatByUid", private_selection)
        self.assertIn("_features.chatFeatureController.selectPrivateByIndex(index);", private_selection)
        self.assertIn("_features.chatFeatureController.selectPrivateByUid(uid);", private_selection)
        self.assertNotIn("ChatDialogSelectionService::selectPrivateByIndex", private_selection)
        self.assertNotIn("ChatDialogSelectionService::selectPrivateByUid", private_selection)
        self.assertIn("void ContactController::selectContactIndex", contact_controller)
        self.assertIn("QVariantMap ContactController::contactProfileByUid", contact_controller)
        self.assertIn("void ContactController::deleteFriend", contact_controller)
        self.assertIn("void AppController::selectGroupIndex", group_selection)
        self.assertIn("void AppController::selectGroupByDialogUid", group_selection)
        self.assertIn("_features.chatFeatureController.selectGroupByIndex(index);", group_selection)
        self.assertIn("_features.chatFeatureController.selectGroupByDialogUid(dialogUid, groupId);", group_selection)
        self.assertNotIn("ChatDialogSelectionService::selectGroupByIndex", group_selection)
        self.assertNotIn("ChatDialogSelectionService::selectGroupByDialogUid", group_selection)
        self.assertIn("void AppController::selectDialogByUid", dialog_selection)
        self.assertIn("void AppController::jumpChatWithCurrentContact", dialog_selection)
        self.assertNotIn("ChatDialogSelectionPort AppController::chatDialogSelectionPort", dialog_selection)
        self.assertIn(
            "memochat::app::ChatDialogSelectionActions AppController::chatDialogSelectionActions", dialog_selection
        )
        self.assertIn("ChatDialogSelectionPortFactory.h", dialog_selection)
        self.assertIn("struct ChatDialogSelectionActions", selection_factory_h)
        self.assertIn(
            "ChatDialogSelectionPort makeChatDialogSelectionPort(ChatDialogSelectionActions actions)",
            selection_factory_cpp,
        )
        self.assertIn("memochat::app::makeChatDialogSelectionPort(chatDialogSelectionActions())", chat_binding)
        self.assertNotIn("AppController", selection_factory_h + selection_factory_cpp)
        self.assertIn("_features.chatFeatureController.selectDialogByUid(uid);", dialog_selection)
        self.assertIn("void ChatDialogSelectionService::selectPrivateByUid", selection_service)
        self.assertIn("void ChatDialogSelectionService::selectGroupByDialogUid", selection_service)
        self.assertIn("void ContactController::showApplyRequests", contact_controller)
        self.assertIn("void AppController::loadMoreChats", pagination)
        load_more_chats = extract_function(pagination, "void AppController::loadMoreChats")
        self.assertIn("_features.chatFeatureController.loadMoreChats();", load_more_chats)
        self.assertNotIn("GetChatListPerPage", load_more_chats)
        self.assertNotIn("UpdateChatLoadedCount", load_more_chats)
        self.assertNotIn("appendFriends", load_more_chats)
        self.assertNotIn("void AppController::loadMorePrivateHistory", pagination)
        self.assertIn("ChatCurrentHistoryPort currentHistoryPort;", chat_binding)
        self.assertIn(
            "_features.chatFeatureController.setCurrentHistoryPort(std::move(currentHistoryPort));", chat_binding
        )
        self.assertIn("void ContactController::loadMoreContacts", contact_controller)
        self.assertNotIn("selectGroupIndex", private_selection)
        self.assertNotIn("selectChatIndex", group_selection)
        self.assertLess(len(private_selection.splitlines()), 30)
        self.assertLess(len(group_selection.splitlines()), 30)

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
        self.assertNotRegex(model_props, r"FriendListModel\s*\*\s*AppController::dialogListModel")
        self.assertRegex(model_props, r"ContactController\s*\*\s*AppController::contactController")
        self.assertRegex(model_props, r"GroupController\s*\*\s*AppController::groupController")
        self.assertRegex(model_props, r"PetController\s*\*\s*AppController::petController")
        self.assertNotIn("currentGroupCanChangeInfo", ui_props)
        self.assertNotIn("dialogListModel", group_props)
        self.assertLess(len(ui_props.splitlines()), 220)
        self.assertLess(len(model_props.splitlines()), 130)

    def test_heavy_app_controller_state_concerns_are_split(self):
        header = (QML_DIR / "app/controller/AppController.h").read_text(encoding="utf-8")
        coordinators = (QML_DIR / "app/coordinators/AppCoordinators.h").read_text(encoding="utf-8")
        connection_state_header = (QML_DIR / "app/connection/AppControllerConnectionState.h").read_text(
            encoding="utf-8"
        )
        runtime_state_header = (QML_DIR / "app/controller/AppControllerRuntimeState.h").read_text(encoding="utf-8")
        user_state_header = (QML_DIR / "app/controller/AppControllerUserState.h").read_text(encoding="utf-8")
        shell_state_header = (QML_DIR / "app/shell/AppShellStateController.h").read_text(encoding="utf-8")
        shell_state_source = (QML_DIR / "app/shell/AppShellStateController.cpp").read_text(encoding="utf-8")
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
        self.assertNotIn("AppPendingLoginState _pending_login_state", header)
        self.assertNotIn("AppChatEndpointState _chat_endpoint_state", header)
        self.assertNotIn("AppChatRecoveryState _chat_recovery_state", header)
        self.assertIn("AppPendingLoginState _pending_login_state", coordinators)
        self.assertIn("AppChatEndpointState _chat_endpoint_state", coordinators)
        self.assertIn("AppChatRecoveryState _chat_recovery_state", coordinators)
        self.assertNotIn("int _pending_uid", header)
        self.assertNotIn("QString _chat_server_host", header)
        self.assertNotIn("bool _reconnecting_chat", header)
        self.assertNotIn("qint64 _last_heartbeat_sent_ms", header)
        self.assertIn("struct AppLoadingRuntimeState", runtime_state_header)
        self.assertIn("struct AppLazyBootstrapState", runtime_state_header)
        self.assertIn("struct AppMediaUploadRuntimeState", runtime_state_header)
        self.assertNotIn("AppLoadingRuntimeState _loading_state", header)
        self.assertNotIn("AppLazyBootstrapState _bootstrap_state", header)
        self.assertIn("AppLoadingRuntimeState _loading_state", shell_state_header)
        self.assertIn("AppLazyBootstrapState _bootstrap_state", shell_state_header)
        self.assertIn("AppMediaUploadRuntimeState _media_upload_state", header)
        registry = APP_FEATURE_REGISTRY_H.read_text(encoding="utf-8")
        self.assertIn("AppFeatureRegistry _features", header)
        self.assertIn("ShellViewModel shellViewModel", registry)
        self.assertIn("AuthViewModel authViewModel", registry)
        self.assertIn("ContactController contactController", registry)
        self.assertIn("GroupController groupController", registry)
        self.assertIn("ProfileController profileController", registry)
        self.assertIn("CallController callController", registry)
        self.assertNotIn("QString _tip_text", header)
        self.assertNotIn("bool _search_pending", header)
        self.assertNotIn("QString _auth_status_text", header)
        self.assertNotIn("bool _chat_loading_more", header)
        self.assertNotIn("bool _dialogs_ready", header)
        self.assertNotIn("bool _media_upload_in_progress", header)
        self.assertIn("struct AppCurrentUserState", user_state_header)
        self.assertIn("struct AppCurrentChatState", user_state_header)
        self.assertIn("AppShellStateController _shell_state", header)
        self.assertIn("AppCurrentUserState _current_user", shell_state_header)
        self.assertIn("bool AppShellStateController::syncCurrentUser", shell_state_source)
        self.assertIn("bool AppShellStateController::resetCurrentUser", shell_state_source)
        self.assertNotIn("AppCurrentUserState _user_state", header)
        self.assertIn("AppCurrentChatState _chat_state", header)
        self.assertNotIn("AppGroupDialogState _group_dialog_state", header)
        self.assertNotIn("AppCurrentContactState _contact_state", header)
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
        self.assertIn("void AppController::syncCurrentUserProfileState", profile_state)
        self.assertIn("QString AppController::normalizeIconPath", profile_state)
        self.assertNotIn("userInfo->_icon = nextIcon;", profile_state)
        self.assertIn("void AppController::setDialogsReady", bootstrap_state)
        self.assertIn("void AppController::setApplyReady", bootstrap_state)
        self.assertNotIn("applyCurrentUserProfile", contact_state)
        self.assertNotIn("setPendingReplyContext", status_state)
        self.assertLess(len(contact_state.splitlines()), 100)
        self.assertLess(len(profile_state.splitlines()), 110)

    def test_heavy_app_controller_connection_concerns_are_split(self):
        app_controller = (QML_DIR / "app/controller/AppController.cpp").read_text(encoding="utf-8")
        connection_binder = (QML_DIR / "app/composition/AppConnectionPortBinder.cpp").read_text(encoding="utf-8")
        signal_binder = composition_signal_binder_text()
        app_header = (QML_DIR / "app/controller/AppController.h").read_text(encoding="utf-8")
        coordinator = (QML_DIR / "app/connection/AppChatConnectionCoordinator.cpp").read_text(encoding="utf-8")
        coordinator_header = (QML_DIR / "app/connection/AppChatConnectionCoordinator.h").read_text(encoding="utf-8")
        policy = (QML_DIR / "app/connection/AppChatConnectionPolicy.cpp").read_text(encoding="utf-8")
        chat_dispatcher_router = (QML_DIR / "app/events/AppChatDispatcherEventRouter.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/AppControllerConnection.cpp").exists())
        self.assertFalse((QML_DIR / "app/AppControllerConnectionLogin.cpp").exists())
        self.assertFalse((QML_DIR / "app/AppControllerHeartbeat.cpp").exists())
        self.assertFalse((QML_DIR / "app/AppControllerReconnect.cpp").exists())
        self.assertIn("app/composition/AppPortBinder.cpp", qml_cmake_text())
        self.assertIn("app/composition/AppConnectionPortBinder.cpp", qml_cmake_text())
        self.assertIn("app/composition/AppSignalBinder.cpp", qml_cmake_text())
        for path in APP_SIGNAL_BINDER_FILES:
            with self.subTest(signal_binder_manifest=path):
                self.assertIn(path, qml_cmake_text())
        self.assertIn("bindAppControllerPorts();", app_controller)
        self.assertIn("bindAppControllerSignals();", app_controller)
        self.assertIn("std::make_unique<AppChatConnectionCoordinator>", connection_binder)
        self.assertNotIn("std::make_unique<AppChatConnectionCoordinator>", app_controller)
        self.assertIn("_chat_connection_coordinator->onTcpConnectFinished(success);", signal_binder)
        self.assertIn("_connection_coordinator.onChatLoginFailed(err);", chat_dispatcher_router)
        self.assertIn("_chat_connection_coordinator->onHeartbeatTimeout();", signal_binder)
        self.assertIn("_connection_coordinator.onHeartbeatAck(ackAtMs);", chat_dispatcher_router)
        self.assertIn("_connection_coordinator.onNotifyOffline();", chat_dispatcher_router)
        self.assertIn("_chat_connection_coordinator->onConnectionClosed();", signal_binder)
        self.assertNotIn("_chat_connection_coordinator->onConnectionClosed();", app_controller)
        self.assertNotIn("&AppController::onTcpConnectFinished", app_controller + signal_binder)
        self.assertNotIn("&AppController::onChatLoginFailed", app_controller + signal_binder)
        self.assertNotIn("&AppController::onHeartbeatTimeout", app_controller + signal_binder)
        self.assertNotIn("&AppController::onHeartbeatAck", app_controller + signal_binder)
        self.assertNotIn("&AppController::onNotifyOffline", app_controller + signal_binder)
        self.assertNotIn("&AppController::onConnectionClosed", app_controller + signal_binder)
        self.assertNotIn("void onTcpConnectFinished(bool success);", app_header)
        self.assertNotIn("void onChatLoginFailed(int err);", app_header)
        self.assertNotIn("void onHeartbeatTimeout();", app_header)
        self.assertNotIn("void onHeartbeatAck(qint64 ackAtMs);", app_header)
        self.assertNotIn("void onNotifyOffline();", app_header)
        self.assertNotIn("void onConnectionClosed();", app_header)
        self.assertNotIn("bool tryLoginFallbackToTcp(const QString& reason);", app_header)
        self.assertNotIn("bool tryReconnectChat();", app_header)
        self.assertNotIn("void resetHeartbeatTracking();", app_header)
        self.assertNotIn("friend class AppChatConnectionCoordinator;", app_header)
        self.assertIn("ChatConnectionPort", coordinator_header)
        self.assertNotIn("class AppController;", coordinator_header)
        self.assertNotIn("AppController&", coordinator_header)
        self.assertNotIn("AppController*", coordinator_header)
        self.assertNotIn('#include "AppController.h"', coordinator_header)
        self.assertNotIn('#include "AppController.h"', coordinator)
        self.assertNotIn("AppController::", coordinator)
        self.assertNotIn("_app.", coordinator)
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

    def test_app_port_binder_is_thin_and_split_by_responsibility(self):
        app_sources = (QML_DIR / "app/sources.cmake").read_text(encoding="utf-8")
        orchestrator = (QML_DIR / "app/composition/AppPortBinder.cpp").read_text(encoding="utf-8")
        app_header = (QML_DIR / "app/controller/AppController.h").read_text(encoding="utf-8")
        port_registry = (QML_DIR / "app/composition/AppPortRegistry.h").read_text(encoding="utf-8")
        port_registry_source = (QML_DIR / "app/composition/AppPortRegistry.cpp").read_text(encoding="utf-8")
        registry_owned_port_binders = "\n".join(
            (QML_DIR / path).read_text(encoding="utf-8")
            for path in APP_PORT_BINDER_FILES
            if path != "app/composition/AppPortBinder.cpp"
        )

        for path in APP_PORT_BINDER_FILES:
            with self.subTest(port_binder_source=path):
                self.assertTrue((QML_DIR / path).is_file())
                self.assertIn(path, app_sources)
        self.assertIn("app/composition/AppPortRegistry.cpp", app_sources)
        self.assertIn("app/composition/AppPortRegistry.h", app_sources)

        self.assertLessEqual(len(orchestrator.splitlines()), 40)
        self.assertIn("void AppController::bindAppControllerPorts()", orchestrator)
        for helper in (
            "_port_registry->bindSessionPorts",
            "_port_registry->bindMediaPorts",
            "_port_registry->bindCallPorts",
            "_port_registry->bindFeaturePorts",
            "_port_registry->bindConnectionPorts",
        ):
            with self.subTest(helper=helper):
                self.assertIn(f"{helper}();", orchestrator)
        for removed_helper in (
            "void bindAppSessionPorts();",
            "void bindAppMediaPorts();",
            "void bindAppCallPorts();",
            "void bindAppFeaturePorts();",
            "void bindAppAuthFeaturePorts();",
            "void bindAppContactFeaturePorts();",
            "void bindAppGroupFeaturePorts();",
            "void bindAppProfileFeaturePorts();",
            "void bindAppConnectionPorts();",
        ):
            with self.subTest(removed_helper=removed_helper):
                self.assertNotIn(removed_helper, app_header)

        for contract_token in (
            "struct AppPortRegistryRefs",
            "struct AppPortRegistryConstants",
            "struct AppPortRegistryQueries",
            "struct AppPortRegistryActions",
            "struct AppPortRegistryEvents",
            "struct AppPortRegistryContext",
            "explicit AppPortRegistry(AppPortRegistryContext context);",
        ):
            with self.subTest(port_registry_contract=contract_token):
                self.assertIn(contract_token, port_registry)
        self.assertNotIn("friend class AppPortRegistry;", app_header)
        for token in (
            '#include "AppController.h"',
            "class AppController;",
            "AppController&",
            "AppController*",
            "AppPortRegistry(AppController",
            "_controller",
        ):
            with self.subTest(port_registry_forbidden_token=token):
                self.assertNotIn(token, port_registry)
                self.assertNotIn(token, port_registry_source)
                self.assertNotIn(token, registry_owned_port_binders)
        for enum_token in (
            "AppController::LoginPage",
            "AppController::ChatPage",
            "AppController::ApplyRequestPane",
        ):
            with self.subTest(port_registry_enum_token=enum_token):
                self.assertNotIn(enum_token, port_registry_source)
                self.assertNotIn(enum_token, registry_owned_port_binders)

        feature_orchestrator = (QML_DIR / "app/composition/AppFeaturePortBinder.cpp").read_text(encoding="utf-8")
        for helper in (
            "bindAuthFeaturePorts",
            "bindContactFeaturePorts",
            "bindGroupFeaturePorts",
            "bindProfileFeaturePorts",
        ):
            with self.subTest(feature_helper=helper):
                self.assertIn(f"{helper}();", feature_orchestrator)
                self.assertIn(
                    f"void {helper}();", (QML_DIR / "app/composition/AppPortRegistry.h").read_text(encoding="utf-8")
                )

        forbidden_orchestrator_tokens = (
            "std::make_unique<",
            "setCommandPort(",
            "setStatePort(",
            "setBootstrapPort(",
            "_gateway",
            "_media_coordinator",
            "_session_coordinator",
            "_features.chatFeatureController",
            "SessionAuthPort{",
            "PostLoginBootstrapPort{",
            "RelationBootstrapPort{",
            "RegisterCountdownPort{",
            "SessionLogoutPort{",
            "MediaSendPort{",
            "CallShellPort{",
            "ChatConnectionPort{",
        )
        for token in forbidden_orchestrator_tokens:
            with self.subTest(forbidden_orchestrator_token=token):
                self.assertNotIn(token, orchestrator)
                self.assertNotIn(token, feature_orchestrator)

        max_lines = {
            "app/composition/AppFeaturePortBinder.cpp": 20,
            "app/composition/AppSignalBinder.cpp": 25,
            "app/composition/AppHttpSignalBinder.cpp": 45,
            "app/composition/AppChatTransportSignalBinder.cpp": 45,
            "app/composition/AppChatDispatcherSignalBinder.cpp": 95,
            "app/composition/AppCallSignalBinder.cpp": 85,
            "app/composition/AppFeatureFacadeSignalBinder.cpp": 65,
            "app/composition/AppShellSignalBinder.cpp": 45,
            "app/composition/AppChatProjectionSignalBinder.cpp": 45,
            "app/composition/AppTimerSignalBinder.cpp": 45,
            "app/composition/AppAuthFeaturePortBinder.cpp": 60,
            "app/composition/AppContactFeaturePortBinder.cpp": 80,
            "app/composition/AppGroupFeaturePortBinder.cpp": 150,
            "app/composition/AppProfileFeaturePortBinder.cpp": 100,
            "app/composition/AppSessionLogoutPortBinder.cpp": 320,
            "app/composition/AppPostLoginBootstrapPortBinder.cpp": 280,
        }
        for path in APP_PORT_BINDER_FILES:
            line_count = len((QML_DIR / path).read_text(encoding="utf-8").splitlines())
            with self.subTest(port_binder_line_count=path):
                self.assertLessEqual(line_count, max_lines.get(path, 240))
        for path in APP_SIGNAL_BINDER_FILES:
            line_count = len((QML_DIR / path).read_text(encoding="utf-8").splitlines())
            with self.subTest(signal_binder_line_count=path):
                self.assertLessEqual(line_count, max_lines[path])

    def test_heavy_app_controller_session_concerns_are_split(self):
        coordinator = (QML_DIR / "app/session/AppSessionCoordinator.cpp").read_text(encoding="utf-8")
        auth_coordinator = (QML_DIR / "app/session/SessionAuthCoordinator.cpp").read_text(encoding="utf-8")
        auth_commands = (QML_DIR / "app/session/SessionAuthCoordinatorCommands.cpp").read_text(encoding="utf-8")
        auth_login = (QML_DIR / "app/session/SessionAuthCoordinatorLoginResponse.cpp").read_text(encoding="utf-8")
        auth_responses = (QML_DIR / "app/session/SessionAuthCoordinatorAuthResponses.cpp").read_text(encoding="utf-8")
        chat_entry = (QML_DIR / "app/session/SessionChatEntryCoordinator.cpp").read_text(encoding="utf-8")
        relation = (QML_DIR / "app/session/SessionRelationBootstrap.cpp").read_text(encoding="utf-8")
        countdown = (QML_DIR / "app/coordinators/RegisterCountdownController.cpp").read_text(encoding="utf-8")
        session_timers = (QML_DIR / "app/session/AppSessionCoordinatorAuthTimers.cpp").read_text(encoding="utf-8")
        chat_dispatcher_router = (QML_DIR / "app/events/AppChatDispatcherEventRouter.cpp").read_text(encoding="utf-8")
        http_event_router = (QML_DIR / "app/events/AppHttpEventRouter.cpp").read_text(encoding="utf-8")
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
        self.assertIn("void AppHttpEventRouter::onLoginHttpFinished", http_event_router)
        self.assertIn("void AppChatDispatcherEventRouter::onSwitchToChat", chat_dispatcher_router)
        self.assertIn("void AppController::runPostLoginBootstrap", chat_bootstrap)
        self.assertIn("void AppController::requestRelationBootstrap", relation_bootstrap)
        self.assertIn("void AppChatDispatcherEventRouter::onRelationBootstrapUpdated", chat_dispatcher_router)
        self.assertIn("void AppSessionCoordinator::onRegisterCountdownTimeout", session_timers)
        self.assertIn("void AppSessionCoordinator::onRegisterCodeCooldownTimeout", session_timers)
        self.assertIn("void AppSessionCoordinator::stopRegisterCountdownTimer", session_timers)
        self.assertIn("_session_coordinator.onLoginHttpFinished", http_event_router)
        self.assertIn("_session_coordinator.onSwitchToChat", chat_dispatcher_router)
        self.assertIn("_session_coordinator->runPostLoginBootstrap", chat_bootstrap)
        self.assertIn("_session_coordinator->requestRelationBootstrap", relation_bootstrap)
        self.assertIn("_session_coordinator.onRelationBootstrapUpdated", chat_dispatcher_router)
        self.assertNotIn("_session_coordinator.onRegisterCountdownTimeout", http_event_router)
        self.assertIn("_auth->onLoginHttpFinished", coordinator)
        self.assertIn("_chat_entry->onSwitchToChat", coordinator)
        self.assertIn("_relation_bootstrap->requestRelationBootstrap", coordinator)
        self.assertIn("_register_countdown->onRegisterCountdownTimeout", session_timers)
        self.assertIn("_auth->onRegisterCodeCooldownTimeout", session_timers)
        self.assertNotIn("parseTransportKind", coordinator)
        self.assertNotIn("parseTransportKind", http_event_router)
        self.assertNotIn("parseTransportKind", auth_coordinator)
        self.assertNotIn("gateAuthBusinessErrorTip", auth_coordinator)
        self.assertNotIn("QTimer::singleShot", chat_bootstrap)
        self.assertNotIn("GetFriendListSnapshot", relation_bootstrap)
        self.assertNotIn("onRelationBootstrapUpdated", http_event_router)
        self.assertNotIn("onSwitchToChat", chat_bootstrap)
        self.assertNotIn("onRelationBootstrapUpdated", relation_bootstrap)
        self.assertNotIn("onLoginHttpFinished", chat_bootstrap)
        self.assertLess(len(chat_bootstrap.splitlines()), 20)
        self.assertLess(len(http_event_router.splitlines()), 90)

    def test_session_bootstrap_and_relation_coordinators_use_narrow_ports(self):
        coordinator = (QML_DIR / "app/session/AppSessionCoordinator.cpp").read_text(encoding="utf-8")
        chat_entry = (QML_DIR / "app/session/SessionChatEntryCoordinator.cpp").read_text(encoding="utf-8")
        relation = (QML_DIR / "app/session/SessionRelationBootstrap.cpp").read_text(encoding="utf-8")
        coordinators = (QML_DIR / "app/coordinators/AppCoordinators.h").read_text(encoding="utf-8")
        app_header = (QML_DIR / "app/controller/AppController.h").read_text(encoding="utf-8")
        chat_class = extract_class(coordinators, "SessionChatEntryCoordinator")
        relation_class = extract_class(coordinators, "SessionRelationBootstrap")

        self.assertNotIn("friend class SessionChatEntryCoordinator;", app_header)
        self.assertNotIn("friend class SessionRelationBootstrap;", app_header)

        self.assertIn("struct PostLoginBootstrapPort", coordinators)
        self.assertIn("struct RelationBootstrapPort", coordinators)
        self.assertIn("explicit SessionChatEntryCoordinator(PostLoginBootstrapPort port);", chat_class)
        self.assertIn("PostLoginBootstrapPort _port;", chat_class)
        self.assertIn("explicit SessionRelationBootstrap(RelationBootstrapPort port);", relation_class)
        self.assertIn("RelationBootstrapPort _port;", relation_class)

        for class_body, class_name in (
            (chat_class, "SessionChatEntryCoordinator"),
            (relation_class, "SessionRelationBootstrap"),
        ):
            with self.subTest(class_name=class_name):
                self.assertNotIn("AppController&", class_body)
                self.assertNotIn("AppController*", class_body)
                self.assertNotIn("_app", class_body)

        for source, source_name in (
            (chat_entry, "SessionChatEntryCoordinator.cpp"),
            (relation, "SessionRelationBootstrap.cpp"),
        ):
            with self.subTest(source=source_name):
                self.assertNotIn('#include "AppController.h"', source)
                self.assertNotIn("AppController::", source)
                self.assertNotIn("AppController&", source)
                self.assertNotIn("AppController*", source)
                self.assertNotIn("_app.", source)
                self.assertNotIn("&_app", source)

        self.assertIn("PostLoginBootstrapPort chatEntryPort", coordinator)
        self.assertIn("RelationBootstrapPort relationBootstrapPort", coordinator)
        self.assertIn("std::make_unique<SessionChatEntryCoordinator>(std::move(chatEntryPort))", coordinator)
        self.assertIn(
            "std::make_unique<SessionRelationBootstrap>(std::move(relationBootstrapPort))",
            coordinator,
        )
        self.assertNotIn("std::make_unique<SessionChatEntryCoordinator>(controller)", coordinator)
        self.assertNotIn("std::make_unique<SessionRelationBootstrap>(controller)", coordinator)

    def test_session_auth_workflow_is_owned_by_session_coordinator(self):
        coordinator = (QML_DIR / "app/session/AppSessionCoordinator.cpp").read_text(encoding="utf-8")
        auth_coordinator = (QML_DIR / "app/session/SessionAuthCoordinator.cpp").read_text(encoding="utf-8")
        auth_commands = (QML_DIR / "app/session/SessionAuthCoordinatorCommands.cpp").read_text(encoding="utf-8")
        auth_login = (QML_DIR / "app/session/SessionAuthCoordinatorLoginResponse.cpp").read_text(encoding="utf-8")
        auth_responses = (QML_DIR / "app/session/SessionAuthCoordinatorAuthResponses.cpp").read_text(encoding="utf-8")
        chat_entry = (QML_DIR / "app/session/SessionChatEntryCoordinator.cpp").read_text(encoding="utf-8")
        relation = (QML_DIR / "app/session/SessionRelationBootstrap.cpp").read_text(encoding="utf-8")
        countdown = (QML_DIR / "app/coordinators/RegisterCountdownController.cpp").read_text(encoding="utf-8")
        session_timers = (QML_DIR / "app/session/AppSessionCoordinatorAuthTimers.cpp").read_text(encoding="utf-8")
        chat_dispatcher_router = (QML_DIR / "app/events/AppChatDispatcherEventRouter.cpp").read_text(encoding="utf-8")
        http_event_router = (QML_DIR / "app/events/AppHttpEventRouter.cpp").read_text(encoding="utf-8")
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
        self.assertIn("RegisterCountdownPort", countdown)
        self.assertIn("stopTimer();", countdown)
        self.assertIn("_timer.stop();", countdown)
        self.assertNotIn("_app.", countdown)
        self.assertIn("SessionAuthPort", coordinator)
        self.assertIn("_port.sendLogin", auth_commands)
        self.assertIn("_port.prepareLoginAttempt", auth_commands)
        self.assertIn("parseTransportKind", auth_login)
        self.assertIn("gateAuthBusinessErrorTip", auth_responses)
        self.assertIn("_port.startRegisterCountdown()", auth_responses)
        self.assertNotIn("_app.", auth_commands)
        self.assertNotIn("_app.", auth_responses)
        self.assertNotIn("_app.", auth_login)
        self.assertNotIn("parseTransportKind", auth_coordinator)
        self.assertNotIn("gateAuthBusinessErrorTip", auth_coordinator)

        self.assertIn("_session_coordinator.onLoginHttpFinished", http_event_router)
        self.assertIn("_session_coordinator.onRegisterHttpFinished", http_event_router)
        self.assertIn("_session_coordinator.onResetHttpFinished", http_event_router)
        self.assertIn("_session_coordinator.onSwitchToChat", chat_dispatcher_router)
        self.assertIn("_session_coordinator->requestRelationBootstrap", relation_bootstrap)
        self.assertIn("_session_coordinator.onRelationBootstrapUpdated", chat_dispatcher_router)
        self.assertNotIn("_session_coordinator.onRegisterCountdownTimeout", http_event_router)
        self.assertIn("_register_countdown->onRegisterCountdownTimeout", session_timers)
        self.assertIn("_auth->onRegisterCodeCooldownTimeout", session_timers)

        self.assertNotIn("_features.authController.sendLogin", http_event_router)
        self.assertNotIn("_register_countdown_timer.start(1000)", http_event_router)
        self.assertNotIn("_features.authController.sendLogin", coordinator)
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
        chat_binding = app_chat_binding_text()
        controller = (
            (QML_DIR / "app/controller/AppController.cpp").read_text(encoding="utf-8")
            + "\n"
            + composition_port_binder_text()
            + "\n"
            + chat_binding
        )
        media = (QML_DIR / "app/coordinators/MediaCoordinator.cpp").read_text(encoding="utf-8")
        attachments = (QML_DIR / "app/controller/AppControllerPendingAttachments.cpp").read_text(encoding="utf-8")
        upload_queue = (QML_DIR / "app/controller/AppControllerMediaUploadQueue.cpp").read_text(encoding="utf-8")
        runner = (QML_DIR / "app/coordinators/MediaPendingAttachmentRunner.cpp").read_text(encoding="utf-8")
        runner_header = (QML_DIR / "app/coordinators/MediaPendingAttachmentRunner.h").read_text(encoding="utf-8")
        dispatch = (QML_DIR / "app/controller/AppControllerMessageDispatch.cpp").read_text(encoding="utf-8")
        call = call_coordinator_text()
        call_payload = (QML_DIR / "app/coordinators/CallCoordinatorPayloadPolicy.cpp").read_text(encoding="utf-8")
        call_payload_header = (QML_DIR / "app/coordinators/CallCoordinatorPayloadPolicy.h").read_text(encoding="utf-8")
        coordinators = (QML_DIR / "app/coordinators/AppCoordinators.h").read_text(encoding="utf-8")
        header = (QML_DIR / "app/controller/AppController.h").read_text(encoding="utf-8")
        qml_cmake = qml_cmake_text()

        self.assertFalse((QML_DIR / "app/controller/AppControllerMedia.cpp").exists())
        self.assertNotIn("app/controller/AppControllerMedia.cpp", qml_cmake)
        self.assertIn("void MediaCoordinator::sendTextMessage", media)
        self.assertIn("std::make_unique<MediaCoordinator>", controller)
        self.assertIn("MediaSendPort{", controller)
        self.assertIn("_media_coordinator->sendTextMessage(text)", controller)
        self.assertIn("_media_coordinator->sendCurrentComposerPayload(text)", controller)
        self.assertIn("_media_coordinator->sendImageMessage()", controller)
        self.assertIn("_media_coordinator->sendFileMessage()", controller)
        self.assertIn("_media_coordinator->removePendingAttachment(attachmentId)", controller)
        self.assertIn("_media_coordinator->clearPendingAttachments()", controller)
        self.assertNotIn("void AppController::startMediaUploadAndSend", attachments)
        self.assertNotIn("startMediaUploadAndSend", header)
        self.assertIn("void AppController::addPendingAttachments", attachments)
        self.assertIn("void AppController::removePendingAttachmentById", attachments)
        self.assertIn("void AppController::startPendingAttachmentRunner", upload_queue)
        self.assertIn("MediaPendingAttachmentRunnerPort{", upload_queue)
        self.assertIn("new MediaPendingAttachmentRunner", upload_queue)
        self.assertIn(
            "_features.chatFeatureController.dispatchUploadedAttachment(attachment, uploaded, destination)",
            upload_queue,
        )
        self.assertIn("MediaPendingAttachmentRunner::processNext", runner)
        self.assertIn("void MediaPendingAttachmentRunner::processAll", runner)
        self.assertIn("void MediaPendingAttachmentRunner::processNextAsync", runner)
        self.assertIn("void MediaPendingAttachmentRunner::handleUploadFinished", runner)
        self.assertIn("QFutureWatcher<MediaUploadResult>", runner)
        self.assertIn("QtConcurrent::run", runner)
        self.assertIn("Qt::QueuedConnection", runner)
        self.assertIn("std::function<void()> resetQueue", runner_header)
        self.assertIn("std::function<bool()> advanceQueue", runner_header)
        self.assertNotIn("AppController", runner + runner_header)
        self.assertNotIn("processPendingAttachmentQueue", upload_queue + runner + runner_header)
        self.assertNotIn("bool AppController::sendUploadedAttachmentToDialog", upload_queue)
        self.assertNotIn("MessageContentCodec::encodeImage", upload_queue)
        self.assertNotIn("MessageContentCodec::encodeFile", upload_queue)
        self.assertNotIn("UploadedAttachmentDispatchPort", upload_queue)
        self.assertNotIn("dispatchPrivatePacket", upload_queue)
        self.assertNotIn("dispatchGroupPayload", upload_queue)
        self.assertNotIn("slot_send_data", upload_queue)
        self.assertIn("features/chat/services/UploadedAttachmentDispatchService.cpp", qml_cmake)
        self.assertIn("features/chat/services/UploadedAttachmentDispatchService.h", qml_cmake)
        self.assertIn("app/coordinators/MediaPendingAttachmentRunner.cpp", qml_cmake)
        self.assertIn("app/coordinators/MediaPendingAttachmentRunner.h", qml_cmake)
        self.assertNotIn("bool AppController::dispatchChatContent", dispatch)
        self.assertNotIn("bool AppController::dispatchGroupChatContent", dispatch)
        self.assertIn("ChatContentDispatchPort contentDispatchPort;", chat_binding)
        self.assertIn(
            "_features.chatFeatureController.setContentDispatchPort(std::move(contentDispatchPort));", chat_binding
        )
        self.assertIn("_features.chatFeatureController.dispatchCurrentPrivateContent(content, preview)", chat_binding)
        self.assertIn("_features.chatFeatureController.dispatchCurrentGroupContent(content, preview)", chat_binding)
        self.assertNotIn("PrivateChatSendRequest", dispatch)
        self.assertNotIn("PrivateChatSendDependencies", dispatch)
        self.assertNotIn("GroupSendRequest", dispatch)
        self.assertNotIn("ChatCurrentPrivateSendCommand", dispatch)
        self.assertNotIn("ChatCurrentGroupSendCommand", dispatch)
        self.assertFalse((QML_DIR / "app/AppControllerCallTarget.cpp").exists())
        self.assertIn("bool CallCoordinator::ensureCallTargetFromCurrentChat", call)
        self.assertIn("void CallCoordinator::startCallFlow", call)
        self.assertIn("void CallCoordinator::finalizeEndedCall", call)
        self.assertIn("void CallCoordinator::onCallHttpFinished", call)
        self.assertIn("void CallCoordinator::onLivekitMediaError", call)
        self.assertIn("explicit CallCoordinator(CallShellPort port);", coordinators)
        self.assertIn("CallShellPort _port;", coordinators)
        self.assertIn("call->startCall", call)
        self.assertIn("_port.parseJson(res, obj)", call)
        self.assertNotIn("_app.", call)
        self.assertNotIn("AppController&", call)
        self.assertNotIn('#include "AppController.h"', call)
        self.assertNotIn("friend class CallCoordinator", header)
        for rel_path in CALL_COORDINATOR_FILES:
            with self.subTest(call_coordinator_source=rel_path):
                self.assertIn(rel_path, qml_cmake)
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
        self.assertNotIn("void AppController::sendTextMessage", header)
        self.assertNotIn("void AppController::processPendingAttachmentQueue", header + media + upload_queue)
        self.assertNotIn("bool AppController::dispatchGroupChatContent", media)
        self.assertNotIn("bool AppController::ensureCallTargetFromCurrentChat", media)
        self.assertLess(len(media.splitlines()), 180)

    def test_heavy_private_event_concerns_are_split(self):
        events = (QML_DIR / "app/controller/AppControllerPrivateEvents.cpp").read_text(encoding="utf-8")
        chat_dispatcher_router = (QML_DIR / "app/events/AppChatDispatcherEventRouter.cpp").read_text(encoding="utf-8")

        self.assertFalse((QML_DIR / "app/controller/AppControllerMessageStatus.cpp").exists())
        self.assertFalse((QML_DIR / "app/controller/AppControllerPrivateHistoryResponses.cpp").exists())
        self.assertFalse((QML_DIR / "app/controller/AppControllerPrivateMessageEvents.cpp").exists())
        self.assertFalse((QML_DIR / "app/controller/AppControllerPrivateReadEvents.cpp").exists())
        self.assertNotIn("void AppController::onTextChatMsg", events)
        self.assertIn("void AppChatDispatcherEventRouter::onTextChatMsg", chat_dispatcher_router)
        self.assertIn("void AppChatDispatcherEventRouter::onPrivateHistoryRsp", chat_dispatcher_router)
        self.assertIn("void AppChatDispatcherEventRouter::onPrivateMsgChanged", chat_dispatcher_router)
        self.assertIn("void AppChatDispatcherEventRouter::onPrivateReadAck", chat_dispatcher_router)
        self.assertIn("void AppChatDispatcherEventRouter::onMessageStatus", chat_dispatcher_router)
        self.assertNotIn("void AppController::onMessageStatus", events)
        self.assertNotIn("void AppController::onPrivateHistoryRsp", events)
        self.assertNotIn("void AppController::onPrivateMsgChanged", events)
        self.assertNotIn("void AppController::onPrivateReadAck", events)
        self.assertIn("PrivateChatEventService.h", events)
        self.assertIn("PrivateIncomingMessageRequest", events)
        self.assertIn("_features.chatFeatureController.handlePrivateIncomingMessage", events)
        self.assertIn("PrivateMessageChangedRequest", chat_dispatcher_router)
        self.assertIn("PrivateReadAckRequest", chat_dispatcher_router)
        self.assertIn("_chat_controller.handlePrivateMessageChanged", chat_dispatcher_router)
        self.assertIn("_chat_controller.handlePrivateReadAck", chat_dispatcher_router)
        self.assertIn("ChatEventDependenciesFactory.h", events)
        self.assertIn("memochat::app::makePrivateChatEventDependencies(", events)
        self.assertNotIn("memochat::app::PrivateChatEventActions AppController::privateChatEventActions", events)
        self.assertNotIn("memochat::app::GroupConversationActions AppController::groupConversationActions", events)
        self.assertLess(len(events.splitlines()), 340)

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
