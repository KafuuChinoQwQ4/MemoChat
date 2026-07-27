import hashlib
import os
import re
import shutil
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
MONGO_PROVISION = REPO_ROOT / "infra/deploy/local/provision/mongo.js"
MINIO_PROVISION = REPO_ROOT / "infra/deploy/local/provision/minio.sh"
VARIFY_CONFIGS = (
    REPO_ROOT / "apps/server/core/VarifyServer/config.ini",
    REPO_ROOT / "apps/server/core/VarifyServer/varify2.ini",
)


class ComposeLoader(yaml.SafeLoader):
    pass


ComposeLoader.add_constructor("!override", lambda loader, node: loader.construct_sequence(node))


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
    def test_runtime_image_consumes_only_an_explicit_service_bundle(self):
        text = DOCKERFILE.read_text(encoding="utf-8")

        self.assertIn("FROM service_bundle AS service_bundle", text)
        self.assertIn("FROM server_entrypoint AS server_entrypoint", text)
        self.assertRegex(text, r"COPY\s+--from=service_bundle[^\n]+/bin/\$\{TARGET\}")
        self.assertRegex(text, r"COPY\s+--from=service_bundle[^\n]+/lib/")
        self.assertRegex(text, r"COPY\s+--from=server_entrypoint[^\n]+/server-entrypoint\.sh")
        self.assertIn("libmsquic.so.2", text)
        self.assertIn("not found", text)
        self.assertNotRegex(text, r"(?m)^COPY\s+\.\s")
        self.assertNotRegex(text, r"(?m)^COPY\s+apps/")
        self.assertNotRegex(text, r"(?m)^COPY\s+infra/Memo_ops/runtime")
        self.assertNotRegex(text, r"(?m)^COPY\s+.*config\.ini")
        self.assertNotIn("GateServer", text)
        self.assertRegex(text, r"(?m)^USER\s+memochat$")

        for target in topology_targets():
            with self.subTest(target=target):
                self.assertIn(target, text)

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
        self.assertEqual(services["memochat-cadvisor"]["profiles"], ["observability"])

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
        for service_name, variable_name in (
            ("memochat-influxdb", "MEMOCHAT_INFLUXDB_ADMIN_TOKEN"),
            ("memochat-grafana", "MEMOCHAT_GRAFANA_ADMIN_PASSWORD"),
        ):
            with self.subTest(optional_infrastructure=service_name):
                serialized = yaml.safe_dump(services[service_name])
                self.assertIn(f"${{{variable_name}:-}}", serialized)

        for service_name in (
            "memochat-release-provision-postgres",
            "memochat-release-provision-mongo",
            "memochat-release-provision-minio",
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

        for service_name, service in services.items():
            if "image" not in service or "MEMOCHAT_SERVICE" in service.get("environment", {}):
                continue
            with self.subTest(immutable_infrastructure_image=service_name):
                self.assertRegex(service["image"], r"@sha256:[0-9a-f]{64}$")

        livekit = yaml.safe_load(LIVEKIT_COMPOSE_FILE.read_text(encoding="utf-8"))
        self.assertRegex(
            livekit["services"]["memochat-livekit"]["image"],
            r"@sha256:[0-9a-f]{64}$",
        )

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

    def test_release_login_advertises_only_authenticated_quic_chat_transport(self):
        source = AUTH_SERVICE.read_text(encoding="utf-8")

        self.assertIn("ReleaseModeEnabled()", source)
        self.assertIn("release mode requires an authenticated QUIC chat route", source)
        self.assertIn("if (!release_mode)", source)
        self.assertIn(".tls = true", source)
        self.assertIn("release_mode ? auth_algo::QuicTransport()", source)

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
            "MinioPass2026!",
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
                (bundle / "MANIFEST.txt").write_text(
                    f"format=memochat-cpp-service-bundle-v1\ntarget={target}\n",
                    encoding="utf-8",
                )
                checksum_lines = []
                for path in sorted(
                    [path for path in (bundle / "bin").iterdir()] + [path for path in (bundle / "lib").iterdir()]
                ):
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
                self.assertNotIn(str(REPO_ROOT), matching[0].split()[-1])


if __name__ == "__main__":
    unittest.main()
