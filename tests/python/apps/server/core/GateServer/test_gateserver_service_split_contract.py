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
GATE_SERVER = SERVER_CORE / "GateShared"
GATE_CORE = GATE_SERVER / "core"
GATE_MODULES = GATE_SERVER / "modules"
GATE_SERVICES = GATE_SERVER / "services"
VARIFY_SERVER = SERVER_CORE / "VarifyServer"
FOCUSED_SERVICE_DIRS = {
    "AIGateway": SERVER_CORE / "AIGatewayService",
    "Media": SERVER_CORE / "MediaService",
    "Moments": SERVER_CORE / "MomentsService",
    "Call": SERVER_CORE / "CallService",
    "R18": SERVER_CORE / "R18Service",
    "Register": SERVER_CORE / "RegisterService",
    "Login": SERVER_CORE / "LoginService",
    "Account": SERVER_CORE / "AccountService",
}
# Slice j: each microservice physically owns its domain code under <Service>/domain
# (+ <Service>/core for its support library). The shared account aggregate lives in
# AccountShared/. GateShared/core keeps only GateInfraCore.
AIGATEWAY_DOMAIN = FOCUSED_SERVICE_DIRS["AIGateway"] / "domain"
MEDIA_DOMAIN = FOCUSED_SERVICE_DIRS["Media"] / "domain"
MOMENTS_DOMAIN = FOCUSED_SERVICE_DIRS["Moments"] / "domain"
CALL_SVC = FOCUSED_SERVICE_DIRS["Call"]
R18_DOMAIN = FOCUSED_SERVICE_DIRS["R18"] / "domain"
ACCOUNT_SHARED = SERVER_CORE / "AccountShared"
ACCOUNT_DOMAIN = ACCOUNT_SHARED / "domain"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


def strip_comments(source: str) -> str:
    source = re.sub(r"//.*", "", source)
    return re.sub(r"/\*.*?\*/", "", source, flags=re.S)


# Future service -> (owning module file, owning service facade file). Each now
# lives in its microservice's domain/ tree (auth/profile/account in AccountShared).
SERVICE_MODULE_OWNERS = {
    "ai": (
        AIGATEWAY_DOMAIN / "modules" / "ai" / "AIRouteModule.cpp",
        AIGATEWAY_DOMAIN / "services" / "ai" / "AIService.cpp",
    ),
    "auth": (
        ACCOUNT_DOMAIN / "modules" / "auth" / "AuthRouteModule.cpp",
        ACCOUNT_DOMAIN / "services" / "auth" / "AuthService.cpp",
    ),
    "media": (
        MEDIA_DOMAIN / "modules" / "media" / "MediaRouteModule.cpp",
        MEDIA_DOMAIN / "services" / "media" / "MediaService.cpp",
    ),
    "moments": (
        MOMENTS_DOMAIN / "modules" / "moments" / "MomentsRouteModule.cpp",
        MOMENTS_DOMAIN / "services" / "moments" / "MomentsService.cpp",
    ),
    "r18": (R18_DOMAIN / "modules" / "r18" / "R18RouteModule.cpp", R18_DOMAIN / "services" / "r18" / "R18Service.cpp"),
}

# call + profile + account have a different module/owner shape.
CALL_MODULE = CALL_SVC / "domain" / "modules" / "call" / "CallRouteModule.cpp"
CALL_SERVICE = CALL_SVC / "core" / "support" / "CallService.cpp"
PROFILE_MODULE = ACCOUNT_DOMAIN / "modules" / "profile" / "ProfileRouteModule.cpp"
ACCOUNT_PERSISTENCE = ACCOUNT_DOMAIN / "services" / "account" / "AccountPersistence.cpp"
ACCOUNT_PERSISTENCE_H = ACCOUNT_DOMAIN / "services" / "account" / "AccountPersistence.h"
HEALTH_MODULE = GATE_MODULES / "health" / "HealthRouteModule.cpp"

AUTH_SERVICE = ACCOUNT_DOMAIN / "services" / "auth" / "AuthService.cpp"
MOMENTS_SERVICE = MOMENTS_DOMAIN / "services" / "moments" / "MomentsService.cpp"
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
    MEDIA_DOMAIN / "services" / "media" / "MediaService.cpp",
    MOMENTS_DOMAIN / "services" / "moments" / "MomentsService.cpp",
    R18_DOMAIN / "services" / "r18" / "R18Service.cpp",
    CALL_SVC / "core" / "support" / "CallService.cpp",
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

    def test_focused_gate_targets_link_narrow_domain_libraries(self):
        """P1: focused gateway executables must not link the old broad app/core
        aggregate. Each executable chooses its domain library explicitly."""
        root_cmake = strip_comments(read(SERVER_CORE / "CMakeLists.txt"))
        helper_match = re.search(
            r"function\s*\(\s*memochat_configure_gate_focused_service\s+target_name\s*\)(?P<body>.*?)endfunction\s*\(",
            root_cmake,
            re.S,
        )
        self.assertIsNotNone(helper_match, "focused service helper should exist")
        helper = helper_match.group("body")
        self.assertIn("DOMAINS", helper)
        self.assertIn("GateRuntimeCore", helper)
        self.assertIn("${MEMOCHAT_GATE_FOCUSED_DOMAINS}", helper)
        self.assertNotIn("GateAppCore", helper)
        self.assertNotIn("GateServerCore", helper)
        self.assertNotIn("GateServerHttp3", helper)

        expected_domains = {
            "AIGateway": ("AIGatewayServer", "GateAiDomain"),
            "Media": ("MediaGatewayServer", "GateMediaDomain"),
            "Moments": ("MomentsGatewayServer", "GateMomentsDomain"),
            "Call": ("CallGatewayServer", "GateCallDomain"),
            "R18": ("R18GatewayServer", "GateR18Domain"),
            "Register": ("RegisterServer", "GateAuthDomain"),
            "Login": ("LoginServer", "GateAuthDomain"),
            "Account": ("AccountServer", "GateAccountDomain"),
        }
        for service, (exe, domain) in expected_domains.items():
            with self.subTest(service=service):
                cmake = strip_comments(read(FOCUSED_SERVICE_DIRS[service] / "CMakeLists.txt"))
                self.assertIn(f"memochat_configure_gate_focused_service({exe} DOMAINS {domain})", cmake)

    def test_logic_system_runtime_does_not_include_domain_modules(self):
        """P1: LogicSystem is runtime plumbing. Domain registration is supplied
        by the executable/domain library, so runtime code does not drag every
        gateway domain into every focused process."""
        logic_source = strip_comments(read(GATE_SERVER / "LogicSystem.cpp"))
        header = strip_comments(read(GATE_SERVER / "LogicSystem.h"))

        self.assertIn("RouteProfileRegistrar", header)
        self.assertIn("AddRouteProfileRegistrar", header)
        self.assertIn("ClearRouteProfileRegistrars", header)
        self.assertIn("HealthRouteModule().RegisterRoutes(_route_registry)", logic_source)
        for token in (
            "modules/ai/AIRouteModule.h",
            "modules/auth/AuthRouteModule.h",
            "modules/media/MediaRouteModule.h",
            "modules/moments/MomentsRouteModule.h",
            "modules/call/CallRouteModule.h",
            "modules/r18/R18RouteModule.h",
            "modules/profile/ProfileRouteModule.h",
            "GateHttp3ServiceRoutes.h",
            "AIHttpServiceRoutes::RegisterRoutes",
        ):
            with self.subTest(token=token):
                self.assertNotIn(token, logic_source)

    def test_gate_app_cmake_splits_runtime_and_domain_libraries(self):
        # Slice j: GateShared/CMakeLists defines only the runtime framework
        # (GateRuntimeCore) + the GateAppCore compat aggregate. Each domain
        # library is defined in its owning microservice's CMakeLists.
        cmake = strip_comments(read(GATE_SERVER / "CMakeLists.txt"))
        for target in ("GateRuntimeCore", "GateAppCore"):
            with self.subTest(target=target):
                self.assertRegex(cmake, rf"add_library\s*\(\s*{target}\s+STATIC")

        domain_lib_homes = {
            "GateAiDomain": FOCUSED_SERVICE_DIRS["AIGateway"] / "CMakeLists.txt",
            "GateMediaDomain": FOCUSED_SERVICE_DIRS["Media"] / "CMakeLists.txt",
            "GateMomentsDomain": FOCUSED_SERVICE_DIRS["Moments"] / "CMakeLists.txt",
            "GateCallDomain": FOCUSED_SERVICE_DIRS["Call"] / "CMakeLists.txt",
            "GateR18Domain": FOCUSED_SERVICE_DIRS["R18"] / "CMakeLists.txt",
            "GateAuthDomain": ACCOUNT_SHARED / "CMakeLists.txt",
            "GateAccountDomain": ACCOUNT_SHARED / "CMakeLists.txt",
        }
        for target, cmake_path in domain_lib_homes.items():
            with self.subTest(target=target):
                self.assertRegex(strip_comments(read(cmake_path)), rf"add_library\s*\(\s*{target}\s+STATIC")

        runtime_match = re.search(r"add_library\s*\(\s*GateRuntimeCore\s+STATIC(?P<body>.*?)\)", cmake, re.S)
        self.assertIsNotNone(runtime_match)
        runtime = runtime_match.group("body")
        self.assertIn("LogicSystem.cpp", runtime)
        self.assertIn("modules/health/HealthRouteModule.cpp", runtime)
        self.assertNotIn("modules/ai/AIRouteModule.cpp", runtime)
        self.assertNotIn("services/media/MediaService.cpp", runtime)

        ai_cmake = strip_comments(read(FOCUSED_SERVICE_DIRS["AIGateway"] / "CMakeLists.txt"))
        ai_match = re.search(r"add_library\s*\(\s*GateAiDomain\s+STATIC(?P<body>.*?)\)", ai_cmake, re.S)
        self.assertIsNotNone(ai_match)
        ai_domain = ai_match.group("body")
        self.assertIn("domain/modules/ai/AIRouteModule.cpp", ai_domain)
        self.assertNotIn("Postgres", ai_domain)
        self.assertNotIn("MediaStorage", ai_domain)

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
        """The reusable H1 application layer is GateRuntimeCore. GateAppCore is
        now only a compatibility aggregate."""
        cmake = strip_comments(read(GATE_SERVER / "CMakeLists.txt"))
        self.assertRegex(cmake, r"add_library\s*\(\s*GateRuntimeCore\s+STATIC", "GateRuntimeCore should exist")

        runtime_match = re.search(r"add_library\s*\(\s*GateRuntimeCore\s+STATIC(?P<body>.*?)\)", cmake, re.S)
        self.assertIsNotNone(runtime_match)
        runtime_core = runtime_match.group("body")
        for src in ("CServer.cpp", "HttpConnection.cpp", "LogicSystem.cpp", "app/GateDomainServer.cpp"):
            with self.subTest(src=src):
                self.assertIn(src, runtime_core, f"GateRuntimeCore should compile {src}")
        self.assertNotIn("modules/ai/AIRouteModule.cpp", runtime_core)

        # The retired monolith exe + its entrypoint must be gone.
        self.assertNotIn("add_executable(GateServer", cmake, "GateServer monolith exe must be removed")
        self.assertNotIn("GateServer.cpp", cmake)
        self.assertFalse((GATE_SERVER / "GateServer.cpp").exists(), "monolith entrypoint must be deleted")

    def test_aigateway_server_executable_links_app_core_and_owns_only_entrypoint(self):
        """Phase 3/7: AIGatewayServer is a top-level executable using GateAiDomain."""
        service_dir = FOCUSED_SERVICE_DIRS["AIGateway"]
        entry = service_dir / "app" / "AIGatewayServer.cpp"
        self.assertTrue(entry.exists(), "AIGatewayServer entrypoint should exist")
        entry_src = read(entry)
        self.assertIn(
            "LogicSystem::AddRouteProfileRegistrar(memochat::gate::profiles::RegisterAIGateway)",
            entry_src,
            "AIGatewayServer must register the AI domain before LogicSystem init",
        )

        cmake = strip_comments(read(service_dir / "CMakeLists.txt"))
        exe_match = re.search(r"add_executable\s*\(\s*AIGatewayServer\b(?P<body>.*?)\)", cmake, re.S)
        self.assertIsNotNone(exe_match, "AIGatewayServer executable should be declared")
        self.assertIn("app/AIGatewayServer.cpp", exe_match.group("body"))
        self.assertIn("memochat_configure_gate_focused_service(AIGatewayServer DOMAINS GateAiDomain)", cmake)

        root_cmake = strip_comments(read(SERVER_CORE / "CMakeLists.txt"))
        helper_match = re.search(
            r"function\s*\(\s*memochat_configure_gate_focused_service\s+target_name\s*\)(?P<body>.*?)endfunction\s*\(",
            root_cmake,
            re.S,
        )
        self.assertIsNotNone(helper_match)
        helper = helper_match.group("body")
        self.assertIn("GateRuntimeCore", helper)
        self.assertNotIn("GateAppCore", helper)
        self.assertIn("add_subdirectory(AIGatewayService)", root_cmake)
        self.assertTrue((service_dir / "aigateway.ini").exists(), "AIGatewayServer config should exist")

    def test_logic_system_route_profile_seam_defaults_to_full(self):
        """The route profile seam is now a registrar list supplied by each
        focused executable/domain library. LogicSystem stays runtime-only."""
        header = strip_comments(read(GATE_SERVER / "LogicSystem.h"))
        source = strip_comments(read(GATE_SERVER / "LogicSystem.cpp"))

        self.assertNotIn("enum class RouteProfile", header)
        self.assertIn("using RouteProfileRegistrar", header)
        self.assertIn("static void AddRouteProfileRegistrar(RouteProfileRegistrar registrar)", header)
        self.assertIn("static void ClearRouteProfileRegistrars()", header)
        self.assertIn("RouteProfileRegistrars().push_back(std::move(registrar))", source)
        self.assertIn("RouteProfileRegistrars().clear()", source)
        self.assertIn("HealthRouteModule().RegisterRoutes(_route_registry)", source)
        self.assertIn("registrar(_route_registry)", source)

        for module in ("AuthRouteModule", "MediaRouteModule", "MomentsRouteModule", "R18RouteModule", "AIRouteModule"):
            with self.subTest(module=module):
                self.assertNotIn(module, source, f"LogicSystem must not directly include/register {module}")

    def test_aigateway_is_opt_in_in_runtime_topology(self):
        """AIGatewayServer must be a topology service but NOT in the default
        core start groups; start-all-services starts it explicitly after Envoy cut-over."""
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

    def test_phase4_domain_gateways_exist_as_top_level_services(self):
        """Phase 4: Media/Moments/Call/R18 each peeled into a standalone gateway
        exe linking GateRuntimeCore + one narrow domain library."""
        topology = read(REPO_ROOT / "tools" / "scripts" / "status" / "runtime_topology.sh")
        start = read(REPO_ROOT / "tools" / "scripts" / "status" / "start-all-services.sh")
        root_cmake = strip_comments(read(SERVER_CORE / "CMakeLists.txt"))
        core_match = re.search(r"MEMOCHAT_CORE_START_GROUPS=\(\n(?P<body>.*?)\n\)", topology, re.S)
        self.assertIsNotNone(core_match)
        core_groups = core_match.group("body")

        domains = {
            "Media": ("MediaGatewayServer", "mediagateway", "MediaGatewayService1", "MediaService", "GateMediaDomain"),
            "Moments": (
                "MomentsGatewayServer",
                "momentsgateway",
                "MomentsGatewayService1",
                "MomentsService",
                "GateMomentsDomain",
            ),
            "Call": ("CallGatewayServer", "callgateway", "CallGatewayService1", "CallService", "GateCallDomain"),
            "R18": ("R18GatewayServer", "r18gateway", "R18GatewayService1", "R18Service", "GateR18Domain"),
        }
        for profile, (exe, group, topo_dir, service_dir_name, domain_target) in domains.items():
            with self.subTest(domain=profile):
                service_dir = SERVER_CORE / service_dir_name
                cmake = strip_comments(read(service_dir / "CMakeLists.txt"))
                self.assertTrue((service_dir / "app" / f"{exe}.cpp").exists(), f"{exe}.cpp entry missing")
                self.assertTrue((service_dir / f"{group}.ini").exists(), f"{group}.ini config missing")
                entry = read(service_dir / "app" / f"{exe}.cpp")
                self.assertIn(f"profiles::Register{profile}", entry)
                self.assertRegex(cmake, rf"add_executable\s*\(\s*{exe}\b")
                self.assertIn(f"memochat_configure_gate_focused_service({exe} DOMAINS {domain_target})", cmake)
                self.assertIn(f"add_subdirectory({service_dir_name})", root_cmake)
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
        runtime_core_match = re.search(r"add_library\s*\(\s*GateRuntimeCore\s+STATIC(?P<body>.*?)\)", cmake, re.S)
        self.assertIsNotNone(runtime_core_match)
        self.assertIn("app/GateDomainServer.cpp", runtime_core_match.group("body"))

    def test_phase5_account_aggregate_split_register_login_account(self):
        """Phase 5/7 (D-ACCOUNT): Register/Login/Account peeled into separate
        processes. Register + Login reach account data only via account-core; the
        auth module exposes granular register/login registration so each process
        serves only its own routes."""
        topology = read(REPO_ROOT / "tools" / "scripts" / "status" / "runtime_topology.sh")
        start = read(REPO_ROOT / "tools" / "scripts" / "status" / "start-all-services.sh")
        auth_header = read(ACCOUNT_DOMAIN / "modules" / "auth" / "AuthRouteModule.h")
        root_cmake = strip_comments(read(SERVER_CORE / "CMakeLists.txt"))
        core_match = re.search(r"MEMOCHAT_CORE_START_GROUPS=\(\n(?P<body>.*?)\n\)", topology, re.S)
        self.assertIsNotNone(core_match)
        core_groups = core_match.group("body")

        # Granular auth registration (register vs login).
        self.assertIn("RegisterRegisterRoutes", auth_header)
        self.assertIn("RegisterLoginRoutes", auth_header)

        accounts = {
            "Register": (
                "RegisterServer",
                "register",
                "RegisterService1",
                "RegisterService",
                "GateAuthDomain",
                "RegisterRegister",
            ),
            "Login": ("LoginServer", "login", "LoginService1", "LoginService", "GateAuthDomain", "RegisterLogin"),
            "Account": (
                "AccountServer",
                "account",
                "AccountService1",
                "AccountService",
                "GateAccountDomain",
                "RegisterAccount",
            ),
        }
        for profile, (exe, group, topo_dir, service_dir_name, domain_target, registrar) in accounts.items():
            with self.subTest(service=profile):
                service_dir = SERVER_CORE / service_dir_name
                cmake = strip_comments(read(service_dir / "CMakeLists.txt"))
                self.assertTrue((service_dir / "app" / f"{exe}.cpp").exists(), f"{exe}.cpp entry missing")
                self.assertTrue((service_dir / f"{group}.ini").exists(), f"{group}.ini config missing")
                entry = read(service_dir / "app" / f"{exe}.cpp")
                self.assertIn(f"profiles::{registrar}", entry)
                self.assertRegex(cmake, rf"add_executable\s*\(\s*{exe}\b")
                self.assertIn(f"memochat_configure_gate_focused_service({exe} DOMAINS {domain_target})", cmake)
                self.assertIn(f"add_subdirectory({service_dir_name})", root_cmake)
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
        for service_name, exe in (
            ("RegisterService", "RegisterServer"),
            ("LoginService", "LoginServer"),
            ("AccountService", "AccountServer"),
        ):
            entry = read(SERVER_CORE / service_name / "app" / f"{exe}.cpp")
            self.assertNotIn("PostgresMgr", entry, f"{exe} entrypoint must not touch PostgresMgr directly")


if __name__ == "__main__":
    unittest.main()
