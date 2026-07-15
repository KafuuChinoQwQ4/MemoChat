"""ChatServer account-directory isolation seam contract (finding #3).

Locks in option C of the architecture review: ChatServer must not reach the
account DB / user profile tables through ad-hoc PostgresMgr calls scattered
across transport, clients, and domain free-functions. All live account-data
reads funnel through IAccountDirectory (today backed by ChatAccountDirectory,
which still uses the existing [AccountPostgres] bridge — behavior unchanged).

This freezes the seam so a later swap to a gRPC snapshot API or event-synced
projection only needs a new adapter. Static source scanners only.
"""

from __future__ import annotations

import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CHAT = REPO_ROOT / "apps" / "server" / "core" / "ChatServer"
PORT = CHAT / "domain" / "ports" / "IAccountDirectory.hpp"
ADAPTER_H = CHAT / "persistence" / "ChatAccountDirectory.hpp"
ADAPTER_CPP = CHAT / "persistence" / "ChatAccountDirectory.cpp"
CMAKE = CHAT / "CMakeLists.txt"
DOC = REPO_ROOT / "apps" / "server" / "core" / "docs" / "database-ownership.md"

# Call sites that previously leaked PostgresMgr account reads / direct
# GetUsersByUids enrichment and must now use the seam. Paths relative to ChatServer/.
REROUTED_CALLERS = (
    "persistence/ChatRelationRepository.cpp",
    "domain/users/ChatUserSupport.cpp",
    "transport/ChatServiceImpl.cpp",
    "clients/ChatGrpcClient.cpp",
    "persistence/PostgresDaoGroups.cpp",
    "persistence/PostgresDaoGroupMessages.cpp",
    "persistence/PostgresDaoUsers.cpp",
)

# Direct PostgresMgr account-method patterns that must not appear outside the
# adapter (and the PostgresMgr facade itself).
LEAK_PATTERNS = (
    re.compile(r"PostgresMgr::GetInstance\(\)\s*->\s*GetUser\s*\("),
    re.compile(r"PostgresMgr::GetInstance\(\)\s*->\s*GetUidByUserId\s*\("),
    re.compile(r"PostgresMgr::GetInstance\(\)\s*->\s*GetUsersByUids\s*\("),
)

# Files allowed to still call PostgresMgr account methods (the adapter + facade).
ALLOWED_POSTGRES_MGR_ACCOUNT_FILES = {
    "persistence/ChatAccountDirectory.cpp",
    "persistence/PostgresMgr.cpp",
    "persistence/PostgresMgr.hpp",
    "persistence/PostgresDao.hpp",
    "persistence/PostgresDaoUsers.cpp",
}


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


def rel(path: Path) -> str:
    return path.relative_to(CHAT).as_posix()


class AccountDirectorySeamFilesTest(unittest.TestCase):
    def test_port_and_adapter_exist_with_minimal_surface(self):
        self.assertTrue(PORT.is_file())
        self.assertTrue(ADAPTER_H.is_file())
        self.assertTrue(ADAPTER_CPP.is_file())
        port = read(PORT)
        for method in ("GetByUid", "GetByName", "ResolveUidByPublicId", "GetManyByUids"):
            with self.subTest(method=method):
                self.assertIn(method, port)
        adapter = read(ADAPTER_H)
        self.assertIn("class ChatAccountDirectory", adapter)
        self.assertIn("AccountDirectory()", adapter)
        self.assertIn("IAccountDirectory", adapter)

    def test_adapter_is_compiled(self):
        cmake = read(CMAKE)
        self.assertIn("persistence/ChatAccountDirectory.cpp", cmake)


class AccountDirectoryNoLeaksTest(unittest.TestCase):
    def test_rerouted_callers_use_the_seam(self):
        # PostgresDaoUsers still owns the SQL implementation of GetUsersByUids
        # (the adapter's backend); enrichment call sites inside it must use the
        # seam, but LEAK_PATTERNS only cover PostgresMgr::GetInstance()->...
        for rel_path in REROUTED_CALLERS:
            src = read(CHAT / rel_path)
            with self.subTest(file=rel_path):
                self.assertIn(
                    "AccountDirectory()",
                    src,
                    f"{rel_path} must call AccountDirectory() after the isolation seam.",
                )
                self.assertIn("ChatAccountDirectory.hpp", src)
                for pattern in LEAK_PATTERNS:
                    self.assertIsNone(
                        pattern.search(src),
                        f"{rel_path} still leaks account reads via {pattern.pattern}",
                    )
                # Enrichment must not call the private DAO member directly anymore
                # (except the method definition itself in PostgresDaoUsers.cpp).
                if rel_path != "persistence/PostgresDaoUsers.cpp":
                    self.assertNotRegex(
                        src,
                        r"(?<!::)GetUsersByUids\s*\(",
                        f"{rel_path} must not call GetUsersByUids() directly; use AccountDirectory().GetManyByUids().",
                    )

    def test_no_chatserver_file_outside_adapter_calls_postgresmgr_account_methods(self):
        offenders: list[str] = []
        for path in CHAT.rglob("*"):
            if path.suffix not in {".cpp", ".hpp", ".h"}:
                continue
            if "cxx_modules" in path.parts or path.name.endswith(".cppm"):
                continue
            rel_path = rel(path)
            if rel_path in ALLOWED_POSTGRES_MGR_ACCOUNT_FILES:
                continue
            src = read(path)
            for pattern in LEAK_PATTERNS:
                if pattern.search(src):
                    offenders.append(f"{rel_path} matches {pattern.pattern}")
        self.assertEqual(
            [],
            offenders,
            "ChatServer account-data reads must go through IAccountDirectory / "
            f"AccountDirectory(); leaked PostgresMgr account calls: {offenders}",
        )


class AccountDirectoryDocumentedTest(unittest.TestCase):
    def test_ownership_doc_mentions_the_seam(self):
        self.assertTrue(DOC.is_file())
        doc = read(DOC)
        self.assertIn("IAccountDirectory", doc)
        self.assertIn("ChatAccountDirectory", doc)


if __name__ == "__main__":
    unittest.main()
