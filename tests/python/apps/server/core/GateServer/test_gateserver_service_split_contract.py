"""GateServer microservice-split service-boundary contract (Phase 0 safety net).

This test captures the *service ownership* invariants that the GateServer
microservice split must preserve. It is intentionally complementary to:

- ``test_gateserver_structure.py`` (module/service facade existence, transport
  neutrality, CMake structure), and
- ``test_gateserver_route_inventory.py`` (per-transport route matrices).

Here we guard the boundaries that the split roadmap depends on (see
``.ai/gateserver-microservice-split/a/contracts.md`` and ``inventory.md``):

1. Each future service still has its owning module + service facade on disk.
2. ``memo_account`` access is funnelled through ``AccountPersistence`` — the
   auth service never reaches ``PostgresMgr`` directly (D-ACCOUNT / C-ACCOUNT).
3. The only cross-domain ``users`` read by a non-account service is the known
   moments author lookup (D-DATA). No *new* non-account service may read users.
4. ``VarifyServer`` owns its own thin ``ConfigMgr`` and no longer links
   ``GateServerCore`` — the Phase 1 severing of that coupling.
5. The Redis key-prefix ownership matrix matches what ``const.h`` declares.

Phase 0 changes no product code; this only encodes the current, verified state
as an executable contract for later phases.
"""

import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps" / "server" / "core"
GATE_SERVER = SERVER_CORE / "GateServer"
GATE_CORE = GATE_SERVER / "core"
GATE_MODULES = GATE_SERVER / "modules"
GATE_SERVICES = GATE_SERVER / "services"
VARIFY_SERVER = SERVER_CORE / "VarifyServer"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


def strip_comments(source: str) -> str:
    source = re.sub(r"//.*", "", source)
    return re.sub(r"/\*.*?\*/", "", source, flags=re.S)


# Future service -> (owning module dir, owning service facade file).
# call lives in core/support; profile data access lives in services/account.
SERVICE_MODULE_OWNERS = {
    "ai": (GATE_MODULES / "ai" / "AIRouteModule.cpp", GATE_SERVICES / "ai" / "AIService.cpp"),
    "auth": (GATE_MODULES / "auth" / "AuthRouteModule.cpp", GATE_SERVICES / "auth" / "AuthService.cpp"),
    "media": (GATE_MODULES / "media" / "MediaRouteModule.cpp", GATE_SERVICES / "media" / "MediaService.cpp"),
    "moments": (
        GATE_MODULES / "moments" / "MomentsRouteModule.cpp",
        GATE_SERVICES / "moments" / "MomentsService.cpp",
    ),
    "r18": (GATE_MODULES / "r18" / "R18RouteModule.cpp", GATE_SERVICES / "r18" / "R18Service.cpp"),
}

# call + profile + account have a different module/owner shape.
CALL_MODULE = GATE_MODULES / "call" / "CallRouteModule.cpp"
CALL_SERVICE = GATE_CORE / "support" / "CallService.cpp"
PROFILE_MODULE = GATE_MODULES / "profile" / "ProfileRouteModule.cpp"
ACCOUNT_PERSISTENCE = GATE_SERVICES / "account" / "AccountPersistence.cpp"
ACCOUNT_PERSISTENCE_H = GATE_SERVICES / "account" / "AccountPersistence.h"
HEALTH_MODULE = GATE_MODULES / "health" / "HealthRouteModule.cpp"

AUTH_SERVICE = GATE_SERVICES / "auth" / "AuthService.cpp"
MOMENTS_SERVICE = GATE_SERVICES / "moments" / "MomentsService.cpp"
CONST_H = GATE_CORE / "config" / "const.h"

# Redis key-prefix ownership matrix asserted against const.h declarations.
CONST_H_KEY_PREFIXES = {
    "CODEPREFIX": '"code_"',
    "USERTOKENPREFIX": '"utoken_"',
    "USERIPPREFIX": '"uip_"',
    "SERVER_ONLINE_USERS_PREFIX": '"server_online_users_"',
}

# Non-account services allowed to read the users table directly. Only moments,
# only the known author lookup. D-DATA Phase 2 will move this to cache/API.
KNOWN_CROSS_DOMAIN_USER_READ = "PostgresMgr::GetInstance()->GetUserInfo"

NON_ACCOUNT_SERVICE_SOURCES = (
    GATE_SERVICES / "media" / "MediaService.cpp",
    GATE_SERVICES / "moments" / "MomentsService.cpp",
    GATE_SERVICES / "r18" / "R18Service.cpp",
    GATE_CORE / "support" / "CallService.cpp",
)

# Gate-core headers VarifyServer must NOT include (only ConfigMgr.h is allowed).
FORBIDDEN_GATE_CORE_INCLUDES_FOR_VARIFY = (
    "RedisMgr.h",
    "PostgresMgr.h",
    "PostgresDao.h",
    "AuthCache.h",
    "GateGlobals.h",
    "AuthLoginSupport.h",
    "CallService.h",
    "MediaStorage.h",
    "MongoMgr.h",
)


class GateServerServiceSplitContractTests(unittest.TestCase):
    def test_each_future_service_has_owning_module_and_facade(self):
        for name, (module_path, service_path) in SERVICE_MODULE_OWNERS.items():
            with self.subTest(service=name):
                self.assertTrue(module_path.exists(), f"{name}: owning module missing: {module_path}")
                self.assertTrue(service_path.exists(), f"{name}: owning service facade missing: {service_path}")

        # call/profile/account/health have their own boundary shapes.
        self.assertTrue(CALL_MODULE.exists(), "call route module missing")
        self.assertTrue(CALL_SERVICE.exists(), "call service (core/support) missing")
        self.assertTrue(PROFILE_MODULE.exists(), "profile route module missing")
        self.assertTrue(ACCOUNT_PERSISTENCE.exists(), "account persistence (account-core) missing")
        self.assertTrue(HEALTH_MODULE.exists(), "health route module missing")

    def test_account_access_is_funnelled_through_account_persistence(self):
        """C-ACCOUNT: auth service must reach account data via AccountPersistence,
        never PostgresMgr directly."""
        auth_source = strip_comments(read(AUTH_SERVICE))

        self.assertIn(
            '#include "services/account/AccountPersistence.h"',
            auth_source,
            "AuthService should depend on AccountPersistence (account-core)",
        )
        self.assertIn(
            "AccountPersistence::Instance()",
            auth_source,
            "AuthService should access account data through AccountPersistence",
        )
        self.assertNotRegex(
            auth_source,
            r"PostgresMgr::GetInstance\(\)",
            "AuthService must not reach PostgresMgr directly; route through AccountPersistence",
        )

        # AccountPersistence is the single owner that talks to PostgresMgr for account data.
        account_source = strip_comments(read(ACCOUNT_PERSISTENCE))
        self.assertIn(
            "PostgresMgr::GetInstance()->GetUserPublicId",
            account_source,
            "AccountPersistence should own the account users-table access",
        )

    def test_only_moments_reads_users_table_cross_domain(self):
        """D-DATA: the single known cross-domain users read is the moments author
        lookup. No other non-account service may read users directly."""
        moments_source = strip_comments(read(MOMENTS_SERVICE))
        self.assertIn(
            KNOWN_CROSS_DOMAIN_USER_READ,
            moments_source,
            "the known moments author lookup must remain registered (Phase 2 moves it to cache/API)",
        )

        for source_path in NON_ACCOUNT_SERVICE_SOURCES:
            if source_path == MOMENTS_SERVICE:
                # moments has exactly the one known author lookup, no other users reads.
                hits = re.findall(r"GetUserInfo\s*\(", strip_comments(read(source_path)))
                self.assertEqual(
                    len(hits),
                    1,
                    f"moments should keep exactly one cross-domain users read, found {len(hits)}",
                )
                continue
            with self.subTest(service=source_path.name):
                source = strip_comments(read(source_path))
                self.assertNotIn(
                    "GetUserInfo(",
                    source,
                    f"{source_path.name} must not read the users table cross-domain (D-DATA)",
                )

    def test_varifyserver_owns_its_config_and_is_decoupled_from_gate_core(self):
        """Phase 1: VarifyServer owns its own thin ConfigMgr and no longer links
        GateServerCore. This is the "sever VarifyServer -> GateServerCore" step."""
        varify_sources = list(VARIFY_SERVER.glob("*.cpp")) + list(VARIFY_SERVER.glob("*.h"))
        self.assertTrue(varify_sources, "VarifyServer sources should exist")

        # VarifyServer carries its own ConfigMgr now.
        self.assertTrue((VARIFY_SERVER / "ConfigMgr.h").exists(), "VarifyServer should own ConfigMgr.h")
        self.assertTrue((VARIFY_SERVER / "ConfigMgr.cpp").exists(), "VarifyServer should own ConfigMgr.cpp")

        includes_configmgr = False
        for path in varify_sources:
            source = read(path)
            if '#include "ConfigMgr.h"' in source:
                includes_configmgr = True
            for forbidden in FORBIDDEN_GATE_CORE_INCLUDES_FOR_VARIFY:
                with self.subTest(file=path.name, forbidden=forbidden):
                    self.assertNotIn(
                        f'#include "{forbidden}"',
                        source,
                        f"{path.name} couples to gate-core {forbidden}; VarifyServer must stay gate-core-free",
                    )
        self.assertTrue(includes_configmgr, "VarifyServer should still use its own ConfigMgr")

        cmake = read(VARIFY_SERVER / "CMakeLists.txt")
        self.assertIn("ConfigMgr.cpp", cmake, "VarifyServer must compile its own ConfigMgr.cpp")
        self.assertNotRegex(
            cmake,
            r"target_link_libraries\s*\(\s*VarifyServer\b[^)]*\bGateServerCore\b",
            "VarifyServer must no longer link GateServerCore (Phase 1 decoupling)",
        )

    def test_redis_key_prefix_ownership_matrix_matches_const_header(self):
        """C-DATA: the Redis key-prefix ownership matrix in contracts.md is anchored
        to const.h declarations."""
        const_source = read(CONST_H)
        for macro, value in CONST_H_KEY_PREFIXES.items():
            with self.subTest(prefix=macro):
                self.assertRegex(
                    const_source,
                    rf"#define\s+{macro}\s+{re.escape(value)}",
                    f"const.h should declare key prefix {macro} = {value}",
                )

    def test_account_persistence_header_is_account_owner_surface(self):
        header = strip_comments(read(ACCOUNT_PERSISTENCE_H))
        self.assertIn("GetUserPublicId", header)

    # ---- Phase 3: AIGateway pilot (first peeled microservice) ----------------

    def test_gate_app_core_library_holds_shared_h1_application_layer(self):
        """Phase 3: GateServer's reusable H1 application layer is a GateAppCore
        static library (so focused services can link it), and the GateServer
        executable is just the entrypoint linking it."""
        cmake = strip_comments(read(GATE_SERVER / "CMakeLists.txt"))
        self.assertRegex(cmake, r"add_library\s*\(\s*GateAppCore\s+STATIC", "GateAppCore static lib should exist")

        app_core_match = re.search(r"add_library\s*\(\s*GateAppCore\s+STATIC(?P<body>.*?)\)", cmake, re.S)
        self.assertIsNotNone(app_core_match)
        app_core = app_core_match.group("body")
        for src in ("CServer.cpp", "HttpConnection.cpp", "LogicSystem.cpp", "modules/ai/AIRouteModule.cpp"):
            with self.subTest(src=src):
                self.assertIn(src, app_core, f"GateAppCore should compile {src}")

        gate_exe_match = re.search(r"add_executable\s*\(\s*GateServer\b(?P<body>.*?)\)", cmake, re.S)
        self.assertIsNotNone(gate_exe_match)
        self.assertIn("GateServer.cpp", gate_exe_match.group("body"))
        self.assertNotIn(
            "LogicSystem.cpp", gate_exe_match.group("body"), "GateServer exe should only carry its entrypoint"
        )
        self.assertRegex(
            cmake,
            r"target_link_libraries\s*\(\s*GateServer\b[^)]*\bGateAppCore\b",
            "GateServer must link GateAppCore",
        )

    def test_aigateway_server_executable_links_app_core_and_owns_only_entrypoint(self):
        """Phase 3: AIGatewayServer is a standalone executable reusing GateAppCore."""
        entry = GATE_SERVER / "app" / "AIGatewayServer.cpp"
        self.assertTrue(entry.exists(), "AIGatewayServer entrypoint should exist")
        entry_src = read(entry)
        self.assertIn(
            "LogicSystem::SetRouteProfile(LogicSystem::RouteProfile::AIGateway)",
            entry_src,
            "AIGatewayServer must select the AIGateway route profile before LogicSystem init",
        )

        cmake = strip_comments(read(GATE_SERVER / "CMakeLists.txt"))
        exe_match = re.search(r"add_executable\s*\(\s*AIGatewayServer\b(?P<body>.*?)\)", cmake, re.S)
        self.assertIsNotNone(exe_match, "AIGatewayServer executable should be declared")
        self.assertIn("app/AIGatewayServer.cpp", exe_match.group("body"))
        self.assertRegex(
            cmake,
            r"target_link_libraries\s*\(\s*AIGatewayServer\b[^)]*\bGateAppCore\b",
            "AIGatewayServer must link GateAppCore",
        )

        self.assertTrue((GATE_SERVER / "aigateway.ini").exists(), "AIGatewayServer config should exist")

    def test_logic_system_route_profile_seam_defaults_to_full(self):
        """The RouteProfile seam must default to Full so GateServer registers
        every module unchanged; AIGateway registers only health + ai."""
        header = read(GATE_SERVER / "LogicSystem.h")
        source = strip_comments(read(GATE_SERVER / "LogicSystem.cpp"))

        self.assertIn("enum class RouteProfile", header)
        self.assertIn("Full", header)
        self.assertIn("AIGateway", header)
        self.assertIn("static void SetRouteProfile(RouteProfile profile)", header)

        # Default is Full.
        self.assertRegex(source, r"g_route_profile\s*=\s*LogicSystem::RouteProfile::Full")
        # Full registers every module; focused profiles register only their own.
        self.assertIn("const bool full = (profile == RouteProfile::Full)", source)
        for module in ("AuthRouteModule", "MediaRouteModule", "MomentsRouteModule", "R18RouteModule", "AIRouteModule"):
            with self.subTest(module=module):
                idx = source.find(module)
                self.assertNotEqual(idx, -1, f"{module} should still be registered")

    def test_aigateway_is_opt_in_in_runtime_topology(self):
        """AIGatewayServer must be a topology service but NOT in the default
        core start groups (opt-in until Envoy flip)."""
        topology = read(REPO_ROOT / "tools" / "scripts" / "status" / "runtime_topology.sh")
        start = read(REPO_ROOT / "tools" / "scripts" / "status" / "start-all-services.sh")

        self.assertIn("aigateway|AIGatewayService1|AIGatewayServer|AIGatewayServer", topology)
        # Not in core start groups.
        core_match = re.search(r"MEMOCHAT_CORE_START_GROUPS=\(\n(?P<body>.*?)\n\)", topology, re.S)
        self.assertIsNotNone(core_match)
        self.assertNotIn("aigateway", core_match.group("body"), "aigateway must stay opt-in (not a core start group)")
        # Phase 6 cut-over: AIGateway now starts by default (Envoy routes /ai/* to it).
        self.assertRegex(
            start, r"START_AIGATEWAY=\"\$\{START_AIGATEWAY_OVERRIDE:-\$\{MEMOCHAT_START_AIGATEWAY:-1\}\}\""
        )
        self.assertIn("--start-aigateway", start)

    def test_phase4_domain_gateways_exist_and_are_opt_in(self):
        """Phase 4: Media/Moments/Call/R18 each peeled into a standalone gateway
        exe reusing GateAppCore + its own RouteProfile, all opt-in (default OFF)."""
        cmake = strip_comments(read(GATE_SERVER / "CMakeLists.txt"))
        topology = read(REPO_ROOT / "tools" / "scripts" / "status" / "runtime_topology.sh")
        start = read(REPO_ROOT / "tools" / "scripts" / "status" / "start-all-services.sh")
        header = read(GATE_SERVER / "LogicSystem.h")
        core_match = re.search(r"MEMOCHAT_CORE_START_GROUPS=\(\n(?P<body>.*?)\n\)", topology, re.S)
        self.assertIsNotNone(core_match)
        core_groups = core_match.group("body")

        domains = {
            "Media": ("MediaGatewayServer", "mediagateway", "MediaGatewayService1"),
            "Moments": ("MomentsGatewayServer", "momentsgateway", "MomentsGatewayService1"),
            "Call": ("CallGatewayServer", "callgateway", "CallGatewayService1"),
            "R18": ("R18GatewayServer", "r18gateway", "R18GatewayService1"),
        }
        for profile, (exe, group, topo_dir) in domains.items():
            with self.subTest(domain=profile):
                self.assertRegex(header, rf"\b{profile}\b")
                self.assertTrue((GATE_SERVER / "app" / f"{exe}.cpp").exists(), f"{exe}.cpp entry missing")
                self.assertTrue((GATE_SERVER / f"{group}.ini").exists(), f"{group}.ini config missing")
                entry = read(GATE_SERVER / "app" / f"{exe}.cpp")
                self.assertIn(f"RouteProfile::{profile}", entry)
                self.assertIn(f"memochat_add_gate_domain_server({exe}", cmake)
                self.assertIn(f"{group}|{topo_dir}|{exe}|{exe}", topology)
                self.assertNotIn(group, core_groups, f"{group} must stay opt-in (not a core start group)")
                upper = group.upper()
                self.assertRegex(
                    start,
                    rf"START_{upper}=\"\$\{{START_{upper}_OVERRIDE:-\$\{{MEMOCHAT_START_{upper}:-1\}}\}}\"",
                )
                self.assertIn(f"--start-{group}", start)

    def test_gate_domain_server_shared_entrypoint_helper_exists(self):
        """The 4 domain gateways share one parameterized bootstrap helper."""
        self.assertTrue((GATE_SERVER / "app" / "GateDomainServer.cpp").exists())
        self.assertTrue((GATE_SERVER / "app" / "GateDomainServer.h").exists())
        cmake = strip_comments(read(GATE_SERVER / "CMakeLists.txt"))
        app_core_match = re.search(r"add_library\s*\(\s*GateAppCore\s+STATIC(?P<body>.*?)\)", cmake, re.S)
        self.assertIsNotNone(app_core_match)
        self.assertIn("app/GateDomainServer.cpp", app_core_match.group("body"))

    def test_phase5_account_aggregate_split_register_login_account(self):
        """Phase 5 (D-ACCOUNT): Register/Login/Account peeled into separate opt-in
        processes. Register + Login reach account data only via account-core; the
        auth module exposes granular register/login registration so each process
        serves only its own routes."""
        cmake = strip_comments(read(GATE_SERVER / "CMakeLists.txt"))
        topology = read(REPO_ROOT / "tools" / "scripts" / "status" / "runtime_topology.sh")
        start = read(REPO_ROOT / "tools" / "scripts" / "status" / "start-all-services.sh")
        header = read(GATE_SERVER / "LogicSystem.h")
        auth_header = read(GATE_MODULES / "auth" / "AuthRouteModule.h")
        core_match = re.search(r"MEMOCHAT_CORE_START_GROUPS=\(\n(?P<body>.*?)\n\)", topology, re.S)
        self.assertIsNotNone(core_match)
        core_groups = core_match.group("body")

        # Granular auth registration (register vs login).
        self.assertIn("RegisterRegisterRoutes", auth_header)
        self.assertIn("RegisterLoginRoutes", auth_header)

        accounts = {
            "Register": ("RegisterServer", "register", "RegisterService1"),
            "Login": ("LoginServer", "login", "LoginService1"),
            "Account": ("AccountServer", "account", "AccountService1"),
        }
        for profile, (exe, group, topo_dir) in accounts.items():
            with self.subTest(service=profile):
                self.assertRegex(header, rf"\b{profile}\b")
                self.assertTrue((GATE_SERVER / "app" / f"{exe}.cpp").exists(), f"{exe}.cpp entry missing")
                self.assertTrue((GATE_SERVER / f"{group}.ini").exists(), f"{group}.ini config missing")
                entry = read(GATE_SERVER / "app" / f"{exe}.cpp")
                self.assertIn(f"RouteProfile::{profile}", entry)
                self.assertIn(f"memochat_add_gate_domain_server({exe}", cmake)
                self.assertIn(f"{group}|{topo_dir}|{exe}|{exe}", topology)
                self.assertNotIn(f'"{group}"', core_groups, f"{group} must stay opt-in")
                upper = group.upper()
                self.assertRegex(
                    start,
                    rf"START_{upper}=\"\$\{{START_{upper}_OVERRIDE:-\$\{{MEMOCHAT_START_{upper}:-1\}}\}}\"",
                )

        # D-ACCOUNT: Register/Login entrypoints reach account data via account-core,
        # never PostgresMgr directly (that invariant is enforced on AuthService by
        # test_account_access_is_funnelled_through_account_persistence).
        for exe in ("RegisterServer", "LoginServer", "AccountServer"):
            entry = read(GATE_SERVER / "app" / f"{exe}.cpp")
            self.assertNotIn("PostgresMgr", entry, f"{exe} entrypoint must not touch PostgresMgr directly")


if __name__ == "__main__":
    unittest.main()
