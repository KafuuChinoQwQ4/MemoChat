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

    def test_feed_delegate_media_blocks_contribute_to_card_height(self):
        delegate = read(FEATURE_VIEW / "MomentsDelegate.qml")
        media_column_start = delegate.index("id: mediaColumn")
        action_bar_start = delegate.index("MomentsActionBar", media_column_start)
        media_column_block = delegate[media_column_start:action_bar_start]

        self.assertIn("clip: true", delegate[delegate.index("id: root") : delegate.index("implicitHeight:")])
        self.assertIn("Math.max(contentLayout.implicitHeight, contentLayout.childrenRect.height) + 24", delegate)
        self.assertIn("Layout.preferredHeight: root.mediaContentHeight(root.items)", media_column_block)
        self.assertIn("Layout.minimumHeight: root.mediaContentHeight(root.items)", media_column_block)
        self.assertIn("visible: root.hasMediaContent", media_column_block)
        self.assertIn("implicitHeight: root.mediaContentHeight(root.items)", media_column_block)
        self.assertNotIn("height: visible ? root.mediaContentHeight(root.items) : 0", media_column_block)
        self.assertIn("id: mediaStack", media_column_block)
        self.assertIn("width: mediaStack.width", media_column_block)
        self.assertIn("Repeater {", media_column_block)
        self.assertIn("implicitHeight: root.mediaItemHeight(blockRoot.modelData)", media_column_block)
        self.assertIn("height: visible ? root.mediaItemHeight(blockRoot.modelData) : 0", media_column_block)
        self.assertNotIn("Layout.preferredHeight: blockColumn.implicitHeight", media_column_block)
        self.assertNotIn("blockColumn.implicitHeight", media_column_block)
        self.assertIn("function mediaItemHeight(item)", delegate)
        self.assertIn("function mediaContentHeight(items)", delegate)


if __name__ == "__main__":
    unittest.main()
