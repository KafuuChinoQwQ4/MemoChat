"""Architecture review slice d — remediation contract tests.

Locks in the fixes applied in response to
`.ai/backend-microservice-architecture-review/architecture-review-20260617.html`.

Each test asserts the *positive* invariant of a fix, so it fails if the fix is
reverted. These are static source/config scanners (no build, no live service),
matching the other structure/contract tests under this tree.

Findings covered: (1) Telemetry init parity, (2) media download defense-in-depth,
(3) R18SourceService relocation, (4) shared user-token validator, (6) HmacSecret
env override + dev-secret guard, (7) GroupMessageService response-formatter
extraction, (8) ChatServer DAO/message helper de-duplication.
"""

import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps" / "server" / "core"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class TelemetryInitParityTest(unittest.TestCase):
    """(1) Every Gate-family focused service bootstraps through GateDomainServer;
    it must initialize Telemetry, not only Logger (the original blind spot)."""

    BOOTSTRAP = SERVER_CORE / "GateShared" / "app" / "GateDomainServer.cpp"

    def test_bootstrap_initializes_logger_and_telemetry(self):
        src = read(self.BOOTSTRAP)
        self.assertIn("Logger::Init", src)
        self.assertIn(
            "Telemetry::Init",
            src,
            "GateDomainServer must call Telemetry::Init so the 7 focused services "
            "(Media/Moments/Call/R18/Register/Login/Account) emit traces/metrics.",
        )

    def test_bootstrap_shuts_telemetry_down(self):
        src = read(self.BOOTSTRAP)
        self.assertIn("Telemetry::Shutdown", src)
        # Telemetry shutdown must precede logger shutdown, mirroring ChatServer.
        self.assertLess(
            src.index("Telemetry::Shutdown"),
            src.rindex("Logger::Shutdown"),
            "Telemetry::Shutdown should run before Logger::Shutdown.",
        )


class MediaDownloadOwnershipTest(unittest.TestCase):
    """(2) Cross-owner downloads are audited but NOT blocked (chat/moments share
    media; there is no share table, key is an unguessable UUIDv4 capability)."""

    MEDIA = SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaService.cpp"
    DOC = SERVER_CORE / "docs" / "media-access-control.md"

    def test_download_audits_cross_owner_access(self):
        src = read(self.MEDIA)
        self.assertIn("HandleMediaDownload", src)
        self.assertIn("owner_uid", src)
        self.assertIn(
            "media.download.cross_owner",
            src,
            "Owner mismatch on download must emit the cross_owner audit log.",
        )

    def test_download_does_not_hard_reject_on_owner_mismatch(self):
        """Guard against a regression to naive owner-only 403 that would break
        receiving images. The owner-mismatch branch must not return an error."""
        src = read(self.MEDIA)
        match = re.search(r"if \(asset\.owner_uid != uid\)\s*\{(.*?)\}", src, re.DOTALL)
        self.assertIsNotNone(match, "Expected the owner-mismatch audit branch.")
        branch = match.group(1)
        self.assertIn("media.download.cross_owner", branch)
        self.assertNotIn("return", branch, "Owner mismatch must not block the download.")

    def test_threat_model_documented(self):
        self.assertTrue(self.DOC.is_file(), "docs/media-access-control.md must exist.")
        doc = read(self.DOC)
        self.assertIn("UUIDv4", doc)
        self.assertIn("owner_uid", doc)


class R18SourceRelocationTest(unittest.TestCase):
    """(3) R18SourceService is domain code; it moved out of common/ into R18Service."""

    OLD_DIR = SERVER_CORE / "common" / "r18"
    NEW_CPP = SERVER_CORE / "R18Service" / "domain" / "services" / "r18" / "R18SourceService.cpp"
    NEW_H = SERVER_CORE / "R18Service" / "domain" / "services" / "r18" / "R18SourceService.h"
    R18_CMAKE = SERVER_CORE / "R18Service" / "CMakeLists.txt"

    def test_old_common_location_removed(self):
        self.assertFalse(
            (self.OLD_DIR / "R18SourceService.cpp").exists(),
            "R18SourceService.cpp must no longer live under common/r18.",
        )
        self.assertFalse((self.OLD_DIR / "R18SourceService.h").exists())

    def test_new_service_location_present(self):
        self.assertTrue(self.NEW_CPP.is_file())
        self.assertTrue(self.NEW_H.is_file())

    def test_cmake_uses_local_source(self):
        cmake = read(self.R18_CMAKE)
        self.assertIn("domain/services/r18/R18SourceService.cpp", cmake)

    def test_no_cmake_compiles_from_common_r18(self):
        for cmake in SERVER_CORE.rglob("CMakeLists.txt"):
            for line in read(cmake).splitlines():
                stripped = line.strip()
                if stripped.startswith("#"):
                    continue  # comments may still mention the old path
                self.assertNotIn(
                    "common/r18/R18SourceService",
                    stripped,
                    f"{cmake} still compiles R18SourceService from common/r18.",
                )


class SharedUserTokenValidatorTest(unittest.TestCase):
    """(4) The utoken_<uid> Redis validation lives in ONE shared validator;
    services delegate instead of each re-implementing it."""

    VALIDATOR_H = SERVER_CORE / "GateShared" / "core" / "support" / "UserTokenValidator.h"
    VALIDATOR_CPP = SERVER_CORE / "GateShared" / "core" / "support" / "UserTokenValidator.cpp"
    INFRA_CMAKE = SERVER_CORE / "GateShared" / "core" / "CMakeLists.txt"

    DELEGATORS = [
        SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaService.cpp",
        SERVER_CORE / "MediaService" / "core" / "support" / "Http2MediaSupport.cpp",
        SERVER_CORE / "R18Service" / "domain" / "services" / "r18" / "R18Service.cpp",
    ]

    def test_validator_exists_with_contract(self):
        self.assertTrue(self.VALIDATOR_H.is_file())
        self.assertTrue(self.VALIDATOR_CPP.is_file())
        header = read(self.VALIDATOR_H)
        self.assertIn("namespace memochat::auth", header)
        self.assertIn("ValidateUserToken", header)

    def test_validator_built_in_infra_core(self):
        cmake = read(self.INFRA_CMAKE)
        self.assertIn("support/UserTokenValidator.cpp", cmake)

    def test_services_delegate_to_shared_validator(self):
        for path in self.DELEGATORS:
            src = read(path)
            self.assertIn(
                "memochat::auth::ValidateUserToken",
                src,
                f"{path.name} must delegate to the shared validator, not re-implement it.",
            )


class HmacSecretManagementTest(unittest.TestCase):
    """(6) Dev HMAC secret is overridable by env and flagged at startup; every ini
    shipping the dev secret documents the override so new services can't ship it silently."""

    AUTH_SECRET_H = SERVER_CORE / "common" / "auth" / "AuthSecret.h"
    SIGN_SITE = SERVER_CORE / "AccountShared" / "core" / "support" / "AuthLoginSupport.cpp"
    DOC = SERVER_CORE / "docs" / "secret-management.md"
    DEV_SECRET = "memochat-dev-chat-secret"
    ENV_VAR = "MEMOCHAT_CHATAUTH_HMACSECRET"

    def test_dev_secret_marker_header(self):
        self.assertTrue(self.AUTH_SECRET_H.is_file())
        header = read(self.AUTH_SECRET_H)
        self.assertIn("IsWellKnownDevHmacSecret", header)
        self.assertIn("kWellKnownDevHmacSecret", header)
        self.assertIn(self.DEV_SECRET, header)

    def test_startup_guard_present(self):
        self.assertIn("IsWellKnownDevHmacSecret", read(self.SIGN_SITE))

    def test_doc_present(self):
        self.assertTrue(self.DOC.is_file())
        self.assertIn(self.ENV_VAR, read(self.DOC))

    def test_every_dev_secret_ini_documents_override(self):
        offenders = []
        for ini in SERVER_CORE.rglob("*.ini"):
            text = read(ini)
            if self.DEV_SECRET not in text:
                continue
            if self.ENV_VAR not in text:
                offenders.append(str(ini.relative_to(REPO_ROOT)))
        self.assertEqual(
            [],
            offenders,
            f"These ini ship the dev HMAC secret without documenting {self.ENV_VAR}: {offenders}",
        )


class GroupResponseFormatterTest(unittest.TestCase):
    """(7) Response/JSON building extracted from the 1758-line GroupMessageService;
    the public IGroupMessageService surface is preserved."""

    FORMATTER_H = SERVER_CORE / "ChatServer" / "domain" / "message" / "GroupResponseFormatter.h"
    FORMATTER_CPP = SERVER_CORE / "ChatServer" / "domain" / "message" / "GroupResponseFormatter.cpp"
    SERVICE_H = SERVER_CORE / "ChatServer" / "domain" / "message" / "GroupMessageService.h"
    CHAT_CMAKE = SERVER_CORE / "ChatServer" / "CMakeLists.txt"

    def test_formatter_files_exist(self):
        self.assertTrue(self.FORMATTER_H.is_file())
        self.assertTrue(self.FORMATTER_CPP.is_file())

    def test_formatter_compiled(self):
        self.assertIn("GroupResponseFormatter.cpp", read(self.CHAT_CMAKE))

    def test_public_interface_preserved(self):
        header = read(self.SERVICE_H)
        for method in ("CreateGroup", "GroupChatMessage", "GroupHistory", "DissolveGroup"):
            self.assertIn(method, header, f"{method} must remain on GroupMessageService.")


class ChatServerHelperDedupTest(unittest.TestCase):
    """(8) Duplicated DAO/message helpers consolidated into shared headers."""

    DAO_UTIL = SERVER_CORE / "ChatServer" / "persistence" / "PostgresDaoUtil.h"
    MSG_UTIL = SERVER_CORE / "ChatServer" / "domain" / "message" / "MessageServiceUtil.h"
    DAO_GROUPS = SERVER_CORE / "ChatServer" / "persistence" / "PostgresDaoGroups.cpp"
    DAO_DIALOGS = SERVER_CORE / "ChatServer" / "persistence" / "PostgresDaoDialogs.cpp"

    BUILD_PREVIEW_DEF = re.compile(r"std::string BuildPreviewText\(const std::string")

    def test_dao_util_defines_shared_helpers(self):
        self.assertTrue(self.DAO_UTIL.is_file())
        util = read(self.DAO_UTIL)
        self.assertIn("BuildPreviewText", util)
        self.assertIn("kOwnerPermBits", util)

    def test_message_util_exists(self):
        self.assertTrue(self.MSG_UTIL.is_file())

    def test_build_preview_no_longer_duplicated_in_cpp(self):
        for path in (self.DAO_GROUPS, self.DAO_DIALOGS):
            src = read(path)
            self.assertIsNone(
                self.BUILD_PREVIEW_DEF.search(src),
                f"{path.name} must use the shared BuildPreviewText, not define its own.",
            )
            self.assertIn("PostgresDaoUtil.h", src)


if __name__ == "__main__":
    unittest.main()
