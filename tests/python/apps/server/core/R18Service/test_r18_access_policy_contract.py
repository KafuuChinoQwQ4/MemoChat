import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps/server/core"
R18_ROOT = SERVER_CORE / "R18Service"
ACCOUNT_SHARED = SERVER_CORE / "AccountShared"
GATE_PERSISTENCE = SERVER_CORE / "GateShared/core/persistence"
MIGRATIONS = REPO_ROOT / "apps/server/migrations/postgresql/business"


def read(path):
    return path.read_text(encoding="utf-8-sig")


class R18AccessPolicyContractTests(unittest.TestCase):
    def test_account_schema_defaults_r18_access_to_denied(self):
        migration_path = MIGRATIONS / "013_r18_access_policy.sql"
        self.assertTrue(migration_path.exists())
        migration = read(migration_path)
        account_schema = read(MIGRATIONS / "009_memo_account_schema.sql")
        local_init = read(REPO_ROOT / "infra/deploy/local/init/postgresql/001-business.sql")

        for source in (migration, account_schema, local_init):
            with self.subTest(source=source[:40]):
                self.assertIn("adult_attested_at_ms", source)
                self.assertIn("r18_access_state", source)
                self.assertRegex(source, r"r18_access_state\s+smallint\s+[^,\n]*DEFAULT\s+0")
                self.assertIn("CHECK (r18_access_state IN (0, 1, 2))", source)
                self.assertIn("CHECK (adult_attested_at_ms >= 0)", source)

    def test_account_facade_owns_attestation_and_revocation_invariant(self):
        header = read(ACCOUNT_SHARED / "domain/services/account/AccountPersistence.hpp")
        implementation = read(ACCOUNT_SHARED / "domain/services/account/AccountPersistence.cpp")
        dao_header = read(GATE_PERSISTENCE / "PostgresDao.hpp")
        dao = read(GATE_PERSISTENCE / "PostgresDaoAccount.cpp")

        for token in (
            "struct R18AccessPolicy",
            "GetR18AccessPolicy",
            "AttestAdultForR18",
            "adult_attested_at_ms",
            "r18_access_state",
        ):
            with self.subTest(token=token):
                self.assertIn(token, header + implementation + dao_header + dao)

        self.assertIn("CASE WHEN r18_access_state = 2 THEN 2 ELSE 1 END", dao)
        self.assertNotIn("uid == 1", implementation + dao)

    def test_r18_routes_use_account_owned_policy_and_fail_closed(self):
        service = read(R18_ROOT / "domain/services/r18/R18Service.cpp")
        route_module = read(R18_ROOT / "domain/modules/r18/R18RouteModule.cpp")
        registration = read(R18_ROOT / "domain/modules/r18/cxx_modules/R18RouteRegistration.cppm")
        cmake = read(R18_ROOT / "CMakeLists.txt")
        config = read(R18_ROOT / "r18gateway.ini")

        self.assertIn("GateAccountCore", cmake)
        self.assertIn("Database=memo_account", config)
        self.assertIn("R18AccessStatusPath", registration)
        self.assertIn("R18AccessAttestPath", registration)
        self.assertIn("/api/r18/access", registration)
        self.assertIn("/api/r18/access/attest", registration)
        self.assertIn("HandleAccessStatus", service)
        self.assertIn("HandleAccessAttest", service)
        self.assertIn("RequireR18Access", service)
        self.assertIn("AccountPersistence::Instance().GetR18AccessPolicy", service)
        self.assertIn("response.status = 403", service)
        self.assertIn("HandleAccessStatus", route_module)
        self.assertIn("HandleAccessAttest", route_module)

        helper_start = service.index("bool HandleJsonRequest")
        helper_end = service.index("bool HandleAdminJsonRequest", helper_start)
        self.assertIn("RequireR18Access", service[helper_start:helper_end])

        shared_guard_handlers = (
            "HandleSearch",
            "HandleComicDetail",
            "HandleChapterPages",
            "HandleFavoriteToggle",
            "HandleHistoryUpdate",
        )
        for handler in shared_guard_handlers:
            start = service.index(f"bool R18Service::{handler}(")
            next_handler = service.find("\nbool R18Service::", start + 1)
            body = service[start:] if next_handler < 0 else service[start:next_handler]
            with self.subTest(handler=handler):
                self.assertIn("HandleJsonRequest", body)

        for handler in ("HandleListSources", "HandleHistory", "HandleImage"):
            start = service.index(f"bool R18Service::{handler}(")
            next_handler = service.find("\nbool R18Service::", start + 1)
            body = service[start:] if next_handler < 0 else service[start:next_handler]
            with self.subTest(handler=handler):
                self.assertIn("RequireR18Access", body)

        for helper in ("HandleJsonRequest", "HandleAdminJsonRequest"):
            start = service.index(f"bool {helper}(")
            next_helper = service.find("\nbool ", start + 1)
            body = service[start:] if next_helper < 0 else service[start:next_helper]
            with self.subTest(helper=helper):
                self.assertIn("UnauthorizedHttpStatus", body)

        for handler in ("HandleListSources", "HandleHistory"):
            start = service.index(f"bool R18Service::{handler}(")
            next_handler = service.find("\nbool R18Service::", start + 1)
            body = service[start:] if next_handler < 0 else service[start:next_handler]
            with self.subTest(handler=handler):
                self.assertIn("UnauthorizedHttpStatus", body)

    def test_web_requires_server_attestation_before_loading_content(self):
        endpoints = read(REPO_ROOT / "apps/web/src/core/config/endpoints.ts")
        component = read(REPO_ROOT / "apps/web/src/features/r18/components/R18ShellContent.tsx")

        self.assertIn('r18Access:         "/api/r18/access"', endpoints)
        self.assertIn('r18AccessAttest:   "/api/r18/access/attest"', endpoints)
        self.assertIn("accessQuery", component)
        self.assertIn("attestR18Access", component)
        self.assertIn("我已年满 18 岁", component)
        self.assertIn("enabled: authReady && accessQuery.data?.allowed === true", component)

    def test_qml_requires_server_attestation_before_loading_content(self):
        controller = read(REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/r18/controller/R18Controller.cpp")
        header = read(REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/r18/controller/R18Controller.h")
        shell = read(REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/r18/view/R18ShellPane.qml")

        self.assertIn('/api/r18/access"', controller)
        self.assertIn('/api/r18/access/attest"', controller)
        self.assertIn("accessAllowed", header)
        self.assertIn("root.r18Controller.refreshAccess()", shell)
        self.assertIn("我已年满 18 岁", shell)
        self.assertNotIn("root.r18Controller.refreshSources()\n            root.r18Controller.refreshHistory()", shell)


if __name__ == "__main__":
    unittest.main()
