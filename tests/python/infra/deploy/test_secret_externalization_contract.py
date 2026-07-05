import re
import unittest
from pathlib import Path

import yaml

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps/server/core"
LOCAL_DEPLOY = REPO_ROOT / "infra/deploy/local"
K8S_CHART = REPO_ROOT / "infra/deploy/kubernetes/charts/memochat"
AI_ORCHESTRATOR = SERVER_CORE / "AIOrchestrator"
SECRET_DOC = SERVER_CORE / "docs/secret-management.md"
SPLIT_COMPOSE_FILES = {
    "datastores": LOCAL_DEPLOY / "compose/datastores.yml",
    "rabbitmq": LOCAL_DEPLOY / "compose/rabbitmq.yml",
    "kafka": LOCAL_DEPLOY / "compose/kafka.yml",
    "observability": LOCAL_DEPLOY / "compose/observability.yml",
    "livekit": LOCAL_DEPLOY / "compose/livekit.yml",
}


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


def load_yaml(path: Path) -> dict:
    return yaml.safe_load(read(path))


def published_ports(path: Path) -> set[str]:
    compose = load_yaml(path)
    ports: set[str] = set()
    for service in compose.get("services", {}).values():
        for port in service.get("ports", []):
            if isinstance(port, str):
                ports.add(port)
            else:
                host_ip = port.get("host_ip", "")
                published = port.get("published")
                target = port.get("target")
                protocol = port.get("protocol")
                value = f"{host_ip}:{published}:{target}"
                if protocol:
                    value = f"{value}/{protocol}"
                ports.add(value)
    return ports


class SecretExternalizationContractTests(unittest.TestCase):
    def test_service_runtime_configs_do_not_commit_real_or_usable_third_party_secrets(self):
        varify_configs = [
            read(SERVER_CORE / "VarifyServer/config.ini"),
            read(SERVER_CORE / "VarifyServer/varify2.ini"),
        ]
        for text in varify_configs:
            with self.subTest(kind="varify"):
                self.assertNotIn("kafu_chino", text)
                self.assertNotIn("hrkhkvgptixfdfja", text)
                self.assertIn("MEMOCHAT_EMAIL_SMTPPASS", text)
                self.assertRegex(text, r"(?m)^SMTPUser=$")
                self.assertRegex(text, r"(?m)^SMTPPass=$")

        media = read(SERVER_CORE / "MediaService/mediagateway.ini")
        self.assertNotIn("MinioPass2026!", media)
        self.assertNotIn("AccessKey=memochat_admin", media)
        self.assertIn("MEMOCHAT_MINIO_SECRETKEY", media)
        self.assertRegex(media, r"(?m)^AccessKey=$")
        self.assertRegex(media, r"(?m)^SecretKey=$")

        call = read(SERVER_CORE / "CallService/callgateway.ini")
        self.assertNotIn("ApiKey=devkey", call)
        self.assertNotIn("ApiSecret=secret", call)
        self.assertIn("MEMOCHAT_CALL_APISECRET", call)
        self.assertRegex(call, r"(?m)^ApiKey=$")
        self.assertRegex(call, r"(?m)^ApiSecret=$")

        picacg_module = read(SERVER_CORE / "R18Service/domain/services/r18/cxx_modules/R18PicacgAdapter.cppm")
        self.assertNotIn("C69BAF41DA5ABD1FFEDC6D2FEA56B", picacg_module)
        self.assertNotIn("RK/P.RM4", picacg_module)
        self.assertIn("MEMOCHAT_R18_PICACG_API_KEY", picacg_module)
        self.assertIn("MEMOCHAT_R18_PICACG_HMAC_KEY", picacg_module)

    def test_local_compose_credentials_are_env_substitutable(self):
        files = {
            "main": read(LOCAL_DEPLOY / "docker-compose.yml"),
            "datastores": read(LOCAL_DEPLOY / "compose/datastores.yml"),
            "observability": read(LOCAL_DEPLOY / "compose/observability.yml"),
            "livekit": read(LOCAL_DEPLOY / "compose/livekit.yml"),
            "rabbitmq": read(LOCAL_DEPLOY / "compose/rabbitmq.yml"),
        }
        grafana_datasource = read(LOCAL_DEPLOY / "observability/grafana/provisioning/datasources/datasources.yml")

        expected_tokens = (
            "${MEMOCHAT_REDIS_PASSWORD:-123456}",
            "${MEMOCHAT_POSTGRES_PASSWORD:-123456}",
            "${MEMOCHAT_MONGO_ROOT_PASSWORD:-123456}",
            "${MEMOCHAT_RABBITMQ_PASSWORD:-123456}",
            "${MEMOCHAT_INFLUXDB_PASSWORD:-adminadmin}",
            "${MEMOCHAT_INFLUXDB_ADMIN_TOKEN:-my-super-secret-admin-token}",
            "${MEMOCHAT_GRAFANA_ADMIN_PASSWORD:-admin}",
            'MEMOCHAT_INFLUXDB_ADMIN_TOKEN: "${MEMOCHAT_INFLUXDB_ADMIN_TOKEN:-my-super-secret-admin-token}"',
            "${MEMOCHAT_LIVEKIT_API_SECRET:-secret}",
            "${MEMOCHAT_TURN_PASSWORD:-123456}",
        )
        combined = "\n".join(files.values())
        for token in expected_tokens:
            with self.subTest(token=token):
                self.assertIn(token, combined)

        forbidden_literal_patterns = (
            r"POSTGRES_PASSWORD:\s+[\"']?123456[\"']?",
            r"MONGO_INITDB_ROOT_PASSWORD:\s+[\"']?123456[\"']?",
            r"RABBITMQ_DEFAULT_PASS:\s+[\"']?(?:guest|123456)[\"']?",
            r"DOCKER_INFLUXDB_INIT_ADMIN_TOKEN:\s+my-super-secret-admin-token",
            r"GF_SECURITY_ADMIN_PASSWORD:\s+admin\b",
            r"LIVEKIT_KEYS:\s+[\"']devkey: secret[\"']",
            r"--user=memochat:123456",
        )
        for pattern in forbidden_literal_patterns:
            with self.subTest(pattern=pattern):
                self.assertIsNone(re.search(pattern, combined))

        self.assertIn("secureJsonData:", grafana_datasource)
        self.assertIn("token: ${MEMOCHAT_INFLUXDB_ADMIN_TOKEN}", grafana_datasource)
        self.assertNotIn("token: my-super-secret-admin-token", grafana_datasource)

    def test_split_local_compose_host_ports_bind_to_loopback(self):
        expected_ports = {
            "datastores": {
                "127.0.0.1:6379:6379",
                "127.0.0.1:15432:5432",
                "127.0.0.1:27017:27017",
            },
            "rabbitmq": {
                "127.0.0.1:5672:5672",
                "127.0.0.1:15672:15672",
            },
            "kafka": {
                "127.0.0.1:19092:19092",
                "127.0.0.1:18082:8082",
            },
            "observability": {
                "127.0.0.1:9090:9090",
                "127.0.0.1:8086:8086",
                "127.0.0.1:3000:3000",
                "127.0.0.1:3100:3100",
                "127.0.0.1:3200:3200",
                "127.0.0.1:4317:4317",
                "127.0.0.1:4318:4318",
                "127.0.0.1:9411:9411",
                "127.0.0.1:9464:9464",
                "127.0.0.1:8088:8080",
                "127.0.0.1:9400:9400",
            },
            "livekit": {
                "127.0.0.1:6379:6379",
                "127.0.0.1:7880:7880",
                "127.0.0.1:7881:7881/udp",
                "127.0.0.1:3478:3478",
                "127.0.0.1:49160-49200:49160-49200/udp",
            },
        }
        for name, expected in expected_ports.items():
            with self.subTest(compose=name):
                self.assertEqual(published_ports(SPLIT_COMPOSE_FILES[name]), expected)
                for port in expected:
                    self.assertTrue(port.startswith("127.0.0.1:"), port)

    def test_ai_stack_does_not_commit_usable_dependency_password_defaults(self):
        config_yaml = read(AI_ORCHESTRATOR / "config.yaml")
        config_py = read(AI_ORCHESTRATOR / "config.py")
        compose = read(AI_ORCHESTRATOR / "docker-compose.yml")
        k8s_config = read(K8S_CHART / "templates/bootstrap/configmap-services.yaml")
        k8s_secret = read(K8S_CHART / "templates/bootstrap/secret.yaml")
        k8s_ai = read(K8S_CHART / "templates/prod/ai.yaml")
        k8s_values = read(K8S_CHART / "values.yaml")

        for pattern in (
            r"(?m)^\s+password:\s+\"123456\"$",
            r"(?m)^\s+password:\s+\"password\"$",
            r"password:\s+str\s*=\s*\"123456\"",
            r"password:\s+str\s*=\s*\"password\"",
            r"Password=password",
        ):
            with self.subTest(pattern=pattern):
                self.assertIsNone(re.search(pattern, config_yaml + "\n" + config_py + "\n" + k8s_config))

        for token in (
            "MEMOCHAT_AI_POSTGRES__PASSWORD=${MEMOCHAT_AI_POSTGRES__PASSWORD:-123456}",
            "MEMOCHAT_AI_AGENT_QUEUE__RABBITMQ__PASSWORD=${MEMOCHAT_AI_AGENT_QUEUE__RABBITMQ__PASSWORD:-123456}",
            "MEMOCHAT_AI_SEMANTIC_CACHE__REDIS__PASSWORD=${MEMOCHAT_AI_SEMANTIC_CACHE__REDIS__PASSWORD:-123456}",
            "MEMOCHAT_AI_NEO4J__PASSWORD=${MEMOCHAT_AI_NEO4J__PASSWORD:-password}",
            "NEO4J_AUTH=neo4j/${MEMOCHAT_AI_NEO4J__PASSWORD:-password}",
        ):
            with self.subTest(token=token):
                self.assertIn(token, compose)

        self.assertIn(
            'neo4j-password: {{ required "aiOrchestrator.dependencies.neo4jPassword is required" .Values.aiOrchestrator.dependencies.neo4jPassword | quote }}',
            k8s_secret,
        )
        self.assertIn(
            'chat-auth-secret: {{ required "secrets.chatAuthSecret is required" .Values.secrets.chatAuthSecret | quote }}',
            k8s_secret,
        )
        self.assertNotIn("chatAuthSecret: memochat-dev-chat-secret", k8s_values)
        self.assertNotIn("livekitApiKey: devkey", k8s_values)
        self.assertNotIn("livekitApiSecret: secret", k8s_values)
        self.assertNotIn("apiKey: devkey", k8s_values)
        self.assertNotIn("apiSecret: secret", k8s_values)
        for token in (
            "MEMOCHAT_AI_POSTGRES__PASSWORD",
            "MEMOCHAT_AI_SEMANTIC_CACHE__REDIS__PASSWORD",
            "MEMOCHAT_AI_AGENT_QUEUE__RABBITMQ__PASSWORD",
            "MEMOCHAT_AI_NEO4J__PASSWORD",
            "MEMOCHAT_AI_INTERNAL_API_KEY",
            "MEMOCHAT_AI_PROVIDER_ADMIN_KEY",
            "neo4j-password",
            "ai-internal-api-key",
            "ai-provider-admin-key",
        ):
            with self.subTest(k8s_token=token):
                self.assertIn(token, k8s_ai + "\n" + k8s_secret)

    def test_helm_production_secret_template_requires_non_weak_values(self):
        secret_template = read(K8S_CHART / "templates/bootstrap/secret.yaml")
        values = read(K8S_CHART / "values.yaml")
        required_names = (
            "externalServices.postgres.password",
            "externalServices.redis.password",
            "externalServices.mongodb.uri",
            "externalServices.rabbitmq.username",
            "externalServices.rabbitmq.password",
            "externalServices.mysql.password",
            "externalServices.smtp.pass",
            "externalServices.livekit.apiKey",
            "externalServices.livekit.apiSecret",
            "aiOrchestrator.dependencies.neo4jPassword",
            "secrets.chatAuthSecret",
            "secrets.authRefreshPepper",
            "secrets.livekitApiKey",
            "secrets.livekitApiSecret",
            "secrets.minioAccessKey",
            "secrets.minioSecretKey",
            "secrets.aiInternalApiKey",
            "secrets.aiProviderAdminKey",
            "secrets.r18PicacgApiKey",
            "secrets.r18PicacgHmacKey",
        )
        for name in required_names:
            with self.subTest(name=name):
                self.assertIn(f'"{name}"', secret_template)
        self.assertIn('required (printf "%s is required" $name) $raw', secret_template)

        for weak_default in ("", "change-me", "guest", "password", "123456"):
            with self.subTest(weak_default=weak_default):
                self.assertIn(f'(eq $normalized "{weak_default}")', secret_template)
        self.assertIn('fail (printf "%s must be overridden with a production secret" $name)', secret_template)
        self.assertIn(
            'fail "externalServices.postgres.sslMode must not be disable in the production Helm chart"',
            secret_template,
        )
        self.assertIn("sslMode: verify-full", values)
        self.assertNotIn("sslMode: disable", values)

    def test_helm_chart_has_service_mesh_and_external_secret_manager_contracts(self):
        values = read(K8S_CHART / "values.yaml")
        namespace_template = read(K8S_CHART / "templates/bootstrap/namespaces.yaml")
        mesh_template = read(K8S_CHART / "templates/bootstrap/mesh-istio.yaml")
        external_secret_template = read(K8S_CHART / "templates/bootstrap/external-secrets.yaml")
        secret_template = read(K8S_CHART / "templates/bootstrap/secret.yaml")
        gate_template = read(K8S_CHART / "templates/prod/gate.yaml")
        focused_template = read(K8S_CHART / "templates/prod/focused-gateways.yaml")
        ai_template = read(K8S_CHART / "templates/prod/ai.yaml")

        self.assertIn("mesh:", values)
        self.assertIn("provider: istio", values)
        self.assertIn("mtlsMode: STRICT", values)
        self.assertIn("externalSecrets:", values)
        self.assertIn("secretStoreRef:", values)
        self.assertIn("memochat/prod/app", values)
        self.assertIn("memochat/prod/etcd-tls", values)

        self.assertIn("istio-injection:", namespace_template)
        self.assertIn("PeerAuthentication", mesh_template)
        self.assertIn("mode: {{ .Values.mesh.istio.mtlsMode", mesh_template)
        self.assertIn("DestinationRule", mesh_template)
        self.assertIn("ISTIO_MUTUAL", mesh_template)
        self.assertIn("AuthorizationPolicy", mesh_template)
        self.assertIn(".Values.mesh.istio.authorizationPolicy.allowedNamespaces", mesh_template)

        self.assertIn("{{- if not .Values.externalSecrets.enabled }}", secret_template)
        self.assertIn("ExternalSecret", external_secret_template)
        self.assertIn("ClusterSecretStore", external_secret_template)
        self.assertIn("creationPolicy: Owner", external_secret_template)
        self.assertIn("kubernetes.io/tls", external_secret_template)
        for key in (
            "auth-refresh-pepper",
            "minio-access-key",
            "minio-secret-key",
            "ai-internal-api-key",
            "ai-provider-admin-key",
            "r18-picacg-api-key",
            "r18-picacg-hmac-key",
        ):
            with self.subTest(key=key):
                self.assertIn(key, external_secret_template)
                self.assertIn(key, secret_template)

        combined_workloads = gate_template + "\n" + focused_template + "\n" + ai_template
        for env_name in (
            "MEMOCHAT_AUTH_REFRESH_PEPPER",
            "MEMOCHAT_MINIO_ACCESSKEY",
            "MEMOCHAT_MINIO_SECRETKEY",
            "MEMOCHAT_AI_INTERNAL_API_KEY",
            "MEMOCHAT_AI_PROVIDER_ADMIN_KEY",
            "MEMOCHAT_R18_PICACG_API_KEY",
            "MEMOCHAT_R18_PICACG_HMAC_KEY",
        ):
            with self.subTest(env_name=env_name):
                self.assertIn(env_name, combined_workloads)

    def test_secret_management_doc_lists_required_production_overrides(self):
        docs = read(SECRET_DOC)
        for token in (
            "MEMOCHAT_CHATAUTH_HMACSECRET",
            "MEMOCHAT_EMAIL_SMTPPASS",
            "MEMOCHAT_MINIO_SECRETKEY",
            "MINIO_SECRET_KEY",
            "MEMOCHAT_CALL_APISECRET",
            "MEMOCHAT_R18_PICACG_API_KEY",
            "MEMOCHAT_R18_PICACG_HMAC_KEY",
            "MEMOCHAT_AI_POSTGRES__PASSWORD",
            "MEMOCHAT_AI_AGENT_QUEUE__RABBITMQ__PASSWORD",
            "MEMOCHAT_AI_SEMANTIC_CACHE__REDIS__PASSWORD",
            "MEMOCHAT_AI_NEO4J__PASSWORD",
            "MEMOCHAT_AI_INTERNAL_API_KEY",
            "MEMOCHAT_AI_PROVIDER_ADMIN_KEY",
            "${ENV:-dev_default}",
        ):
            with self.subTest(token=token):
                self.assertIn(token, docs)


if __name__ == "__main__":
    unittest.main()
