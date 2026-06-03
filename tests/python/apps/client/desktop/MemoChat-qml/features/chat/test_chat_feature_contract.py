import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
CHAT = CLIENT / "features/chat"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class ChatFeatureContractTests(unittest.TestCase):
    def test_chat_feature_has_viewmodel_state_and_resources(self):
        expected = (
            CHAT / "viewmodel/ChatViewModel.h",
            CHAT / "viewmodel/ChatViewModel.cpp",
            CHAT / "model/ChatUiState.h",
            CHAT / "resources/chat.qrc",
        )
        for path in expected:
            with self.subTest(path=path):
                self.assertTrue(path.is_file())

    def test_feature_manifest_lists_chat_viewmodel_state_and_resources(self):
        manifest = read(CHAT / "sources.cmake")
        expected_tokens = (
            "features/chat/viewmodel/ChatViewModel.cpp",
            "features/chat/viewmodel/ChatViewModel.h",
            "features/chat/model/ChatUiState.h",
            "features/chat/resources/chat.qrc",
        )
        for token in expected_tokens:
            with self.subTest(token=token):
                self.assertIn(token, manifest)

    def test_root_cmake_adds_chat_viewmodel_include_path(self):
        cmake = read(CLIENT / "CMakeLists.txt")
        self.assertIn("${MEMOCHAT_QML_FEATURE_DIR}/chat/viewmodel", cmake)


if __name__ == "__main__":
    unittest.main()
