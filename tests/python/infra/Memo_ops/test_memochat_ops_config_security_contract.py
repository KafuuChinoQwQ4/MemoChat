import unittest

import yaml

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
MEMO_OPS = REPO_ROOT / "infra/Memo_ops"
K8S_SECRETS = MEMO_OPS / "k8s/base/secrets.yaml"
K8S_PROD_OVERLAY = MEMO_OPS / "k8s/overlays/prod/kustomization.yaml"
ETCD_STATEFULSET = MEMO_OPS / "k8s/etcd/statefulset.yaml"
OPSSERVER = MEMO_OPS / "config/opsserver.yaml"
OPSCOLLECTOR = MEMO_OPS / "config/opscollector.yaml"


class MemoOpsConfigSecurityContractTests(unittest.TestCase):
    def test_k8s_base_secret_manifest_uses_placeholders_not_static_secrets(self):
        content = K8S_SECRETS.read_text(encoding="utf-8")
        manifest = yaml.safe_load(content)
        string_data = manifest["stringData"]

        forbidden_literals = {
            "memochat_pg_pass_2024",
            "memochat_redis_pass_2024",
            "memochat_app:memochat_mongo_2024",
            "memochat_kafka_pass_2024",
            "memochat_rabbitmq_pass_2024",
            "memochat_jwt_secret_key_2024_change_in_production",
            "memochat_rpc_shared_key_2024",
            "your_email_auth_code",
        }
        for value in string_data.values():
            with self.subTest(value=value):
                self.assertNotIn(value, forbidden_literals)
                self.assertTrue(str(value).startswith("CHANGE_ME_"), value)

        self.assertIn("Do not commit real production secrets", content)

    def test_prod_overlay_deletes_base_placeholder_secret_manifest(self):
        content = K8S_PROD_OVERLAY.read_text(encoding="utf-8")

        self.assertIn("- ../../base", content)
        self.assertIn("$patch: delete", content)
        self.assertRegex(
            content,
            r"(?s)kind:\s+Secret\s+metadata:\s+name:\s+memochat-secrets.*target:\s+kind:\s+Secret\s+name:\s+memochat-secrets",
        )
        self.assertNotIn("CHANGE_ME_", content)

    def test_memoops_password_config_values_are_env_substitutable(self):
        for path in (OPSSERVER, OPSCOLLECTOR):
            with self.subTest(path=path):
                data = yaml.safe_load(path.read_text(encoding="utf-8"))
                for section, env_name in (
                    ("mysql", "MEMOCHAT_OPS_MYSQL_PASSWORD"),
                    ("postgresql", "MEMOCHAT_OPS_POSTGRES_PASSWORD"),
                    ("redis", "MEMOCHAT_OPS_REDIS_PASSWORD"),
                ):
                    value = data[section]["password"]
                    self.assertIn("${", value)
                    self.assertIn(":-", value)
                    self.assertIn(env_name, value)
                    self.assertTrue(value.endswith("}"))

    def test_memoops_etcd_requires_external_root_password_secret(self):
        content = ETCD_STATEFULSET.read_text(encoding="utf-8")
        docs = list(yaml.safe_load_all(content))
        statefulset = next(doc for doc in docs if doc["kind"] == "StatefulSet")
        container = statefulset["spec"]["template"]["spec"]["containers"][0]
        env = {item["name"]: item for item in container["env"]}

        self.assertEqual(env["ALLOW_NONE_AUTHENTICATION"]["value"], "no")
        self.assertNotEqual(env["ALLOW_NONE_AUTHENTICATION"]["value"].lower(), "yes")
        self.assertNotIn("no-auth", content.lower())

        root_password = env["ETCD_ROOT_PASSWORD"]
        self.assertNotIn("value", root_password)
        self.assertEqual(
            root_password["valueFrom"]["secretKeyRef"],
            {
                "name": "memochat-etcd-secrets",
                "key": "root-password",
            },
        )
