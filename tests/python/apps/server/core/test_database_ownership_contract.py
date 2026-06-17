"""Backend database ownership matrix contract.

The microservice split is only real if local configs, Helm config, and the
documented ownership matrix agree on each service database and app role.
"""

import configparser
import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps" / "server" / "core"
DOC = SERVER_CORE / "docs" / "database-ownership.md"
HELM_VALUES = REPO_ROOT / "infra" / "deploy" / "kubernetes" / "charts" / "memochat" / "values.yaml"
HELM_CONFIGMAP = (
    REPO_ROOT
    / "infra"
    / "deploy"
    / "kubernetes"
    / "charts"
    / "memochat"
    / "templates"
    / "bootstrap"
    / "configmap-services.yaml"
)

LOCAL_POSTGRES = {
    "MediaGatewayServer": (
        SERVER_CORE / "MediaService" / "mediagateway.ini",
        "Postgres",
        "memo_media",
        "memo_media_app",
    ),
    "MomentsGatewayServer": (
        SERVER_CORE / "MomentsService" / "momentsgateway.ini",
        "Postgres",
        "memo_moments",
        "memo_moments_app",
    ),
    "CallGatewayServer": (SERVER_CORE / "CallService" / "callgateway.ini", "Postgres", "memo_call", "memo_call_app"),
    "RegisterServer": (
        SERVER_CORE / "RegisterService" / "register.ini",
        "Postgres",
        "memo_account",
        "memo_account_app",
    ),
    "LoginServer": (SERVER_CORE / "LoginService" / "login.ini", "Postgres", "memo_account", "memo_account_app"),
    "AccountServer": (SERVER_CORE / "AccountService" / "account.ini", "Postgres", "memo_account", "memo_account_app"),
    "R18GatewayServer": (SERVER_CORE / "R18Service" / "r18gateway.ini", "Postgres", "memo_pg", "memochat"),
    "AIServer": (SERVER_CORE / "AIServer" / "config.ini", "Postgres", "memo_pg", "memochat"),
}

CHAT_LOCAL_CONFIGS = (
    SERVER_CORE / "ChatServer" / "chatserver1.ini",
    SERVER_CORE / "ChatServer" / "chatserver2.ini",
    SERVER_CORE / "ChatServer" / "chatmessageservice1.ini",
    SERVER_CORE / "ChatServer" / "chatrelationquery1.ini",
    SERVER_CORE / "ChatServer" / "chatrelationservice1.ini",
    SERVER_CORE / "ChatServer" / "chatdeliveryworker1.ini",
)

HELM_POSTGRES = {
    "mediagateway.ini": ("memo_media", "{{ .Values.externalServices.postgres.roles.media }}"),
    "momentsgateway.ini": ("memo_moments", "{{ .Values.externalServices.postgres.roles.moments }}"),
    "callgateway.ini": ("memo_call", "{{ .Values.externalServices.postgres.roles.call }}"),
    "register.ini": ("memo_account", "{{ .Values.externalServices.postgres.roles.account }}"),
    "login.ini": ("memo_account", "{{ .Values.externalServices.postgres.roles.account }}"),
    "account.ini": ("memo_account", "{{ .Values.externalServices.postgres.roles.account }}"),
    "r18gateway.ini": (
        "{{ .Values.externalServices.postgres.database }}",
        "{{ .Values.externalServices.postgres.user }}",
    ),
    "chat.ini": ("{{ .Values.externalServices.postgres.database }}", "{{ .Values.externalServices.postgres.user }}"),
}


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


def read_ini(path: Path) -> configparser.ConfigParser:
    parser = configparser.ConfigParser()
    parser.optionxform = str
    parser.read(path, encoding="utf-8-sig")
    return parser


def helm_ini_block(source: str, key: str) -> str:
    match = re.search(rf"^  {re.escape(key)}: \|\n(?P<body>(?:    .*\n?)*)", source, re.M)
    if not match:
        raise AssertionError(f"Helm ConfigMap block missing: {key}")
    return "\n".join(line[4:] if line.startswith("    ") else line for line in match.group("body").splitlines())


def ini_value(block: str, section: str, key: str) -> str:
    current = None
    for raw_line in block.splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("[") and line.endswith("]"):
            current = line[1:-1]
            continue
        if current == section and "=" in line:
            name, value = line.split("=", 1)
            if name == key:
                return value
    raise AssertionError(f"missing {section}.{key}")


class DatabaseOwnershipContractTests(unittest.TestCase):
    def test_documented_matrix_lists_current_database_owners(self):
        doc = read(DOC)
        for token in (
            "| MediaGatewayServer | `memo_media` | `memo_media_app` |",
            "| MomentsGatewayServer | `memo_moments` | `memo_moments_app` |",
            "| CallGatewayServer | `memo_call` | `memo_call_app` |",
            "| RegisterServer | `memo_account` | `memo_account_app` |",
            "| ChatServer ingress | `memo_pg` chat-owned legacy name | `memochat` |",
            "| R18GatewayServer | `memo_pg` legacy shared | `memochat` legacy shared |",
            "| AIServer | `memo_pg` legacy shared | `memochat` |",
            "`memochat:ai:semantic_cache:`",
            "Qdrant collection prefix `user_`",
        ):
            with self.subTest(token=token):
                self.assertIn(token, doc)

    def test_local_focused_service_postgres_ownership_matches_matrix(self):
        for service, (path, section, database, user) in LOCAL_POSTGRES.items():
            with self.subTest(service=service):
                ini = read_ini(path)
                self.assertTrue(ini.has_section(section), f"{path} missing [{section}]")
                self.assertEqual(database, ini[section]["Database"])
                self.assertEqual(user, ini[section]["User"])

    def test_local_chat_configs_keep_chat_db_and_account_bridge(self):
        for path in CHAT_LOCAL_CONFIGS:
            with self.subTest(config=path.name):
                ini = read_ini(path)
                self.assertEqual("memo_pg", ini["Postgres"]["Database"])
                self.assertEqual("memochat", ini["Postgres"]["User"])
                if path.name in {"chatserver1.ini", "chatserver2.ini"}:
                    self.assertTrue(ini.has_section("AccountPostgres"))
                    self.assertEqual("memo_account", ini["AccountPostgres"]["Database"])
                    self.assertEqual("memo_account_app", ini["AccountPostgres"]["User"])

    def test_helm_values_expose_domain_postgres_roles(self):
        values = read(HELM_VALUES)
        for role_key, role_name in (
            ("media", "memo_media_app"),
            ("moments", "memo_moments_app"),
            ("call", "memo_call_app"),
            ("account", "memo_account_app"),
        ):
            with self.subTest(role=role_key):
                self.assertRegex(values, rf"\b{role_key}:\s+{role_name}\b")

    def test_helm_configmap_postgres_ownership_matches_matrix(self):
        configmap = read(HELM_CONFIGMAP)
        for key, (database, user) in HELM_POSTGRES.items():
            with self.subTest(config=key):
                block = helm_ini_block(configmap, key)
                self.assertEqual(database, ini_value(block, "Postgres", "Database"))
                self.assertEqual(user, ini_value(block, "Postgres", "User"))

    def test_ai_orchestrator_external_store_names_are_explicit(self):
        config = read(SERVER_CORE / "AIOrchestrator" / "config.yaml")
        for token in (
            'collection_prefix: "user_"',
            'key_prefix: "memochat:ai:semantic_cache:"',
            'database: "neo4j"',
        ):
            with self.subTest(token=token):
                self.assertIn(token, config)


if __name__ == "__main__":
    unittest.main()
