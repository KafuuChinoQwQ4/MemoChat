import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
DESKTOP_CMAKE = REPO_ROOT / "apps/client/desktop/CMakeLists.txt"
QML_CMAKE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/CMakeLists.txt"
QML_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
CORE_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/core"
OLD_SHARED_DIR = REPO_ROOT / "apps/client/desktop/MemoChatShared"
CORE_QRC = CORE_DIR / "rc.qrc"
MOMENTS_CONTROLLER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/moments/MomentsController.h"


class MemoChatQmlCoreLayoutTests(unittest.TestCase):
    def test_core_lives_under_memochat_qml_and_legacy_shared_module_is_removed(self):
        self.assertTrue(CORE_DIR.is_dir())
        self.assertTrue((CORE_DIR / "CMakeLists.txt").is_file())
        self.assertTrue((CORE_DIR / "network/ChatMessageDispatcher.cpp").is_file())
        self.assertTrue((CORE_DIR / "network/httpmgr.cpp").is_file())
        self.assertTrue((CORE_DIR / "network/IChatTransport.h").is_file())
        self.assertTrue((CORE_DIR / "session/usermgr.cpp").is_file())
        self.assertTrue((CORE_DIR / "common/global.h").is_file())
        self.assertTrue((CORE_DIR / "media/imagecropperdialog.h").is_file())
        self.assertFalse(OLD_SHARED_DIR.exists())

    def test_qml_cpp_sources_are_grouped_by_responsibility(self):
        expected_files = (
            "app/main.cpp",
            "app/AppController.cpp",
            "features/agent/AgentController.cpp",
            "features/auth/AuthController.cpp",
            "features/chat/ChatMessageModel.cpp",
            "features/chat/PrivateChatCacheStore.cpp",
            "features/contact/FriendListModel.cpp",
            "features/moments/MomentsControllerParsing.cpp",
            "features/moments/MomentsControllerPublish.cpp",
            "features/moments/MomentsControllerRequests.cpp",
            "features/moments/MomentsControllerResponses.cpp",
            "shared/gateway/ClientGateway.cpp",
            "shared/media/MediaUploadService.cpp",
            "shared/utils/IconPathUtils.h",
            "platform/windows/ClientWinCompat.h",
            "live2d/Live2DRenderItem.cpp",
        )

        for rel_path in expected_files:
            with self.subTest(path=rel_path):
                self.assertTrue((QML_DIR / rel_path).is_file())

        root_cpp_or_h = [
            path
            for path in QML_DIR.iterdir()
            if path.is_file() and path.suffix in {".cpp", ".h"}
        ]
        self.assertEqual([], root_cpp_or_h)

        old_horizontal_dirs = ("controllers", "models", "services", "storage", "utils")
        for rel_dir in old_horizontal_dirs:
            with self.subTest(old_dir=rel_dir):
                self.assertFalse((QML_DIR / rel_dir).exists())

    def test_cmake_adds_core_from_qml_module_without_desktop_shared_entry(self):
        desktop_cmake = DESKTOP_CMAKE.read_text(encoding="utf-8")
        qml_cmake = QML_CMAKE.read_text(encoding="utf-8")

        self.assertNotIn("add_subdirectory(MemoChatShared)", desktop_cmake)
        self.assertIn("add_subdirectory(MemoChat-qml)", desktop_cmake)
        self.assertIn("add_subdirectory(core)", qml_cmake)
        self.assertIn("set(MEMOCHAT_QML_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/core)", qml_cmake)
        self.assertIn("set(MEMOCHAT_QML_FEATURE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/features)", qml_cmake)
        self.assertIn("set(MEMOCHAT_QML_SHARED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shared)", qml_cmake)
        self.assertIn("app/AppController.cpp", qml_cmake)
        self.assertIn("features/agent/AgentController.cpp", qml_cmake)
        self.assertIn("features/chat/ChatMessageModel.cpp", qml_cmake)
        self.assertIn("features/chat/PrivateChatCacheStore.cpp", qml_cmake)
        self.assertIn("features/moments/MomentsControllerParsing.cpp", qml_cmake)
        self.assertIn("features/moments/MomentsControllerPublish.cpp", qml_cmake)
        self.assertIn("features/moments/MomentsControllerRequests.cpp", qml_cmake)
        self.assertIn("features/moments/MomentsControllerResponses.cpp", qml_cmake)
        self.assertIn("shared/gateway/ClientGateway.cpp", qml_cmake)
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

    def test_heavy_moments_controller_concerns_are_split(self):
        controller = (QML_DIR / "features/moments/MomentsController.cpp").read_text(encoding="utf-8")
        parsing = (QML_DIR / "features/moments/MomentsControllerParsing.cpp").read_text(encoding="utf-8")
        publish = (QML_DIR / "features/moments/MomentsControllerPublish.cpp").read_text(encoding="utf-8")
        requests = (QML_DIR / "features/moments/MomentsControllerRequests.cpp").read_text(encoding="utf-8")
        responses = (QML_DIR / "features/moments/MomentsControllerResponses.cpp").read_text(encoding="utf-8")

        self.assertIn("MomentEntry MomentsController::parseMomentEntry", parsing)
        self.assertIn("void MomentsController::publishDraftMoment", publish)
        self.assertIn("void MomentsController::toggleLike", requests)
        self.assertIn("void MomentsController::onLoadFeedRsp", responses)
        self.assertNotIn("void MomentsController::publishDraftMoment", controller)
        self.assertNotIn("void MomentsController::onLoadFeedRsp", controller)
        self.assertLess(len(controller.splitlines()), 180)

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
