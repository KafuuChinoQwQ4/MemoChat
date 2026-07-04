import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
TOOLS = REPO_ROOT / "tools"
SECRET_DOC = REPO_ROOT / "apps/server/core/docs/secret-management.md"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


class ToolSecretExternalizationContractTests(unittest.TestCase):
    def test_docker_services_mcp_reads_admin_credentials_from_environment(self):
        text = read(TOOLS / "mcps/user-docker-services/user_docker_services_mcp_server.py")

        for token in (
            "MEMOCHAT_GRAFANA_ADMIN_USER",
            "MEMOCHAT_GRAFANA_ADMIN_PASSWORD",
            "MEMOCHAT_RABBITMQ_USER",
            "MEMOCHAT_RABBITMQ_PASSWORD",
            "MEMOCHAT_INFLUXDB_ADMIN_TOKEN",
            "MEMOCHAT_MINIO_ROOT_USER",
            "MEMOCHAT_MINIO_ROOT_PASSWORD",
            "MEMOCHAT_MINIO_ACCESSKEY",
            "MEMOCHAT_MINIO_SECRETKEY",
        ):
            with self.subTest(token=token):
                self.assertIn(token, text)

        for forbidden in (
            '("admin", "admin")',
            '("memochat", "123456")',
            "INFLUX_TOKEN =",
            "my-super-secret-admin-token",
            'access_key="memochat_admin"',
            'secret_key="MinioPass2026!"',
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, text)

    def test_mongodb_mcp_builds_uri_from_environment(self):
        text = read(TOOLS / "mcps/user-mongodb/user_mongodb_mcp_server.py")

        for token in (
            "MEMOCHAT_MONGODB_URI",
            "MEMOCHAT_MONGO_URI",
            "MEMOCHAT_MONGO_APP_USER",
            "MEMOCHAT_MONGO_APP_PASSWORD",
            "MEMOCHAT_MONGO_HOST",
            "urllib.parse.quote_plus",
            "_redact_known_secrets",
        ):
            with self.subTest(token=token):
                self.assertIn(token, text)

        self.assertNotIn("mongodb://memochat_app:123456@127.0.0.1:27017/memochat", text)
        self.assertNotIn('MONGO_URI = "mongodb://', text)

    def test_neo4j_mcp_reads_password_from_environment(self):
        text = read(TOOLS / "mcps/user-neo4j/user_neo4j_mcp_server.py")

        for token in (
            "MEMOCHAT_NEO4J_URI",
            "MEMOCHAT_NEO4J_USER",
            "MEMOCHAT_NEO4J_PASSWORD",
            "MEMOCHAT_AI_NEO4J__PASSWORD",
            "_redact_known_secrets",
        ):
            with self.subTest(token=token):
                self.assertIn(token, text)

        self.assertNotIn('NEO4J_PASSWORD = "password"', text)
        self.assertNotIn("auth=(NEO4J_USER, NEO4J_PASSWORD)", text)

    def test_system_bench_uses_environment_backed_connection_settings(self):
        text = read(TOOLS / "loadtest/python-loadtest/system_bench.py")

        for token in (
            "MEMOCHAT_REDIS_PASSWORD",
            "MEMOCHAT_POSTGRES_PASSWORD",
            "MEMOCHAT_MONGODB_URI",
            "MEMOCHAT_MONGO_APP_PASSWORD",
            "postgres_conn_kwargs",
            "mongo_uri_from_env",
            "urllib.parse.quote_plus",
        ):
            with self.subTest(token=token):
                self.assertIn(token, text)

        forbidden_patterns = (
            r"password\s*=\s*[\"']123456[\"']",
            r"password=123456",
            r"mongodb://memochat_app:123456@127\.0\.0\.1:27017/memochat",
            r"conninfo\s*=\s*[\"'][^\"']*password=123456",
        )
        for pattern in forbidden_patterns:
            with self.subTest(pattern=pattern):
                self.assertIsNone(re.search(pattern, text))

    def test_minio_helper_scripts_do_not_embed_usable_object_store_credentials(self):
        paths = (
            TOOLS / "scripts/loadtest-accounts/migrate_media_to_minio.py",
            TOOLS / "scripts/status/ensure_minio_buckets.sh",
            TOOLS / "scripts/dev/runtime_smoke_full_chat.py",
            TOOLS / "scripts/status/smoke_domain_gateway_runtime.sh",
            TOOLS / "scripts/gateserver/start_gateserver.bat",
            REPO_ROOT / "infra/Memo_ops/bin/start_minio.bat",
        )

        combined = "\n".join(read(path) for path in paths)
        for token in (
            "MEMOCHAT_MINIO_ROOT_USER",
            "MEMOCHAT_MINIO_ROOT_PASSWORD",
            "MEMOCHAT_MINIO_ACCESSKEY",
            "MEMOCHAT_MINIO_SECRETKEY",
        ):
            with self.subTest(token=token):
                self.assertIn(token, combined)

        for forbidden in (
            "MINIO_SECRET_KEY:-MinioPass2026!",
            'secret_key="MinioPass2026!"',
            'set "MINIO_ROOT_PASSWORD=MinioPass2026!"',
            "Console 密码: MinioPass2026!",
            "AccessKey=memochat_admin",
            "SecretKey=MinioPass2026!",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, combined)

    def test_local_maintenance_scripts_externalize_infrastructure_passwords(self):
        paths = (
            TOOLS / "scripts/infra-init/init_rabbitmq_topology.ps1",
            TOOLS / "scripts/db/cleanup_password_redis_cache.sh",
            TOOLS / "scripts/verify/read_verify_code.js",
            TOOLS / "scripts/status/migrate_phase2_account_split.sh",
            TOOLS / "scripts/status/migrate_phase2_db_split.sh",
            REPO_ROOT / "apps/server/migrations/postgresql/business/008_db_split_media_moments_call.sql",
            TOOLS / "loadtest/python-loadtest/py_loadtest.py",
        )
        combined = "\n".join(read(path) for path in paths)

        for token in (
            "MEMOCHAT_RABBITMQ_PASSWORD",
            "MEMOCHAT_REDIS_PASSWORD",
            "MEMOCHAT_PHASE2_SERVICE_ROLE_PASSWORD",
            "MEMOCHAT_ACCOUNT_DB_ROLE_PASSWORD",
            "MEMOCHAT_CHATAUTH_HMACSECRET",
        ):
            with self.subTest(token=token):
                self.assertIn(token, combined)

        for forbidden in (
            'else { "123456" }',
            'REDIS_PASSWORD="${MEMOCHAT_REDIS_PASSWORD:-123456}"',
            "password: '123456'",
            "PGPASSWORD=123456",
            "PASSWORD '123456'",
            'config.get("quic_ticket_secret", "memochat-dev-chat-secret")',
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, combined)

    def test_secret_management_doc_lists_developer_tool_overrides(self):
        docs = read(SECRET_DOC)

        for token in (
            "MEMOCHAT_GRAFANA_ADMIN_PASSWORD",
            "MEMOCHAT_RABBITMQ_PASSWORD",
            "MEMOCHAT_INFLUXDB_ADMIN_TOKEN",
            "MEMOCHAT_MINIO_ROOT_PASSWORD",
            "MEMOCHAT_REDIS_PASSWORD",
            "MEMOCHAT_POSTGRES_PASSWORD",
            "MEMOCHAT_MONGODB_URI",
            "MEMOCHAT_MONGO_APP_PASSWORD",
            "MEMOCHAT_NEO4J_PASSWORD",
            "MEMOCHAT_AI_NEO4J__PASSWORD",
            "MEMOCHAT_PHASE2_SERVICE_ROLE_PASSWORD",
            "MEMOCHAT_ACCOUNT_DB_ROLE_PASSWORD",
        ):
            with self.subTest(token=token):
                self.assertIn(token, docs)


if __name__ == "__main__":
    unittest.main()
