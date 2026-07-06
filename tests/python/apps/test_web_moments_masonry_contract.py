import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
WEB_APP = REPO_ROOT / "apps/web"
MOMENTS_COMPONENT = WEB_APP / "src/features/moments/components/MomentsShellContent.tsx"


def read(path):
    return path.read_text(encoding="utf-8")


class WebMomentsMasonryContractTests(unittest.TestCase):
    def test_moment_mapping_preserves_text_and_media_resources(self):
        source = read(MOMENTS_COMPONENT)
        map_start = source.index("function mapMoment")
        map_end = source.index("function HeartIcon", map_start)
        map_body = source[map_start:map_end]

        self.assertIn("const textParts: string[] = []", map_body)
        self.assertIn("const media: MomentMedia[] = []", map_body)
        self.assertIn('const mediaKey = part.media_key?.trim() ?? ""', map_body)
        self.assertIn('const thumbKey = part.thumb_key?.trim() ?? ""', map_body)
        self.assertIn("const previewKey = thumbKey || mediaKey", map_body)
        self.assertIn("mediaKey: mediaKey || previewKey", map_body)
        self.assertIn("previewKey,", map_body)
        self.assertIn("width: Number(part.width ?? 0)", map_body)
        self.assertIn("height: Number(part.height ?? 0)", map_body)
        self.assertIn("durationMs: Number(part.duration_ms ?? 0)", map_body)
        self.assertIn("return", map_body[map_body.index("media.push") : map_body.index("if (content)")])
        self.assertIn("textParts.push(content)", map_body)
        self.assertIn("media,", map_body)

    def test_feed_cards_preview_long_text_and_media_before_detail_overlay(self):
        source = read(MOMENTS_COMPONENT)

        for token in (
            "const TEXT_PREVIEW_MAX_LINES = 7",
            "const MEDIA_PREVIEW_MAX_HEIGHT = 420",
            "function hasTextPreviewOverflow",
            "function hasMediaPreviewOverflow",
            "function MomentMediaBlocks",
            "maxHeight: full ? undefined : MEDIA_PREVIEW_MAX_HEIGHT",
            "查看全文",
            "查看全部",
            "function MomentDetailOverlay",
            "setSelectedMoment(item)",
            "content: textContent || location",
        ):
            with self.subTest(token=token):
                self.assertIn(token, source)

    def test_feed_keeps_masonry_columns_and_renders_media_urls(self):
        source = read(MOMENTS_COMPONENT)

        self.assertIn('import { resolveMediaUrl } from "@/shared/media/mediaUrl"', source)
        self.assertIn("columnWidth: 236", source)
        self.assertIn('breakInside: "avoid"', source)
        self.assertIn("src={source}", source)
        self.assertIn("resolveMediaUrl(full ? media.mediaKey : media.previewKey)", source)
        self.assertIn("内容未保存，请重新发布", source)


if __name__ == "__main__":
    unittest.main()
