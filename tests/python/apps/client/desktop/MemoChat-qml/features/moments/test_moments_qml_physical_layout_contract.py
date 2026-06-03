import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
FEATURE_VIEW = CLIENT / "features/moments/view"
LEGACY_VIEW = CLIENT / "qml/moments"
FEATURE_QRC = CLIENT / "features/moments/resources/moments.qrc"
LEGACY_QRC = CLIENT / "resources/qrc/qml-moments.qrc"
SOURCES = CLIENT / "features/moments/sources.cmake"

FEATURE_FILES = (
    "MomentsActionBar.qml",
    "MomentsAttachmentStrip.qml",
    "MomentsCommentComposer.qml",
    "MomentsCommentList.qml",
    "MomentsDelegate.qml",
    "MomentsDetailAuthorRow.qml",
    "MomentsDetailHeader.qml",
    "MomentsDetailMediaGrid.qml",
    "MomentsDetailPopup.qml",
    "MomentsFeedPane.qml",
    "MomentsFriendSidePane.qml",
    "MomentsHeader.qml",
    "MomentsImagePreviewPopup.qml",
    "MomentsPostComposer.qml",
    "MomentsPublishHeader.qml",
    "MomentsPublishPage.qml",
    "MomentsVisibilitySelector.qml",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.exists() else ""


class MomentsQmlPhysicalLayoutContractTests(unittest.TestCase):
    def test_moments_qml_lives_under_feature_view(self):
        for file_name in FEATURE_FILES:
            with self.subTest(file=file_name):
                self.assertTrue((FEATURE_VIEW / file_name).is_file())
                self.assertFalse((LEGACY_VIEW / file_name).exists())

    def test_moments_qrc_uses_feature_aliases_only(self):
        self.assertFalse(LEGACY_QRC.exists())
        feature_qrc = read(FEATURE_QRC)
        legacy_qrc = read(LEGACY_QRC)
        for file_name in FEATURE_FILES:
            with self.subTest(file=file_name):
                self.assertIn(f'alias="features/moments/view/{file_name}">../view/{file_name}</file>', feature_qrc)
                self.assertNotIn(f'alias="qml/moments/{file_name}"', legacy_qrc)

    def test_moments_qrc_is_registered_by_feature_manifest(self):
        self.assertIn("features/moments/resources/moments.qrc", read(SOURCES))


if __name__ == "__main__":
    unittest.main()
