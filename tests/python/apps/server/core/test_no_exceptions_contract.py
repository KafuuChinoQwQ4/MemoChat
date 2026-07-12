from __future__ import annotations

import bisect
import re
import unittest
from collections.abc import Iterator

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SOURCE_ROOTS = (
    REPO_ROOT / "apps/server/core",
    REPO_ROOT / "apps/client/desktop",
    REPO_ROOT / "infra/Memo_ops",
    REPO_ROOT / "tests/cpp",
)
CPP_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl", ".ixx", ".cppm"}
BANNED_TOKENS = {
    "ASSERT_ANY_THROW",
    "ASSERT_NO_THROW",
    "ASSERT_THROW",
    "BOOST_THROW_EXCEPTION",
    "EXPECT_ANY_THROW",
    "EXPECT_NO_THROW",
    "EXPECT_THROW",
    "try",
    "catch",
    "throw",
    "exception_ptr",
    "current_exception",
    "rethrow_exception",
    "nested_exception",
}
BANNED_QUALIFIED_TOKENS = (
    ("std", "async"),
    ("std", "jthread"),
    ("std", "thread"),
    ("boost", "asio", "thread_pool"),
    ("boost", "uuids", "random_generator"),
)


def _skip_quoted(source: str, start: int, quote_index: int) -> int:
    quote = source[quote_index]
    index = quote_index + 1
    while index < len(source):
        if source[index] == "\\":
            index += 2
            continue
        if source[index] == quote:
            return index + 1
        index += 1
    return len(source)


def _raw_literal_end(source: str, start: int) -> int | None:
    for prefix in ('u8R"', 'uR"', 'UR"', 'LR"', 'R"'):
        if not source.startswith(prefix, start):
            continue
        delimiter_start = start + len(prefix)
        open_paren = source.find("(", delimiter_start, delimiter_start + 17)
        if open_paren < 0:
            return None
        delimiter = source[delimiter_start:open_paren]
        if any(character.isspace() or character in "\\()" for character in delimiter):
            return None
        close_marker = ")" + delimiter + '"'
        close = source.find(close_marker, open_paren + 1)
        return len(source) if close < 0 else close + len(close_marker)
    return None


def cpp_tokens(source: str) -> Iterator[tuple[str, int]]:
    index = 0
    while index < len(source):
        if source.startswith("//", index):
            newline = source.find("\n", index + 2)
            index = len(source) if newline < 0 else newline + 1
            continue
        if source.startswith("/*", index):
            close = source.find("*/", index + 2)
            index = len(source) if close < 0 else close + 2
            continue

        raw_end = _raw_literal_end(source, index)
        if raw_end is not None:
            index = raw_end
            continue

        quoted = False
        for prefix in ('u8"', 'u"', 'U"', 'L"', '"', "u8'", "u'", "U'", "L'", "'"):
            if source.startswith(prefix, index):
                index = _skip_quoted(source, index, index + len(prefix) - 1)
                quoted = True
                break
        if quoted:
            continue

        character = source[index]
        if character.isalpha() or character == "_":
            end = index + 1
            while end < len(source) and (source[end].isalnum() or source[end] == "_"):
                end += 1
            yield source[index:end], index
            index = end
            continue
        if character in "()":
            yield character, index
        index += 1


def line_and_column(source: str, offset: int) -> tuple[int, int]:
    newlines = [-1]
    newlines.extend(index for index, character in enumerate(source) if character == "\n")
    line = bisect.bisect_left(newlines, offset)
    return line, offset - newlines[line - 1]


class NoExceptionsContractTests(unittest.TestCase):
    def test_project_cpp_has_no_exception_tokens(self):
        violations: list[str] = []
        for root in SOURCE_ROOTS:
            for path in sorted(candidate for candidate in root.rglob("*") if candidate.suffix.lower() in CPP_SUFFIXES):
                source = path.read_text(encoding="utf-8-sig", errors="replace")
                tokens = list(cpp_tokens(source))
                token_names = [item[0] for item in tokens]
                for token_index, (token, offset) in enumerate(tokens):
                    banned = token in BANNED_TOKENS
                    disabled_noexcept = token == "noexcept" and [
                        item[0] for item in tokens[token_index + 1 : token_index + 4]
                    ] == ["(", "false", ")"]
                    if not banned and not disabled_noexcept:
                        continue
                    line, column = line_and_column(source, offset)
                    relative = path.relative_to(REPO_ROOT)
                    violations.append(f"{relative}:{line}:{column}: {token}")

                for qualified in BANNED_QUALIFIED_TOKENS:
                    width = len(qualified)
                    for token_index in range(len(tokens) - width + 1):
                        if tuple(token_names[token_index : token_index + width]) != qualified:
                            continue
                        _, offset = tokens[token_index]
                        line, column = line_and_column(source, offset)
                        relative = path.relative_to(REPO_ROOT)
                        violations.append(f"{relative}:{line}:{column}: {'::'.join(qualified)}")

        self.assertEqual([], violations, "C++ exception syntax is forbidden:\n" + "\n".join(violations))

    def test_build_and_dependencies_enforce_no_exceptions(self):
        root_cmake = (REPO_ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        server_cmake = (REPO_ROOT / "apps/server/core/CMakeLists.txt").read_text(encoding="utf-8")
        presets = (REPO_ROOT / "CMakePresets.json").read_text(encoding="utf-8")
        manifest = (REPO_ROOT / "vcpkg.json").read_text(encoding="utf-8")

        self.assertIn("option(MEMOCHAT_NO_EXCEPTIONS", root_cmake)
        self.assertIn("-fno-exceptions", root_cmake)
        self.assertIn("/EHs-c-", root_cmake)
        self.assertIn("BOOST_NO_EXCEPTIONS", root_cmake)
        self.assertIn("BOOST_EXCEPTION_DISABLE", root_cmake)
        self.assertIn("FMT_EXCEPTIONS=0", root_cmake)
        self.assertIn("SPDLOG_NO_EXCEPTIONS", root_cmake)
        self.assertIn('"MEMOCHAT_NO_EXCEPTIONS": "ON"', presets)
        self.assertNotIn("/EHsc", server_cmake)
        self.assertNotIn("find_package(libpqxx", server_cmake)
        self.assertNotIn("find_package(mongocxx", server_cmake)
        self.assertNotIn("find_package(cppkafka", server_cmake)
        self.assertNotIn("find_package(stdexec", server_cmake)
        self.assertNotIn('"cppkafka"', manifest)
        self.assertNotIn('"libpqxx"', manifest)
        self.assertNotIn('"mongo-cxx-driver"', manifest)
        self.assertNotIn('"stdexec"', manifest)

    def test_postgres_adapter_keeps_success_status_after_commit(self):
        adapter = (REPO_ROOT / "apps/server/core/common/db/PqxxCompat.hpp").read_text(encoding="utf-8")
        transaction = adapter.split("class transaction", maxsplit=1)[1]
        transaction_ok = re.search(
            r"bool ok\(\) const noexcept\s*\{(?P<body>.*?)\n\s*\}",
            transaction,
            flags=re.S,
        )

        self.assertIsNotNone(transaction_ok)
        self.assertIn("state_->ok", transaction_ok.group("body"))
        self.assertNotIn("active_", transaction_ok.group("body"))
        self.assertGreaterEqual(adapter.count('state_->Fail("PostgreSQL transaction is not active")'), 3)

    def test_mongo_runtime_pairs_global_initialization_and_cleanup(self):
        adapter = (REPO_ROOT / "apps/server/core/common/db/MongoCCompat.hpp").read_text(encoding="utf-8")

        self.assertEqual(adapter.count("mongoc_init();"), 1)
        self.assertEqual(adapter.count("mongoc_cleanup();"), 1)

    def test_chat_entrypoints_reject_unready_postgres(self):
        chat_app = REPO_ROOT / "apps/server/core/ChatServer/app"
        for source_name in (
            "ChatServer.cpp",
            "ChatDeliveryWorker.cpp",
            "ChatMessageService.cpp",
            "ChatRelationQueryService.cpp",
            "ChatRelationServiceWorker.cpp",
        ):
            source = (chat_app / source_name).read_text(encoding="utf-8")
            self.assertIn("postgres_mgr->Ready()", source, source_name)
            self.assertIn("postgres_mgr->startupError()", source, source_name)

    def test_gate_entrypoints_reject_unready_io_pool(self):
        wrapper = (REPO_ROOT / "apps/server/core/GateShared/core/runtime/AsioIOServicePool.hpp").read_text(
            encoding="utf-8"
        )
        implementation = (REPO_ROOT / "apps/server/core/GateShared/core/runtime/AsioIOServicePool.cpp").read_text(
            encoding="utf-8"
        )

        self.assertIn("bool Ready() const noexcept", wrapper)
        self.assertIn("startupError() const noexcept", wrapper)
        self.assertIn("return _pool->Ready();", implementation)
        self.assertIn("return _pool->startupError();", implementation)

        for relative in (
            "apps/server/core/GateShared/app/GateDomainServer.cpp",
            "apps/server/core/AIGatewayService/app/AIGatewayServer.cpp",
        ):
            source = (REPO_ROOT / relative).read_text(encoding="utf-8")
            self.assertIn("auto io_pool = AsioIOServicePool::GetInstance();", source, relative)
            self.assertIn("io_pool->Ready()", source, relative)
            self.assertIn("io_pool->startupError()", source, relative)
            self.assertLess(source.index("io_pool->Ready()"), source.index("server->Open("), relative)

    def test_media_gateway_validates_only_the_selected_storage_before_listening(self):
        storage_header = (REPO_ROOT / "apps/server/core/MediaService/core/storage/MediaStorage.hpp").read_text(
            encoding="utf-8"
        )
        storage_source = (REPO_ROOT / "apps/server/core/MediaService/core/storage/MediaStorage.cpp").read_text(
            encoding="utf-8"
        )
        s3_header = (REPO_ROOT / "apps/server/core/MediaService/core/storage/S3MediaStorage.hpp").read_text(
            encoding="utf-8"
        )
        media_entrypoint = (REPO_ROOT / "apps/server/core/MediaService/app/MediaGatewayServer.cpp").read_text(
            encoding="utf-8"
        )
        gate_domain = (REPO_ROOT / "apps/server/core/GateShared/app/GateDomainServer.cpp").read_text(encoding="utf-8")

        self.assertIn("virtual bool Ready() const noexcept = 0", storage_header)
        self.assertIn("virtual const std::string& StartupError() const noexcept = 0", storage_header)
        self.assertIn("bool InitializeMediaStorage(std::string* error", storage_header)
        self.assertIn("bool Ready() const noexcept override", s3_header)
        s3_source = (REPO_ROOT / "apps/server/core/MediaService/core/storage/S3MediaStorage.cpp").read_text(
            encoding="utf-8"
        )
        self.assertIn("HeadBucket(request)", s3_source)
        self.assertIn("std::make_unique<LocalMediaStorage>()", storage_source)
        self.assertIn("std::make_unique<S3MediaStorage>()", storage_source)
        self.assertNotIn("static LocalMediaStorage local_instance", storage_source)
        self.assertNotIn("static S3MediaStorage s3_instance", storage_source)
        self.assertIn("return InitializeMediaStorage(error);", media_entrypoint)
        self.assertIn("ShutdownMediaStorage();", media_entrypoint)
        self.assertLess(gate_domain.index("if (on_start)"), gate_domain.index("server->Open("))

    def test_logger_io_never_uses_spdlog_exception_sinks(self):
        logger_header = (REPO_ROOT / "apps/server/core/common/logging/Logger.hpp").read_text(encoding="utf-8")
        logger_source = (REPO_ROOT / "apps/server/core/common/logging/Logger.cpp").read_text(encoding="utf-8")

        self.assertIn("static bool Init(", logger_header)
        self.assertIn("class NonThrowingJsonSink", logger_source)
        self.assertIn("std::fopen(", logger_source)
        self.assertIn("std::fwrite(", logger_source)
        self.assertIn("std::fflush(", logger_source)
        self.assertIn("using stderr fallback", logger_source)
        self.assertNotIn("daily_file_sink", logger_source)
        self.assertNotIn("rotating_file_sink", logger_source)
        self.assertNotIn("stdout_color_sink", logger_source)
        self.assertNotIn("register_logger", logger_source)

        for relative in (
            "apps/server/core/GateShared/app/GateDomainServer.cpp",
            "apps/server/core/AIGatewayService/app/AIGatewayServer.cpp",
            "apps/server/core/AIServer/AIServer.cpp",
            "apps/server/core/ChatServer/app/ChatDeliveryWorker.cpp",
            "apps/server/core/ChatServer/app/ChatMessageService.cpp",
            "apps/server/core/ChatServer/app/ChatRelationQueryService.cpp",
            "apps/server/core/ChatServer/app/ChatRelationServiceWorker.cpp",
            "apps/server/core/ChatServer/app/ChatServer.cpp",
            "apps/server/core/VarifyServer/VarifyServer.cpp",
        ):
            source = (REPO_ROOT / relative).read_text(encoding="utf-8")
            self.assertIn("if (!memolog::Logger::Init(", source, relative)

    def test_postgres_startup_validates_schema_and_serializes_ddl(self):
        persistence = REPO_ROOT / "apps/server/core/ChatServer/persistence"
        dao = (persistence / "PostgresDao.cpp").read_text(encoding="utf-8")
        dialogs = (persistence / "PostgresDaoDialogs.cpp").read_text(encoding="utf-8")

        self.assertIn(r"FROM \"user\" WHERE FALSE", dao)
        self.assertIn("SELECT id FROM user_id WHERE FALSE", dao)
        self.assertLess(dao.index("EnsureChatMessageIdempotencySchema()"), dao.index("ready_ = true"))
        self.assertLess(dao.index("EnsureChatEventOutboxSchema()"), dao.index("ready_ = true"))
        self.assertLess(dao.index("WarmupRelationBootstrapQueries()"), dao.index("ready_ = true"))
        self.assertEqual(dialogs.count("SELECT pg_advisory_xact_lock($1)"), 2)


if __name__ == "__main__":
    unittest.main()
