import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
FEATURE_VIEW = CLIENT / "features/call/view"
LEGACY_VIEW = CLIENT / "qml/call"
FEATURE_QRC = CLIENT / "features/call/resources/call.qrc"
LEGACY_QRC = CLIENT / "resources/qrc/qml-chat.qrc"
SOURCES = CLIENT / "features/call/sources.cmake"

FEATURE_FILES = ("CallOverlay.qml",)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.exists() else ""


class CallQmlPhysicalLayoutContractTests(unittest.TestCase):
    def test_call_qml_lives_under_feature_view(self):
        for file_name in FEATURE_FILES:
            with self.subTest(file=file_name):
                self.assertTrue((FEATURE_VIEW / file_name).is_file())
                self.assertFalse((LEGACY_VIEW / file_name).exists())

    def test_call_qrc_uses_feature_aliases_only(self):
        self.assertFalse(LEGACY_QRC.exists())
        feature_qrc = read(FEATURE_QRC)
        legacy_qrc = read(LEGACY_QRC)
        for file_name in FEATURE_FILES:
            with self.subTest(file=file_name):
                self.assertIn(f'alias="features/call/view/{file_name}">../view/{file_name}</file>', feature_qrc)
                self.assertNotIn(f'alias="qml/call/{file_name}"', legacy_qrc)

    def test_call_qrc_is_registered_by_feature_manifest(self):
        self.assertIn("features/call/resources/call.qrc", read(SOURCES))


if __name__ == "__main__":
    unittest.main()
