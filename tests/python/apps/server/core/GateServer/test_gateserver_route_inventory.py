import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps" / "server" / "core"
GATE_H3_LEGACY_ROUTES = SERVER_CORE / "GateShared" / "transports" / "h3" / "legacy_routes"
GATE_H2 = SERVER_CORE / "GateShared" / "transports" / "h2"
GATE_H2_ROUTES = GATE_H2 / "routes"
GATE_H2_SUPPORT = GATE_H2 / "support"
GATE_H1_LEGACY = SERVER_CORE / "GateShared" / "transports" / "h1" / "legacy_standalone"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


def strip_comments(source: str) -> str:
    source = re.sub(r"//.*", "", source)
    return re.sub(r"/\*.*?\*/", "", source, flags=re.S)


def normalize_space(source: str) -> str:
    return re.sub(r"\s+", " ", source)


def cxx_string_after(source: str, index: int) -> str | None:
    match = re.search(r'"([^"\\]*(?:\\.[^"\\]*)*)"', source[index:])
    if match is None:
        return None
    return match.group(1)


def extract_logic_routes(path: Path) -> set[tuple[str, str]]:
    source = strip_comments(read(path))
    routes: set[tuple[str, str]] = set()
    method_by_call = {
        "RegGet": "GET",
        "RegGetPrefix": "GET_PREFIX",
        "RegPost": "POST",
        "RegPostPrefix": "POST_PREFIX",
        "RegDelete": "DELETE",
        "RegDeletePrefix": "DELETE_PREFIX",
    }
    for call, method in method_by_call.items():
        for match in re.finditer(rf"\b(?:logic\.)?{call}\s*\(", source):
            route = cxx_string_after(source, match.end())
            if route:
                routes.add((method, route))
    return routes


def extract_route_registry_routes(path: Path) -> set[tuple[str, str]]:
    source = strip_comments(read(path))
    routes: set[tuple[str, str]] = set()
    pattern = re.compile(
        r"(?:_route_registry|registry)\.(?P<call>Register|RegisterPrefix)\s*\(\s*"
        r'"(?P<method>GET|POST|DELETE|PUT|PATCH)"\s*,\s*"(?P<path>[^"]+)"',
        re.S,
    )
    for match in pattern.finditer(source):
        method = match.group("method")
        if match.group("call") == "RegisterPrefix":
            method = f"{method}_PREFIX"
        routes.add((method, match.group("path")))
    return routes


def extract_h2_routes() -> set[tuple[str, str]]:
    routes: set[tuple[str, str]] = set()
    for path in (
        GATE_H2_ROUTES / "Http2Routes.cpp",
        GATE_H2_SUPPORT / "Http2MomentsSupport.cpp",
    ):
        source = strip_comments(read(path))
        pattern = re.compile(
            r"(?:Http2Routes::)?RegisterHandler\s*\(\s*"
            r'"(?P<method>GET|POST|DELETE|PUT|PATCH)"\s*,\s*"(?P<path>[^"]+)"',
            re.S,
        )
        routes.update((match.group("method"), match.group("path")) for match in pattern.finditer(source))
    return routes


MAIN_H1_FILES = (
    SERVER_CORE / "GateShared" / "LogicSystem.cpp",
    SERVER_CORE / "AccountShared" / "domain" / "modules" / "auth" / "AuthRouteModule.cpp",
    SERVER_CORE / "AIGatewayService" / "domain" / "modules" / "ai" / "AIRouteModule.cpp",
    SERVER_CORE / "CallService" / "domain" / "modules" / "call" / "CallRouteModule.cpp",
    SERVER_CORE / "GateShared" / "modules" / "health" / "HealthRouteModule.cpp",
    SERVER_CORE / "MediaService" / "domain" / "modules" / "media" / "MediaRouteModule.cpp",
    SERVER_CORE / "MomentsService" / "domain" / "modules" / "moments" / "MomentsRouteModule.cpp",
    SERVER_CORE / "AccountShared" / "domain" / "modules" / "profile" / "ProfileRouteModule.cpp",
    SERVER_CORE / "R18Service" / "domain" / "modules" / "r18" / "R18RouteModule.cpp",
    SERVER_CORE / "GateShared" / "GateServerH1Routes.cpp",
    SERVER_CORE / "MediaService" / "domain" / "MediaHttpService.cpp",
    SERVER_CORE / "MomentsService" / "domain" / "MomentsRouteModules.cpp",
    SERVER_CORE / "AIGatewayService" / "domain" / "AIRouteModules.cpp",
    SERVER_CORE / "R18Service" / "domain" / "GateR18Routes.cpp",
)

H1_LEGACY_SOURCE_FILES = (
    GATE_H1_LEGACY / "H1LogicSystem.cpp",
    GATE_H1_LEGACY / "H1AuthRoutes.cpp",
    GATE_H1_LEGACY / "H1MediaService.cpp",
    GATE_H1_LEGACY / "H1MomentsRoutes.cpp",
    GATE_H1_LEGACY / "H1R18Routes.cpp",
)

CORE_SHARED_ROUTES = {
    ("GET", "/healthz"),
    ("GET", "/readyz"),
    ("POST", "/get_varifycode"),
    ("POST", "/user_register"),
    ("POST", "/reset_pwd"),
    ("POST", "/user_login"),
    ("POST", "/user_update_profile"),
    ("POST", "/api/call/start"),
    ("POST", "/api/call/accept"),
    ("POST", "/api/call/reject"),
    ("POST", "/api/call/cancel"),
    ("POST", "/api/call/hangup"),
    ("GET", "/api/call/token"),
    ("POST", "/api/call/token"),
    ("GET", "/upload_media_status"),
    ("GET", "/media/download"),
    ("POST", "/upload_media_init"),
    ("POST", "/upload_media_chunk"),
    ("POST", "/upload_media_complete"),
    ("POST", "/upload_media"),
    ("POST", "/api/moments/publish"),
    ("POST", "/api/moments/list"),
    ("POST", "/api/moments/detail"),
    ("POST", "/api/moments/delete"),
    ("POST", "/api/moments/like"),
    ("POST", "/api/moments/comment"),
    ("POST", "/api/moments/comment/list"),
    ("POST", "/api/moments/comment/like"),
}

H2_EXPECTED_ROUTES = CORE_SHARED_ROUTES | {
    ("POST", "/user_logout"),
    ("POST", "/get_user_info"),
}

H3_AUTH_ROUTES = {
    "/get_varifycode": "HandleGetVarifyCode",
    "/user_register": "HandleUserRegister",
    "/reset_pwd": "HandleResetPwd",
    "/user_login": "HandleUserLogin",
}

CALL_STATE_POST_ROUTES = {
    ("POST", "/api/call/start"),
    ("POST", "/api/call/accept"),
    ("POST", "/api/call/reject"),
    ("POST", "/api/call/cancel"),
    ("POST", "/api/call/hangup"),
}

CALL_TOKEN_ROUTES = {
    ("GET", "/api/call/token"),
    ("POST", "/api/call/token"),
}

MEDIA_ROUTES = {
    ("POST", "/upload_media_init"),
    ("POST", "/upload_media_chunk"),
    ("GET", "/upload_media_status"),
    ("POST", "/upload_media_complete"),
    ("POST", "/upload_media"),
    ("GET", "/media/download"),
}

MOMENTS_ROUTES = {
    ("POST", "/api/moments/publish"),
    ("POST", "/api/moments/list"),
    ("POST", "/api/moments/detail"),
    ("POST", "/api/moments/delete"),
    ("POST", "/api/moments/like"),
    ("POST", "/api/moments/comment"),
    ("POST", "/api/moments/comment/list"),
    ("POST", "/api/moments/comment/like"),
}

AI_EXACT_ROUTES = {
    ("POST", "/ai/chat"),
    ("POST", "/ai/smart"),
    ("GET", "/ai/history"),
    ("POST", "/ai/session"),
    ("GET", "/ai/session/list"),
    ("POST", "/ai/session/delete"),
    ("GET", "/ai/model/list"),
    ("POST", "/ai/model/api/register"),
    ("POST", "/ai/model/api/delete"),
    ("POST", "/ai/kb/upload"),
    ("POST", "/ai/kb/search"),
    ("GET", "/ai/kb/list"),
    ("POST", "/ai/kb/delete"),
    ("GET", "/ai/memory/list"),
    ("POST", "/ai/memory"),
    ("POST", "/ai/memory/delete"),
    ("POST", "/ai/tasks"),
    ("GET", "/ai/tasks"),
    ("GET", "/ai/tasks/detail"),
    ("POST", "/ai/tasks/cancel"),
    ("POST", "/ai/tasks/resume"),
}

AI_LEGACY_PREFIX_STREAM_ROUTES = {
    ("GET_PREFIX", "/ai/games"),
    ("POST_PREFIX", "/ai/games"),
    ("DELETE_PREFIX", "/ai/games"),
    ("GET_PREFIX", "/ai/pet"),
    ("POST_PREFIX", "/ai/pet"),
    ("POST", "/ai/chat/stream"),
}

R18_ROUTES = {
    ("GET", "/api/r18/sources"),
    ("POST", "/api/r18/source/import"),
    ("POST", "/api/r18/source/enable"),
    ("POST", "/api/r18/source/disable"),
    ("POST", "/api/r18/search"),
    ("POST", "/api/r18/comic/detail"),
    ("POST", "/api/r18/chapter/pages"),
    ("POST", "/api/r18/favorite/toggle"),
    ("POST", "/api/r18/history/update"),
    ("GET", "/api/r18/history"),
    ("GET", "/api/r18/image"),
}


def extract_logic_route_block(source: str, route: str) -> str:
    source = strip_comments(source)
    match = re.search(rf'\blogic\.RegPost\s*\(\s*"{re.escape(route)}"', source)
    if match is None:
        raise AssertionError(f"missing H3 route block for {route}")

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

    raise AssertionError(f"unterminated H3 route block for {route}")


class GateServerRouteInventoryTests(unittest.TestCase):
    def main_h1_routes(self) -> set[tuple[str, str]]:
        routes: set[tuple[str, str]] = set()
        for path in MAIN_H1_FILES:
            if not path.exists():
                continue
            routes.update(extract_logic_routes(path))
            routes.update(extract_route_registry_routes(path))
        return routes

    def h1_legacy_source_routes(self) -> set[tuple[str, str]]:
        routes: set[tuple[str, str]] = set()
        for path in H1_LEGACY_SOURCE_FILES:
            routes.update(extract_logic_routes(path))
        return routes

    def h3_routes(self) -> set[tuple[str, str]]:
        return extract_logic_routes(GATE_H3_LEGACY_ROUTES / "GateHttp3ServiceRoutes.cpp")

    def test_main_h1_has_current_gateway_route_groups(self):
        routes = self.main_h1_routes()
        missing = CORE_SHARED_ROUTES - routes
        self.assertFalse(missing, f"main H1 route inventory missing {sorted(missing)}")

        expected_h1_only_routes = R18_ROUTES | AI_EXACT_ROUTES | AI_LEGACY_PREFIX_STREAM_ROUTES
        for route in expected_h1_only_routes:
            self.assertIn(route, routes)

    def test_main_h1_ai_inventory_scans_neutral_module_and_legacy_adapter(self):
        ai_module_path = SERVER_CORE / "AIGatewayService" / "domain" / "modules" / "ai" / "AIRouteModule.cpp"
        legacy_ai_path = SERVER_CORE / "AIGatewayService" / "domain" / "AIRouteModules.cpp"

        self.assertIn(ai_module_path, MAIN_H1_FILES)
        self.assertIn(legacy_ai_path, MAIN_H1_FILES)
        self.assertTrue(ai_module_path.exists())
        self.assertTrue(legacy_ai_path.exists())

        module_routes = extract_route_registry_routes(ai_module_path)
        missing_from_module = AI_EXACT_ROUTES - module_routes
        self.assertFalse(
            missing_from_module,
            f"AIRouteModule route inventory missing {sorted(missing_from_module)}",
        )

        legacy_routes = extract_logic_routes(legacy_ai_path)
        missing_from_legacy_adapter = AI_LEGACY_PREFIX_STREAM_ROUTES - legacy_routes
        self.assertFalse(
            missing_from_legacy_adapter,
            f"AIRouteModules legacy adapter inventory missing {sorted(missing_from_legacy_adapter)}",
        )

        main_routes = self.main_h1_routes()
        missing_from_main = (AI_EXACT_ROUTES | AI_LEGACY_PREFIX_STREAM_ROUTES) - main_routes
        self.assertFalse(missing_from_main, f"main H1 AI route inventory missing {sorted(missing_from_main)}")

    def test_main_h1_r18_inventory_scans_neutral_route_module_and_legacy_shim(self):
        r18_module_path = SERVER_CORE / "R18Service" / "domain" / "modules" / "r18" / "R18RouteModule.cpp"
        legacy_r18_path = SERVER_CORE / "R18Service" / "domain" / "GateR18Routes.cpp"

        self.assertIn(r18_module_path, MAIN_H1_FILES)
        self.assertIn(legacy_r18_path, MAIN_H1_FILES)
        self.assertTrue(r18_module_path.exists())
        self.assertTrue(legacy_r18_path.exists())

        module_routes = extract_route_registry_routes(r18_module_path)
        missing_from_module = R18_ROUTES - module_routes
        self.assertFalse(
            missing_from_module,
            f"R18RouteModule route inventory missing {sorted(missing_from_module)}",
        )

        main_routes = self.main_h1_routes()
        missing_from_main = R18_ROUTES - main_routes
        self.assertFalse(missing_from_main, f"main H1 R18 route inventory missing {sorted(missing_from_main)}")

    def test_main_h1_auth_migration_tracks_all_core_auth_routes_in_neutral_module(self):
        auth_module_path = SERVER_CORE / "AccountShared" / "domain" / "modules" / "auth" / "AuthRouteModule.cpp"
        auth_module = read(auth_module_path)
        compact_auth_module = normalize_space(strip_comments(auth_module))
        legacy_h1 = read(SERVER_CORE / "GateShared" / "GateServerH1Routes.cpp")
        registered_routes = extract_route_registry_routes(auth_module_path)

        for route in (
            ("POST", "/get_varifycode"),
            ("POST", "/user_register"),
            ("POST", "/reset_pwd"),
            ("POST", "/user_login"),
        ):
            with self.subTest(route=route):
                self.assertIn(route, registered_routes)
        self.assertNotIn('logic.RegPost("/get_varifycode"', legacy_h1)
        self.assertNotIn('logic.RegPost(\n        "/user_register"', legacy_h1)
        self.assertNotIn('logic.RegPost(\n        "/reset_pwd"', legacy_h1)
        self.assertNotIn('logic.RegPost(\n        "/user_login"', legacy_h1)

    def test_main_h1_moments_inventory_scans_neutral_route_module(self):
        moments_module_path = (
            SERVER_CORE / "MomentsService" / "domain" / "modules" / "moments" / "MomentsRouteModule.cpp"
        )

        self.assertIn(moments_module_path, MAIN_H1_FILES)
        self.assertTrue(moments_module_path.exists())
        module_routes = extract_route_registry_routes(moments_module_path)
        missing_from_module = MOMENTS_ROUTES - module_routes
        self.assertFalse(
            missing_from_module,
            f"MomentsRouteModule route inventory missing {sorted(missing_from_module)}",
        )

        main_routes = self.main_h1_routes()
        missing_from_main = MOMENTS_ROUTES - main_routes
        self.assertFalse(missing_from_main, f"main H1 route inventory missing {sorted(missing_from_main)}")

    def test_h2_inventory_tracks_routes_that_need_shared_registry_bridge(self):
        routes = extract_h2_routes()
        missing = H2_EXPECTED_ROUTES - routes
        self.assertFalse(missing, f"H2 route inventory missing {sorted(missing)}")

        self.assertIn(("POST", "/user_logout"), routes)
        self.assertNotIn(("POST", "/user_logout"), self.main_h1_routes())
        self.assertNotIn(("POST", "/user_logout"), self.h3_routes())
        self.assertNotIn(("POST", "/ai/chat"), routes)
        for route in R18_ROUTES:
            with self.subTest(route=route):
                self.assertNotIn(route, routes)

    def test_h3_inventory_tracks_routes_that_need_shared_registry_bridge(self):
        routes = self.h3_routes()
        h3_expected_routes = CORE_SHARED_ROUTES - {("GET", "/api/call/token")}
        missing = h3_expected_routes - routes
        self.assertFalse(missing, f"H3 route inventory missing {sorted(missing)}")

        self.assertIn(("GET", "/healthz"), routes)
        self.assertIn(("GET", "/readyz"), routes)
        self.assertIn(("POST", "/api/call/token"), routes)
        self.assertIn(("POST", "/get_user_info"), routes)
        self.assertNotIn(("POST", "/user_logout"), routes)
        self.assertNotIn(("POST", "/ai/chat"), routes)
        for route in R18_ROUTES:
            with self.subTest(route=route):
                self.assertNotIn(route, routes)

    def test_media_route_matrix_tracks_current_h1_h2_h3_contract(self):
        matrix = {
            "main H1": self.main_h1_routes(),
            "H2": extract_h2_routes(),
            "H3": self.h3_routes(),
        }

        for transport, routes in matrix.items():
            with self.subTest(transport=transport):
                missing = MEDIA_ROUTES - routes
                self.assertFalse(missing, f"{transport} media route matrix missing {sorted(missing)}")

    def test_profile_route_support_matrix_tracks_h1_h2_h3_differences(self):
        matrix = {
            "main H1": {
                "routes": self.main_h1_routes(),
                "expected_present": {("POST", "/user_update_profile")},
                "expected_absent": {("POST", "/get_user_info")},
            },
            "H2": {
                "routes": extract_h2_routes(),
                "expected_present": {("POST", "/user_update_profile"), ("POST", "/get_user_info")},
                "expected_absent": set(),
            },
            "H3": {
                "routes": self.h3_routes(),
                "expected_present": {("POST", "/user_update_profile"), ("POST", "/get_user_info")},
                "expected_absent": set(),
            },
        }

        for transport, spec in matrix.items():
            with self.subTest(transport=transport, present=True):
                missing = spec["expected_present"] - spec["routes"]
                self.assertFalse(missing, f"{transport} profile matrix missing {sorted(missing)}")
            with self.subTest(transport=transport, absent=True):
                unexpected = spec["expected_absent"] & spec["routes"]
                self.assertFalse(unexpected, f"{transport} profile matrix unexpectedly exposes {sorted(unexpected)}")

    def test_call_route_support_matrix_tracks_h1_h2_h3_differences(self):
        matrix = {
            "main H1": {
                "routes": self.main_h1_routes(),
                "expected_present": CALL_STATE_POST_ROUTES | CALL_TOKEN_ROUTES,
                "expected_absent": set(),
            },
            "H2": {
                "routes": extract_h2_routes(),
                "expected_present": CALL_STATE_POST_ROUTES | CALL_TOKEN_ROUTES,
                "expected_absent": set(),
            },
            "H3": {
                "routes": self.h3_routes(),
                "expected_present": CALL_STATE_POST_ROUTES | {("POST", "/api/call/token")},
                "expected_absent": {("GET", "/api/call/token")},
            },
        }

        for transport, spec in matrix.items():
            with self.subTest(transport=transport, present=True):
                missing = spec["expected_present"] - spec["routes"]
                self.assertFalse(missing, f"{transport} call matrix missing {sorted(missing)}")
            with self.subTest(transport=transport, absent=True):
                unexpected = spec["expected_absent"] & spec["routes"]
                self.assertFalse(unexpected, f"{transport} call matrix unexpectedly exposes {sorted(unexpected)}")

    def test_legacy_h1_source_inventory_marks_auth_profile_as_incomplete_legacy_path(self):
        routes = self.h1_legacy_source_routes()
        for route in (
            ("GET", "/healthz"),
            ("GET", "/readyz"),
            ("POST", "/get_varifycode"),
            ("POST", "/user_register"),
            ("POST", "/reset_pwd"),
            ("POST", "/user_login"),
            ("POST", "/user_update_profile"),
            ("POST", "/api/call/start"),
            ("POST", "/api/call/hangup"),
            ("POST", "/upload_media_init"),
            ("GET", "/media/download"),
            ("POST", "/api/moments/publish"),
            ("POST", "/api/moments/comment/like"),
            ("GET", "/api/r18/sources"),
            ("POST", "/api/r18/search"),
        ):
            self.assertIn(route, routes, f"legacy H1.1 source inventory missing {route}")

        auth_source = read(GATE_H1_LEGACY / "H1AuthRoutes.cpp")
        self.assertIn("MakeNotImplementedResponse", auth_source)
        for route in ("/get_varifycode", "/user_register", "/reset_pwd", "/user_login", "/user_update_profile"):
            self.assertIn(f'MakeNotImplementedResponse("{route}")', auth_source)

    def test_h3_reset_password_uses_shared_auth_service_contract(self):
        source = read(GATE_H3_LEGACY_ROUTES / "GateHttp3ServiceRoutes.cpp")
        reset_block = extract_logic_route_block(source, "/reset_pwd")

        self.assertNotIn("VerifyGrpcClient::GetInstance()->GetVarifyCode(email)", reset_block)
        self.assertIn("HandleResetPwd", reset_block)
        self.assertIn("AuthService", reset_block)

        h1_auth_service = read(SERVER_CORE / "AccountShared" / "domain" / "services" / "auth" / "AuthService.cpp")
        account_persistence = read(
            SERVER_CORE / "AccountShared" / "domain" / "services" / "account" / "AccountPersistence.cpp"
        )
        h2 = read(GATE_H2_SUPPORT / "Http2AuthSupport.cpp")
        self.assertIn("UpdatePassword(email, pwd)", h1_auth_service)
        self.assertIn("PostgresMgr::GetInstance()->UpdatePwd(email, password)", account_persistence)
        self.assertIn("PublishCacheInvalidate", h1_auth_service)
        self.assertIn("PostgresMgr::GetInstance()->UpdatePwd(email, pwd)", h2)
        self.assertIn('GateAsyncSideEffects::Instance().PublishCacheInvalidate(email, name, "reset_pwd")', h2)

    def test_h3_auth_routes_delegate_to_auth_service(self):
        source = read(GATE_H3_LEGACY_ROUTES / "GateHttp3ServiceRoutes.cpp")

        for route, handler in H3_AUTH_ROUTES.items():
            with self.subTest(route=route):
                block = extract_logic_route_block(source, route)
                self.assertIn(handler, block)
                self.assertIn("AuthService", block)


if __name__ == "__main__":
    unittest.main()
