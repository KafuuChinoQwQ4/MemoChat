import ast
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps" / "server" / "core"
GATE_SERVER = SERVER_CORE / "GateServer"
GATE_CORE = GATE_SERVER / "core"
GATE_CORE_CACHE = GATE_CORE / "cache"
GATE_CORE_CLIENTS = GATE_CORE / "clients"
GATE_CORE_CONFIG = GATE_CORE / "config"
GATE_CORE_PERSISTENCE = GATE_CORE / "persistence"
GATE_CORE_SUPPORT = GATE_CORE / "support"
GATE_H3 = GATE_SERVER / "transports" / "h3"
GATE_H3_LISTENER = GATE_H3 / "listener"
GATE_H3_LEGACY_ROUTES = GATE_H3 / "legacy_routes"
GATE_H2 = GATE_SERVER / "transports" / "h2"
GATE_H2_ROUTES = GATE_H2 / "routes"
GATE_H2_HANDLERS = GATE_H2 / "handlers"
GATE_H2_SUPPORT = GATE_H2 / "support"
GATE_H1_LEGACY = GATE_SERVER / "transports" / "h1" / "legacy_standalone"
GATE_R18_PLUGINS = GATE_SERVER / "plugins" / "r18"
LEGACY_GATE_HTTP1_OPTION = "MEMOCHAT_BUILD_LEGACY_GATE_HTTP1_STANDALONE"
LEGACY_GATE_HTTP2_OPTION = "MEMOCHAT_BUILD_LEGACY_GATE_HTTP2_STANDALONE"
EMBEDDED_GATE_HTTP2_OPTION = "MEMOCHAT_BUILD_EMBEDDED_GATE_HTTP2"
EMBEDDED_GATE_HTTP2_SOURCES = (
    "NgHttp2Server.cpp",
    "transports/h2/adapters/h2/H2RouteAdapter.cpp",
    "transports/h2/routes/Http2Routes.cpp",
    "transports/h2/handlers/Http2Handlers.cpp",
    "transports/h2/handlers/Http2AuthHandlers.cpp",
    "transports/h2/support/Http2AuthSupport.cpp",
    "transports/h2/handlers/Http2CallHandlers.cpp",
    "transports/h2/support/Http2CallSupport.cpp",
    "transports/h2/handlers/Http2MediaHandlers.cpp",
    "transports/h2/handlers/Http2ProfileHandlers.cpp",
    "transports/h2/support/Http2MomentsSupport.cpp",
    "transports/h2/support/Http2MediaSupport.h",
    "transports/h2/support/Http2ProfileSupport.h",
)
H5_STALE_GATE_ARTIFACTS = (
    "message.proto",
    "message.pb.cc",
    "message.pb.h",
    "message.grpc.pb.cc",
    "message.grpc.pb.h",
    "GateServer.sln",
    "GateServer.vcxproj",
    "GateServer.vcxproj.filters",
    "GateServer.vcxproj.user",
    "mysqlcppconn-9-vs14.dll",
    "mysqlcppconn8-2-vs14.dll",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


def strip_comments(source: str) -> str:
    source = re.sub(r"//.*", "", source)
    return re.sub(r"/\*.*?\*/", "", source, flags=re.S)


def strip_cmake_comments(source: str) -> str:
    return "\n".join(re.sub(r"#.*", "", line) for line in source.splitlines())


def normalize_space(source: str) -> str:
    return re.sub(r"\s+", " ", source)


def extract_cmake_if_block(source: str, condition: str) -> str:
    lines = source.splitlines()
    start = None
    condition_pattern = re.compile(rf"^\s*if\s*\(\s*{re.escape(condition)}\s*\)\s*(?:#.*)?$")
    for index, line in enumerate(lines):
        if condition_pattern.match(line):
            start = index
            break
    if start is None:
        raise AssertionError(f"missing CMake if block for {condition}")

    depth = 0
    for index in range(start, len(lines)):
        line = re.sub(r"#.*", "", lines[index]).strip()
        if re.match(r"^if\s*\(", line):
            depth += 1
        elif re.match(r"^endif\s*(?:\(|$)", line):
            depth -= 1
            if depth == 0:
                return "\n".join(lines[start : index + 1])

    raise AssertionError(f"unterminated CMake if block for {condition}")


def extract_line_window(source: str, token: str, before: int = 2, after: int = 2) -> str:
    lines = source.splitlines()
    for index, line in enumerate(lines):
        if token in line:
            start = max(index - before, 0)
            end = min(index + after + 1, len(lines))
            return "\n".join(lines[start:end])
    raise AssertionError(f"missing line containing {token}")


def extract_python_string_list_assignment(source: str, variable_name: str) -> list[str]:
    tree = ast.parse(source)
    for node in ast.walk(tree):
        if not isinstance(node, ast.Assign):
            continue
        if not any(isinstance(target, ast.Name) and target.id == variable_name for target in node.targets):
            continue
        if not isinstance(node.value, ast.List):
            continue
        values: list[str] = []
        for element in node.value.elts:
            if not isinstance(element, ast.Constant) or not isinstance(element.value, str):
                break
            values.append(element.value)
        else:
            return values
    raise AssertionError(f"missing Python string list assignment for {variable_name}")


def extract_re_search_patterns(source: str) -> list[str]:
    tree = ast.parse(source)
    patterns: list[str] = []
    for node in ast.walk(tree):
        if not isinstance(node, ast.Call):
            continue
        if not isinstance(node.func, ast.Attribute) or node.func.attr != "search":
            continue
        if not isinstance(node.func.value, ast.Name) or node.func.value.id != "re":
            continue
        if node.args and isinstance(node.args[0], ast.Constant) and isinstance(node.args[0].value, str):
            patterns.append(node.args[0].value)
    return patterns


def extract_function_body(source: str, signature_pattern: str) -> str:
    source = strip_comments(source)
    match = re.search(signature_pattern, source, re.S)
    if match is None:
        raise AssertionError(f"missing function matching {signature_pattern}")

    brace_start = source.find("{", match.end())
    if brace_start < 0:
        raise AssertionError(f"missing function body for {signature_pattern}")

    depth = 0
    in_string = False
    escaped = False
    for index in range(brace_start, len(source)):
        char = source[index]
        if in_string:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
            continue
        if char == '"':
            in_string = True
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[brace_start : index + 1]

    raise AssertionError(f"unterminated function body for {signature_pattern}")


def assert_contains_any(testcase: unittest.TestCase, source: str, label: str, tokens: tuple[str, ...]) -> None:
    testcase.assertTrue(
        any(token in source for token in tokens),
        f"{label} missing; expected one of: {tokens}",
    )


def assert_no_logic_registration(testcase: unittest.TestCase, source: str, register_call: str, route: str) -> None:
    testcase.assertIsNone(
        re.search(rf'\blogic\.{register_call}\s*\(\s*"{re.escape(route)}"', source),
        f"legacy route shim should not directly register {register_call} {route}",
    )


def assert_registry_registration(testcase: unittest.TestCase, source: str, method: str, route: str) -> None:
    testcase.assertRegex(
        source,
        rf'\bregistry\.Register\s*\(\s*"{re.escape(method)}"\s*,\s*"{re.escape(route)}"',
        f"neutral route module should register {method} {route}",
    )


def extract_logic_registration_block(source: str, register_call: str, route: str) -> str:
    source = strip_comments(source)
    match = re.search(rf'\blogic\.{register_call}\s*\(\s*"{re.escape(route)}"', source)
    if match is None:
        raise AssertionError(f"missing H3 {register_call} route block for {route}")

    paren_start = source.find("(", match.start())
    depth = 0
    in_string = False
    escaped = False
    for index in range(paren_start, len(source)):
        char = source[index]
        if in_string:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
            continue
        if char == '"':
            in_string = True
        elif char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                return source[match.start() : index + 1]

    raise AssertionError(f"unterminated H3 {register_call} route block for {route}")


H3_AUTH_ROUTES = {
    "/get_varifycode": "HandleGetVarifyCode",
    "/user_register": "HandleUserRegister",
    "/reset_pwd": "HandleResetPwd",
    "/user_login": "HandleUserLogin",
}

CALL_ROUTES = (
    ("POST", "/api/call/start"),
    ("POST", "/api/call/accept"),
    ("POST", "/api/call/reject"),
    ("POST", "/api/call/cancel"),
    ("POST", "/api/call/hangup"),
    ("GET", "/api/call/token"),
    ("POST", "/api/call/token"),
)

MEDIA_ROUTES = (
    ("POST", "/upload_media_init", "HandleUploadMediaInit"),
    ("POST", "/upload_media_chunk", "HandleUploadMediaChunk"),
    ("GET", "/upload_media_status", "HandleUploadMediaStatus"),
    ("POST", "/upload_media_complete", "HandleUploadMediaComplete"),
    ("POST", "/upload_media", "HandleUploadMediaSimple"),
    ("GET", "/media/download", "HandleMediaDownload"),
)

MOMENTS_ROUTES = (
    ("POST", "/api/moments/publish", "HandlePublish"),
    ("POST", "/api/moments/list", "HandleList"),
    ("POST", "/api/moments/detail", "HandleDetail"),
    ("POST", "/api/moments/delete", "HandleDelete"),
    ("POST", "/api/moments/like", "HandleLike"),
    ("POST", "/api/moments/comment", "HandleComment"),
    ("POST", "/api/moments/comment/list", "HandleCommentList"),
    ("POST", "/api/moments/comment/like", "HandleCommentLike"),
)

AI_EXACT_ROUTES = (
    ("POST", "/ai/chat", "HandleChat"),
    ("POST", "/ai/smart", "HandleSmart"),
    ("GET", "/ai/history", "HandleHistory"),
    ("POST", "/ai/session", "HandleCreateSession"),
    ("GET", "/ai/session/list", "HandleListSessions"),
    ("POST", "/ai/session/delete", "HandleDeleteSession"),
    ("GET", "/ai/model/list", "HandleListModels"),
    ("POST", "/ai/model/api/register", "HandleRegisterApiProvider"),
    ("POST", "/ai/model/api/delete", "HandleDeleteApiProvider"),
    ("POST", "/ai/kb/upload", "HandleKbUpload"),
    ("POST", "/ai/kb/search", "HandleKbSearch"),
    ("GET", "/ai/kb/list", "HandleListKb"),
    ("POST", "/ai/kb/delete", "HandleDeleteKb"),
    ("GET", "/ai/memory/list", "HandleMemoryList"),
    ("POST", "/ai/memory", "HandleMemoryCreate"),
    ("POST", "/ai/memory/delete", "HandleMemoryDelete"),
    ("POST", "/ai/tasks", "HandleTaskCreate"),
    ("GET", "/ai/tasks", "HandleTaskList"),
    ("GET", "/ai/tasks/detail", "HandleTaskDetail"),
    ("POST", "/ai/tasks/cancel", "HandleTaskCancel"),
    ("POST", "/ai/tasks/resume", "HandleTaskResume"),
)

AI_LEGACY_PREFIX_STREAM_ROUTES = (
    ("RegGetPrefix", "/ai/games"),
    ("RegPostPrefix", "/ai/games"),
    ("RegDeletePrefix", "/ai/games"),
    ("RegGetPrefix", "/ai/pet"),
    ("RegPostPrefix", "/ai/pet"),
    ("RegPost", "/ai/chat/stream"),
)

R18_ROUTES = (
    ("GET", "/api/r18/sources", "HandleListSources"),
    ("POST", "/api/r18/source/import", "HandleImportSource"),
    ("POST", "/api/r18/source/enable", "HandleEnableSource"),
    ("POST", "/api/r18/source/disable", "HandleDisableSource"),
    ("POST", "/api/r18/search", "HandleSearch"),
    ("POST", "/api/r18/comic/detail", "HandleComicDetail"),
    ("POST", "/api/r18/chapter/pages", "HandleChapterPages"),
    ("POST", "/api/r18/favorite/toggle", "HandleFavoriteToggle"),
    ("POST", "/api/r18/history/update", "HandleHistoryUpdate"),
    ("GET", "/api/r18/history", "HandleHistory"),
    ("GET", "/api/r18/image", "HandleImage"),
)

ACCOUNT_DAO_METHODS = (
    "RegUser",
    "RegUserTransaction",
    "CheckEmail",
    "UpdatePwd",
    "CheckPwd",
    "GetUserPublicId",
    "WarmupAuthQueries",
    "GenerateUserPublicId",
    "GetUserInfo",
    "TestProcedure",
)

NON_ACCOUNT_DAO_METHODS = (
    "UpdateUserProfile",
    "GetCallUserProfile",
    "InsertMediaAsset",
    "AddMoment",
)


def has_postgres_dao_definition(source: str, method_name: str) -> bool:
    return (
        re.search(
            rf"(?m)^\s*(?:[A-Za-z_][\w:<>*&]*\s+)+PostgresDao::{re.escape(method_name)}\s*\(",
            strip_comments(source),
        )
        is not None
    )


AI_SERVICE_CLIENT_CALL_TOKENS = (
    "Chat",
    "Smart",
    "GetHistory",
    "CreateSession",
    "ListSessions",
    "DeleteSession",
    "ListModels",
    "RegisterApiProvider",
    "DeleteApiProvider",
    "KbUpload",
    "KbSearch",
    "ListKb",
    "DeleteKb",
    "MemoryList",
    "MemoryCreate",
    "MemoryDelete",
    "AgentTaskCreate",
    "AgentTaskList",
    "AgentTaskGet",
    "AgentTaskCancel",
    "AgentTaskResume",
)

FORBIDDEN_TRANSPORT_TOKENS = (
    "HttpConnection",
    "GateHttp3Connection",
    "boost/beast",
    "boost/asio",
    "nghttp2",
    "msquic",
    "GateHttpJsonSupport",
)

AUTH_CACHE_FORBIDDEN_HEADER_TOKENS = (
    "GateRequest",
    "GateResponse",
    "RouteRegistry",
    "RouteModule",
    "routing/",
    "HttpConnection",
    "GateHttp3Connection",
    "boost/beast",
    "boost/asio",
    "nghttp2",
    "msquic",
    "GateHttpJsonSupport",
    "JsonValue",
    "JsonReader",
    "json/",
    "GlazeCompat",
    "Postgres",
    "PostgresMgr",
    "PostgresDao",
    "VerifyGrpcClient",
    "StatusGrpcClient",
    "ChatLoginTicket",
    "boost/uuid",
    "boost::uuids",
    "GateAsyncSideEffects",
    "GateWorkerPool",
    "transport",
    "Transport",
)

AUTH_VERIFY_CLIENT_FORBIDDEN_HEADER_TOKENS = (
    "VerifyGrpcClient",
    "VerifyGrpcClient.h",
    "GetVarifyRsp",
    "VarifyService",
    "message.pb",
    "message.grpc",
    ".pb.h",
    "grpcpp",
    "grpc/",
    "grpc::",
    "ClientContext",
    "protobuf",
    "GateRequest",
    "GateResponse",
    "RouteRegistry",
    "RouteModule",
    "routing/",
    "HttpConnection",
    "GateHttp3Connection",
    "boost/beast",
    "boost/asio",
    "nghttp2",
    "msquic",
    "GateHttpJsonSupport",
    "transport",
    "Transport",
    "JsonValue",
    "JsonReader",
    "json/",
    "GlazeCompat",
    "Redis",
    "RedisMgr",
    "Postgres",
    "PostgresMgr",
    "PostgresDao",
    "AuthCache",
    "ChatLoginTicket",
    "boost/uuid",
    "boost::uuids",
    "GateAsyncSideEffects",
    "GateWorkerPool",
    "CODEPREFIX",
    "USERTOKENPREFIX",
)

AUTH_VERIFY_CLIENT_FORBIDDEN_CPP_TOKENS = (
    "RedisMgr",
    "PostgresMgr",
    "PostgresDao",
    "AuthCache",
    "ChatLoginTicket",
    "GateAsyncSideEffects",
    "GateWorkerPool",
    "GateRequest",
    "GateResponse",
    "RouteRegistry",
    "RouteModule",
    "routing/",
    "HttpConnection",
    "GateHttp3Connection",
    "boost/beast",
    "boost/asio",
    "nghttp2",
    "msquic",
    "GateHttpJsonSupport",
    "transport",
    "Transport",
    "JsonValue",
    "JsonReader",
    "json/",
    "GlazeCompat",
    "login_ticket",
    "ChatLoginTicketClaims",
    "EncodeTicket",
    "token",
    "CODEPREFIX",
    "USERTOKENPREFIX",
)

AUTH_REDIS_KEY_CONSTRUCTION_PATTERNS = {
    "verification code key": r"(?:\bCODEPREFIX|std::string\s*\(\s*CODEPREFIX\s*\))\s*\+\s*email\b",
    "http token key": r"(?:\bUSERTOKENPREFIX|std::string\s*\(\s*USERTOKENPREFIX\s*\))\s*\+\s*std::to_string\s*\(\s*uid\s*\)",
    "login profile email key": r'"ulogin_profile_"\s*\+\s*email\b',
    "login profile uid key": r'"ulogin_profile_uid_"\s*\+\s*std::to_string\s*\(\s*uid\s*\)',
}

AUTH_REDIS_KEY_CALLER_PATHS = (
    SERVER_CORE / "GateServer" / "services" / "auth" / "AuthService.cpp",
    GATE_H2_SUPPORT / "Http2AuthSupport.cpp",
    GATE_CORE_SUPPORT / "AuthLoginSupport.cpp",
)


def extract_logic_route_block(source: str, route: str) -> str:
    return extract_logic_registration_block(source, "RegPost", route)


class GateServerStructureTests(unittest.TestCase):
    def test_top_level_server_cmake_keeps_main_gate_as_default_primary_target(self):
        source = read(SERVER_CORE / "CMakeLists.txt")

        self.assertIn("add_subdirectory(GateServer/core)", source)
        self.assertIn("add_subdirectory(GateServer/transports/h1/legacy_standalone)", source)
        self.assertIn("add_subdirectory(GateServer/plugins/r18)", source)
        self.assertIn("add_subdirectory(GateServer/transports/h3)", source)
        self.assertIn("add_subdirectory(GateServer)", source)
        self.assertTrue(GATE_CORE.exists(), "GateServerCore implementation should now live under GateServer/core")
        self.assertTrue(
            GATE_H3.exists(), "GateServerHttp3 implementation should now live under GateServer/transports/h3"
        )
        self.assertTrue(
            GATE_H2.exists(), "GateServerHttp2 implementation should now live under GateServer/transports/h2"
        )
        self.assertTrue(
            GATE_H1_LEGACY.exists(),
            "GateServerHttp1.1 implementation should now live under GateServer/transports/h1/legacy_standalone",
        )
        self.assertTrue(GATE_R18_PLUGINS.exists(), "R18 helper targets should now live under GateServer/plugins/r18")
        self.assertFalse(
            (SERVER_CORE / "GateServerCore").exists(),
            "GateServerCore should not remain as a sibling physical module directory",
        )
        self.assertFalse(
            (SERVER_CORE / "GateServerHttp3").exists(),
            "GateServerHttp3 should not remain as a sibling physical module directory",
        )
        self.assertFalse(
            (SERVER_CORE / "GateServerHttp2").exists(),
            "GateServerHttp2 should not remain as a sibling physical module directory",
        )
        self.assertFalse(
            (SERVER_CORE / "GateServerHttp1.1").exists(),
            "GateServerHttp1.1 should not remain as a sibling physical module directory",
        )
        self.assertLess(
            source.index("add_subdirectory(GateServer/core)"),
            source.index("add_subdirectory(GateServer)"),
        )
        self.assertLess(
            source.index("add_subdirectory(GateServer/transports/h1/legacy_standalone)"),
            source.index("add_subdirectory(GateServer)"),
        )
        self.assertLess(
            source.index("add_subdirectory(GateServer/plugins/r18)"), source.index("add_subdirectory(GateServer)")
        )
        h1_subdir_window = extract_line_window(
            source, "add_subdirectory(GateServer/transports/h1/legacy_standalone)", before=1, after=1
        )
        self.assertIn("legacy", h1_subdir_window.lower())
        self.assertIn("plugin", h1_subdir_window.lower())
        self.assertIn("R18", h1_subdir_window)
        self.assertIn("default-buildable", h1_subdir_window)

    def test_root_cmake_gates_legacy_h2_standalone_subdirectory_behind_opt_in_and_libh2o(self):
        source = read(SERVER_CORE / "CMakeLists.txt")
        compact = normalize_space(source)

        self.assertRegex(
            compact,
            rf"option\s*\(\s*{LEGACY_GATE_HTTP2_OPTION}\b[^)]*\bOFF\s*\)",
        )
        legacy_h2_block = extract_cmake_if_block(source, f"{LEGACY_GATE_HTTP2_OPTION} AND libh2o_FOUND")
        self.assertIn("add_subdirectory(GateServer/transports/h2)", legacy_h2_block)
        self.assertIn(LEGACY_GATE_HTTP2_OPTION, legacy_h2_block)
        self.assertIn("libh2o_FOUND", legacy_h2_block)

        outside_legacy_h2_gate = strip_cmake_comments(source.replace(legacy_h2_block, ""))
        self.assertNotRegex(
            outside_legacy_h2_gate,
            r"\badd_subdirectory\s*\(\s*GateServer/transports/h2\s*\)",
            "GateServerHttp2 must only be processed through the legacy standalone opt-in gate",
        )
        self.assertNotIn("FATAL_ERROR", legacy_h2_block)

    def test_legacy_h1_standalone_executable_is_opt_in_and_default_off(self):
        source = read(GATE_H1_LEGACY / "CMakeLists.txt")
        compact = normalize_space(source)

        self.assertRegex(
            compact,
            rf"option\s*\(\s*{LEGACY_GATE_HTTP1_OPTION}\b[^)]*\bOFF\s*\)",
        )
        legacy_block = extract_cmake_if_block(source, LEGACY_GATE_HTTP1_OPTION)
        self.assertIn("add_executable(GateServerHttp1.1", legacy_block)
        self.assertIn("memochat_configure_server_target(GateServerHttp1.1)", legacy_block)
        self.assertIn("target_link_libraries(GateServerHttp1.1", legacy_block)
        self.assertIn("target_include_directories(GateServerHttp1.1", legacy_block)
        self.assertNotIn("R18PluginHost", legacy_block)
        self.assertNotIn("R18MockSourcePlugin", legacy_block)

        outside_legacy_gate = source.replace(legacy_block, "")
        self.assertNotIn("add_executable(GateServerHttp1.1", outside_legacy_gate)

    def test_r18_plugin_helper_targets_stay_outside_legacy_standalone_gate(self):
        source = read(GATE_H1_LEGACY / "CMakeLists.txt")
        legacy_block = extract_cmake_if_block(source, LEGACY_GATE_HTTP1_OPTION)

        self.assertNotIn("R18PluginHost", legacy_block)
        self.assertNotIn("R18MockSourcePlugin", legacy_block)

        plugin_cmake = read(GATE_R18_PLUGINS / "CMakeLists.txt")
        self.assertIn("add_executable(R18PluginHost", plugin_cmake)
        self.assertIn("add_library(R18MockSourcePlugin SHARED", plugin_cmake)
        self.assertIn("target_compile_features(R18PluginHost", plugin_cmake)
        self.assertIn("target_compile_features(R18MockSourcePlugin", plugin_cmake)

    def test_legacy_h1_readme_documents_opt_in_and_plugin_helper_ownership(self):
        readme_path = GATE_H1_LEGACY / "README.md"
        self.assertTrue(readme_path.exists())
        source = read(readme_path)
        lower_source = source.lower()

        self.assertIn("legacy", lower_source)
        self.assertIn("opt-in", lower_source)
        self.assertIn("production", lower_source)
        self.assertIn("default", lower_source)
        self.assertIn("gateserver", lower_source)
        self.assertIn("R18PluginHost", source)
        self.assertIn("R18MockSourcePlugin", source)
        self.assertIn("GateServer/plugins/r18", source)

    def test_default_ops_surfaces_do_not_list_legacy_h1_standalone_service(self):
        source = read(REPO_ROOT / "infra" / "Memo_ops" / "server" / "ops_server" / "routes" / "system.py")
        services = extract_python_string_list_assignment(source, "services")
        patterns = extract_re_search_patterns(source)

        self.assertIn("GateServer", services)
        self.assertNotIn("GateServerHttp1.1", services)
        self.assertNotIn("GateServerHttp1", services)
        self.assertNotIn("GateServerHttp1.1", "\n".join(patterns))

        gate_patterns = [pattern for pattern in patterns if "GateServer" in pattern]
        self.assertTrue(gate_patterns, "Ops process scanner should still detect the primary GateServer")
        for pattern in gate_patterns:
            with self.subTest(pattern=pattern):
                self.assertIsNone(re.search(pattern, "GateServerHttp1.1"))

    def test_windows_stop_script_default_survival_check_omits_legacy_h1_standalone(self):
        source = read(REPO_ROOT / "tools" / "scripts" / "status" / "stop-all-services.bat")
        check_lines = [
            line.strip() for line in source.splitlines() if re.match(r"^call\s+:check\b", line.strip(), flags=re.I)
        ]
        kill_lines = [
            line.strip()
            for line in source.splitlines()
            if re.match(r"^call\s+:kill_by_name\b", line.strip(), flags=re.I)
        ]

        self.assertTrue(check_lines, "stop-all-services.bat should keep a survival check section")
        self.assertTrue(any("GateServer.exe" in line for line in check_lines))
        self.assertFalse(
            any("GateServerHttp1.1" in line or "GateServerHttp1" in line for line in check_lines),
            "GateServerHttp1.1 should not be reported as a default service in survival checks",
        )
        self.assertFalse(
            any("GateServerHttp1.1" in line or "GateServerHttp1" in line for line in kill_lines),
            "GateServerHttp1.1 should not be stopped as a default backend service",
        )

    def test_main_gate_target_links_core_h3_and_conditionally_embeds_h2_sources(self):
        source = read(SERVER_CORE / "GateServer" / "CMakeLists.txt")

        self.assertIn("add_executable(GateServer", source)
        self.assertIn("routing/RouteRegistry.cpp", source)
        self.assertIn("modules/auth/AuthRouteModule.cpp", source)
        self.assertIn("modules/call/CallRouteModule.cpp", source)
        self.assertIn("services/auth/AuthService.cpp", source)
        self.assertIn("modules/health/HealthRouteModule.cpp", source)
        self.assertIn("modules/media/MediaRouteModule.cpp", source)
        self.assertIn("modules/moments/MomentsRouteModule.cpp", source)
        self.assertIn("modules/profile/ProfileRouteModule.cpp", source)
        self.assertIn("modules/ai/AIRouteModule.cpp", source)
        self.assertIn("modules/r18/R18RouteModule.cpp", source)
        self.assertIn("services/media/MediaService.cpp", source)
        self.assertIn("services/moments/MomentsService.cpp", source)
        self.assertIn("services/ai/AIService.cpp", source)
        self.assertIn("services/r18/R18Service.cpp", source)
        self.assertIn("adapters/h1/H1RouteAdapter.cpp", source)
        self.assertIn("GateServerH1Routes.cpp", source)
        self.assertIn("LogicSystem.cpp", source)
        self.assertIn("MediaHttpService.cpp", source)
        self.assertIn("AIRouteModules.cpp", source)
        self.assertIn("target_link_libraries(GateServer PRIVATE", source)
        self.assertIn("GateServerCore", source)
        self.assertIn("GateServerHttp3", source)

        compact = normalize_space(source)
        self.assertRegex(
            compact,
            rf"option\s*\(\s*{EMBEDDED_GATE_HTTP2_OPTION}\b[^)]*\bOFF\s*\)",
        )
        embedded_h2_block = extract_cmake_if_block(
            source,
            f"{EMBEDDED_GATE_HTTP2_OPTION} AND MEMOCHAT_HAVE_NGHTTP2",
        )
        self.assertIn("target_sources(GateServer PRIVATE", embedded_h2_block)
        self.assertIn("${CMAKE_CURRENT_SOURCE_DIR}/transports/h2", embedded_h2_block)
        for h2_source in EMBEDDED_GATE_HTTP2_SOURCES:
            with self.subTest(h2_source=h2_source):
                self.assertIn(h2_source, embedded_h2_block)

        outside_embedded_h2_gate = strip_cmake_comments(source.replace(embedded_h2_block, ""))
        for h2_source in EMBEDDED_GATE_HTTP2_SOURCES:
            with self.subTest(outside_h2_gate=h2_source):
                self.assertNotIn(
                    h2_source,
                    outside_embedded_h2_gate,
                    "embedded HTTP/2 sources must not be added solely because MEMOCHAT_HAVE_NGHTTP2 is true",
                )

    def test_standalone_h2_target_builds_adapter_and_shared_health_registry_sources(self):
        source = read(GATE_H2 / "CMakeLists.txt")

        self.assertIn("add_executable(GateServerHttp2", source)
        self.assertIn("adapters/h2/H2RouteAdapter.cpp", source)
        self.assertIn("routes/Http2Routes.cpp", source)
        self.assertIn("handlers/Http2AuthHandlers.cpp", source)
        self.assertIn("support/Http2AuthSupport.cpp", source)
        self.assertIn("standalone/main.cpp", source)
        self.assertIn("../../routing/RouteRegistry.cpp", source)
        self.assertIn("../../modules/health/HealthRouteModule.cpp", source)
        self.assertIn("${CMAKE_CURRENT_SOURCE_DIR}/../..", source)
        self.assertIn("${CMAKE_CURRENT_SOURCE_DIR}/../../../common", source)
        self.assertNotIn("${CMAKE_CURRENT_SOURCE_DIR}/common", source)

    def test_standalone_h3_target_builds_named_adapter_source(self):
        source = read(GATE_H3 / "CMakeLists.txt")

        self.assertIn("add_library(GateServerHttp3 STATIC", source)
        self.assertIn("listener/GateHttp3Listener.cpp", source)
        self.assertIn("legacy_routes/GateHttp3ServiceRoutes.cpp", source)
        self.assertIn("adapters/h3/H3RouteAdapter.cpp", source)

    def test_main_gate_runtime_currently_starts_h1_and_keeps_h2_h3_edge_disabled(self):
        source = read(SERVER_CORE / "GateServer" / "GateServer.cpp")

        self.assertIn("std::make_shared<CServer>(ioc, gate_port)->Start();", source)
        self.assertIn("GateServer listening", source)
        self.assertIn("embedded HTTP/2 and HTTP/3 entrypoints are disabled", source)
        self.assertIn("Envoy serves HTTP/2 and HTTP/3 on 8443", source)
        self.assertNotIn("NgHttp2Server::GetInstance()->Run()", source)
        self.assertNotIn("GateHttp3Listener", source)

    def test_current_gate_tree_contains_large_files_to_split_later(self):
        large_file_thresholds = {
            GATE_CORE_PERSISTENCE / "PostgresDao.cpp": 1000,
            GATE_H3_LEGACY_ROUTES / "GateHttp3ServiceRoutes.cpp": 1000,
        }
        for path, minimum_lines in large_file_thresholds.items():
            with self.subTest(path=path.relative_to(REPO_ROOT)):
                line_count = len(read(path).splitlines())
                self.assertGreaterEqual(line_count, minimum_lines)

        media_shim = SERVER_CORE / "GateServer" / "MediaHttpService.cpp"
        media_module = SERVER_CORE / "GateServer" / "modules" / "media" / "MediaRouteModule.cpp"
        media_service = SERVER_CORE / "GateServer" / "services" / "media" / "MediaService.cpp"
        self.assertTrue(
            len(read(media_shim).splitlines()) < 900 or (media_module.exists() and media_service.exists()),
            "MediaHttpService should either shrink below the legacy large-file threshold or be split into media "
            "module/service files",
        )

    def test_gate_core_compiles_split_account_dao_source(self):
        account_dao_path = GATE_CORE_PERSISTENCE / "PostgresDaoAccount.cpp"
        self.assertTrue(account_dao_path.exists(), "GateServerCore account DAO split source should exist")

        cmake_source = strip_cmake_comments(read(GATE_CORE / "CMakeLists.txt"))
        # Phase 1 split: the whole Dao/Mgr persistence layer is a single shared
        # unit in GateInfraCore (PostgresMgr forwards to all PostgresDao methods,
        # and PostgresDao is one class split across two .cpp files). Both DAO
        # sources therefore compile into GateInfraCore.
        infra_match = re.search(r"add_library\s*\(\s*GateInfraCore\s+STATIC(?P<body>.*?)\)", cmake_source, re.S)
        self.assertIsNotNone(infra_match, "GateInfraCore static library source list should be explicit")
        infra_sources = infra_match.group("body")

        self.assertIn("PostgresDao.cpp", infra_sources)
        self.assertIn("PostgresDaoAccount.cpp", infra_sources)

    def test_gate_core_account_dao_methods_live_only_in_account_source(self):
        account_dao_path = GATE_CORE_PERSISTENCE / "PostgresDaoAccount.cpp"
        self.assertTrue(account_dao_path.exists(), "GateServerCore account DAO split source should exist")

        broad_dao = read(GATE_CORE_PERSISTENCE / "PostgresDao.cpp")
        account_dao = read(account_dao_path)

        for method in ACCOUNT_DAO_METHODS:
            with self.subTest(account_method=method):
                self.assertTrue(
                    has_postgres_dao_definition(account_dao, method),
                    f"PostgresDaoAccount.cpp should define PostgresDao::{method}",
                )
                self.assertFalse(
                    has_postgres_dao_definition(broad_dao, method),
                    f"PostgresDao.cpp should not define account method PostgresDao::{method}",
                )

        for method in NON_ACCOUNT_DAO_METHODS:
            with self.subTest(non_account_method=method):
                self.assertTrue(
                    has_postgres_dao_definition(broad_dao, method),
                    f"PostgresDao.cpp should keep non-account method PostgresDao::{method}",
                )
                self.assertFalse(
                    has_postgres_dao_definition(account_dao, method),
                    f"PostgresDaoAccount.cpp should not absorb non-account method PostgresDao::{method}",
                )

    def test_h1_auth_route_file_is_shrinking_as_auth_routes_move_to_module(self):
        source = read(SERVER_CORE / "GateServer" / "GateServerH1Routes.cpp")
        line_count = len(source.splitlines())

        self.assertLess(line_count, 500)
        self.assertNotIn('logic.RegPost("/get_varifycode"', source)
        self.assertNotIn('logic.RegPost(\n        "/user_register"', source)
        self.assertNotIn('logic.RegPost(\n        "/reset_pwd"', source)
        self.assertNotIn('logic.RegPost(\n        "/user_login"', source)

    def test_call_route_module_registers_h1_call_routes_neutrally(self):
        call_dir = SERVER_CORE / "GateServer" / "modules" / "call"
        header_path = call_dir / "CallRouteModule.h"
        source_path = call_dir / "CallRouteModule.cpp"

        self.assertTrue(header_path.exists())
        self.assertTrue(source_path.exists())

        header = strip_comments(read(header_path))
        source = strip_comments(read(source_path))
        compact_source = normalize_space(source)
        combined = header + "\n" + source

        self.assertIn("namespace memochat::gate::modules::call", combined)
        self.assertIn("class CallRouteModule final", header)
        self.assertIn("public memochat::gate::routing::RouteModule", header)
        self.assertIn("void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override", header)
        for method, route in CALL_ROUTES:
            with self.subTest(route=(method, route)):
                assert_registry_registration(self, source, method, route)

        expected_behavior_tokens = (
            "StartCall",
            "AcceptCall",
            "RejectCall",
            "CancelCall",
            "HangupCall",
            "GetToken",
            "application/json",
            "text/json",
            "request.query",
            "std::atoi",
            'root["trace_id"]',
        )
        for token in expected_behavior_tokens:
            with self.subTest(token=token):
                self.assertIn(token, source)

        forbidden_transport_tokens = (
            "HttpConnection",
            "GateHttp3Connection",
            "boost/beast",
            "nghttp2",
            "msquic",
            "GateHttpJsonSupport",
        )
        for token in forbidden_transport_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, combined)

    def test_logic_system_registers_call_module_and_legacy_h1_does_not(self):
        logic_source = read(SERVER_CORE / "GateServer" / "LogicSystem.cpp")
        legacy_h1_source = strip_comments(read(SERVER_CORE / "GateServer" / "GateServerH1Routes.cpp"))

        self.assertIn('#include "modules/call/CallRouteModule.h"', logic_source)
        self.assertIn("memochat::gate::modules::call::CallRouteModule().RegisterRoutes(_route_registry)", logic_source)

        for method, route in CALL_ROUTES:
            register_name = "RegGet" if method == "GET" else "RegPost"
            with self.subTest(route=(method, route)):
                self.assertIsNone(re.search(rf'\blogic\.{register_name}\s*\(\s*"{re.escape(route)}"', legacy_h1_source))

    def test_media_route_module_registers_h1_media_routes_neutrally(self):
        media_dir = SERVER_CORE / "GateServer" / "modules" / "media"
        header_path = media_dir / "MediaRouteModule.h"
        source_path = media_dir / "MediaRouteModule.cpp"

        self.assertTrue(header_path.exists())
        self.assertTrue(source_path.exists())

        header = strip_comments(read(header_path))
        source = strip_comments(read(source_path))
        compact_source = normalize_space(source)
        combined = header + "\n" + source

        self.assertIn("namespace memochat::gate::modules::media", combined)
        self.assertIn("class MediaRouteModule final", header)
        self.assertIn("public memochat::gate::routing::RouteModule", header)
        self.assertIn("void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override", header)
        self.assertIn("MediaService::Instance()", source)

        for method, route, handler in MEDIA_ROUTES:
            with self.subTest(route=(method, route)):
                assert_registry_registration(self, source, method, route)
                self.assertRegex(compact_source, rf"\b{handler}\s*\(\s*request\s*,\s*response\s*\)")

        for token in FORBIDDEN_TRANSPORT_TOKENS:
            with self.subTest(token=token):
                self.assertNotIn(token, combined)

    def test_media_service_facade_owns_h1_media_behavior_and_stays_transport_neutral(self):
        service_dir = SERVER_CORE / "GateServer" / "services" / "media"
        header_path = service_dir / "MediaService.h"
        source_path = service_dir / "MediaService.cpp"

        self.assertTrue(header_path.exists())
        self.assertTrue(source_path.exists())

        header = strip_comments(read(header_path))
        source = strip_comments(read(source_path))
        combined = header + "\n" + source

        self.assertIn("class MediaService", header)
        self.assertIn("static MediaService& Instance()", header)
        for _, _, handler in MEDIA_ROUTES:
            with self.subTest(handler=handler):
                self.assertIn(handler, header)
                self.assertIn(handler, source)

        expected_behavior_tokens = (
            "text/json",
            "root.toStyledString",
            "DecodeBase64Local",
            "ValidateUserTokenLocal",
            "SaveJsonFileLocal",
            "LoadJsonFileLocal",
            "StoreMergedFile",
            "InsertMediaAsset",
            "GetMediaAssetByKey",
            "ReadObject",
            "ResolvePublicUrl",
            "ResolveReadPath",
            "ResolveLegacyMediaPathLocal",
        )
        for token in expected_behavior_tokens:
            with self.subTest(token=token):
                self.assertIn(token, source)

        for token in FORBIDDEN_TRANSPORT_TOKENS:
            with self.subTest(token=token):
                self.assertNotIn(token, combined)

    def test_logic_system_registers_media_module_and_legacy_service_does_not(self):
        logic_source = read(SERVER_CORE / "GateServer" / "LogicSystem.cpp")
        legacy_media_source = strip_comments(read(SERVER_CORE / "GateServer" / "MediaHttpService.cpp"))

        self.assertIn('#include "modules/media/MediaRouteModule.h"', logic_source)
        self.assertIn(
            "memochat::gate::modules::media::MediaRouteModule().RegisterRoutes(_route_registry)", logic_source
        )
        self.assertNotIn("MediaHttpService::RegisterRoutes(*this)", logic_source)

        for method, route, _ in MEDIA_ROUTES:
            register_name = "RegGet" if method == "GET" else "RegPost"
            with self.subTest(route=(method, route)):
                self.assertIsNone(
                    re.search(rf'\blogic\.{register_name}\s*\(\s*"{re.escape(route)}"', legacy_media_source)
                )

    def test_moments_route_module_registers_h1_moments_routes_neutrally(self):
        moments_dir = SERVER_CORE / "GateServer" / "modules" / "moments"
        header_path = moments_dir / "MomentsRouteModule.h"
        source_path = moments_dir / "MomentsRouteModule.cpp"

        self.assertTrue(header_path.exists())
        self.assertTrue(source_path.exists())

        header = strip_comments(read(header_path))
        source = strip_comments(read(source_path))
        compact_source = normalize_space(source)
        combined = header + "\n" + source

        self.assertIn("namespace memochat::gate::modules::moments", combined)
        self.assertIn("class MomentsRouteModule final", header)
        self.assertIn("public memochat::gate::routing::RouteModule", header)
        self.assertIn("void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override", header)
        self.assertIn("MomentsService::Instance()", source)

        registered_moments_posts = re.findall(
            r'registry\.Register\s*\(\s*"POST"\s*,\s*"/api/moments/',
            source,
        )
        self.assertEqual(len(MOMENTS_ROUTES), len(registered_moments_posts))
        for method, route, handler in MOMENTS_ROUTES:
            with self.subTest(route=(method, route)):
                assert_registry_registration(self, source, method, route)
                self.assertRegex(compact_source, rf"\b{handler}\s*\(\s*request\s*,\s*response\s*\)")

        forbidden_route_module_tokens = FORBIDDEN_TRANSPORT_TOKENS + ("PostgresMgr", "MongoMgr")
        for token in forbidden_route_module_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, combined)

    def test_moments_service_facade_owns_h1_behavior_and_stays_transport_neutral(self):
        service_dir = SERVER_CORE / "GateServer" / "services" / "moments"
        header_path = service_dir / "MomentsService.h"
        source_path = service_dir / "MomentsService.cpp"

        self.assertTrue(header_path.exists())
        self.assertTrue(source_path.exists())

        header = strip_comments(read(header_path))
        source = strip_comments(read(source_path))
        combined = header + "\n" + source

        self.assertIn("namespace memochat::gate::services::moments", combined)
        self.assertIn("class MomentsService", header)
        self.assertIn("static MomentsService& Instance()", header)
        self.assertIn("MomentsService::Instance()", source)
        for _, _, handler in MOMENTS_ROUTES:
            with self.subTest(handler=handler):
                self.assertIn(handler, header)
                self.assertIn(handler, source)

        expected_behavior_tokens = (
            "ValidateAuth",
            "NowMs",
            "NormalizeMomentItem",
            "BuildMomentJson",
            "BuildCommentList",
            "FillCommentLikes",
            "PostgresMgr::GetInstance()->AddMoment",
            "MongoMgr::GetInstance()->InsertMomentContent",
            "GetMomentsFeed",
            "CanViewMoment",
            "DeleteMoment",
            "AddMomentLike",
            "RemoveMomentLike",
            "AddMomentComment",
            "DeleteMomentComment",
            "GetMomentComments",
            "AddMomentCommentLike",
            "RemoveMomentCommentLike",
            'root["trace_id"]',
        )
        for token in expected_behavior_tokens:
            with self.subTest(token=token):
                self.assertIn(token, source)

        for token in FORBIDDEN_TRANSPORT_TOKENS:
            with self.subTest(token=token):
                self.assertNotIn(token, combined)

    def test_logic_system_registers_moments_module_and_legacy_service_does_not(self):
        logic_source = read(SERVER_CORE / "GateServer" / "LogicSystem.cpp")
        legacy_moments_path = SERVER_CORE / "GateServer" / "MomentsRouteModules.cpp"
        legacy_moments_source = strip_comments(read(legacy_moments_path)) if legacy_moments_path.exists() else ""

        self.assertIn('#include "modules/moments/MomentsRouteModule.h"', logic_source)
        self.assertIn(
            "memochat::gate::modules::moments::MomentsRouteModule().RegisterRoutes(_route_registry)", logic_source
        )
        self.assertNotIn("MomentsHttpServiceRoutes::RegisterRoutes(*this)", logic_source)

        for method, route, _ in MOMENTS_ROUTES:
            register_name = "RegGet" if method == "GET" else "RegPost"
            with self.subTest(route=(method, route)):
                self.assertIsNone(
                    re.search(rf'\blogic\.{register_name}\s*\(\s*"{re.escape(route)}"', legacy_moments_source)
                )

    def test_ai_route_module_registers_exact_h1_ai_routes_neutrally(self):
        ai_dir = SERVER_CORE / "GateServer" / "modules" / "ai"
        header_path = ai_dir / "AIRouteModule.h"
        source_path = ai_dir / "AIRouteModule.cpp"

        self.assertTrue(header_path.exists())
        self.assertTrue(source_path.exists())

        header = strip_comments(read(header_path))
        source = strip_comments(read(source_path))
        compact_source = normalize_space(source)
        combined = header + "\n" + source

        self.assertIn("namespace memochat::gate::modules::ai", combined)
        self.assertIn("class AIRouteModule final", header)
        self.assertIn("public memochat::gate::routing::RouteModule", header)
        self.assertIn("void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override", header)
        self.assertIn("AIService::Instance()", source)

        for method, route, handler in AI_EXACT_ROUTES:
            with self.subTest(route=(method, route)):
                assert_registry_registration(self, source, method, route)
                self.assertRegex(compact_source, rf"\b{handler}\s*\(\s*request\s*,\s*response\s*\)")

        for token in FORBIDDEN_TRANSPORT_TOKENS + ("AIServiceClient",):
            with self.subTest(token=token):
                self.assertNotIn(token, combined)

    def test_ai_service_facade_owns_exact_json_ai_behavior_and_stays_transport_neutral(self):
        service_dir = SERVER_CORE / "GateServer" / "services" / "ai"
        header_path = service_dir / "AIService.h"
        source_path = service_dir / "AIService.cpp"

        self.assertTrue(header_path.exists())
        self.assertTrue(source_path.exists())

        header = strip_comments(read(header_path))
        source = strip_comments(read(source_path))
        combined = header + "\n" + source

        self.assertIn("namespace memochat::gate::services::ai", combined)
        self.assertIn("class AIService", header)
        self.assertIn("static AIService& Instance()", header)
        self.assertIn("AIService::Instance()", source)
        for _, _, handler in AI_EXACT_ROUTES:
            with self.subTest(handler=handler):
                self.assertIn(handler, header)
                self.assertIn(handler, source)

        for token in AI_SERVICE_CLIENT_CALL_TOKENS:
            with self.subTest(client_call=token):
                self.assertIn(token, source)

        for token in ('"invalid json"', '"content is empty"'):
            with self.subTest(token=token):
                self.assertIn(token, source)

        forbidden_service_tokens = FORBIDDEN_TRANSPORT_TOKENS + (
            "StartSseStream",
            "WriteStreamChunk",
            "FinishStream",
        )
        for token in forbidden_service_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, combined)

    def test_logic_system_registers_ai_module_and_keeps_legacy_ai_adapter(self):
        logic_source = read(SERVER_CORE / "GateServer" / "LogicSystem.cpp")

        self.assertIn('#include "modules/ai/AIRouteModule.h"', logic_source)
        self.assertIn("memochat::gate::modules::ai::AIRouteModule().RegisterRoutes(_route_registry)", logic_source)
        self.assertIn("AIHttpServiceRoutes::RegisterRoutes(*this)", logic_source)

    def test_legacy_ai_adapter_only_keeps_prefix_proxy_and_stream_routes(self):
        legacy_source = strip_comments(read(SERVER_CORE / "GateServer" / "AIRouteModules.cpp"))
        compact_legacy_source = normalize_space(legacy_source)

        for method, route, _ in AI_EXACT_ROUTES:
            register_call = "RegGet" if method == "GET" else "RegPost"
            with self.subTest(moved_route=(method, route)):
                assert_no_logic_registration(self, legacy_source, register_call, route)

        for register_call, route in AI_LEGACY_PREFIX_STREAM_ROUTES:
            with self.subTest(legacy_route=(register_call, route)):
                self.assertRegex(
                    legacy_source,
                    rf'\blogic\.{register_call}\s*\(\s*"{re.escape(route)}"',
                )

        expected_legacy_tokens = (
            "ProxyAiOrchestratorPrefix",
            "ProxyAiOrchestratorPetStream",
            "StartSseStream",
            "GateWorkerPool::GetInstance()->post",
            "WriteStreamChunk",
            "FinishStream",
        )
        for token in expected_legacy_tokens:
            with self.subTest(token=token):
                self.assertIn(token, compact_legacy_source)

    def test_r18_route_module_registers_h1_r18_routes_neutrally(self):
        r18_dir = SERVER_CORE / "GateServer" / "modules" / "r18"
        header_path = r18_dir / "R18RouteModule.h"
        source_path = r18_dir / "R18RouteModule.cpp"

        self.assertTrue(header_path.exists())
        self.assertTrue(source_path.exists())

        header = strip_comments(read(header_path))
        source = strip_comments(read(source_path))
        compact_source = normalize_space(source)
        combined = header + "\n" + source

        self.assertIn("namespace memochat::gate::modules::r18", combined)
        self.assertIn("class R18RouteModule final", header)
        self.assertIn("public memochat::gate::routing::RouteModule", header)
        self.assertIn("void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override", header)
        self.assertIn("R18Service::Instance()", source)

        registered_r18_routes = re.findall(
            r'registry\.Register\s*\(\s*"(?:GET|POST)"\s*,\s*"/api/r18/',
            source,
        )
        self.assertEqual(len(R18_ROUTES), len(registered_r18_routes))
        for method, route, handler in R18_ROUTES:
            with self.subTest(route=(method, route)):
                assert_registry_registration(self, source, method, route)
                self.assertRegex(compact_source, rf"\b{handler}\s*\(\s*request\s*,\s*response\s*\)")

        forbidden_route_module_tokens = FORBIDDEN_TRANSPORT_TOKENS + (
            "RedisMgr",
            "R18SourceService",
            "USERTOKENPREFIX",
            "DecodeBase64",
        )
        for token in forbidden_route_module_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, combined)

    def test_r18_service_facade_owns_h1_r18_behavior_and_stays_transport_neutral(self):
        service_dir = SERVER_CORE / "GateServer" / "services" / "r18"
        header_path = service_dir / "R18Service.h"
        source_path = service_dir / "R18Service.cpp"

        self.assertTrue(header_path.exists())
        self.assertTrue(source_path.exists())

        header = strip_comments(read(header_path))
        source = strip_comments(read(source_path))
        combined = header + "\n" + source

        self.assertIn("namespace memochat::gate::services::r18", combined)
        self.assertIn("class R18Service", header)
        self.assertIn("static R18Service& Instance()", header)
        self.assertIn("R18Service::Instance()", source)
        self.assertIn("const memochat::gate::routing::GateRequest& request", header)
        self.assertIn("memochat::gate::routing::GateResponse& response", header)
        for _, _, handler in R18_ROUTES:
            with self.subTest(handler=handler):
                self.assertIn(handler, header)
                self.assertIn(handler, source)

        expected_behavior_tokens = (
            "ValidateUserToken",
            "R18SourceService::Instance()",
            "ListSources",
            "ImportZip",
            "EnableSource",
            "Search",
            "Detail",
            "Pages",
            "FetchImage",
            "DecodeBase64",
            "USERTOKENPREFIX",
            "TokenInvalid",
            "Error_Json",
            "application/json",
            "text/json",
            "text/plain",
            'root["trace_id"]',
            "status = 401",
            "status = 502",
        )
        for token in expected_behavior_tokens:
            with self.subTest(token=token):
                self.assertIn(token, source)

        for token in FORBIDDEN_TRANSPORT_TOKENS:
            with self.subTest(token=token):
                self.assertNotIn(token, combined)

    def test_logic_system_registers_r18_module_and_legacy_shim_does_not(self):
        logic_source = read(SERVER_CORE / "GateServer" / "LogicSystem.cpp")
        legacy_r18_source = strip_comments(read(SERVER_CORE / "GateServer" / "GateR18Routes.cpp"))

        self.assertIn('#include "modules/r18/R18RouteModule.h"', logic_source)
        self.assertIn("memochat::gate::modules::r18::R18RouteModule().RegisterRoutes(_route_registry)", logic_source)
        self.assertNotIn("R18HttpServiceRoutes::RegisterRoutes(*this)", logic_source)
        self.assertIn("void R18HttpServiceRoutes::RegisterRoutes(LogicSystem& logic)", legacy_r18_source)
        self.assertRegex(legacy_r18_source, r"\(void\)\s*logic")

        for method, route, _ in R18_ROUTES:
            register_name = "RegGet" if method == "GET" else "RegPost"
            with self.subTest(route=(method, route)):
                assert_no_logic_registration(self, legacy_r18_source, register_name, route)

    def test_h5_gate_tree_does_not_contain_stale_or_non_source_artifacts(self):
        gate_dir = SERVER_CORE / "GateServer"
        for artifact in H5_STALE_GATE_ARTIFACTS:
            with self.subTest(artifact=artifact):
                self.assertFalse(
                    (gate_dir / artifact).exists(),
                    f"GateServer stale H5 artifact should be deleted: {artifact}",
                )

    def test_canonical_message_proto_is_common_proto_and_root_cmake_points_to_it(self):
        canonical_proto = SERVER_CORE / "common" / "proto" / "message.proto"
        self.assertTrue(canonical_proto.exists())

        root_cmake = strip_cmake_comments(read(SERVER_CORE / "CMakeLists.txt"))
        self.assertRegex(
            root_cmake,
            r'set\s*\(\s*MEMOCHAT_PROTO_SRC\s+"?\$\{CMAKE_CURRENT_SOURCE_DIR\}/common/proto/message\.proto"?\s*\)',
        )
        proto_src_window = extract_line_window(root_cmake, "MEMOCHAT_PROTO_SRC", before=0, after=0)
        self.assertNotIn("GateServer/message.proto", proto_src_window)

    def test_root_cmake_generates_message_outputs_under_generated_common_proto(self):
        root_cmake = strip_cmake_comments(read(SERVER_CORE / "CMakeLists.txt"))

        self.assertRegex(
            root_cmake,
            r'set\s*\(\s*MEMOCHAT_PROTO_GEN_DIR\s+"?\$\{CMAKE_BINARY_DIR\}/generated"?\s*\)',
        )
        expected_generated_outputs = {
            "MEMOCHAT_PROTO_CC": "message.pb.cc",
            "MEMOCHAT_PROTO_H": "message.pb.h",
            "MEMOCHAT_GRPC_CC": "message.grpc.pb.cc",
            "MEMOCHAT_GRPC_H": "message.grpc.pb.h",
        }
        for variable, filename in expected_generated_outputs.items():
            with self.subTest(variable=variable):
                self.assertRegex(
                    root_cmake,
                    rf'set\s*\(\s*{variable}\s+"?\$\{{MEMOCHAT_PROTO_GEN_DIR\}}/common/proto/{re.escape(filename)}"?\s*\)',
                )

        self.assertIn('COMMAND ${CMAKE_COMMAND} -E make_directory "${MEMOCHAT_PROTO_GEN_DIR}/common/proto"', root_cmake)

        memochat_proto_match = re.search(
            r"add_library\s*\(\s*memochat_proto\s+STATIC(?P<body>.*?)\)",
            root_cmake,
            re.S,
        )
        self.assertIsNotNone(memochat_proto_match, "memochat_proto source list should be explicit")
        memochat_proto_sources = memochat_proto_match.group("body")
        for variable in expected_generated_outputs:
            with self.subTest(source_variable=variable):
                self.assertIn(f'"${{{variable}}}"', memochat_proto_sources)

    def test_gate_server_cmake_does_not_reference_h5_deleted_local_artifacts(self):
        gate_cmake = strip_cmake_comments(read(SERVER_CORE / "GateServer" / "CMakeLists.txt"))
        for artifact in H5_STALE_GATE_ARTIFACTS:
            with self.subTest(artifact=artifact):
                self.assertNotIn(artifact, gate_cmake)

    def test_route_registration_surfaces_are_still_transport_specific(self):
        logic_header = read(SERVER_CORE / "GateServer" / "LogicSystem.h")
        h2_header = read(GATE_H2_ROUTES / "Http2Routes.h")
        h1_header = read(GATE_H1_LEGACY / "H1LogicSystem.h")

        self.assertIn("std::shared_ptr<HttpConnection>", logic_header)
        self.assertIn("std::shared_ptr<GateHttp3Connection>", logic_header)
        self.assertIn("static void RegisterHandler(const std::string& method", h2_header)
        self.assertIn("std::shared_ptr<H1Connection>", h1_header)

    def test_neutral_routing_contract_files_exist_and_stay_transport_neutral(self):
        routing_dir = SERVER_CORE / "GateServer" / "routing"
        expected = (
            "GateRequest.h",
            "GateResponse.h",
            "RouteModule.h",
            "RouteRegistry.h",
            "RouteRegistry.cpp",
        )
        for name in expected:
            with self.subTest(name=name):
                self.assertTrue((routing_dir / name).exists())

        request_header = read(routing_dir / "GateRequest.h")
        response_header = read(routing_dir / "GateResponse.h")
        registry_header = read(routing_dir / "RouteRegistry.h")
        registry_source = read(routing_dir / "RouteRegistry.cpp")
        module_header = read(routing_dir / "RouteModule.h")
        combined = "\n".join((request_header, response_header, registry_header, registry_source, module_header))

        self.assertIn("namespace memochat::gate::routing", combined)
        self.assertIn("struct GateRequest", request_header)
        self.assertIn("std::string method", request_header)
        self.assertIn("std::string path", request_header)
        self.assertIn("std::unordered_map<std::string, std::string> query", request_header)
        self.assertIn("struct GateResponse", response_header)
        self.assertIn("int status = 200", response_header)
        self.assertIn("std::string content_type", response_header)
        self.assertIn("std::unordered_map<std::string, std::string> headers", response_header)
        self.assertIn("std::string body", response_header)
        self.assertIn("using RouteHandler = std::function<bool(const GateRequest&, GateResponse&)>", registry_header)
        self.assertIn("void Register(std::string method, std::string path, RouteHandler handler)", registry_header)
        self.assertIn(
            "void RegisterPrefix(std::string method, std::string prefix, RouteHandler handler)", registry_header
        )
        self.assertIn("bool Dispatch(const GateRequest& request, GateResponse& response) const", registry_header)
        self.assertIn("virtual void RegisterRoutes(RouteRegistry& registry) = 0", module_header)

        forbidden_transport_tokens = ("boost/beast", "HttpConnection", "GateHttp3Connection", "nghttp2", "msquic")
        for token in forbidden_transport_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, combined)

    def test_transport_adapter_contract_doc_exists_and_locks_g1_sections(self):
        contract_path = SERVER_CORE / "GateServer" / "routing" / "TransportAdapterContract.md"
        self.assertTrue(contract_path.exists())

        source = read(contract_path)
        required_tokens = (
            "# GateServer Transport Adapter Contract",
            "Ownership Rule",
            "Transport owns protocol mechanics, modules/services own business behavior.",
            "GateRequest Mapping",
            "GateResponse Mapping",
            "H1 Adapter",
            "H2 Adapter",
            "H3 Adapter",
            "Legacy Adapter Exceptions",
            "Later Slice Gates",
            "must not directly call",
            "RouteRegistry",
            "GateResponseBodyKind::File",
        )
        for token in required_tokens:
            with self.subTest(token=token):
                self.assertIn(token, source)

        forbidden_direct_call_tokens = (
            "PostgresMgr",
            "MongoMgr",
            "RedisMgr",
            "VerifyGrpcClient",
            "CallService",
            "Http2MediaSupport",
            "Http2ProfileSupport",
            "R18SourceService",
            "AIServiceClient",
        )
        for token in forbidden_direct_call_tokens:
            with self.subTest(forbidden_direct_call_token=token):
                self.assertIn(token, source)

    def test_h1_logic_system_uses_adapter_after_legacy_handlers(self):
        source = read(SERVER_CORE / "GateServer" / "LogicSystem.cpp")

        for token in (
            '#include "adapters/h1/H1RouteAdapter.h"',
            "H1RouteAdapter::BuildGateRequest",
            "H1RouteAdapter::ApplyGateResponse",
            "_route_registry.Dispatch",
        ):
            with self.subTest(token=token):
                self.assertIn(token, source)

        self.assertNotRegex(
            source,
            r"memochat::gate::routing::GateRequest\s+LogicSystem::BuildGateRequest\s*\(",
        )
        self.assertNotRegex(source, r"void\s+LogicSystem::ApplyGateResponse\s*\(")

        for method, handler_map, prefix_map in (
            ("Get", "_get_handlers", "_get_prefix_handlers"),
            ("Post", "_post_handlers", "_post_prefix_handlers"),
            ("Delete", "_delete_handlers", "_delete_prefix_handlers"),
        ):
            with self.subTest(method=method):
                body = extract_function_body(
                    source,
                    rf"bool\s+LogicSystem::Handle{method}\s*\(\s*std::string\s+path\s*,\s*std::shared_ptr<HttpConnection>\s+con\s*\)",
                )
                compact = normalize_space(body)
                self.assertIn(handler_map, body)
                self.assertIn(prefix_map, body)
                self.assertIn("H1RouteAdapter::BuildGateRequest", compact)
                self.assertIn("H1RouteAdapter::ApplyGateResponse", compact)
                self.assertIn("_route_registry.Dispatch(request, response)", compact)
                self.assertLess(compact.index(handler_map), compact.index("_route_registry.Dispatch"))
                self.assertLess(compact.index(prefix_map), compact.index("_route_registry.Dispatch"))

    def test_h2_routes_keep_current_adapter_source_shape_and_map_dispatch(self):
        header = strip_comments(read(GATE_H2_ROUTES / "Http2Routes.h"))
        source = strip_comments(read(GATE_H2_ROUTES / "Http2Routes.cpp"))
        compact_source = normalize_space(source)

        request_match = re.search(r"struct\s+Http2Request\s*\{(?P<body>.*?)\};", header, re.S)
        self.assertIsNotNone(request_match)
        request_body = request_match.group("body")
        for field in ("method", "path", "query", "body", "remote_addr", "trace_id", "headers"):
            with self.subTest(request_field=field):
                self.assertRegex(request_body, rf"\b{re.escape(field)}\b")

        response_match = re.search(r"struct\s+Http2Response\s*\{(?P<body>.*?)\n\};", header, re.S)
        self.assertIsNotNone(response_match)
        response_body = response_match.group("body")
        for field in ("status_code", "status_message", "body", "content_type", "headers"):
            with self.subTest(response_field=field):
                self.assertRegex(response_body, rf"\b{re.escape(field)}\b")

        self.assertIn("static void HandleRequest(const Http2Request& req, Http2Response& resp)", header)
        self.assertIn(
            "static void RegisterHandler(const std::string& method, const std::string& path, Http2Handler handler)",
            header,
        )
        self.assertIn("void Http2Routes::RegisterHandler", source)
        self.assertIn("void Http2Routes::HandleRequest", source)
        self.assertIn("static std::unordered_map<RouteKey, Http2Handler, RouteKeyHash> g_routes", source)
        self.assertIn("g_routes[{method, path}] = std::move(handler)", compact_source)
        self.assertIn("auto it = g_routes.find(key)", compact_source)
        self.assertIn("it->second(req, resp)", compact_source)

    def test_h2_route_adapter_owns_request_response_and_registry_dispatch_mapping(self):
        adapter_dir = GATE_H2 / "adapters" / "h2"
        header_path = adapter_dir / "H2RouteAdapter.h"
        source_path = adapter_dir / "H2RouteAdapter.cpp"

        self.assertTrue(header_path.exists())
        self.assertTrue(source_path.exists())

        header = strip_comments(read(header_path))
        raw_source = read(source_path)
        source = strip_comments(read(source_path))
        combined = header + "\n" + source
        compact_header = normalize_space(header)

        self.assertTrue(raw_source.startswith('#include "WinCompat.h"'))
        self.assertIn("namespace memochat::gate::adapters::h2", combined)
        self.assertIn("class H2RouteAdapter", header)
        self.assertRegex(
            compact_header,
            r"static memochat::gate::routing::GateRequest BuildGateRequest\s*\(\s*const Http2Request&",
        )
        self.assertRegex(
            compact_header,
            r"static void ApplyGateResponse\s*\(\s*const memochat::gate::routing::GateResponse&",
        )
        self.assertRegex(
            compact_header,
            r"static bool Dispatch\s*\(\s*const Http2Request&",
        )

        request_body = extract_function_body(
            source,
            r"memochat::gate::routing::GateRequest\s+H2RouteAdapter::BuildGateRequest\s*\(",
        )
        apply_body = extract_function_body(
            source,
            r"void\s+H2RouteAdapter::ApplyGateResponse\s*\(",
        )
        dispatch_body = extract_function_body(
            source,
            r"bool\s+H2RouteAdapter::Dispatch\s*\(",
        )
        compact_dispatch = normalize_space(dispatch_body)

        for token in (
            "request.method",
            "request.path",
            "request.query",
            "request.headers",
            "request.body",
            "request.trace_id",
            "request.remote_addr",
            "remote_address",
        ):
            with self.subTest(request_mapping_token=token):
                self.assertIn(token, request_body)

        for token in (
            "route_response.status",
            "SetStatus",
            "route_response.content_type",
            "route_response.headers",
            "GateResponseBodyKind::Inline",
            "route_response.body",
            "GateResponseBodyKind::File",
        ):
            with self.subTest(response_mapping_token=token):
                self.assertIn(token, apply_body)

        self.assertIn("TraceContext", dispatch_body)
        self.assertIn("registry.Dispatch", compact_dispatch)
        self.assertLess(compact_dispatch.index("TraceContext"), compact_dispatch.index("registry.Dispatch"))

    def test_h2_health_routes_delegate_through_shared_registry_adapter(self):
        source = strip_comments(read(GATE_H2_ROUTES / "Http2Routes.cpp"))
        compact_source = normalize_space(source)

        for token in (
            "adapters/h2/H2RouteAdapter.h",
            "modules/health/HealthRouteModule.h",
            "RouteRegistry",
            'HealthRouteModule("GateServerHttp2")',
        ):
            with self.subTest(token=token):
                self.assertIn(token, source)

        for route in ("/healthz", "/readyz"):
            with self.subTest(route=route):
                self.assertRegex(
                    compact_source,
                    rf'RegisterHandler\s*\(\s*"GET"\s*,\s*"{re.escape(route)}".*?DispatchSharedRoute\s*\(',
                )

        dispatch_shared_body = extract_function_body(
            source,
            r"void\s+DispatchSharedRoute\s*\(",
        )
        self.assertIn("H2RouteAdapter::Dispatch", dispatch_shared_body)
        self.assertIn("SharedRouteRegistry()", dispatch_shared_body)

    def test_h3_route_adapter_files_exist_and_declare_contract(self):
        adapter_dir = GATE_H3 / "adapters" / "h3"
        header_path = adapter_dir / "H3RouteAdapter.h"
        source_path = adapter_dir / "H3RouteAdapter.cpp"

        self.assertTrue(header_path.exists())
        self.assertTrue(source_path.exists())

        header = strip_comments(read(header_path))
        source = strip_comments(read(source_path))
        combined = header + "\n" + source
        compact_header = normalize_space(header)

        self.assertIn("namespace memochat::gate::adapters::h3", combined)
        self.assertIn("class H3RouteAdapter", header)
        self.assertRegex(
            compact_header,
            r"static memochat::gate::routing::GateRequest BuildGateRequest\s*\(\s*"
            r"const std::shared_ptr<GateHttp3Connection>&",
        )
        self.assertRegex(
            compact_header,
            r"static void ApplyGateResponse\s*\(\s*"
            r"const memochat::gate::routing::GateResponse&\s+route_response\s*,\s*"
            r"const std::shared_ptr<GateHttp3Connection>&\s+connection",
        )
        self.assertRegex(
            compact_header,
            r"static bool Dispatch\s*\(\s*"
            r"const std::shared_ptr<GateHttp3Connection>&\s+connection\s*,\s*"
            r"const memochat::gate::routing::RouteRegistry&\s+registry",
        )

    def test_h3_route_adapter_owns_request_response_and_registry_dispatch_mapping(self):
        adapter_dir = GATE_H3 / "adapters" / "h3"
        header_path = adapter_dir / "H3RouteAdapter.h"
        source_path = adapter_dir / "H3RouteAdapter.cpp"

        self.assertTrue(header_path.exists())
        self.assertTrue(source_path.exists())

        source = strip_comments(read(source_path))

        request_body = extract_function_body(
            source,
            r"(?:memochat::gate::routing::)?GateRequest\s+H3RouteAdapter::BuildGateRequest\s*\(",
        )
        apply_body = extract_function_body(
            source,
            r"void\s+H3RouteAdapter::ApplyGateResponse\s*\(",
        )
        dispatch_body = extract_function_body(
            source,
            r"bool\s+H3RouteAdapter::Dispatch\s*\(",
        )
        compact_dispatch = normalize_space(dispatch_body)

        for token in (
            "GetRequestMethod",
            "GetRequestPath",
            "GetQueryString",
            "GetRequestHeaders",
            "GetRequestBody",
            "GetTraceId",
            "GetRequestId",
            "request_id",
            "target",
            "query",
        ):
            with self.subTest(request_mapping_token=token):
                self.assertIn(token, request_body)

        for token in (
            "route_response.status",
            "route_response.content_type",
            "GateResponseBodyKind::Inline",
            "route_response.body",
            "GateResponseBodyKind::File",
            "SendResponse",
            "501",
        ):
            with self.subTest(response_mapping_token=token):
                self.assertIn(token, apply_body)
        self.assertIn("file", apply_body.lower())
        assert_contains_any(self, apply_body.lower(), "H3 file response limitation", ("limitation", "not supported"))

        self.assertIn("TraceContext::SetTraceId", dispatch_body)
        self.assertIn("TraceContext::SetRequestId", dispatch_body)
        self.assertIn("registry.Dispatch", compact_dispatch)
        self.assertIn("ApplyGateResponse", dispatch_body)
        self.assertLess(compact_dispatch.index("TraceContext::SetTraceId"), compact_dispatch.index("registry.Dispatch"))
        self.assertLess(
            compact_dispatch.index("TraceContext::SetRequestId"),
            compact_dispatch.index("registry.Dispatch"),
        )

    def test_h3_health_routes_delegate_through_shared_registry_adapter(self):
        source = strip_comments(read(GATE_H3_LEGACY_ROUTES / "GateHttp3ServiceRoutes.cpp"))
        compact_source = normalize_space(source)

        for token in (
            '#include "adapters/h3/H3RouteAdapter.h"',
            '#include "modules/health/HealthRouteModule.h"',
            '#include "routing/RouteRegistry.h"',
            "RouteRegistry",
            'HealthRouteModule("GateServer-HTTP3")',
            "SharedRouteRegistry()",
            "H3RouteAdapter::Dispatch",
        ):
            with self.subTest(token=token):
                self.assertIn(token, source)

        ensure_registry_body = extract_function_body(
            source,
            r"void\s+EnsureSharedRouteRegistry\s*\(",
        )
        self.assertIn('HealthRouteModule("GateServer-HTTP3")', ensure_registry_body)
        self.assertIn("RegisterRoutes", ensure_registry_body)
        self.assertIn("SharedRouteRegistry()", ensure_registry_body)
        self.assertNotIn("const_cast", ensure_registry_body)

        dispatch_body = extract_function_body(source, r"bool\s+DispatchSharedRoute\s*\(")
        self.assertIn("EnsureSharedRouteRegistry()", dispatch_body)
        self.assertIn("H3RouteAdapter::Dispatch", dispatch_body)

        for route in ("/healthz", "/readyz"):
            with self.subTest(route=route):
                route_block = extract_logic_registration_block(source, "RegGet", route)
                self.assertIn("DispatchSharedRoute", route_block)
                self.assertNotIn('root["status"]', route_block)
                self.assertNotIn("toStyledString", route_block)

        self.assertNotIn('root["status"] = "ok"', source)
        self.assertNotIn('root["status"] = "ready"', source)

    def test_h3_current_adapter_surface_and_legacy_business_tokens_are_visible(self):
        connection_header = read(GATE_H3_LISTENER / "GateHttp3Connection.h")
        service_source = read(GATE_H3_LEGACY_ROUTES / "GateHttp3ServiceRoutes.cpp")

        for token in (
            "GetRequestPath",
            "GetRequestMethod",
            "GetRequestBody",
            "GetTraceId",
            "GetRequestId",
            "GetRequestHeaders",
            "GetQueryString",
            "GetQueryParam",
            "SendResponse",
        ):
            with self.subTest(connection_token=token):
                self.assertIn(token, connection_header)

        for token in (
            "BuildHttp3AuthRequest",
            "HandleHttp3AuthRoute",
            "GateRequest",
            "GateResponse",
        ):
            with self.subTest(auth_adapter_token=token):
                self.assertIn(token, service_source)

        legacy_business_tokens = (
            "PostgresMgr",
            "MongoMgr",
            "Http2MediaSupport",
        )
        for token in legacy_business_tokens:
            with self.subTest(legacy_business_token=token):
                self.assertIn(token, service_source)

    def test_gate_response_body_kind_supports_inline_and_file_without_transport_tokens(self):
        response_header = strip_comments(read(SERVER_CORE / "GateServer" / "routing" / "GateResponse.h"))
        compact = normalize_space(response_header)

        enum_match = re.search(r"enum\s+class\s+GateResponseBodyKind\s*\{(?P<body>.*?)\};", response_header, re.S)
        self.assertIsNotNone(enum_match)
        enum_values = re.findall(r"\b[A-Za-z_][A-Za-z0-9_]*\b", enum_match.group("body"))
        self.assertIn("Inline", enum_values)
        self.assertIn("File", enum_values)

        self.assertRegex(
            compact, r"struct GateResponse \{.*GateResponseBodyKind body_kind = GateResponseBodyKind::Inline"
        )
        self.assertRegex(compact, r"struct GateResponse \{.*std::string file_path")
        self.assertLess(compact.index("GateResponseBodyKind body_kind"), compact.index("std::string file_path"))

        forbidden_transport_tokens = (
            "HttpConnection",
            "GateHttp3Connection",
            "boost/beast",
            "nghttp2",
            "msquic",
            "http::response",
            "file_body",
        )
        for token in forbidden_transport_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, response_header)

    def test_h1_route_adapter_owns_request_and_response_conversion(self):
        adapter_dir = SERVER_CORE / "GateServer" / "adapters" / "h1"
        header_path = adapter_dir / "H1RouteAdapter.h"
        source_path = adapter_dir / "H1RouteAdapter.cpp"

        self.assertTrue(header_path.exists())
        self.assertTrue(source_path.exists())

        header = strip_comments(read(header_path))
        source = strip_comments(read(source_path))
        combined = header + "\n" + source

        self.assertIn("namespace memochat::gate::adapters::h1", combined)
        self.assertIn("class H1RouteAdapter", header)
        self.assertRegex(
            header,
            r"static\s+memochat::gate::routing::GateRequest\s+BuildGateRequest\s*\(",
        )
        self.assertRegex(header, r"static\s+void\s+ApplyGateResponse\s*\(")

        request_body = extract_function_body(
            source,
            r"memochat::gate::routing::GateRequest\s+H1RouteAdapter::BuildGateRequest\s*\(",
        )
        apply_body = extract_function_body(
            source,
            r"void\s+H1RouteAdapter::ApplyGateResponse\s*\([^)]*route_response[^)]*connection[^)]*\)",
        )
        compact_apply = normalize_space(apply_body)

        for token in (
            "RequestTargetString",
            "RequestBodyString",
            "GetTraceId",
            "GetRequestId",
            "GetParams",
            "GetRequest",
            "GetSocket",
            "remote_endpoint",
        ):
            with self.subTest(request_token=token):
                self.assertIn(token, request_body)

        for token in (
            "route_response.status",
            "route_response.content_type",
            "route_response.headers",
            "GetResponse",
            "GateResponseBodyKind::File",
            "SetFileResponse",
            "route_response.body",
        ):
            with self.subTest(response_token=token):
                self.assertIn(token, apply_body)
        self.assertIn("return", compact_apply[compact_apply.index("SetFileResponse") :])

    def test_profile_route_module_registers_h1_profile_update_neutrally(self):
        profile_dir = SERVER_CORE / "GateServer" / "modules" / "profile"
        header_path = profile_dir / "ProfileRouteModule.h"
        source_path = profile_dir / "ProfileRouteModule.cpp"

        self.assertTrue(header_path.exists())
        self.assertTrue(source_path.exists())

        header = strip_comments(read(header_path))
        source = strip_comments(read(source_path))
        compact_source = normalize_space(source)
        combined = header + "\n" + source

        self.assertIn("namespace memochat::gate::modules::profile", combined)
        self.assertIn("class ProfileRouteModule final", header)
        self.assertIn("public memochat::gate::routing::RouteModule", header)
        self.assertIn("void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override", header)
        assert_registry_registration(self, source, "POST", "/user_update_profile")

        expected_behavior_tokens = (
            "UpdateUserProfile",
            "ubaseinfo_",
            "nameinfo_",
            "InvalidateLoginCacheByUid",
            "ErrorCodes::ProfileUpFailed",
            'response.content_type = "application/json"',
        )
        for token in expected_behavior_tokens:
            with self.subTest(token=token):
                self.assertIn(token, source)

        forbidden_transport_tokens = (
            "HttpConnection",
            "GateHttp3Connection",
            "boost/beast",
            "nghttp2",
            "msquic",
            "GateHttpJsonSupport",
        )
        for token in forbidden_transport_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, combined)

    def test_health_route_module_registers_neutral_health_routes(self):
        health_dir = SERVER_CORE / "GateServer" / "modules" / "health"
        header = read(health_dir / "HealthRouteModule.h")
        source = read(health_dir / "HealthRouteModule.cpp")
        combined = header + "\n" + source

        self.assertIn("class HealthRouteModule final", header)
        self.assertIn("public memochat::gate::routing::RouteModule", header)
        self.assertIn("void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override", header)
        self.assertIn('registry.Register("GET",', source)
        self.assertIn('"/healthz"', source)
        self.assertIn('"/readyz"', source)
        self.assertIn('root["status"] = "ok"', source)
        self.assertIn('root["status"] = "ready"', source)
        self.assertIn('root["service"] = service_name', source)
        self.assertIn('response.content_type = "application/json"', source)

        forbidden_transport_tokens = ("boost/beast", "HttpConnection", "GateHttp3Connection", "nghttp2", "msquic")
        for token in forbidden_transport_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, combined)

    def test_auth_service_facade_files_exist_and_are_built_into_gate_server(self):
        auth_service_dir = SERVER_CORE / "GateServer" / "services" / "auth"
        account_service_dir = SERVER_CORE / "GateServer" / "services" / "account"
        self.assertTrue((auth_service_dir / "AuthService.h").exists())
        self.assertTrue((auth_service_dir / "AuthService.cpp").exists())
        self.assertTrue((account_service_dir / "AccountPersistence.h").exists())
        self.assertTrue((account_service_dir / "AccountPersistence.cpp").exists())

        cmake_source = read(SERVER_CORE / "GateServer" / "CMakeLists.txt")
        self.assertIn("services/auth/AuthService.cpp", cmake_source)
        self.assertIn("services/account/AccountPersistence.cpp", cmake_source)

    def test_auth_cache_facade_files_exist_and_gate_core_builds_source(self):
        self.assertTrue((GATE_CORE_CACHE / "AuthCache.h").exists(), "GateServerCore AuthCache header should exist")
        self.assertTrue((GATE_CORE_CACHE / "AuthCache.cpp").exists(), "GateServerCore AuthCache source should exist")

        cmake_source = strip_cmake_comments(read(GATE_CORE / "CMakeLists.txt"))
        target_match = re.search(r"add_library\s*\(\s*GateAccountCore\s+STATIC(?P<body>.*?)\)", cmake_source, re.S)
        self.assertIsNotNone(target_match, "GateAccountCore static library source list should be explicit")
        self.assertIn("cache/AuthCache.cpp", target_match.group("body"))

    def test_auth_cache_cpp_owns_h3_auth_redis_key_construction(self):
        source = strip_comments(read(GATE_CORE_CACHE / "AuthCache.cpp"))
        compact_source = normalize_space(source)

        for label, pattern in AUTH_REDIS_KEY_CONSTRUCTION_PATTERNS.items():
            with self.subTest(label=label):
                self.assertRegex(compact_source, pattern)

    def test_auth_cache_public_header_stays_storage_only_and_dependency_light(self):
        header = strip_comments(read(GATE_CORE_CACHE / "AuthCache.h"))

        self.assertIn("class AuthCache", header)
        self.assertIn("static AuthCache& Instance()", header)
        for token in AUTH_CACHE_FORBIDDEN_HEADER_TOKENS:
            with self.subTest(token=token):
                self.assertNotIn(token, header)

    def test_auth_verify_client_facade_files_exist_and_gate_core_builds_source(self):
        self.assertTrue(
            (GATE_CORE_CLIENTS / "AuthVerifyClient.h").exists(), "GateServerCore AuthVerifyClient header should exist"
        )
        self.assertTrue(
            (GATE_CORE_CLIENTS / "AuthVerifyClient.cpp").exists(), "GateServerCore AuthVerifyClient source should exist"
        )

        cmake_source = strip_cmake_comments(read(GATE_CORE / "CMakeLists.txt"))
        target_match = re.search(r"add_library\s*\(\s*GateInfraCore\s+STATIC(?P<body>.*?)\)", cmake_source, re.S)
        self.assertIsNotNone(target_match, "GateInfraCore static library source list should be explicit")
        self.assertIn("clients/AuthVerifyClient.cpp", target_match.group("body"))

    def test_auth_verify_client_public_header_is_narrow_verify_request_contract(self):
        header = strip_comments(read(GATE_CORE_CLIENTS / "AuthVerifyClient.h"))

        self.assertIn("class AuthVerifyClient", header)
        self.assertIn("static AuthVerifyClient& Instance()", header)
        self.assertRegex(header, r"\bRequestVerifyCode\s*\(")
        self.assertIn("std::string", header)
        self.assertRegex(header, r"\bint\s+\w*error\w*")
        self.assertNotIn("GetVarifyCode", header)

        for token in AUTH_VERIFY_CLIENT_FORBIDDEN_HEADER_TOKENS:
            with self.subTest(token=token):
                self.assertNotIn(token, header)

    def test_auth_verify_client_cpp_delegates_to_verify_grpc_only_for_code_request(self):
        source = strip_comments(read(GATE_CORE_CLIENTS / "AuthVerifyClient.cpp"))
        compact_source = normalize_space(source)

        self.assertIn('#include "AuthVerifyClient.h"', source)
        self.assertIn('#include "VerifyGrpcClient.h"', source)
        self.assertIn("AuthVerifyClient::Instance()", source)
        self.assertRegex(source, r"\bAuthVerifyClient::RequestVerifyCode\s*\(")
        self.assertIn("VerifyGrpcClient::GetInstance()->GetVarifyCode(email)", compact_source)
        self.assertRegex(compact_source, r"\.error\s*=\s*rsp\.error\s*\(\s*\)")

        for token in AUTH_VERIFY_CLIENT_FORBIDDEN_CPP_TOKENS:
            with self.subTest(token=token):
                self.assertNotIn(token, source)

    def test_auth_verify_client_callers_do_not_use_verify_grpc_directly(self):
        caller_paths = (
            SERVER_CORE / "GateServer" / "services" / "auth" / "AuthService.cpp",
            GATE_H2_SUPPORT / "Http2AuthSupport.cpp",
        )
        forbidden_tokens = (
            '#include "VerifyGrpcClient.h"',
            '#include "../GateServerCore/VerifyGrpcClient.h"',
            "VerifyGrpcClient::GetInstance()",
            "->GetVarifyCode(",
            "GetVarifyRsp",
        )

        for path in caller_paths:
            source = strip_comments(read(path))
            compact_source = normalize_space(source)
            with self.subTest(path=path.relative_to(REPO_ROOT), expectation="uses facade"):
                self.assertRegex(source, r'#include\s+"AuthVerifyClient\.h"')
                self.assertIn("AuthVerifyClient::Instance()", source)
                self.assertRegex(compact_source, r"\.RequestVerifyCode\s*\(\s*email\s*\)")
            for token in forbidden_tokens:
                with self.subTest(path=path.relative_to(REPO_ROOT), forbidden=token):
                    self.assertNotIn(token, source)

    def test_auth_redis_key_construction_is_not_reintroduced_in_auth_callers(self):
        forbidden_regexes = {
            "verification code key construction": r"\bCODEPREFIX\s*\+",
            "http token key construction": r"\bUSERTOKENPREFIX\s*\+",
            "BuildLoginCacheKey definition": r"\b(?:std::string|auto)\s+BuildLoginCacheKey\s*\(",
            "BuildLoginCacheUidKey definition": r"\b(?:std::string|auto)\s+BuildLoginCacheUidKey\s*\(",
        }
        forbidden_literals = (
            '"ulogin_profile_"',
            '"ulogin_profile_uid_"',
        )

        for path in AUTH_REDIS_KEY_CALLER_PATHS:
            source = strip_comments(read(path))
            with self.subTest(path=path.relative_to(REPO_ROOT), kind="regex"):
                for label, pattern in forbidden_regexes.items():
                    with self.subTest(label=label):
                        self.assertIsNone(re.search(pattern, source))
            with self.subTest(path=path.relative_to(REPO_ROOT), kind="literal"):
                for token in forbidden_literals:
                    with self.subTest(token=token):
                        self.assertNotIn(token, source)

    def test_account_persistence_public_header_is_narrow_account_contract(self):
        header = strip_comments(read(SERVER_CORE / "GateServer" / "services" / "account" / "AccountPersistence.h"))

        self.assertIn("struct AccountProfile", header)
        self.assertIn("class AccountPersistence", header)
        self.assertIn("static AccountPersistence& Instance()", header)
        self.assertIn("RegisterUser", header)
        self.assertIn("GetUserPublicId", header)
        self.assertIn("EmailMatchesUser", header)
        self.assertIn("UpdatePassword", header)
        self.assertIn("CheckPassword", header)

        expected_profile_fields = (
            "name",
            "password",
            "uid",
            "user_id",
            "email",
            "nick",
            "icon",
            "desc",
            "sex",
        )
        for field in expected_profile_fields:
            with self.subTest(profile_field=field):
                self.assertRegex(header, rf"\b{re.escape(field)}\b")

        forbidden_header_tokens = (
            "PostgresMgr.h",
            "PostgresDao.h",
            "PostgresMgr",
            "PostgresDao",
            "Redis",
            "RedisMgr",
            "VerifyGrpcClient",
            "HttpConnection",
            "GateHttp3Connection",
            "boost/beast",
            "boost/asio",
            "nghttp2",
            "msquic",
            "GateHttpJsonSupport",
            "Json",
            "json/",
            "transport",
            "GateRequest",
            "GateResponse",
        )
        for token in forbidden_header_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, header)

    def test_account_persistence_cpp_adapts_h1_account_calls_to_postgres_mgr(self):
        source = strip_comments(read(SERVER_CORE / "GateServer" / "services" / "account" / "AccountPersistence.cpp"))
        compact_source = normalize_space(source)

        self.assertIn('#include "services/account/AccountPersistence.h"', source)
        self.assertIn('#include "PostgresMgr.h"', source)
        self.assertIn("AccountPersistence::Instance()", source)
        self.assertIn("static AccountPersistence instance", source)
        self.assertIn("AccountProfile ToAccountProfile(const ::UserInfo& userInfo)", source)

        expected_postgres_calls = {
            "register": "PostgresMgr::GetInstance()->RegUser(name, email, password, icon)",
            "public id": "PostgresMgr::GetInstance()->GetUserPublicId(uid)",
            "email check": "PostgresMgr::GetInstance()->CheckEmail(name, email)",
            "password update": "PostgresMgr::GetInstance()->UpdatePwd(email, password)",
            "password check": "PostgresMgr::GetInstance()->CheckPwd(email, password, userInfo)",
        }
        for label, token in expected_postgres_calls.items():
            with self.subTest(label=label):
                self.assertIn(token, compact_source)

        expected_profile_mappings = (
            "profile.name = userInfo.name",
            "profile.password = userInfo.pwd",
            "profile.uid = userInfo.uid",
            "profile.user_id = userInfo.user_id",
            "profile.email = userInfo.email",
            "profile.nick = userInfo.nick",
            "profile.icon = userInfo.icon",
            "profile.desc = userInfo.desc",
            "profile.sex = userInfo.sex",
        )
        for mapping in expected_profile_mappings:
            with self.subTest(mapping=mapping):
                self.assertIn(mapping, compact_source)

    def test_auth_route_module_registers_migrated_auth_routes_through_service_facade(self):
        auth_dir = SERVER_CORE / "GateServer" / "modules" / "auth"
        header = read(auth_dir / "AuthRouteModule.h")
        source = strip_comments(read(auth_dir / "AuthRouteModule.cpp"))
        compact_source = normalize_space(source)
        combined = strip_comments(header) + "\n" + source

        self.assertIn("class AuthRouteModule final", header)
        self.assertIn("public memochat::gate::routing::RouteModule", header)
        self.assertIn("void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override", header)
        assert_registry_registration(self, source, "POST", "/get_varifycode")
        assert_registry_registration(self, source, "POST", "/user_register")
        assert_registry_registration(self, source, "POST", "/reset_pwd")
        assert_registry_registration(self, source, "POST", "/user_login")

        self.assertIn("AuthService::Instance()", source)
        self.assertIn("HandleGetVarifyCode(request, response)", compact_source)
        self.assertIn("HandleUserRegister(request, response)", compact_source)
        self.assertIn("HandleResetPwd(request, response)", compact_source)
        self.assertIn("HandleUserLogin(request, response)", compact_source)

        forbidden_route_module_tokens = (
            "VerifyGrpcClient",
            "RedisMgr",
            "PostgresMgr",
            "GateAsyncSideEffects",
            "GateWorkerPool",
            "ChatLoginTicket",
            "TraceContext",
            "USERTOKENPREFIX",
            "CODEPREFIX",
            "boost::uuids::random_generator",
            "HttpConnection",
            "GateHttp3Connection",
            "boost/beast",
            "nghttp2",
            "msquic",
        )
        for token in forbidden_route_module_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, combined)

    def test_auth_service_facade_owns_h1_auth_behavior_and_infrastructure_tokens(self):
        service_source = strip_comments(read(SERVER_CORE / "GateServer" / "services" / "auth" / "AuthService.cpp"))
        service_header = read(SERVER_CORE / "GateServer" / "services" / "auth" / "AuthService.h")

        self.assertIn("class AuthService", service_header)
        self.assertIn("static AuthService& Instance()", service_header)
        self.assertIn("HandleGetVarifyCode", service_header)
        self.assertIn("HandleUserRegister", service_header)
        self.assertIn("HandleResetPwd", service_header)
        self.assertIn("HandleUserLogin", service_header)

        expected_behavior_categories = {
            "verify request facade include": ('#include "AuthVerifyClient.h"',),
            "verify request facade call": ("AuthVerifyClient::Instance().RequestVerifyCode(email)",),
            "account persistence facade include": ('#include "services/account/AccountPersistence.h"',),
            "account persistence facade entry": (
                "account::AccountPersistence::Instance()",
                "AccountPersistence::Instance()",
            ),
            "register persistence facade": (
                ".RegisterUser(name, email, pwd, icon)",
                "RegisterUser(name, email, pwd, icon)",
            ),
            "auth cache facade": ('#include "AuthCache.h"', "AuthCache::Instance()", "AuthCache"),
            "verification code cache facade": ("GetVerificationCode", "LoadVerificationCode", "AuthCache::Instance()"),
            "reset email check facade": ("EmailMatchesUser(name, email)",),
            "reset password update facade": ("UpdatePassword(email, pwd)",),
            "reset cache invalidation": ("gateauthsupport::InvalidateLoginCacheByEmail", "PublishCacheInvalidate"),
            "login version gate": ("gateauthsupport::IsClientVersionAllowed",),
            "login profile cache": ("gateauthsupport::TryLoadCachedLoginProfile", "gateauthsupport::CacheLoginProfile"),
            "login password check facade": ("CheckPassword(email, pwd, dbUser)",),
            "route selection": ("gateauthsupport::SelectChatRouteForLogin",),
            "http token cache facade": ("GetHttpToken", "SetHttpToken", "AuthCache::Instance()"),
            "random token generation": ("boost::uuids::random_generator",),
            "login ticket claims": ("ChatLoginTicketClaims",),
            "login ticket encoding": ("EncodeTicket",),
            "login ticket response": ('root["login_ticket"]',),
            "async audit": ("GateWorkerPool::GetInstance()->post", "PublishAuditLogin"),
        }
        for label, tokens in expected_behavior_categories.items():
            with self.subTest(label=label):
                assert_contains_any(self, service_source, label, tokens)

        forbidden_direct_persistence_tokens = (
            '#include "PostgresMgr.h"',
            '#include "PostgresDao.h"',
            "PostgresMgr::GetInstance()->RegUser",
            "PostgresMgr::GetInstance()->GetUserPublicId",
            "PostgresMgr::GetInstance()->CheckEmail",
            "PostgresMgr::GetInstance()->UpdatePwd",
            "PostgresMgr::GetInstance()->CheckPwd",
        )
        for token in forbidden_direct_persistence_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, service_source)

        for token in ("VerifyGrpcClient", "->GetVarifyCode(", "GetVarifyRsp"):
            with self.subTest(forbidden_direct_verify_token=token):
                self.assertNotIn(token, service_source)

    def test_h3_auth_routes_are_thin_auth_service_adapters(self):
        source = strip_comments(read(GATE_H3_LEGACY_ROUTES / "GateHttp3ServiceRoutes.cpp"))

        self.assertIn("services/auth/AuthService.h", source)
        self.assertIn("memochat::gate::routing::GateRequest", source)
        self.assertIn("memochat::gate::routing::GateResponse", source)
        self.assertIn("connection->GetRequestPath()", source)
        self.assertIn("connection->GetRequestBody()", source)
        self.assertIn("connection->GetTraceId()", source)
        self.assertIn("connection->GetRequestId()", source)
        self.assertIn("connection->GetRequestHeaders()", source)
        self.assertIn("memolog::TraceContext::SetTraceId(request.trace_id)", source)
        self.assertIn("memolog::TraceContext::SetRequestId(request.request_id)", source)

        forbidden_auth_block_tokens = (
            "VerifyGrpcClient",
            "RedisMgr",
            "PostgresMgr",
            "GateAsyncSideEffects",
            "GateWorkerPool",
            "ChatLoginTicket",
            "USERTOKENPREFIX",
            "CODEPREFIX",
            "boost::uuids::random_generator",
            "gateauthsupport::",
        )
        for route, handler in H3_AUTH_ROUTES.items():
            with self.subTest(route=route):
                block = extract_logic_route_block(source, route)
                self.assertIn("AuthService", block)
                self.assertIn(handler, block)
                for token in forbidden_auth_block_tokens:
                    self.assertNotIn(token, block)

    def test_logic_system_bridges_health_routes_through_neutral_registry(self):
        header = read(SERVER_CORE / "GateServer" / "LogicSystem.h")
        source = read(SERVER_CORE / "GateServer" / "LogicSystem.cpp")

        self.assertIn('#include "routing/RouteRegistry.h"', header)
        self.assertIn("memochat::gate::routing::RouteRegistry _route_registry", header)
        self.assertNotIn("BuildGateRequest", header)
        self.assertNotIn("ApplyGateResponse", header)

        self.assertIn('#include "adapters/h1/H1RouteAdapter.h"', source)
        self.assertIn('#include "modules/health/HealthRouteModule.h"', source)
        self.assertIn('#include "modules/auth/AuthRouteModule.h"', source)
        self.assertIn("HealthRouteModule().RegisterRoutes(_route_registry)", source)
        # Phase 5 split: auth registers via granular register/login entry points.
        self.assertIn("AuthRouteModule::RegisterRegisterRoutes(_route_registry)", source)
        self.assertIn("AuthRouteModule::RegisterLoginRoutes(_route_registry)", source)
        self.assertIn("ApplyRouteTraceContext", source)
        self.assertIn("memolog::TraceContext::SetTraceId(request.trace_id)", source)
        self.assertIn("memolog::TraceContext::SetRequestId(request.request_id)", source)
        self.assertIn('H1RouteAdapter::BuildGateRequest("GET", path, con)', source)
        self.assertIn('H1RouteAdapter::BuildGateRequest("POST", path, con)', source)
        self.assertIn("_route_registry.Dispatch(request, response)", source)
        self.assertIn("H1RouteAdapter::ApplyGateResponse(response, con)", source)

    def test_logic_system_registers_profile_module_and_legacy_h1_does_not(self):
        logic_source = read(SERVER_CORE / "GateServer" / "LogicSystem.cpp")
        legacy_h1_source = strip_comments(read(SERVER_CORE / "GateServer" / "GateServerH1Routes.cpp"))

        self.assertIn('#include "modules/profile/ProfileRouteModule.h"', logic_source)
        self.assertIn(
            "memochat::gate::modules::profile::ProfileRouteModule().RegisterRoutes(_route_registry)", logic_source
        )
        self.assertLess(
            logic_source.index("AuthRouteModule::RegisterRegisterRoutes(_route_registry)"),
            logic_source.index("ProfileRouteModule().RegisterRoutes(_route_registry)"),
        )
        self.assertIsNone(re.search(r'\blogic\.RegPost\s*\(\s*"/user_update_profile"', legacy_h1_source))

    def test_route_registry_dispatches_exact_and_prefix_routes(self):
        compiler = shutil.which("g++-16") or shutil.which("g++") or shutil.which("c++")
        if compiler is None:
            self.skipTest("C++ compiler not available")

        routing_dir = SERVER_CORE / "GateServer" / "routing"
        source = r"""
#include "routing/RouteRegistry.h"

#include <iostream>

int main()
{
    memochat::gate::routing::RouteRegistry registry;
    registry.Register("get",
                      "/healthz",
                      [](const memochat::gate::routing::GateRequest&, memochat::gate::routing::GateResponse& response)
                      {
                          response.status = 200;
                          response.content_type = "application/json";
                          response.body = "ok";
                          return true;
                      });
    registry.RegisterPrefix("POST",
                            "/ai/games",
                            [](const memochat::gate::routing::GateRequest& request,
                               memochat::gate::routing::GateResponse& response)
                            {
                                response.body = request.path;
                                return true;
                            });

    memochat::gate::routing::GateRequest request;
    memochat::gate::routing::GateResponse response;
    request.method = "GET";
    request.path = "/healthz";
    if (!registry.Dispatch(request, response) || response.status != 200 || response.body != "ok")
    {
        return 1;
    }

    request.method = "post";
    request.path = "/ai/games/rounds";
    response = {};
    if (!registry.Dispatch(request, response) || response.body != "/ai/games/rounds")
    {
        return 2;
    }

    request.path = "/ai/gamesx";
    response = {};
    if (registry.Dispatch(request, response))
    {
        return 3;
    }

    registry.Register("GET",
                      "/false",
                      [](const memochat::gate::routing::GateRequest&, memochat::gate::routing::GateResponse&)
                      {
                          return false;
                      });
    request.method = "GET";
    request.path = "/false";
    if (registry.Dispatch(request, response))
    {
        return 4;
    }

    return 0;
}
"""
        with tempfile.TemporaryDirectory() as tmp:
            cpp_path = Path(tmp) / "route_registry_contract.cpp"
            exe_path = Path(tmp) / "route_registry_contract"
            cpp_path.write_text(source, encoding="utf-8")
            compile_result = subprocess.run(
                [
                    compiler,
                    "-std=c++20",
                    "-I",
                    str(SERVER_CORE / "GateServer"),
                    str(cpp_path),
                    str(routing_dir / "RouteRegistry.cpp"),
                    "-o",
                    str(exe_path),
                ],
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
            )
            self.assertEqual(compile_result.returncode, 0, compile_result.stderr)

            run_result = subprocess.run(
                [str(exe_path)],
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
            )
            self.assertEqual(run_result.returncode, 0, run_result.stderr)


if __name__ == "__main__":
    unittest.main()
