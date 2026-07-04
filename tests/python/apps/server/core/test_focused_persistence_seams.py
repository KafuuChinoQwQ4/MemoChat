"""Focused service persistence seam contracts.

Media, Moments, and Call handlers should cross a local domain persistence
Interface instead of reaching through the wide GateShared Postgres manager.
"""

import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps" / "server" / "core"

FOCUSED_SEAMS = {
    "Moments": {
        "root": SERVER_CORE / "MomentsService" / "domain" / "services" / "moments",
        "handler": SERVER_CORE / "MomentsService" / "domain" / "services" / "moments" / "MomentsService.cpp",
        "handler_header": SERVER_CORE / "MomentsService" / "domain" / "services" / "moments" / "MomentsService.hpp",
        "interface": "MomentsPersistence.hpp",
        "adapter": "MomentsPersistence.cpp",
        "cmake": SERVER_CORE / "MomentsService" / "CMakeLists.txt",
        "tokens": ("class MomentsPersistence", "LoadFeedCandidates", "SetCommentLike", "LoadUserProfile"),
    },
    "Media": {
        "root": SERVER_CORE / "MediaService" / "domain" / "services" / "media",
        "handler": SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaService.cpp",
        "handler_header": SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaService.hpp",
        "interface": "MediaPersistence.hpp",
        "adapter": "MediaPersistence.cpp",
        "cmake": SERVER_CORE / "MediaService" / "CMakeLists.txt",
        "tokens": ("class MediaPersistence", "SaveAsset", "LoadAssetByKey", "MediaAssetRecord"),
    },
    "Call": {
        "root": SERVER_CORE / "CallService" / "core" / "support",
        "handler": SERVER_CORE / "CallService" / "core" / "support" / "CallService.cpp",
        "handler_header": SERVER_CORE / "CallService" / "core" / "support" / "CallService.hpp",
        "interface": "CallPersistence.hpp",
        "adapter": "CallPersistence.cpp",
        "cmake": SERVER_CORE / "CallService" / "CMakeLists.txt",
        "tokens": ("class CallPersistence", "AreUsersMutualFriends", "LoadUserProfiles", "SaveSession"),
    },
}


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


class FocusedPersistenceSeamTests(unittest.TestCase):
    def test_handlers_do_not_reach_through_wide_postgres_manager(self):
        forbidden = ('#include "PostgresMgr.hpp"', "#include <PostgresMgr.hpp>", "PostgresMgr::GetInstance()")
        for name, seam in FOCUSED_SEAMS.items():
            source = read(seam["handler"])
            with self.subTest(service=name):
                for token in forbidden:
                    self.assertNotIn(token, source)

    def test_handler_headers_do_not_expose_wide_postgres_dao(self):
        for name, seam in FOCUSED_SEAMS.items():
            header = read(seam["handler_header"])
            with self.subTest(service=name):
                self.assertNotIn("PostgresDao.hpp", header)
                self.assertNotIn("PostgresMgr.hpp", header)

    def test_local_persistence_interface_and_adapter_are_registered(self):
        for name, seam in FOCUSED_SEAMS.items():
            root = seam["root"]
            interface = read(root / seam["interface"])
            cmake = read(seam["cmake"])
            with self.subTest(service=name):
                self.assertTrue((root / seam["adapter"]).exists())
                for token in seam["tokens"]:
                    self.assertIn(token, interface)
                self.assertIn(seam["interface"], cmake)
                self.assertIn(seam["adapter"], cmake)

    def test_postgres_manager_calls_are_local_to_persistence_adapters(self):
        for name, seam in FOCUSED_SEAMS.items():
            root = seam["root"]
            call_sites = sorted(
                path.relative_to(root).as_posix()
                for path in root.rglob("*")
                if path.is_file() and "PostgresMgr::GetInstance()" in read(path)
            )
            with self.subTest(service=name):
                self.assertEqual([seam["adapter"]], call_sites)


if __name__ == "__main__":
    unittest.main()
