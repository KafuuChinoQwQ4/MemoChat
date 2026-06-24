"""gateserver split Phase 2 (slice 2a) database-ownership contract.

Guards the per-service PostgreSQL split for the safely-splittable GateServer
domains (media / moments / call): the migration + rollback scripts exist and are
paired, each gateway service config points at its OWN database with its OWN
least-privilege role, and the schema files declare the expected tables. account
is intentionally NOT split here (slice 2b — ChatServer still JOINs the user
table, which a cross-DB split would break); r18 has no PG tables.

These are static contract checks (no live DB needed); the live migrate/verify/
rollback cycle is exercised by tools/scripts/status/migrate_phase2_db_split.sh.
"""

import configparser
import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps" / "server" / "core"
GATE_SERVER = SERVER_CORE / "GateShared"
MIG_DIR = REPO_ROOT / "apps" / "server" / "migrations" / "postgresql" / "business"
MIGRATE_SCRIPT = REPO_ROOT / "tools" / "scripts" / "status" / "migrate_phase2_db_split.sh"

# Each peeled service now owns its own top-level directory under server/core;
# its config.ini lives there (gateserver split source-tree restructure).
SERVICE_CONFIG = {
    "mediagateway.ini": SERVER_CORE / "MediaService" / "mediagateway.ini",
    "momentsgateway.ini": SERVER_CORE / "MomentsService" / "momentsgateway.ini",
    "callgateway.ini": SERVER_CORE / "CallService" / "callgateway.ini",
    "register.ini": SERVER_CORE / "RegisterService" / "register.ini",
    "login.ini": SERVER_CORE / "LoginService" / "login.ini",
    "account.ini": SERVER_CORE / "AccountService" / "account.ini",
}


def service_config(name: str) -> Path:
    return SERVICE_CONFIG[name]


# domain -> (config file, expected database, expected role, expected tables)
SPLIT = {
    "media": ("mediagateway.ini", "memo_media", "memo_media_app", ["chat_media_asset"]),
    "moments": (
        "momentsgateway.ini",
        "memo_moments",
        "memo_moments_app",
        ["moments", "moments_comment", "moments_comment_like", "moments_like"],
    ),
    "call": ("callgateway.ini", "memo_call", "memo_call_app", ["chat_call_session"]),
}

SCHEMA_FILE = {
    "media": "008_memo_media_schema.sql",
    "moments": "008_memo_moments_schema.sql",
    "call": "008_memo_call_schema.sql",
}


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


def read_ini(path: Path) -> configparser.ConfigParser:
    parser = configparser.ConfigParser()
    parser.optionxform = str
    parser.read(path, encoding="utf-8-sig")
    return parser


class Phase2DbSplitContractTests(unittest.TestCase):
    def test_migration_and_rollback_scripts_are_paired(self):
        self.assertTrue((MIG_DIR / "008_db_split_media_moments_call.sql").exists())
        self.assertTrue((MIG_DIR / "008_db_split_media_moments_call_rollback.sql").exists())
        for sf in SCHEMA_FILE.values():
            with self.subTest(schema=sf):
                self.assertTrue((MIG_DIR / sf).exists(), f"{sf} schema file missing")
        self.assertTrue(MIGRATE_SCRIPT.exists(), "migrate_phase2_db_split.sh missing")
        driver = read(MIGRATE_SCRIPT)
        for verb in ("do_migrate", "do_verify", "do_rollback"):
            self.assertIn(verb, driver, f"migration driver should support {verb}")
        # forward migration must never drop the source memo_pg tables.
        self.assertNotRegex(driver, r"DROP\s+TABLE\s+memo\.", "forward migration must not drop source tables")

    def test_each_gateway_config_targets_its_own_database_and_role(self):
        for domain, (cfg, db, role, _tables) in SPLIT.items():
            with self.subTest(domain=domain):
                ini = read_ini(service_config(cfg))
                self.assertTrue(ini.has_section("Postgres"), f"{cfg} missing [Postgres]")
                self.assertEqual(db, ini["Postgres"]["Database"], f"{cfg} should target {db}")
                self.assertEqual(role, ini["Postgres"]["User"], f"{cfg} should use role {role}")
                # must NOT still point at the shared maintenance db.
                self.assertNotEqual("memo_pg", ini["Postgres"]["Database"])

    def test_schema_files_declare_expected_tables_and_least_privilege_grants(self):
        for domain, (_cfg, _db, role, tables) in SPLIT.items():
            with self.subTest(domain=domain):
                schema = read(MIG_DIR / SCHEMA_FILE[domain])
                for t in tables:
                    self.assertRegex(
                        schema,
                        rf"CREATE TABLE IF NOT EXISTS memo\.{re.escape(t)}\b",
                        f"{SCHEMA_FILE[domain]} should create memo.{t}",
                    )
                # least-privilege grants to the owning role, no superuser grants.
                self.assertIn(f"TO {role}", schema, f"{SCHEMA_FILE[domain]} should grant to {role}")
                self.assertNotIn("memochat", schema, "schema file should not grant to the maintenance role")

    def test_account_split_slice_2b(self):
        """Phase 2b: the account aggregate (user/user_id) is split into
        memo_account. Register/Login/Account services target memo_account with
        the memo_account_app role; ChatServer reads user data via a separate
        [AccountPostgres] connection (its chat tables stay on memo_pg). r18 still
        has no PG tables."""
        # account migration scripts exist + are paired.
        self.assertTrue((MIG_DIR / "009_memo_account_schema.sql").exists())
        acct_migrate = REPO_ROOT / "tools" / "scripts" / "status" / "migrate_phase2_account_split.sh"
        self.assertTrue(acct_migrate.exists(), "account split migration script missing")
        driver = read(acct_migrate)
        for verb in ("do_migrate", "do_verify", "do_rollback"):
            self.assertIn(verb, driver)
        self.assertNotRegex(driver, r"DROP\s+TABLE\s+memo\.", "account migration must not drop source tables")

        # account services point their whole [Postgres] at memo_account.
        for cfg in ("register.ini", "login.ini", "account.ini"):
            ini = read_ini(service_config(cfg))
            with self.subTest(cfg=cfg):
                self.assertEqual("memo_account", ini["Postgres"]["Database"], f"{cfg} should target memo_account")
                self.assertEqual("memo_account_app", ini["Postgres"]["User"])

        # ChatServer and independent chat workers keep chat tables on memo_pg but read user via [AccountPostgres].
        for cfg in (
            "chatserver1.ini",
            "chatserver2.ini",
            "chatrelationquery1.ini",
            "chatrelationservice1.ini",
            "chatmessageservice1.ini",
            "chatdeliveryworker1.ini",
        ):
            ini = read_ini(REPO_ROOT / "apps" / "server" / "core" / "ChatServer" / cfg)
            with self.subTest(cfg=cfg):
                self.assertEqual("memo_pg", ini["Postgres"]["Database"], f"{cfg} chat tables stay on memo_pg")
                self.assertTrue(ini.has_section("AccountPostgres"), f"{cfg} needs [AccountPostgres]")
                self.assertEqual("memo_account", ini["AccountPostgres"]["Database"])
                self.assertEqual("memo_account_app", ini["AccountPostgres"]["User"])

    def test_chatserver_user_joins_replaced_with_batch_resolver(self):
        """Phase 2b prerequisite: ChatServer's cross-table JOIN "user" queries are
        replaced by the GetUsersByUids batch resolver, so the user table can move
        to memo_account without a cross-DB JOIN."""
        persistence = REPO_ROOT / "apps" / "server" / "core" / "ChatServer" / "persistence"
        # the batch resolver exists and uses the account connection.
        dao = read(persistence / "PostgresDaoUsers.cpp")
        self.assertIn("PostgresDao::GetUsersByUids", dao)
        self.assertIn("account_connection_string_", dao)
        # no live pqxx JOIN "user" remains in the relation/group/message queries.
        for fname in (
            "PostgresDao.cpp",
            "PostgresDaoUsers.cpp",
            "PostgresDaoGroups.cpp",
            "PostgresDaoGroupMessages.cpp",
        ):
            src = read(persistence / fname)
            # strip // comments so the explanatory comments do not trip the check.
            code = re.sub(r"//.*", "", src)
            for m in re.finditer(r'JOIN\s+\\"user\\"', code):
                # any remaining JOIN "user" must be in a sql:: (dead MySQL) block, not pqxx.
                start = code.rfind("prepareStatement", max(0, m.start() - 400), m.start())
                with self.subTest(file=fname, pos=m.start()):
                    self.assertNotEqual(-1, start, f'{fname} has a live (pqxx) JOIN "user" that should be decoupled')


if __name__ == "__main__":
    unittest.main()
