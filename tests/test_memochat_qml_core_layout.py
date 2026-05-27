import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
DESKTOP_CMAKE = REPO_ROOT / "apps/client/desktop/CMakeLists.txt"
QML_CMAKE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/CMakeLists.txt"
QML_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
CORE_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/core"
OLD_SHARED_DIR = REPO_ROOT / "apps/client/desktop/MemoChatShared"
CORE_QRC = CORE_DIR / "rc.qrc"
MOMENTS_CONTROLLER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/controllers/MomentsController.h"


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
            "controllers/AgentController.cpp",
            "controllers/AuthController.cpp",
            "models/ChatMessageModel.cpp",
            "models/FriendListModel.cpp",
            "services/ClientGateway.cpp",
            "services/MediaUploadService.cpp",
            "storage/PrivateChatCacheStore.cpp",
            "utils/IconPathUtils.h",
            "platform/ClientWinCompat.h",
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

    def test_cmake_adds_core_from_qml_module_without_desktop_shared_entry(self):
        desktop_cmake = DESKTOP_CMAKE.read_text(encoding="utf-8")
        qml_cmake = QML_CMAKE.read_text(encoding="utf-8")

        self.assertNotIn("add_subdirectory(MemoChatShared)", desktop_cmake)
        self.assertIn("add_subdirectory(MemoChat-qml)", desktop_cmake)
        self.assertIn("add_subdirectory(core)", qml_cmake)
        self.assertIn("set(MEMOCHAT_QML_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/core)", qml_cmake)
        self.assertIn("app/AppController.cpp", qml_cmake)
        self.assertIn("controllers/AgentController.cpp", qml_cmake)
        self.assertIn("models/ChatMessageModel.cpp", qml_cmake)
        self.assertIn("services/ClientGateway.cpp", qml_cmake)
        self.assertIn("storage/PrivateChatCacheStore.cpp", qml_cmake)
        self.assertIn("live2d/Live2DRenderItem.cpp", qml_cmake)
        self.assertIn("${CMAKE_CURRENT_SOURCE_DIR}/controllers", qml_cmake)
        self.assertIn("${CMAKE_CURRENT_SOURCE_DIR}/models", qml_cmake)
        self.assertIn("${MEMOCHAT_QML_CORE_DIR}/rc.qrc", qml_cmake)
        self.assertIn("${MEMOCHAT_QML_CORE_DIR}/config.ini", qml_cmake)
        self.assertNotIn("SHARED_CLIENT_DIR", qml_cmake)
        self.assertNotIn("../MemoChatShared", qml_cmake)

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
