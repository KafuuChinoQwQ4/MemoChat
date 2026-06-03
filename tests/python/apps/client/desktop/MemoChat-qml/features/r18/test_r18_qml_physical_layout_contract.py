import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
FEATURE_VIEW = CLIENT / "features/r18/view"
LEGACY_VIEW = CLIENT / "qml/r18"
FEATURE_QRC = CLIENT / "features/r18/resources/r18.qrc"
LEGACY_QRC = CLIENT / "resources/qrc/qml-r18.qrc"
SOURCES = CLIENT / "features/r18/sources.cmake"

FEATURE_FILES = (
    "R18ComicTile.qml",
    "R18DetailPane.qml",
    "R18FlipFace.qml",
    "R18HistoryPane.qml",
    "R18HomeCard.qml",
    "R18HomePane.qml",
    "R18ImportedSourceRow.qml",
    "R18OfficialSourceCatalogPane.qml",
    "R18OfficialSourceRow.qml",
    "R18ReaderChrome.qml",
    "R18ReaderPane.qml",
    "R18SearchPane.qml",
    "R18ShellPane.qml",
    "R18ShellRuntime.js",
    "R18SourceFeedPane.qml",
    "R18SourceImportPane.qml",
    "R18SourceListPane.qml",
    "R18SourceManagerPane.qml",
    "R18StatusBanner.qml",
    "R18TagPane.qml",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.exists() else ""


class R18QmlPhysicalLayoutContractTests(unittest.TestCase):
    def test_r18_qml_lives_under_feature_view(self):
        for file_name in FEATURE_FILES:
            with self.subTest(file=file_name):
                self.assertTrue((FEATURE_VIEW / file_name).is_file())
                self.assertFalse((LEGACY_VIEW / file_name).exists())

    def test_r18_qrc_uses_feature_aliases_only(self):
        self.assertFalse(LEGACY_QRC.exists())
        feature_qrc = read(FEATURE_QRC)
        legacy_qrc = read(LEGACY_QRC)
        for file_name in FEATURE_FILES:
            with self.subTest(file=file_name):
                self.assertIn(f'alias="features/r18/view/{file_name}">../view/{file_name}</file>', feature_qrc)
                self.assertNotIn(f'alias="qml/r18/{file_name}"', legacy_qrc)

    def test_r18_qrc_is_registered_by_feature_manifest(self):
        self.assertIn("features/r18/resources/r18.qrc", read(SOURCES))


if __name__ == "__main__":
    unittest.main()
