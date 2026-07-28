import hashlib
import os
import re
import shutil
import socket
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path

import yaml

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
DOCKERFILE = REPO_ROOT / "infra/deploy/images/services/cpp-service.Dockerfile"
ENTRYPOINT = REPO_ROOT / "infra/deploy/images/common/entrypoints/server-entrypoint.sh"
COMPOSE_FILE = REPO_ROOT / "infra/deploy/local/compose/backend-services.yml"
BASE_COMPOSE_FILE = REPO_ROOT / "infra/deploy/local/docker-compose.yml"
LIVEKIT_COMPOSE_FILE = REPO_ROOT / "infra/deploy/local/compose/livekit.yml"
ENV_EXAMPLE = REPO_ROOT / "infra/deploy/local/.env.release.example"
TOPOLOGY_FILE = REPO_ROOT / "tools/scripts/status/runtime_topology.sh"
BUILD_IMAGES = REPO_ROOT / "tools/scripts/release/build_backend_images.sh"
AUTH_SERVICE = REPO_ROOT / "apps/server/core/AccountShared/domain/services/auth/AuthService.cpp"
CHAT_QUIC_SERVER = REPO_ROOT / "apps/server/core/ChatServer/transport/QuicChatServer.cpp"
LEGACY_GATE_H3 = REPO_ROOT / "apps/server/core/GateShared/transports/h3/listener/GateHttp3Listener.cpp"
DEV_CERT_GENERATOR = REPO_ROOT / "infra/deploy/local/compose/generate-envoy-certs.sh"
POSTGRES_PROVISION = REPO_ROOT / "infra/deploy/local/provision/postgres.sh"
CHAT_POSTGRES_DIALOGS = REPO_ROOT / "apps/server/core/ChatServer/persistence/PostgresDaoDialogs.cpp"
MONGO_PROVISION = REPO_ROOT / "infra/deploy/local/provision/mongo.js"
MINIO_PROVISION = REPO_ROOT / "infra/deploy/local/provision/minio.sh"
INFLUXDB_PROVISION = REPO_ROOT / "infra/deploy/local/provision/influxdb_observability.sh"
GRAFANA_ENTRYPOINT = REPO_ROOT / "infra/deploy/local/provision/grafana_entrypoint.sh"
PROMETHEUS_CONFIG = REPO_ROOT / "infra/deploy/local/observability/prometheus/prometheus.yml"
OTEL_CONFIG = REPO_ROOT / "infra/deploy/local/observability/otel/config.yaml"
GRAFANA_DATASOURCES = REPO_ROOT / "infra/deploy/local/observability/grafana/provisioning/datasources/datasources.yml"
VARIFY_CONFIGS = (
    REPO_ROOT / "apps/server/core/VarifyServer/config.ini",
    REPO_ROOT / "apps/server/core/VarifyServer/varify2.ini",
)
PINNED_RUNTIME_IMAGE = "ubuntu@sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90"


class ComposeLoader(yaml.SafeLoader):
    pass


def construct_override(loader: ComposeLoader, node: yaml.Node):
    if isinstance(node, yaml.MappingNode):
        return loader.construct_mapping(node)
    if isinstance(node, yaml.SequenceNode):
        return loader.construct_sequence(node)
    return loader.construct_scalar(node)


ComposeLoader.add_constructor("!override", construct_override)


def topology_targets() -> set[str]:
    text = TOPOLOGY_FILE.read_text(encoding="utf-8")
    rows = re.findall(r'^\s*"([^"\n]+)"\s*$', text, flags=re.MULTILINE)
    return {row.split("|")[2] for row in rows if row.count("|") >= 4}


def run_entrypoint(*args: str, environment: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    env = os.environ.copy()
    env.update(environment or {})
    return subprocess.run(
        ["/bin/sh", str(ENTRYPOINT), *args],
        cwd=REPO_ROOT,
        env=env,
        text=True,
        capture_output=True,
        check=False,
    )


class BackendReleaseContractTests(unittest.TestCase):
    def test_observability_discovery_uses_release_service_names_and_only_bundled_targets(self):
        datasources = yaml.safe_load(GRAFANA_DATASOURCES.read_text(encoding="utf-8"))["datasources"]
        by_uid = {datasource["uid"]: datasource for datasource in datasources}

        self.assertEqual(by_uid["prometheus"]["url"], "http://memochat-prometheus:9090")
        self.assertEqual(by_uid["influxdb"]["url"], "http://memochat-influxdb:8086")
        self.assertEqual(by_uid["loki"]["url"], "http://memochat-loki:3100")
        self.assertEqual(by_uid["tempo"]["url"], "http://memochat-tempo:3200")
        self.assertEqual(by_uid["influxdb"]["jsonData"]["version"], "Flux")
        self.assertEqual(by_uid["influxdb"]["jsonData"]["defaultBucket"], "metrics")
        self.assertEqual(
            by_uid["influxdb"]["secureJsonData"]["token"],
            "${MEMOCHAT_INFLUXDB_GRAFANA_TOKEN}",
        )
        self.assertNotIn("ADMIN_TOKEN", by_uid["influxdb"]["secureJsonData"]["token"])

        prometheus = yaml.safe_load(PROMETHEUS_CONFIG.read_text(encoding="utf-8"))
        jobs = {scrape_config["job_name"] for scrape_config in prometheus["scrape_configs"]}
        self.assertNotIn("ai_orchestrator", jobs)
        self.assertNotIn("remote_write", prometheus)
        for rules_file in PROMETHEUS_CONFIG.parent.joinpath("rules").glob("*.yml"):
            rules_text = rules_file.read_text(encoding="utf-8")
            absent_jobs = set(re.findall(r'absent\(up\{job="([^"]+)"\}\)', rules_text))
            with self.subTest(default_absent_alerts=rules_file.name):
                self.assertTrue(absent_jobs.issubset(jobs))

        provision = INFLUXDB_PROVISION.read_text(encoding="utf-8")
        self.assertIn("/api/v2/scrapers", provision)
        self.assertIn("memochat-prometheus:9090/federate", provision)
        self.assertIn("INFLUX_TOKEN", provision)
        self.assertIn("INFLUX_TOKEN_FILE", provision)
        self.assertIn("mode 0400", provision)
        self.assertIn("influx auth create", provision)
        self.assertIn("--read-bucket", provision)
        self.assertIn("grafana-reader.token", provision)
        self.assertNotIn("--token", provision)
        self.assertIn("--config -", provision)

        grafana_entrypoint = GRAFANA_ENTRYPOINT.read_text(encoding="utf-8")
        self.assertIn("grafana-reader.token", grafana_entrypoint)
        self.assertIn("MEMOCHAT_INFLUXDB_GRAFANA_TOKEN", grafana_entrypoint)
        self.assertNotIn("MEMOCHAT_INFLUXDB_ADMIN_TOKEN", grafana_entrypoint)
        self.assertIn("exec /run.sh", grafana_entrypoint)

        otel = yaml.safe_load(OTEL_CONFIG.read_text(encoding="utf-8"))
        health_extension = otel["extensions"]["health_check"]
        self.assertEqual(health_extension["endpoint"], "0.0.0.0:13133")
        self.assertEqual(health_extension["path"], "/healthz")
        self.assertIn("receivers:", health_extension["response_body"]["healthy"])
        self.assertIn("health_check", otel["service"]["extensions"])

    def test_runtime_image_consumes_only_an_explicit_service_bundle(self):
        text = DOCKERFILE.read_text(encoding="utf-8")

        self.assertIn("FROM service_bundle AS service_bundle", text)
        self.assertIn("FROM server_entrypoint AS server_entrypoint", text)
        self.assertRegex(text, r"COPY\s+--from=service_bundle[^\n]+/bin/\$\{TARGET\}")
        self.assertRegex(text, r"COPY\s+--from=service_bundle[^\n]+/lib/")
        self.assertRegex(text, r"COPY\s+--from=server_entrypoint[^\n]+/server-entrypoint\.sh")
        self.assertIn("libmsquic.so.2", text)
        self.assertIn("libturbojpeg \\", text)
        self.assertNotIn("libturbojpeg0", text)
        self.assertNotIn("apt-get upgrade", text)
        self.assertIn(
            "ARG RUNTIME_IMAGE=ubuntu@sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90",
            text,
        )
        self.assertIn(
            "ARG MEMOCHAT_BASE_DIGEST=sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90",
            text,
        )
        self.assertIn("io.memochat.base.digest", text)
        self.assertIn("RUNTIME_IMAGE##*@", text)
        self.assertIn("ARG UBUNTU_SNAPSHOT=20260727T000000Z", text)
        self.assertIn('test "$UBUNTU_SNAPSHOT" = 20260727T000000Z', text)
        self.assertIn(
            "ADD --checksum=sha256:6bac2a01979e210d9eac1d4d56747ec709ea60654744d66705dc3c36e7629e50",
            text,
        )
        self.assertIn(
            "https://snapshot.ubuntu.com/ubuntu/20260727T000000Z/pool/main/c/ca-certificates/ca-certificates_20260601~24.04.1_all.deb",
            text,
        )
        self.assertIn(
            'io.memochat.ubuntu.snapshot="${UBUNTU_SNAPSHOT}"',
            text,
        )
        self.assertIn(
            'io.memochat.ubuntu.runtime-packages="ca-certificates=20260601~24.04.1,'
            'libturbojpeg=1:2.1.5-2ubuntu2,libwebp7=1.3.2-0.4build3"',
            text,
        )
        self.assertIn(
            "URIs: https://snapshot.ubuntu.com/ubuntu/${UBUNTU_SNAPSHOT}",
            text,
        )
        self.assertIn("Components: main universe", text)
        self.assertIn("Suites: noble noble-updates", text)
        self.assertIn(
            "s/^Suites: noble noble-updates noble-backports$/Suites: noble noble-updates/",
            text,
        )
        self.assertIn(
            "Suites: .*noble-backports|Components: .*(restricted|multiverse)",
            text,
        )
        self.assertIn("Check-Valid-Until: no", text)
        self.assertIn(
            "--mount=type=secret,id=memochat_builder_ca,required=false,mode=0444",
            text,
        )
        self.assertIn("APT::Update::Error-Mode=any", text)
        self.assertIn(
            "test \"$(dpkg-query -W ca-certificates | cut -f2)\" = '20260601~24.04.1'",
            text,
        )
        self.assertIn(
            "test \"$(dpkg-query -W libturbojpeg | cut -f2)\" = '1:2.1.5-2ubuntu2'",
            text,
        )
        self.assertIn(
            "test \"$(dpkg-query -W libwebp7 | cut -f2)\" = '1.3.2-0.4build3'",
            text,
        )
        self.assertIn("test -x /usr/bin/bash", text)
        self.assertIn("test -x /usr/bin/timeout", text)
        self.assertRegex(text, r"COPY\s+--from=service_bundle[^\n]+/sbom/")
        self.assertIn("vcpkg-build-dependencies.spdx.json", text)
        snapshot_update = "apt-get -o APT::Update::Error-Mode=any -o Acquire::Retries=3 ${apt_ca_option} update"
        self.assertIn(snapshot_update, text)
        self.assertLess(text.index(snapshot_update), text.index("ARG TARGET=ChatServer"))
        self.assertLess(text.index(snapshot_update), text.index('RUN case "${TARGET}"'))
        self.assertIn("not found", text)
        self.assertNotRegex(text, r"(?m)^COPY\s+\.\s")
        self.assertNotRegex(text, r"(?m)^COPY\s+apps/")
        self.assertNotRegex(text, r"(?m)^COPY\s+infra/Memo_ops/runtime")
        self.assertNotRegex(text, r"(?m)^COPY\s+.*config\.ini")
        self.assertNotIn("GateServer", text)
        self.assertRegex(text, r"(?m)^USER\s+10001:10001$")

        for target in topology_targets():
            with self.subTest(target=target):
                self.assertIn(target, text)

        build_images = BUILD_IMAGES.read_text(encoding="utf-8")
        self.assertIn("vcpkg-build-dependencies.spdx.json", build_images)
        self.assertIn("--vcpkg-installed-root", build_images)
        self.assertIn("vcpkg_sbom_coverage=installed-closure-overapproximation", build_images)
        self.assertIn("sha256sum --check --strict", build_images)
        self.assertIn("--pull=false", build_images)
        self.assertIn('--build-arg "RUNTIME_IMAGE=${RUNTIME_IMAGE}"', build_images)
        self.assertIn("--pull)", build_images)
        self.assertNotIn("GateServer", build_images)

    def test_entrypoint_rejects_unknown_service_and_missing_config(self):
        unknown = run_entrypoint(
            environment={
                "MEMOCHAT_SERVICE": "../../bin/sh",
                "CONFIG_PATH": "/does/not/exist.ini",
                "MEMOCHAT_RELEASE_MODE": "1",
            }
        )
        self.assertNotEqual(unknown.returncode, 0)
        self.assertIn("Unsupported MEMOCHAT_SERVICE", unknown.stderr)

        missing = run_entrypoint(
            environment={
                "MEMOCHAT_SERVICE": "ChatServer",
                "CONFIG_PATH": "/does/not/exist.ini",
                "MEMOCHAT_RELEASE_MODE": "1",
            }
        )
        self.assertNotEqual(missing.returncode, 0)
        self.assertIn("Config file is required", missing.stderr)

    def test_entrypoint_healthcheck_requires_a_real_listener_except_for_delivery_worker(self):
        missing_port = run_entrypoint(
            "--healthcheck",
            environment={
                "MEMOCHAT_SERVICE": "AIGatewayServer",
                "MEMOCHAT_HEALTHCHECK_TCP_PORT": "",
            },
        )
        self.assertNotEqual(missing_port.returncode, 0)
        self.assertIn("MEMOCHAT_HEALTHCHECK_TCP_PORT", missing_port.stderr)

        invalid_port = run_entrypoint(
            "--healthcheck",
            environment={
                "MEMOCHAT_SERVICE": "AIServer",
                "MEMOCHAT_HEALTHCHECK_TCP_PORT": "65536",
            },
        )
        self.assertNotEqual(invalid_port.returncode, 0)
        self.assertIn("range 1..65535", invalid_port.stderr)

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
            listener.bind(("127.0.0.1", 0))
            listener.listen()
            listening_port = listener.getsockname()[1]
            healthy = run_entrypoint(
                "--healthcheck",
                environment={
                    "MEMOCHAT_SERVICE": "ChatMessageService",
                    "MEMOCHAT_HEALTHCHECK_TCP_PORT": str(listening_port),
                },
            )
        self.assertEqual(healthy.returncode, 0, healthy.stderr)

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as closed_listener:
            closed_listener.bind(("127.0.0.1", 0))
            closed_port = closed_listener.getsockname()[1]
            unhealthy = run_entrypoint(
                "--healthcheck",
                environment={
                    "MEMOCHAT_SERVICE": "ChatMessageService",
                    "MEMOCHAT_HEALTHCHECK_TCP_PORT": str(closed_port),
                },
            )
        self.assertNotEqual(unhealthy.returncode, 0)

        entrypoint_text = ENTRYPOINT.read_text(encoding="utf-8")
        self.assertIn("/usr/bin/timeout 2s /usr/bin/bash -c", entrypoint_text)
        self.assertIn("/dev/tcp/127.0.0.1/${1}", entrypoint_text)
        worker_branch = entrypoint_text.split('if [ "${service}" = "ChatDeliveryWorker" ]', 1)[1].split("fi", 1)[0]
        self.assertIn("kill -0 1", worker_branch)

    @unittest.skipUnless(shutil.which("docker"), "requires Docker")
    def test_pinned_runtime_container_executes_the_tcp_healthcheck_as_non_root(self):
        image_inspect = subprocess.run(
            ["docker", "image", "inspect", PINNED_RUNTIME_IMAGE],
            text=True,
            capture_output=True,
            check=False,
        )
        if image_inspect.returncode != 0:
            self.skipTest("pinned Ubuntu runtime image is not available locally")

        probe_script = r"""
set -eu
test -x /usr/bin/bash
test -x /usr/bin/timeout
/usr/bin/perl -MIO::Socket::INET -e '
  my $socket = IO::Socket::INET->new(
    LocalAddr => "127.0.0.1", LocalPort => 43127, Listen => 1, ReuseAddr => 1
  ) or die "$!\n";
  open(my $ready, ">", "/tmp/listener-ready") or die "$!\n";
  close($ready);
  sleep 10;
' &
listener_pid=$!
trap 'kill "$listener_pid" 2>/dev/null || true' EXIT
attempt=0
while [ ! -e /tmp/listener-ready ] && [ "$attempt" -lt 50 ]; do
  attempt=$((attempt + 1))
  sleep 0.02
done
test -e /tmp/listener-ready
MEMOCHAT_SERVICE=ChatMessageService MEMOCHAT_HEALTHCHECK_TCP_PORT=43127 \
  /bin/sh /health-entrypoint.sh --healthcheck
if MEMOCHAT_SERVICE=ChatMessageService MEMOCHAT_HEALTHCHECK_TCP_PORT=43128 \
  /bin/sh /health-entrypoint.sh --healthcheck; then
  exit 70
fi
if MEMOCHAT_SERVICE=AIServer /bin/sh /health-entrypoint.sh --healthcheck; then
  exit 71
fi
MEMOCHAT_SERVICE=ChatDeliveryWorker /bin/sh /health-entrypoint.sh --healthcheck
"""
        result = subprocess.run(
            [
                "docker",
                "run",
                "--rm",
                "--pull",
                "never",
                "--network",
                "none",
                "--read-only",
                "--tmpfs",
                "/tmp:rw,noexec,nosuid,nodev,size=1m",
                "--user",
                "10001:10001",
                "--cap-drop",
                "ALL",
                "--security-opt",
                "no-new-privileges:true",
                "--mount",
                f"type=bind,src={ENTRYPOINT},dst=/health-entrypoint.sh,readonly",
                "--entrypoint",
                "/usr/bin/bash",
                PINNED_RUNTIME_IMAGE,
                "-c",
                probe_script,
            ],
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_entrypoint_rejects_dev_mode_and_placeholder_secrets_in_release(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            config = Path(temp_dir) / "config.ini"
            config.write_text("[Log]\nToConsole=true\n", encoding="utf-8")

            dev_mode = run_entrypoint(
                environment={
                    "MEMOCHAT_SERVICE": "ChatServer",
                    "CONFIG_PATH": str(config),
                    "MEMOCHAT_RELEASE_MODE": "1",
                    "MEMOCHAT_ALLOW_DEV_SECRETS": "1",
                }
            )
            self.assertNotEqual(dev_mode.returncode, 0)
            self.assertIn("MEMOCHAT_ALLOW_DEV_SECRETS", dev_mode.stderr)

            placeholder = run_entrypoint(
                environment={
                    "MEMOCHAT_SERVICE": "ChatServer",
                    "CONFIG_PATH": str(config),
                    "MEMOCHAT_RELEASE_MODE": "1",
                    "REQUIRED_ENV_VARS": "MEMOCHAT_CHATAUTH_HMACSECRET",
                    "MEMOCHAT_CHATAUTH_HMACSECRET": "REPLACE_WITH_GENERATED_SECRET",
                }
            )
            self.assertNotEqual(placeholder.returncode, 0)
            self.assertIn("placeholder or weak value", placeholder.stderr)

            short_secret = run_entrypoint(
                environment={
                    "MEMOCHAT_SERVICE": "ChatServer",
                    "CONFIG_PATH": str(config),
                    "MEMOCHAT_RELEASE_MODE": "1",
                    "REQUIRED_ENV_VARS": "MEMOCHAT_CHATAUTH_HMACSECRET",
                    "MEMOCHAT_CHATAUTH_HMACSECRET": "x" * 16,
                }
            )
            self.assertNotEqual(short_secret.returncode, 0)
            self.assertIn("too short", short_secret.stderr)

            short_pepper = run_entrypoint(
                environment={
                    "MEMOCHAT_SERVICE": "LoginServer",
                    "CONFIG_PATH": str(config),
                    "MEMOCHAT_RELEASE_MODE": "1",
                    "REQUIRED_ENV_VARS": "MEMOCHAT_AUTH_REFRESH_PEPPER",
                    "MEMOCHAT_AUTH_REFRESH_PEPPER": "only-sixteen-byte",
                }
            )
            self.assertNotEqual(short_pepper.returncode, 0)
            self.assertIn("pepper environment variable is too short", short_pepper.stderr)

    def test_compose_covers_current_topology_without_public_ports(self):
        compose_text = COMPOSE_FILE.read_text(encoding="utf-8")
        compose = yaml.load(compose_text, Loader=ComposeLoader)
        services = compose["services"]
        expected = topology_targets() - {"AIGatewayServer", "AIServer"}
        actual = {
            service["environment"]["MEMOCHAT_SERVICE"]
            for service in services.values()
            if "MEMOCHAT_SERVICE" in service.get("environment", {})
        }

        self.assertEqual(actual, expected)
        self.assertNotIn("memochat-ai-gateway", services)
        self.assertNotIn("memochat-ai-server", services)
        expected_health_ports = {
            "ChatServer": "50055",
            "ChatRelationQueryService": "50090",
            "ChatRelationServiceWorker": "50091",
            "ChatMessageService": "50092",
            "MediaGatewayServer": "8094",
            "MomentsGatewayServer": "8099",
            "CallGatewayServer": "8097",
            "R18GatewayServer": "8098",
            "RegisterServer": "8101",
            "LoginServer": "8102",
            "AccountServer": "8103",
            "VarifyServer": "8083",
        }
        for service in services.values():
            environment = service.get("environment", {})
            target = environment.get("MEMOCHAT_SERVICE")
            if not target:
                continue
            with self.subTest(healthcheck_port=target):
                if target == "ChatDeliveryWorker":
                    self.assertNotIn("MEMOCHAT_HEALTHCHECK_TCP_PORT", environment)
                else:
                    self.assertEqual(
                        environment.get("MEMOCHAT_HEALTHCHECK_TCP_PORT"),
                        expected_health_ports[target],
                    )
        profiles = {profile for service in services.values() for profile in service.get("profiles", [])}
        self.assertNotIn("ai", profiles)
        self.assertTrue(profiles.issubset({"calls", "r18", "observability", "provision"}))
        self.assertIn("memochat-envoy-gateway", services)
        self.assertIn("extra_hosts: !override", compose_text)
        self.assertEqual(
            services["memochat-envoy-gateway"]["extra_hosts"],
            ["host.docker.internal=127.0.0.1", "host-gateway=host-gateway"],
        )
        self.assertIn("ports: !override", compose_text)
        self.assertEqual(
            services["memochat-envoy-gateway"]["ports"],
            [
                "127.0.0.1:80:80",
                "127.0.0.1:8443:8443/tcp",
                "127.0.0.1:8443:8443/udp",
                "127.0.0.1:8190:8190/udp",
            ],
        )
        gateway = services["memochat-envoy-gateway"]
        self.assertEqual(
            gateway["user"],
            "${MEMOCHAT_RUNTIME_UID:?set MEMOCHAT_RUNTIME_UID}:${MEMOCHAT_RUNTIME_GID:?set MEMOCHAT_RUNTIME_GID}",
        )
        self.assertTrue(gateway["read_only"])
        self.assertEqual(gateway["cap_drop"], ["ALL"])
        self.assertEqual(gateway["cap_add"], ["NET_BIND_SERVICE"])
        self.assertIn("no-new-privileges:true", gateway["security_opt"])
        self.assertEqual(gateway["entrypoint"], ["/usr/local/bin/envoy"])
        self.assertIn("memochat-chat-tls-init", gateway["depends_on"])
        tls_mounts = [
            mount for mount in gateway["volumes"] if isinstance(mount, dict) and mount["target"] == "/etc/envoy/certs"
        ]
        self.assertEqual(len(tls_mounts), 1)
        self.assertEqual(tls_mounts[0]["type"], "volume")
        self.assertEqual(tls_mounts[0]["source"], "memochat-chat-tls")
        self.assertTrue(tls_mounts[0]["read_only"])
        serialized_gateway_mounts = yaml.safe_dump(gateway["volumes"])
        self.assertNotIn("MEMOCHAT_TLS_KEY_FILE", serialized_gateway_mounts)
        self.assertNotIn("MEMOCHAT_TLS_CERT_FILE", serialized_gateway_mounts)
        tls_init_command = services["memochat-chat-tls-init"]["command"][0]
        self.assertIn("servercert.pem", tls_init_command)
        self.assertIn("serverkey.pem", tls_init_command)

        for name, service in services.items():
            if "MEMOCHAT_SERVICE" not in service.get("environment", {}):
                continue
            with self.subTest(service=name):
                self.assertNotIn("ports", service)
                self.assertEqual(service["network_mode"], "service:memochat-envoy-gateway")
                self.assertEqual(service["restart"], "unless-stopped")
                self.assertEqual(service["environment"]["MEMOCHAT_RELEASE_MODE"], "1")
                self.assertEqual(service["environment"]["MEMOCHAT_ALLOW_DEV_SECRETS"], "0")
                self.assertEqual(service["environment"]["MEMOCHAT_LOG_REDACT"], "true")
                config_mounts = [mount for mount in service["volumes"] if mount["target"] == "/run/memochat/config.ini"]
                self.assertEqual(len(config_mounts), 1)
                self.assertTrue(config_mounts[0]["read_only"])

        for service_name in (
            "memochat-register-server",
            "memochat-login-server",
            "memochat-account-server",
        ):
            with self.subTest(refresh_pepper=service_name):
                environment = services[service_name]["environment"]
                self.assertIn("MEMOCHAT_AUTH_REFRESH_PEPPER", environment)
                self.assertIn(
                    "MEMOCHAT_AUTH_REFRESH_PEPPER",
                    environment["REQUIRED_ENV_VARS"].split(),
                )

        for service_name, target in (
            ("memochat-r18-gateway", "/run/memochat/data/r18"),
            ("memochat-media-gateway", "/run/memochat/uploads"),
        ):
            with self.subTest(mutable_runtime_mount=service_name):
                mounts = services[service_name]["volumes"]
                mutable_mounts = [mount for mount in mounts if mount["target"] == target]
                self.assertEqual(len(mutable_mounts), 1)
                self.assertFalse(mutable_mounts[0].get("read_only", False))
                self.assertFalse(mutable_mounts[0]["bind"]["create_host_path"])

        entrypoint_text = ENTRYPOINT.read_text(encoding="utf-8")
        self.assertIn("umask 077", entrypoint_text)
        self.assertLess(entrypoint_text.index("umask 077"), entrypoint_text.index('exec "${target_bin}"'))

        otel_volumes = services["memochat-otel-collector"]["volumes"]
        serialized_otel_volumes = yaml.safe_dump(otel_volumes)
        self.assertNotIn("Memo_ops/runtime", serialized_otel_volumes)
        self.assertNotIn("build/bin", serialized_otel_volumes)
        self.assertEqual(len(otel_volumes), 2)

        runtime_user = (
            "${MEMOCHAT_RUNTIME_UID:?set MEMOCHAT_RUNTIME_UID}:${MEMOCHAT_RUNTIME_GID:?set MEMOCHAT_RUNTIME_GID}"
        )
        observability_data_targets = {
            "memochat-influxdb": "/var/lib/influxdb2",
            "memochat-grafana": "/var/lib/grafana",
            "memochat-prometheus": "/prometheus",
            "memochat-alertmanager": "/alertmanager",
            "memochat-loki": "/loki",
            "memochat-tempo": "/var/tempo",
        }
        ordinary_observability_services = (*observability_data_targets, "memochat-otel-collector")
        for service_name in ordinary_observability_services:
            with self.subTest(hardened_observability_service=service_name):
                service = services[service_name]
                self.assertEqual(service["profiles"], ["observability"])
                self.assertEqual(service["user"], runtime_user)
                self.assertTrue(service["read_only"])
                self.assertEqual(service["cap_drop"], ["ALL"])
                self.assertIn("no-new-privileges:true", service["security_opt"])
                self.assertTrue(service["healthcheck"]["test"])

        for service_name, data_target in observability_data_targets.items():
            with self.subTest(fail_closed_observability_data=service_name):
                data_mounts = [
                    mount for mount in services[service_name]["volumes"] if mount.get("target") == data_target
                ]
                self.assertEqual(len(data_mounts), 1)
                self.assertFalse(data_mounts[0].get("read_only", False))
                self.assertFalse(data_mounts[0]["bind"]["create_host_path"])

        influx_targets = {mount["target"] for mount in services["memochat-influxdb"]["volumes"]}
        self.assertNotIn("/etc/influxdb2", influx_targets)
        self.assertIn("/run/secrets/memochat-influxdb", influx_targets)
        self.assertEqual(services["memochat-influxdb"]["environment"]["INFLUX_CONFIGS_PATH"], "/tmp/influx-configs")
        influx_environment = services["memochat-influxdb"]["environment"]
        self.assertNotIn("DOCKER_INFLUXDB_INIT_USERNAME", influx_environment)
        self.assertNotIn("DOCKER_INFLUXDB_INIT_PASSWORD", influx_environment)
        self.assertNotIn("DOCKER_INFLUXDB_INIT_ADMIN_TOKEN", influx_environment)
        self.assertEqual(
            influx_environment["DOCKER_INFLUXDB_INIT_ADMIN_TOKEN_FILE"],
            "/run/secrets/memochat-influxdb/influxdb-admin.token",
        )

        grafana = services["memochat-grafana"]
        self.assertNotIn("MEMOCHAT_INFLUXDB_ADMIN_TOKEN", grafana["environment"])
        self.assertEqual(grafana["entrypoint"], ["/bin/sh", "/provision/grafana_entrypoint.sh"])
        grafana_secret_mount = next(
            mount for mount in grafana["volumes"] if mount["target"] == "/run/secrets/memochat-influxdb"
        )
        self.assertTrue(grafana_secret_mount["read_only"])
        self.assertFalse(grafana_secret_mount["bind"]["create_host_path"])

        influx_provision = services["memochat-release-provision-influxdb"]
        self.assertNotIn("INFLUX_TOKEN", influx_provision["environment"])
        self.assertEqual(
            influx_provision["environment"]["INFLUX_TOKEN_FILE"],
            "/run/secrets/memochat-influxdb/influxdb-admin.token",
        )
        provision_secret_mount = next(
            mount
            for mount in influx_provision["volumes"]
            if isinstance(mount, dict) and mount["target"] == "/run/secrets/memochat-influxdb"
        )
        self.assertFalse(provision_secret_mount.get("read_only", False))
        self.assertFalse(provision_secret_mount["bind"]["create_host_path"])

        cadvisor = services["memochat-cadvisor"]
        self.assertEqual(cadvisor["profiles"], ["observability"])
        self.assertEqual(cadvisor["user"], "0:0")
        self.assertTrue(cadvisor["privileged"])
        self.assertTrue(cadvisor["read_only"])
        self.assertIn("no-new-privileges:true", cadvisor["security_opt"])
        self.assertTrue(cadvisor["healthcheck"]["test"])

        self.assertNotIn(":-123456", compose_text)
        self.assertNotIn("memochat-dev-chat-secret", compose_text)
        self.assertNotIn("memochat-dev-access-token-secret", compose_text)
        for service_name, variable_name in (
            ("memochat-redis", "MEMOCHAT_REDIS_PASSWORD"),
            ("memochat-postgres", "MEMOCHAT_POSTGRES_PASSWORD"),
            ("memochat-mongo", "MEMOCHAT_MONGO_ROOT_PASSWORD"),
            ("memochat-minio", "MEMOCHAT_MINIO_ROOT_PASSWORD"),
            ("memochat-rabbitmq", "MEMOCHAT_RABBITMQ_PASSWORD"),
        ):
            with self.subTest(infrastructure=service_name):
                serialized = yaml.safe_dump(services[service_name])
                self.assertIn(f"${{{variable_name}:?", serialized)
        self.assertNotIn("MEMOCHAT_INFLUXDB_ADMIN_TOKEN", yaml.safe_dump(services["memochat-influxdb"]))
        self.assertIn(
            "${MEMOCHAT_GRAFANA_ADMIN_PASSWORD:-}",
            yaml.safe_dump(services["memochat-grafana"]),
        )

        for service_name in (
            "memochat-release-provision-postgres",
            "memochat-release-provision-mongo",
            "memochat-release-provision-minio",
            "memochat-release-provision-influxdb",
        ):
            self.assertEqual(services[service_name]["profiles"], ["provision"])

        self.assertIn("MINIO_ROOT_PASSWORD", services["memochat-minio"]["environment"])
        self.assertIn(
            "MEMOCHAT_MINIO_ROOT_PASSWORD",
            services["memochat-minio"]["environment"]["MINIO_ROOT_PASSWORD"],
        )
        media_environment = services["memochat-media-gateway"]["environment"]
        self.assertIn("MEMOCHAT_MINIO_APP_ACCESS_KEY", media_environment["MEMOCHAT_MINIO_ACCESSKEY"])
        self.assertIn("MEMOCHAT_MINIO_APP_SECRET_KEY", media_environment["MEMOCHAT_MINIO_SECRETKEY"])
        self.assertNotIn("MEMOCHAT_POSTGRES_PASSWORD", yaml.safe_dump(services["memochat-chat-server"]))
        self.assertEqual(services["memochat-chat-server"]["environment"]["MEMOCHAT_INSTANCE_NAME"], "chatserver1")

        call_environment = services["memochat-call-gateway"]["environment"]
        self.assertEqual(call_environment["MEMOCHAT_ACCOUNTPOSTGRES_USER"], "memo_account_app")
        self.assertEqual(call_environment["MEMOCHAT_ACCOUNTPOSTGRES_DATABASE"], "memo_account")
        self.assertIn("MEMOCHAT_ACCOUNT_POSTGRES_PASSWORD", call_environment["MEMOCHAT_ACCOUNTPOSTGRES_PASSWD"])
        self.assertIn("MEMOCHAT_ACCOUNTPOSTGRES_PASSWD", call_environment["REQUIRED_ENV_VARS"].split())
        self.assertIn("memochat-chat-relation-query", services["memochat-call-gateway"]["depends_on"])

        relation_query_environment = services["memochat-chat-relation-query"]["environment"]
        self.assertIn(
            "MEMOCHAT_RELATION_CHAT_QUERY_AUTH_TOKEN",
            relation_query_environment["MEMOCHAT_RELATIONQUERYSERVICE_CHATAUTHTOKEN"],
        )
        self.assertIn(
            "MEMOCHAT_RELATION_CALL_AUTH_TOKEN",
            relation_query_environment["MEMOCHAT_RELATIONQUERYSERVICE_CALLAUTHTOKEN"],
        )
        self.assertIn(
            "MEMOCHAT_RELATION_MOMENTS_AUTH_TOKEN",
            relation_query_environment["MEMOCHAT_RELATIONQUERYSERVICE_MOMENTSAUTHTOKEN"],
        )
        self.assertIn(
            "MEMOCHAT_RELATION_COMMAND_AUTH_TOKEN",
            services["memochat-chat-relation-service"]["environment"]["MEMOCHAT_RELATIONSERVICE_AUTHTOKEN"],
        )

        consumer_tokens = {
            "memochat-chat-server": {
                "MEMOCHAT_RELATIONSERVICE_AUTHTOKEN",
                "MEMOCHAT_RELATIONQUERYSERVICE_CHATAUTHTOKEN",
            },
            "memochat-chat-relation-query": {
                "MEMOCHAT_RELATIONQUERYSERVICE_CHATAUTHTOKEN",
                "MEMOCHAT_RELATIONQUERYSERVICE_CALLAUTHTOKEN",
                "MEMOCHAT_RELATIONQUERYSERVICE_MOMENTSAUTHTOKEN",
            },
            "memochat-chat-relation-service": {"MEMOCHAT_RELATIONSERVICE_AUTHTOKEN"},
            "memochat-call-gateway": {"MEMOCHAT_RELATIONQUERYSERVICE_CALLAUTHTOKEN"},
            "memochat-moments-gateway": {"MEMOCHAT_RELATIONQUERYSERVICE_MOMENTSAUTHTOKEN"},
        }
        all_injected_token_names = {
            "MEMOCHAT_RELATIONSERVICE_AUTHTOKEN",
            "MEMOCHAT_RELATIONQUERYSERVICE_CHATAUTHTOKEN",
            "MEMOCHAT_RELATIONQUERYSERVICE_CALLAUTHTOKEN",
            "MEMOCHAT_RELATIONQUERYSERVICE_MOMENTSAUTHTOKEN",
        }
        for service_name, expected_names in consumer_tokens.items():
            with self.subTest(relation_token_scope=service_name):
                environment = services[service_name]["environment"]
                actual_names = set(environment).intersection(all_injected_token_names)
                self.assertEqual(expected_names, actual_names)
                self.assertTrue(expected_names.issubset(environment["REQUIRED_ENV_VARS"].split()))

        redpanda = services["memochat-redpanda"]
        self.assertEqual(
            redpanda["user"],
            "${MEMOCHAT_RUNTIME_UID:?set MEMOCHAT_RUNTIME_UID}:${MEMOCHAT_RUNTIME_GID:?set MEMOCHAT_RUNTIME_GID}",
        )
        self.assertTrue(redpanda["read_only"])
        self.assertEqual(redpanda["cap_drop"], ["ALL"])
        self.assertIn("no-new-privileges:true", redpanda["security_opt"])
        self.assertTrue(any(entry.startswith("/etc/redpanda:") for entry in redpanda["tmpfs"]))
        redpanda_base = yaml.safe_load(BASE_COMPOSE_FILE.read_text(encoding="utf-8"))["services"]["memochat-redpanda"]
        self.assertIn("--rpc-addr=0.0.0.0:33145", redpanda_base["command"])
        self.assertIn("--advertise-rpc-addr=memochat-redpanda:33145", redpanda_base["command"])
        self.assertIn("rpk cluster health", " ".join(redpanda_base["healthcheck"]["test"]))

        for service_name, service in services.items():
            if "image" not in service or "MEMOCHAT_SERVICE" in service.get("environment", {}):
                continue
            with self.subTest(immutable_infrastructure_image=service_name):
                self.assertRegex(service["image"], r"@sha256:[0-9a-f]{64}$")

        livekit = yaml.safe_load(LIVEKIT_COMPOSE_FILE.read_text(encoding="utf-8"))
        livekit_service = livekit["services"]["memochat-livekit"]
        self.assertRegex(
            livekit_service["image"],
            r"@sha256:[0-9a-f]{64}$",
        )
        self.assertEqual(
            livekit_service["user"],
            "${MEMOCHAT_RUNTIME_UID:?set MEMOCHAT_RUNTIME_UID}:${MEMOCHAT_RUNTIME_GID:?set MEMOCHAT_RUNTIME_GID}",
        )
        self.assertTrue(livekit_service["read_only"])
        self.assertEqual(livekit_service["cap_drop"], ["ALL"])
        self.assertIn("no-new-privileges:true", livekit_service["security_opt"])
        self.assertTrue(any(entry.startswith("/tmp:rw,noexec,nosuid,nodev") for entry in livekit_service["tmpfs"]))
        self.assertIn("wget -q -T 3", " ".join(livekit_service["healthcheck"]["test"]))

    @unittest.skipUnless(shutil.which("docker"), "requires Docker Compose")
    def test_merged_release_envoy_keeps_the_non_root_tls_and_log_boundary(self):
        clean_environment = {
            "HOME": os.environ.get("HOME", "/tmp"),
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        }
        result = subprocess.run(
            [
                "docker",
                "compose",
                "--env-file",
                str(ENV_EXAMPLE),
                "--project-directory",
                str(BASE_COMPOSE_FILE.parent),
                "-f",
                str(BASE_COMPOSE_FILE),
                "-f",
                str(COMPOSE_FILE),
                "-f",
                str(LIVEKIT_COMPOSE_FILE),
                "config",
                "--format",
                "json",
            ],
            cwd=REPO_ROOT,
            env=clean_environment,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        self.assertEqual(0, result.returncode, result.stdout)
        gateway = yaml.safe_load(result.stdout)["services"]["memochat-envoy-gateway"]

        self.assertEqual("10001:10001", gateway["user"])
        self.assertTrue(gateway["read_only"])
        self.assertEqual(["ALL"], gateway["cap_drop"])
        self.assertEqual(["NET_BIND_SERVICE"], gateway["cap_add"])
        self.assertEqual(["/usr/local/bin/envoy"], gateway["entrypoint"])
        tls_mount = next(mount for mount in gateway["volumes"] if mount["target"] == "/etc/envoy/certs")
        self.assertEqual("volume", tls_mount["type"])
        self.assertEqual("memochat-chat-tls", tls_mount["source"])
        self.assertTrue(tls_mount["read_only"])
        self.assertFalse(any(mount["target"] == "/etc/envoy/certs/serverkey.pem" for mount in gateway["volumes"]))

    @unittest.skipUnless(shutil.which("docker"), "requires Docker Compose")
    def test_merged_observability_profile_keeps_security_and_health_boundaries(self):
        clean_environment = {
            "HOME": os.environ.get("HOME", "/tmp"),
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        }
        result = subprocess.run(
            [
                "docker",
                "compose",
                "--env-file",
                str(ENV_EXAMPLE),
                "--project-directory",
                str(BASE_COMPOSE_FILE.parent),
                "-f",
                str(BASE_COMPOSE_FILE),
                "-f",
                str(COMPOSE_FILE),
                "-f",
                str(LIVEKIT_COMPOSE_FILE),
                "--profile",
                "observability",
                "config",
                "--format",
                "json",
            ],
            cwd=REPO_ROOT,
            env=clean_environment,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        self.assertEqual(0, result.returncode, result.stdout)
        services = yaml.safe_load(result.stdout)["services"]

        for service_name in (
            "memochat-influxdb",
            "memochat-grafana",
            "memochat-prometheus",
            "memochat-alertmanager",
            "memochat-loki",
            "memochat-tempo",
            "memochat-otel-collector",
        ):
            with self.subTest(rendered_observability_service=service_name):
                service = services[service_name]
                self.assertEqual(service["user"], "10001:10001")
                self.assertTrue(service["read_only"])
                self.assertEqual(service["cap_drop"], ["ALL"])
                self.assertIn("no-new-privileges:true", service["security_opt"])
                self.assertIn("healthcheck", service)

        prometheus_dependencies = services["memochat-prometheus"]["depends_on"]
        self.assertEqual(
            set(prometheus_dependencies),
            {"memochat-alertmanager", "memochat-cadvisor", "memochat-otel-collector"},
        )
        self.assertTrue(
            all(dependency["condition"] == "service_healthy" for dependency in prometheus_dependencies.values())
        )
        self.assertEqual(
            services["memochat-otel-collector"]["healthcheck"]["test"],
            ["CMD", "/otelcol-contrib", "validate", "--config=http://127.0.0.1:13133/healthz"],
        )
        self.assertNotIn(
            "MEMOCHAT_INFLUXDB_ADMIN_TOKEN",
            services["memochat-grafana"]["environment"],
        )

        cadvisor = services["memochat-cadvisor"]
        self.assertEqual(cadvisor["user"], "0:0")
        self.assertTrue(cadvisor["privileged"])
        self.assertTrue(cadvisor["read_only"])
        cadvisor_host_targets = {mount["target"] for mount in cadvisor["volumes"]}
        self.assertEqual(cadvisor_host_targets, {"/rootfs", "/var/run", "/sys", "/var/lib/docker"})
        self.assertTrue(all(mount["read_only"] for mount in cadvisor["volumes"]))
        self.assertEqual(
            cadvisor["devices"],
            [{"source": "/dev/kmsg", "target": "/dev/kmsg", "permissions": "rwm"}],
        )

    def test_release_login_advertises_only_authenticated_quic_chat_transport(self):
        source = AUTH_SERVICE.read_text(encoding="utf-8")

        self.assertIn("ReleaseModeEnabled()", source)
        self.assertIn("release mode requires an authenticated QUIC chat route", source)
        self.assertIn("if (!release_mode)", source)
        self.assertIn(".tls = true", source)
        self.assertIn("release_mode ? auth_algo::QuicTransport()", source)

    def test_chat_runtime_validates_migrated_schema_without_application_role_ddl(self):
        source = CHAT_POSTGRES_DIALOGS.read_text(encoding="utf-8")
        validation_region = source.split("bool PostgresDao::ValidateChatMessageIdempotencySchema()", 1)[1].split(
            "bool PostgresDao::GetPendingGroupApplyForReviewer", 1
        )[0]

        self.assertIn("pqxx::read_transaction", validation_region)
        self.assertIn("to_regclass('memo.uk_chat_private_msg_msg_id')", validation_region)
        self.assertIn("FROM memo.chat_event_outbox WHERE FALSE", validation_region)
        for ddl in ("CREATE ", "ALTER ", "DROP ", "TRUNCATE "):
            self.assertNotIn(ddl, validation_region)

    def test_quic_pfx_passwords_never_use_a_known_fallback(self):
        chat_source = CHAT_QUIC_SERVER.read_text(encoding="utf-8")
        gate_source = LEGACY_GATE_H3.read_text(encoding="utf-8")

        self.assertIn("quic_pfx_password_required", chat_source)
        self.assertIn("MEMOCHAT_GATE_HTTP3_PFX_PASSWORD", gate_source)
        self.assertNotIn('"memochat"', chat_source)
        self.assertNotIn('"memochat"', gate_source)

    def test_development_certificate_generator_keeps_private_key_owner_only(self):
        source = DEV_CERT_GENERATOR.read_text(encoding="utf-8")

        self.assertIn("umask 077", source)
        self.assertIn('chmod 700 "${OUT_DIR}"', source)
        self.assertIn('chmod 600 "${KEY_FILE}"', source)
        self.assertNotIn('chmod 644 "${CERT_FILE}" "${KEY_FILE}"', source)

        with tempfile.TemporaryDirectory() as temp_dir:
            result = subprocess.run(
                ["sh", str(DEV_CERT_GENERATOR), temp_dir],
                cwd=REPO_ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(stat.S_IMODE(Path(temp_dir).stat().st_mode), 0o700)
            self.assertEqual(stat.S_IMODE((Path(temp_dir) / "servercert.pem").stat().st_mode), 0o644)
            self.assertEqual(stat.S_IMODE((Path(temp_dir) / "serverkey.pem").stat().st_mode), 0o600)

    def test_release_datastore_provisioning_is_resumable_and_rotates_credentials(self):
        postgres = POSTGRES_PROVISION.read_text(encoding="utf-8")
        mongo = MONGO_PROVISION.read_text(encoding="utf-8")
        minio = MINIO_PROVISION.read_text(encoding="utf-8")

        self.assertLess(postgres.index("CREATE ROLE memo_account_app"), postgres.index("for migration in"))
        self.assertIn("postgres_legacy_split.sh", postgres)
        self.assertIn("sync_identity_sequence", postgres)
        self.assertEqual(postgres.count("012_media_access_grants.sql"), 1)
        self.assertIn("appDb.updateUser", mongo)
        self.assertIn('mc admin user add release "$MEMOCHAT_MINIO_APP_ACCESS_KEY"', minio)
        self.assertNotIn("|| \\\n  mc admin user enable", minio)

    def test_release_env_example_contains_placeholders_not_credentials(self):
        text = ENV_EXAMPLE.read_text(encoding="utf-8")
        required_names = (
            "MEMOCHAT_REDIS_PASSWORD",
            "MEMOCHAT_POSTGRES_PASSWORD",
            "MEMOCHAT_CHAT_POSTGRES_PASSWORD",
            "MEMOCHAT_ACCOUNT_POSTGRES_PASSWORD",
            "MEMOCHAT_MEDIA_POSTGRES_PASSWORD",
            "MEMOCHAT_MOMENTS_POSTGRES_PASSWORD",
            "MEMOCHAT_CALL_POSTGRES_PASSWORD",
            "MEMOCHAT_MONGO_APP_PASSWORD",
            "MEMOCHAT_RABBITMQ_PASSWORD",
            "MEMOCHAT_CHATAUTH_HMACSECRET",
            "MEMOCHAT_AUTHTOKEN_JWTSECRET",
            "MEMOCHAT_AUTH_REFRESH_PEPPER",
            "MEMOCHAT_MINIO_ROOT_PASSWORD",
            "MEMOCHAT_MINIO_APP_SECRET_KEY",
            "MEMOCHAT_R18_CREDENTIAL_MASTER_KEY",
            "MEMOCHAT_EMAIL_SMTPPASS",
            "MEMOCHAT_TLS_CERT_FILE",
            "MEMOCHAT_TLS_KEY_FILE",
        )
        for name in required_names:
            with self.subTest(name=name):
                self.assertRegex(text, rf"(?m)^{name}=REPLACE_WITH_[A-Z0-9_]+$")
        self.assertNotIn("MEMOCHAT_AI_INTERNAL_API_KEY", text)
        self.assertNotIn("MEMOCHAT_AI_PROVIDER_ADMIN_KEY", text)
        self.assertNotIn("MEMOCHAT_AI_POSTGRES_PASSWORD", text)

        for weak in (
            "=123456",
            "=password",
            "=admin",
            "memochat-dev-chat-secret",
            "memochat-dev-access-token-secret",
        ):
            with self.subTest(weak=weak):
                self.assertNotIn(weak, text)

    def test_public_varify_configs_do_not_contain_sender_identity_or_credentials(self):
        for config_path in VARIFY_CONFIGS:
            with self.subTest(config=config_path.name):
                source = config_path.read_text(encoding="utf-8")
                self.assertRegex(source, r"(?m)^SMTPUser=$")
                self.assertRegex(source, r"(?m)^SMTPPass=$")
                self.assertRegex(source, r"(?m)^From=$")

    def test_image_builder_maps_every_target_to_the_compose_image_name(self):
        expected_slugs = {
            "AIGatewayServer": "ai-gateway",
            "AIServer": "ai-server",
            "AccountServer": "account-server",
            "CallGatewayServer": "call-gateway",
            "ChatDeliveryWorker": "chat-delivery-worker",
            "ChatMessageService": "chat-message-service",
            "ChatRelationQueryService": "chat-relation-query-service",
            "ChatRelationServiceWorker": "chat-relation-service-worker",
            "ChatServer": "chat-server",
            "LoginServer": "login-server",
            "MediaGatewayServer": "media-gateway",
            "MomentsGatewayServer": "moments-gateway",
            "R18GatewayServer": "r18-gateway",
            "RegisterServer": "register-server",
            "VarifyServer": "varify-server",
        }

        with tempfile.TemporaryDirectory() as temp_dir:
            temp = Path(temp_dir)
            bundles = temp / "bundles"
            fake_bin = temp / "fake-bin"
            docker_log = temp / "docker.log"
            fake_bin.mkdir()

            for target in expected_slugs:
                bundle = bundles / target
                (bundle / "bin").mkdir(parents=True)
                (bundle / "lib").mkdir()
                binary = bundle / "bin" / target
                binary.write_text("release-test-binary\n", encoding="utf-8")
                binary.chmod(0o755)
                for library in ("libmsquic.so.2", "libstdc++.so.6", "libgcc_s.so.1", "libatomic.so.1"):
                    (bundle / "lib" / library).write_text("release-test-library\n", encoding="utf-8")
                sbom = bundle / "sbom/vcpkg-build-dependencies.spdx.json"
                sbom.parent.mkdir()
                sbom.write_text(
                    '{"spdxVersion":"SPDX-2.3","packages":[{"name":"fixture"}]}\n',
                    encoding="utf-8",
                )
                sbom_sha256 = hashlib.sha256(sbom.read_bytes()).hexdigest()
                (bundle / "MANIFEST.txt").write_text(
                    f"format=memochat-cpp-service-bundle-v1\ntarget={target}\n"
                    "vcpkg_sbom_coverage=installed-closure-overapproximation\n"
                    f"vcpkg_sbom_sha256={sbom_sha256}\n",
                    encoding="utf-8",
                )
                checksum_lines = []
                for path in sorted(list((bundle / "bin").iterdir()) + list((bundle / "lib").iterdir()) + [sbom]):
                    digest = hashlib.sha256(path.read_bytes()).hexdigest()
                    checksum_lines.append(f"{digest}  {path.relative_to(bundle)}")
                (bundle / "SHA256SUMS").write_text("\n".join(checksum_lines) + "\n", encoding="utf-8")

            fake_docker = fake_bin / "docker"
            fake_docker.write_text(
                '#!/bin/sh\nprintf \'%s\\n\' "$*" >> "$DOCKER_LOG"\n',
                encoding="utf-8",
            )
            fake_docker.chmod(0o755)

            env = os.environ.copy()
            env["PATH"] = f"{fake_bin}:{env['PATH']}"
            env["DOCKER_LOG"] = str(docker_log)
            result = subprocess.run(
                [
                    "/bin/bash",
                    str(BUILD_IMAGES),
                    "--bundle-root",
                    str(bundles),
                    "--image-prefix",
                    "registry.example/memochat",
                    "--tag",
                    "test-tag",
                ],
                cwd=REPO_ROOT,
                env=env,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)

            build_lines = [
                line for line in docker_log.read_text(encoding="utf-8").splitlines() if "buildx build" in line
            ]
            self.assertEqual(len(build_lines), len(expected_slugs))
            for target, slug in expected_slugs.items():
                matching = [line for line in build_lines if f"TARGET={target}" in line]
                self.assertEqual(len(matching), 1, target)
                self.assertIn(f"service_bundle={bundles / target}", matching[0])
                self.assertIn("--build-context server_entrypoint=", matching[0])
                self.assertIn(f"registry.example/memochat/{slug}:test-tag", matching[0])
                self.assertIn("--load", matching[0])
                self.assertIn("--pull=false", matching[0])
                self.assertIn(
                    "RUNTIME_IMAGE=ubuntu@sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90",
                    matching[0],
                )
                self.assertNotIn(str(REPO_ROOT), matching[0].split()[-1])

            docker_log.unlink()
            pull_result = subprocess.run(
                [
                    "/bin/bash",
                    str(BUILD_IMAGES),
                    "--bundle-root",
                    str(bundles),
                    "--image-prefix",
                    "registry.example/memochat",
                    "--tag",
                    "pull-test",
                    "--pull",
                ],
                cwd=REPO_ROOT,
                env=env,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(pull_result.returncode, 0, pull_result.stderr)
            pull_commands = docker_log.read_text(encoding="utf-8").splitlines()
            self.assertEqual(
                pull_commands[0],
                "pull ubuntu@sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90",
            )
            pulled_build_lines = [line for line in pull_commands if "buildx build" in line]
            self.assertEqual(len(pulled_build_lines), len(expected_slugs))
            self.assertTrue(all("--pull " in line for line in pulled_build_lines))
            self.assertTrue(all("--pull=false" not in line for line in pulled_build_lines))


if __name__ == "__main__":
    unittest.main()
