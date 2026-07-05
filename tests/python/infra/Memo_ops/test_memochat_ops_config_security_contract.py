import unittest

import yaml

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
MEMO_OPS = REPO_ROOT / "infra/Memo_ops"
K8S_SECRETS = MEMO_OPS / "k8s/base/secrets.yaml"
K8S_CONFIGMAP = MEMO_OPS / "k8s/base/configmap.yaml"
K8S_NETWORK_POLICY = MEMO_OPS / "k8s/base/networkpolicy.yaml"
K8S_PROD_OVERLAY = MEMO_OPS / "k8s/overlays/prod/kustomization.yaml"
K8S_PROD_EXTERNAL_SECRETS = MEMO_OPS / "k8s/overlays/prod/external-secrets.yaml"
K8S_PROD_MESH = MEMO_OPS / "k8s/overlays/prod/mesh-istio.yaml"
CD_WORKFLOW = REPO_ROOT / ".github/workflows/cd.yml"
ETCD_STATEFULSET = MEMO_OPS / "k8s/etcd/statefulset.yaml"
OPSSERVER = MEMO_OPS / "config/opsserver.yaml"
OPSCOLLECTOR = MEMO_OPS / "config/opscollector.yaml"
K8S_WORKLOAD_MANIFESTS = (
    ETCD_STATEFULSET,
    MEMO_OPS / "k8s/backend/chatserver.yaml",
    MEMO_OPS / "k8s/backend/gateserver.yaml",
    MEMO_OPS / "k8s/backend/varifyserver.yaml",
    MEMO_OPS / "k8s/infra/kafka.yaml",
    MEMO_OPS / "k8s/infra/mongodb.yaml",
    MEMO_OPS / "k8s/infra/postgres.yaml",
    MEMO_OPS / "k8s/infra/rabbitmq.yaml",
    MEMO_OPS / "k8s/infra/redis.yaml",
)
K8S_OVERLAYS = (
    MEMO_OPS / "k8s/overlays/dev/kustomization.yaml",
    MEMO_OPS / "k8s/overlays/dev-single/kustomization.yaml",
    MEMO_OPS / "k8s/overlays/staging/kustomization.yaml",
    K8S_PROD_OVERLAY,
)


def load_yaml_docs(path):
    return [doc for doc in yaml.safe_load_all(path.read_text(encoding="utf-8")) if doc]


def workload_docs():
    for path in K8S_WORKLOAD_MANIFESTS:
        for doc in load_yaml_docs(path):
            if doc.get("kind") in {"Deployment", "StatefulSet"}:
                yield path, doc


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
        self.assertIn("- external-secrets.yaml", content)
        self.assertIn("- mesh-istio.yaml", content)
        self.assertIn("istio-injection: enabled", content)
        self.assertIn("$patch: delete", content)
        self.assertRegex(
            content,
            r"(?s)kind:\s+Secret\s+metadata:\s+name:\s+memochat-secrets.*target:\s+kind:\s+Secret\s+name:\s+memochat-secrets",
        )
        self.assertNotIn("CHANGE_ME_", content)
        self.assertNotIn("newTag: latest", content)
        self.assertIn("newTag: prod-digest-required", content)

    def test_prod_overlay_uses_istio_strict_mtls_resources(self):
        docs = {doc["kind"]: doc for doc in load_yaml_docs(K8S_PROD_MESH)}

        peer_auth = docs["PeerAuthentication"]
        self.assertEqual(peer_auth["apiVersion"], "security.istio.io/v1")
        self.assertEqual(peer_auth["metadata"]["namespace"], "memochat")
        self.assertEqual(peer_auth["spec"]["mtls"]["mode"], "STRICT")

        destination_rule = docs["DestinationRule"]
        self.assertEqual(destination_rule["apiVersion"], "networking.istio.io/v1")
        self.assertEqual(destination_rule["spec"]["trafficPolicy"]["tls"]["mode"], "ISTIO_MUTUAL")
        self.assertEqual(destination_rule["spec"]["host"], "*.memochat.svc.cluster.local")

        authz = docs["AuthorizationPolicy"]
        self.assertEqual(authz["apiVersion"], "security.istio.io/v1")
        self.assertEqual(authz["spec"]["action"], "ALLOW")
        namespaces = authz["spec"]["rules"][0]["from"][0]["source"]["namespaces"]
        self.assertEqual(namespaces, ["memochat"])

    def test_prod_overlay_uses_external_secrets_for_secret_manager_contract(self):
        docs = load_yaml_docs(K8S_PROD_EXTERNAL_SECRETS)
        by_name = {doc["metadata"]["name"]: doc for doc in docs}

        store = by_name["memochat-vault"]
        self.assertEqual(store["kind"], "ClusterSecretStore")
        vault = store["spec"]["provider"]["vault"]
        self.assertTrue(vault["server"].startswith("https://"), vault["server"])
        self.assertEqual(vault["version"], "v2")
        self.assertIn("kubernetes", vault["auth"])
        self.assertEqual(vault["auth"]["kubernetes"]["role"], "memochat-prod")

        app_secret = by_name["memochat-secrets"]
        self.assertEqual(app_secret["kind"], "ExternalSecret")
        self.assertEqual(app_secret["spec"]["secretStoreRef"], {"name": "memochat-vault", "kind": "ClusterSecretStore"})
        self.assertEqual(app_secret["spec"]["target"]["name"], "memochat-secrets")
        self.assertEqual(app_secret["spec"]["target"]["creationPolicy"], "Owner")
        secret_keys = {item["secretKey"] for item in app_secret["spec"]["data"]}
        for key in (
            "postgres-password",
            "redis-password",
            "mongodb-uri",
            "rabbitmq-username",
            "rabbitmq-password",
            "neo4j-password",
            "mysql-password",
            "smtp-pass",
            "chat-auth-secret",
            "auth-refresh-pepper",
            "livekit-api-key",
            "livekit-api-secret",
            "minio-access-key",
            "minio-secret-key",
            "ai-internal-api-key",
            "ai-provider-admin-key",
            "r18-picacg-api-key",
            "r18-picacg-hmac-key",
        ):
            with self.subTest(key=key):
                self.assertIn(key, secret_keys)

        ops_secret = by_name["memochat-ops-secrets"]
        self.assertEqual(ops_secret["spec"]["target"]["name"], "memochat-ops-secrets")
        self.assertIn("mysql-password", {item["secretKey"] for item in ops_secret["spec"]["data"]})

        etcd_secret = by_name["memochat-etcd-secrets"]
        self.assertEqual(etcd_secret["spec"]["target"]["name"], "memochat-etcd-secrets")
        self.assertIn("root-password", {item["secretKey"] for item in etcd_secret["spec"]["data"]})

        etcd_tls = by_name["memochat-etcd-tls"]
        self.assertEqual(etcd_tls["spec"]["target"]["template"]["type"], "kubernetes.io/tls")
        self.assertEqual({item["secretKey"] for item in etcd_tls["spec"]["data"]}, {"ca.crt", "tls.crt", "tls.key"})

    def test_prod_cd_rewrites_overlay_to_immutable_image_digests(self):
        workflow = CD_WORKFLOW.read_text(encoding="utf-8")
        prod_block = workflow[workflow.index("deploy-prod:") :]

        self.assertNotIn(":latest", prod_block)
        self.assertIn("docker buildx imagetools inspect", prod_block)
        self.assertIn("id: image-digests", prod_block)
        self.assertIn("Materialize production image digests", prod_block)
        self.assertIn("newName: {os.environ['GATESERVER_IMAGE']}", prod_block)
        self.assertIn("digest: {os.environ['GATESERVER_DIGEST']}", prod_block)
        self.assertIn("newName: {os.environ['CHATSERVER_IMAGE']}", prod_block)
        self.assertIn("digest: {os.environ['CHATSERVER_DIGEST']}", prod_block)
        self.assertIn("newName: {os.environ['VARIFYSERVER_IMAGE']}", prod_block)
        self.assertIn("digest: {os.environ['VARIFYSERVER_DIGEST']}", prod_block)
        self.assertLess(
            prod_block.index("Materialize production image digests"),
            prod_block.index("kubectl apply -k infra/Memo_ops/k8s/overlays/prod"),
        )

    def test_memoops_password_config_values_are_env_substitutable(self):
        for path in (OPSSERVER, OPSCOLLECTOR):
            with self.subTest(path=path):
                data = yaml.safe_load(path.read_text(encoding="utf-8"))
                self.assertNotEqual(data["mysql"]["user"], "root")
                self.assertIn("MEMOCHAT_OPS_MYSQL_USER", data["mysql"]["user"])
                self.assertEqual(data["postgresql"]["sslmode"], "require")
                for section, env_name in (
                    ("mysql", "MEMOCHAT_OPS_MYSQL_PASSWORD"),
                    ("postgresql", "MEMOCHAT_OPS_POSTGRES_PASSWORD"),
                    ("redis", "MEMOCHAT_OPS_REDIS_PASSWORD"),
                ):
                    value = data[section]["password"]
                    self.assertIn("${", value)
                    self.assertNotIn(":-", value)
                    self.assertNotIn("123456", value)
                    self.assertIn(env_name, value)
                    self.assertTrue(value.endswith("}"))

    def test_opsserver_cors_does_not_allow_wildcard_origin(self):
        data = yaml.safe_load(OPSSERVER.read_text(encoding="utf-8"))
        origins = data["server"]["cors_origins"]

        self.assertNotIn("*", origins)
        self.assertIn("${MEMOCHAT_OPS_CORS_ORIGIN}", origins)

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

    def test_k8s_workloads_drop_root_and_privilege_escalation(self):
        for path, doc in workload_docs():
            with self.subTest(path=path, workload=doc["metadata"]["name"]):
                pod_spec = doc["spec"]["template"]["spec"]
                pod_security = pod_spec["securityContext"]
                self.assertTrue(pod_security["runAsNonRoot"])
                self.assertGreater(pod_security["runAsUser"], 0)
                self.assertEqual(pod_security["seccompProfile"]["type"], "RuntimeDefault")
                for container in pod_spec["containers"]:
                    container_security = container["securityContext"]
                    self.assertFalse(container_security["allowPrivilegeEscalation"])
                    self.assertIn("ALL", container_security["capabilities"]["drop"])

    def test_k8s_etcd_and_clients_use_tls_endpoint_contract(self):
        etcd_docs = load_yaml_docs(ETCD_STATEFULSET)
        statefulset = next(doc for doc in etcd_docs if doc["kind"] == "StatefulSet")
        container = statefulset["spec"]["template"]["spec"]["containers"][0]
        env = {item["name"]: item["value"] for item in container["env"] if "value" in item}

        for key in (
            "ETCD_ADVERTISE_CLIENT_URLS",
            "ETCD_INITIAL_ADVERTISE_PEER_URLS",
            "ETCD_LISTEN_CLIENT_URLS",
            "ETCD_LISTEN_PEER_URLS",
        ):
            with self.subTest(key=key):
                self.assertTrue(env[key].startswith("https://"), env[key])
        self.assertEqual(env["ETCD_CLIENT_CERT_AUTH"], "true")
        self.assertEqual(env["ETCD_PEER_CLIENT_CERT_AUTH"], "true")
        volumes = statefulset["spec"]["template"]["spec"]["volumes"]
        self.assertIn({"name": "etcd-tls", "secret": {"secretName": "memochat-etcd-tls"}}, volumes)

        combined_config = (
            K8S_CONFIGMAP.read_text(encoding="utf-8")
            + "\n"
            + "\n".join(path.read_text(encoding="utf-8") for path in K8S_OVERLAYS)
        )
        self.assertNotIn("http://memochat-etcd:2379", combined_config)
        self.assertIn("https://memochat-etcd:2379", combined_config)

    def test_k8s_chatserver_mongodb_uri_is_secret_backed_not_plain_config(self):
        config = yaml.safe_load(K8S_CONFIGMAP.read_text(encoding="utf-8"))
        self.assertEqual(config["data"]["MONGODB_URI"], "")

        chatserver = next(
            doc for doc in load_yaml_docs(MEMO_OPS / "k8s/backend/chatserver.yaml") if doc["kind"] == "StatefulSet"
        )
        env = {item["name"]: item for item in chatserver["spec"]["template"]["spec"]["containers"][0]["env"]}
        self.assertEqual(
            env["MEMOCHAT_MONGODB_URI"]["valueFrom"]["secretKeyRef"],
            {"name": "memochat-secrets", "key": "mongodb-uri"},
        )

    def test_k8s_namespace_has_default_deny_and_allowlisted_network_policies(self):
        policies = {doc["metadata"]["name"]: doc for doc in load_yaml_docs(K8S_NETWORK_POLICY)}

        self.assertIn("memochat-default-deny", policies)
        self.assertEqual(policies["memochat-default-deny"]["spec"]["podSelector"], {})
        self.assertEqual(set(policies["memochat-default-deny"]["spec"]["policyTypes"]), {"Ingress", "Egress"})
        self.assertNotIn("ingress", policies["memochat-default-deny"]["spec"])
        self.assertNotIn("egress", policies["memochat-default-deny"]["spec"])

        self.assertIn("memochat-backend-to-internal-services", policies)
        backend_egress = policies["memochat-backend-to-internal-services"]["spec"]["egress"]
        exposed_ports = {port["port"] for rule in backend_egress for port in rule.get("ports", [])}
        for required_port in (2379, 5432, 6379, 27017, 9092, 19092, 5672, 8080, 8081, 8090, 8190, 50051, 50054, 50055):
            with self.subTest(required_port=required_port):
                self.assertIn(required_port, exposed_ports)
