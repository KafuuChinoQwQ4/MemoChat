import shutil
import subprocess
import time
import unittest
import uuid
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
POSTGRES_PROVISION = REPO_ROOT / "infra/deploy/local/provision/postgres.sh"
SPLIT_MIGRATION = REPO_ROOT / "infra/deploy/local/provision/postgres_legacy_split.sh"
PROVISION_DIRECTORY = POSTGRES_PROVISION.parent
MIGRATION_DIRECTORY = REPO_ROOT / "apps/server/migrations/postgresql/business"
POSTGRES_IMAGE = "postgres@sha256:78df81b1442dcc764c1104154da7162635e40cfffe67579c42a1c1b96dfc209c"


@unittest.skipUnless(shutil.which("docker"), "requires Docker")
class PostgresLegacySplitMigrationTests(unittest.TestCase):
    def setUp(self):
        suffix = uuid.uuid4().hex[:12]
        self.network = f"memochat-split-test-{suffix}"
        self.server = f"memochat-split-postgres-{suffix}"
        self.password = f"fixture-{suffix}-password"
        subprocess.run(["docker", "network", "create", self.network], check=True, capture_output=True)
        started = subprocess.run(
            [
                "docker",
                "run",
                "--detach",
                "--rm",
                "--name",
                self.server,
                "--network",
                self.network,
                "--network-alias",
                "memochat-postgres",
                "--tmpfs",
                "/var/lib/postgresql/data:rw,noexec,nosuid,nodev",
                "--env",
                "POSTGRES_USER=memochat",
                "--env",
                f"POSTGRES_PASSWORD={self.password}",
                "--env",
                "POSTGRES_DB=postgres",
                POSTGRES_IMAGE,
            ],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        if started.returncode != 0:
            self._cleanup()
            self.fail(started.stdout)
        for _ in range(100):
            logs = subprocess.run(
                ["docker", "logs", self.server],
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=False,
            ).stdout
            if logs.count("database system is ready to accept connections") >= 2:
                break
            time.sleep(0.2)
        else:
            self.fail("temporary Postgres did not become ready")

    def tearDown(self):
        self._cleanup()

    def _cleanup(self):
        if hasattr(self, "server"):
            subprocess.run(
                ["docker", "rm", "--force", self.server],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=False,
            )
        if hasattr(self, "network"):
            subprocess.run(
                ["docker", "network", "rm", self.network],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=False,
            )

    def sql(self, database: str, sql: str) -> str:
        result = subprocess.run(
            [
                "docker",
                "exec",
                "--interactive",
                self.server,
                "psql",
                "-X",
                "-v",
                "ON_ERROR_STOP=1",
                "-U",
                "memochat",
                "-d",
                database,
                "-tA",
            ],
            input=sql,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        self.assertEqual(0, result.returncode, result.stdout)
        return result.stdout.strip()

    def assert_sql_denied(self, database: str, role: str, sql: str):
        result = subprocess.run(
            [
                "docker",
                "exec",
                "--interactive",
                self.server,
                "psql",
                "-X",
                "-v",
                "ON_ERROR_STOP=1",
                "-U",
                "memochat",
                "-d",
                database,
            ],
            input=f'SET ROLE "{role}";\n{sql}\n',
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        self.assertNotEqual(0, result.returncode, result.stdout)
        self.assertIn("permission denied", result.stdout.lower())

    def run_migration(self) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                "docker",
                "run",
                "--rm",
                "--network",
                self.network,
                "--mount",
                f"type=bind,src={SPLIT_MIGRATION},dst=/provision/postgres_legacy_split.sh,readonly",
                "--env",
                "PGHOST=memochat-postgres",
                "--env",
                "PGPORT=5432",
                "--env",
                "PGUSER=memochat",
                "--env",
                f"PGPASSWORD={self.password}",
                "--env",
                "MEMOCHAT_POSTGRES_DATABASE=memo_pg",
                "--env",
                "MEMOCHAT_PROVISION_CALLS=1",
                POSTGRES_IMAGE,
                "sh",
                "/provision/postgres_legacy_split.sh",
            ],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )

    def run_provisioner(self, *, enable_calls: bool) -> subprocess.CompletedProcess[str]:
        environment = {
            "MEMOCHAT_POSTGRES_USER": "memochat",
            "MEMOCHAT_POSTGRES_DATABASE": "memo_pg",
            "MEMOCHAT_POSTGRES_PASSWORD": self.password,
            "MEMOCHAT_CHAT_POSTGRES_PASSWORD": "fixture-chat-password-0123456789",
            "MEMOCHAT_ACCOUNT_POSTGRES_PASSWORD": "fixture-account-password-0123456789",
            "MEMOCHAT_MEDIA_POSTGRES_PASSWORD": "fixture-media-password-0123456789",
            "MEMOCHAT_MOMENTS_POSTGRES_PASSWORD": "fixture-moments-password-0123456789",
            "MEMOCHAT_PROVISION_CALLS": "1" if enable_calls else "0",
        }
        if enable_calls:
            environment["MEMOCHAT_CALL_POSTGRES_PASSWORD"] = "fixture-call-password-0123456789"

        command = [
            "docker",
            "run",
            "--rm",
            "--network",
            self.network,
            "--mount",
            f"type=bind,src={PROVISION_DIRECTORY},dst=/provision,readonly",
            "--mount",
            f"type=bind,src={MIGRATION_DIRECTORY},dst=/migrations,readonly",
        ]
        for name, value in environment.items():
            command.extend(("--env", f"{name}={value}"))
        command.extend((POSTGRES_IMAGE, "sh", "/provision/postgres.sh"))
        return subprocess.run(
            command,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )

    def prepare_fixture(self):
        self.sql("postgres", "CREATE ROLE memo_account_app;")
        self.sql("postgres", 'CREATE DATABASE "memo_pg";')
        for database in ("memo_account", "memo_media", "memo_moments", "memo_call"):
            self.sql("postgres", f'CREATE DATABASE "{database}";')

        source_tables = (
            '"user"',
            "user_id",
            "auth_refresh_token",
            "chat_media_asset",
            "chat_media_access_grant",
            "moments",
            "moments_comment",
            "moments_comment_like",
            "moments_like",
            "chat_call_session",
        )
        source_sql = ["CREATE SCHEMA memo;"]
        for table in source_tables:
            source_sql.extend(
                (
                    f"CREATE TABLE memo.{table} (id integer PRIMARY KEY);",
                    f"INSERT INTO memo.{table} VALUES (1);",
                )
            )
        self.sql("memo_pg", "\n".join(source_sql))

        destination_tables = {
            "memo_account": ('"user"', "user_id", "auth_refresh_token"),
            "memo_media": ("chat_media_asset", "chat_media_access_grant"),
            "memo_moments": ("moments", "moments_comment", "moments_comment_like", "moments_like"),
            "memo_call": ("chat_call_session",),
        }
        for database, tables in destination_tables.items():
            statements = ["CREATE SCHEMA memo;"]
            for table in tables:
                constraint = " CONSTRAINT reject_legacy_rows CHECK (id < 0)" if table == '"user"' else ""
                statements.append(f"CREATE TABLE memo.{table} (id integer PRIMARY KEY{constraint});")
            self.sql(database, "\n".join(statements))
        self.sql("memo_call", "INSERT INTO memo.chat_call_session VALUES (999);")

    def test_copy_and_marker_are_atomic_and_completed_tables_never_rehydrate(self):
        self.prepare_fixture()

        failed = self.run_migration()
        self.assertNotEqual(0, failed.returncode, failed.stdout)
        self.assertEqual("0", self.sql("memo_account", 'SELECT count(*) FROM memo."user";'))

        self.assertEqual(
            "0",
            self.sql(
                "memo_account",
                "SELECT count(*) FROM memochat_release.data_migration "
                "WHERE migration_id = 'legacy-split-v1:memo_account:user';",
            ),
        )
        self.assert_sql_denied(
            "memo_account",
            "memo_account_app",
            "SELECT * FROM memochat_release.data_migration;",
        )
        self.assert_sql_denied(
            "memo_account",
            "memo_account_app",
            "DELETE FROM memochat_release.data_migration;",
        )

        self.sql("memo_account", 'ALTER TABLE memo."user" DROP CONSTRAINT reject_legacy_rows;')
        succeeded = self.run_migration()
        self.assertEqual(0, succeeded.returncode, succeeded.stdout)
        self.assertEqual("1", self.sql("memo_account", 'SELECT count(*) FROM memo."user";'))
        self.assertEqual("1", self.sql("memo_media", "SELECT count(*) FROM memo.chat_media_asset;"))
        self.assertEqual("1", self.sql("memo_moments", "SELECT count(*) FROM memo.moments;"))
        self.assertEqual("999", self.sql("memo_call", "SELECT id FROM memo.chat_call_session;"))

        self.sql("memo_pg", 'INSERT INTO memo."user" VALUES (2);')
        second = self.run_migration()
        self.assertEqual(0, second.returncode, second.stdout)
        self.assertEqual("1", self.sql("memo_account", 'SELECT count(*) FROM memo."user";'))

        self.sql("memo_account", 'DELETE FROM memo."user";')
        after_clear = self.run_migration()
        self.assertEqual(0, after_clear.returncode, after_clear.stdout)
        self.assertEqual("0", self.sql("memo_account", 'SELECT count(*) FROM memo."user";'))

    def test_base_provision_skips_call_database_until_calls_are_enabled(self):
        self.sql("postgres", 'CREATE DATABASE "memo_pg";')

        base = self.run_provisioner(enable_calls=False)

        self.assertEqual(0, base.returncode, base.stdout)
        self.assertEqual(
            "f",
            self.sql("postgres", "SELECT EXISTS (SELECT 1 FROM pg_database WHERE datname = 'memo_call');"),
        )
        self.assertEqual(
            "f",
            self.sql("postgres", "SELECT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'memo_call_app');"),
        )

        calls = self.run_provisioner(enable_calls=True)

        self.assertEqual(0, calls.returncode, calls.stdout)
        self.assertEqual(
            "t",
            self.sql("postgres", "SELECT EXISTS (SELECT 1 FROM pg_database WHERE datname = 'memo_call');"),
        )
        self.assertEqual(
            "t",
            self.sql("memo_call", "SELECT to_regclass('memo.chat_call_session') IS NOT NULL;"),
        )

    def test_main_provisioner_delegates_to_the_marker_migration(self):
        source = POSTGRES_PROVISION.read_text(encoding="utf-8")

        self.assertIn("/provision/postgres_legacy_split.sh", source)
        self.assertNotIn("copy_table_if_empty", source)


if __name__ == "__main__":
    unittest.main()
