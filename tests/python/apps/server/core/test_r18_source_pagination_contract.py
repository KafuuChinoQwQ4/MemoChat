import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
R18_SERVICE_DIR = REPO_ROOT / "apps/server/core/R18Service/domain/services/r18"
R18_CLIENT_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/r18"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class R18SourcePaginationContractTests(unittest.TestCase):
    def test_jm_search_keeps_full_pages_pageable(self):
        source = read(R18_SERVICE_DIR / "R18JmAdapter.cpp")
        search_body = source.split("json::JsonValue JmSearch", 1)[1].split("json::JsonValue JmDetail", 1)[0]

        self.assertIn("constexpr int kR18SearchPageSize = 40;", source)
        self.assertIn("returned_count", search_body)
        self.assertIn("if (returned_count >= kR18SearchPageSize)", search_body)
        self.assertIn('data["max_page"] = static_cast<int64_t>(normalized_page + 1);', search_body)
        self.assertIn('data["max_page"] = static_cast<int64_t>(normalized_page);', search_body)
        self.assertIsNone(
            re.search(r'FieldInt\s*\(\s*result\s*,\s*"total"', search_body),
            "JM max_page must not depend on the upstream total field; it can describe the current page only.",
        )

    def test_r18_qml_source_feed_uses_incremental_search_pages(self):
        shell = read(R18_CLIENT_DIR / "view/R18ShellPane.qml")
        responses = read(R18_CLIENT_DIR / "controller/R18ControllerResponses.cpp")

        self.assertIn("function maybeLoadMoreComics(gridView)", shell)
        self.assertIn("!root.r18Controller.currentSearchHasMore", shell)
        self.assertIn("root.r18Controller.currentSearchPage + 1", shell)
        self.assertIn("root.sourceFeedKeyword", shell)
        self.assertIn("_comics.appendItems(items)", responses)
        self.assertIn('const int maxPage = data.value(QStringLiteral("max_page")).toInt();', responses)
        self.assertIn("const bool hasMore = maxPage > 0 ? page < maxPage : items.size() >= 40;", responses)

    def test_jm_cover_fetch_waits_instead_of_returning_queued_placeholder(self):
        source = read(R18_SERVICE_DIR / "R18JmAdapter.cpp")
        fetch_body = source.split("R18ImagePayload JmFetchImage", 1)[1].rsplit(
            "} // namespace memochat::r18",
            1,
        )[0]

        self.assertIn("#include <condition_variable>", source)
        self.assertIn("std::condition_variable& JmImageFetchCv()", source)
        self.assertIn("JmImageFetchCv().wait", source)
        self.assertIn("JmImageFetchCv().notify_one()", source)
        self.assertIn("constexpr int kMaxConcurrentJmImageFetches = 8;", source)
        self.assertNotIn("JMComic cover queued", fetch_body)
        self.assertNotIn("slot.acquired()", fetch_body)
        self.assertGreaterEqual(
            fetch_body.count("detail::ReadCachedImage(cache_root, cache_key, &cached)"),
            2,
            "JM fetch should re-check cache after waiting for a fetch slot.",
        )

    def test_source_enable_and_delete_trim_source_id_before_lookup(self):
        source = read(R18_SERVICE_DIR / "R18SourceService.cpp")
        enable_body = source.split("bool R18SourceService::EnableSource", 1)[1].split(
            "bool R18SourceService::DeleteSource",
            1,
        )[0]
        delete_body = source.split("bool R18SourceService::DeleteSource", 1)[1].split(
            "R18SourceRecord R18SourceService::ImportZip",
            1,
        )[0]

        self.assertIn("std::string TrimAscii(std::string value)", source)
        self.assertIn("const std::string normalized_id = TrimAscii(id);", enable_body)
        self.assertIn("auto it = sources_.find(normalized_id);", enable_body)
        self.assertIn("const std::string normalized_id = TrimAscii(id);", delete_body)
        self.assertIn("if (IsBuiltinSourceId(normalized_id))", delete_body)
        self.assertIn("auto it = sources_.find(normalized_id);", delete_body)


if __name__ == "__main__":
    unittest.main()
