import hashlib
import os
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
PACKAGER = REPO_ROOT / "tools/scripts/release/package_backend_deployment_kit.sh"
CI_WORKFLOW = REPO_ROOT / ".github/workflows/ci.yml"

CONFIG_FILES = (
    "AccountService/account.ini",
    "CallService/callgateway.ini",
    "ChatServer/chatdeliveryworker1.ini",
    "ChatServer/chatmessageservice1.ini",
    "ChatServer/chatrelationquery1.ini",
    "ChatServer/chatrelationservice1.ini",
    "ChatServer/chatserver1.ini",
    "LoginService/login.ini",
    "MediaService/mediagateway.ini",
    "MomentsService/momentsgateway.ini",
    "R18Service/r18gateway.ini",
    "RegisterService/register.ini",
    "VarifyServer/config.ini",
)

SENSITIVE_INI_KEYS = (
    "Passwd",
    "Password",
    "PfxPassword",
    "HmacSecret",
    "JwtSecret",
    "InternalApiKey",
    "AdminKey",
    "ApiKey",
    "ApiSecret",
    "AccessKey",
    "SecretKey",
    "SMTPUser",
    "SMTPPass",
    "From",
    "Uri",
)

TARGET_SLUGS = {
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


class BackendDeploymentKitTests(unittest.TestCase):
    def run_packager(self, output: Path, *extra: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            ["bash", str(PACKAGER), "--output", str(output), *extra],
            cwd=REPO_ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )

    def test_packages_a_self_contained_release_layout_with_sanitized_configs(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output = Path(temp_dir) / "deployment"

            result = self.run_packager(output)

            self.assertEqual(0, result.returncode, result.stdout)
            expected_files = (
                "README.md",
                "MANIFEST.txt",
                "SHA256SUMS",
                "legal/LEGAL-STATUS.txt",
                "legal/LICENSE",
                "legal/THIRD_PARTY_NOTICES.md",
                "tools/scripts/release/audit_backend_images.sh",
                "tools/scripts/release/build_backend_images.sh",
                "tools/scripts/release/run_release_compose.sh",
                "tools/scripts/release/verify_release_tree.sh",
                "infra/deploy/images/services/cpp-service.Dockerfile",
                "infra/deploy/images/common/entrypoints/server-entrypoint.sh",
                "infra/deploy/local/.env.release.example",
                "infra/deploy/local/docker-compose.yml",
                "infra/deploy/local/compose/backend-services.yml",
                "infra/deploy/local/compose/livekit.yml",
                "infra/deploy/local/compose/envoy.yaml",
                "infra/deploy/local/provision/postgres.sh",
                "infra/deploy/local/provision/mongo.sh",
                "infra/deploy/local/provision/mongo.js",
                "infra/deploy/local/provision/minio.sh",
                "apps/server/migrations/postgresql/business/001_baseline.sql",
            )
            for relative in expected_files:
                with self.subTest(relative=relative):
                    self.assertTrue(output.joinpath(relative).is_file())

            self.assertTrue(output.joinpath("tools/scripts/release/run_release_compose.sh").stat().st_mode & 0o100)
            self.assertTrue(output.joinpath("tools/scripts/release/audit_backend_images.sh").stat().st_mode & 0o100)
            self.assertTrue(output.joinpath("infra/deploy/local/provision/postgres.sh").stat().st_mode & 0o100)
            self.assertFalse(any(output.rglob("_TREE.md")))
            self.assertFalse(any(output.rglob("*.key")))
            self.assertFalse(any(output.rglob("*.log")))

            source_migrations = {
                path.name for path in REPO_ROOT.joinpath("apps/server/migrations/postgresql/business").glob("*.sql")
            }
            packaged_migrations = {
                path.name for path in output.joinpath("apps/server/migrations/postgresql/business").glob("*.sql")
            }
            self.assertEqual(source_migrations, packaged_migrations)
            source_provisioners = {
                path.name
                for path in REPO_ROOT.joinpath("infra/deploy/local/provision").iterdir()
                if path.is_file() and path.name != "_TREE.md"
            }
            packaged_provisioners = {
                path.name for path in output.joinpath("infra/deploy/local/provision").iterdir() if path.is_file()
            }
            self.assertEqual(source_provisioners, packaged_provisioners)

            for relative in CONFIG_FILES:
                config = output / "apps/server/core" / relative
                self.assertTrue(config.is_file(), relative)
                config_text = config.read_text(encoding="utf-8")
                self.assertNotIn("memochat-dev-", config_text)
                for key in SENSITIVE_INI_KEYS:
                    with self.subTest(config=relative, key=key):
                        self.assertIsNone(
                            re.search(rf"(?mi)^[ \t]*{re.escape(key)}[ \t]*=[ \t]*\S+", config_text),
                            f"{relative} contains a non-empty {key}",
                        )

            base_compose = output.joinpath("infra/deploy/local/docker-compose.yml").read_text(encoding="utf-8")
            for weak_value in ("123456", "adminadmin", "my-super-secret-admin-token"):
                with self.subTest(weak_value=weak_value):
                    self.assertNotIn(weak_value, base_compose)

            manifest = output.joinpath("MANIFEST.txt").read_text(encoding="utf-8")
            self.assertIn("format=memochat-backend-deployment-kit-v1", manifest)
            self.assertIn("supported_profiles=calls,r18,observability", manifest)
            self.assertIn("excluded_components=AIGatewayServer,AIServer,AIOrchestrator,Ollama,Qdrant,Neo4j", manifest)
            self.assertIn("legal_inventory=complete", manifest)
            self.assertIn("third_party_legal_corpus=incomplete", manifest)
            self.assertIn("formal_distribution_ready=false", manifest)
            self.assertNotIn("legal_files=complete", manifest)
            legal_status = output.joinpath("legal/LEGAL-STATUS.txt").read_text(encoding="utf-8")
            self.assertIn("third_party_legal_corpus=incomplete", legal_status)
            self.assertIn("formal_distribution_ready=false", legal_status)
            self.assertFalse(output.joinpath("legal/third-party").exists())
            readme = output.joinpath("README.md").read_text(encoding="utf-8")
            self.assertIn("--bundle-root ../backend", readme)
            self.assertNotIn("`ai`", readme)
            for excluded in ("AIOrchestrator", "Ollama", "Qdrant", "Neo4j"):
                with self.subTest(excluded=excluded):
                    self.assertIn(excluded, readme)
            subprocess.run(
                ["sha256sum", "--check", "--strict", "SHA256SUMS"],
                cwd=output,
                text=True,
                capture_output=True,
                check=True,
            )
            self.assertNotIn("legal file is unavailable", result.stdout)

    def test_bundled_image_builder_resolves_the_sibling_backend_archive(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            archive_root = Path(temp_dir)
            deployment = archive_root / "deployment"
            backend = archive_root / "backend"
            result = self.run_packager(deployment)
            self.assertEqual(0, result.returncode, result.stdout)
            canary = deployment / "operator-private.env"
            canary.write_text("CANARY_PRIVATE_TLS_VALUE\n", encoding="utf-8")

            for target in TARGET_SLUGS:
                bundle = backend / target
                (bundle / "bin").mkdir(parents=True)
                (bundle / "lib").mkdir()
                (bundle / "legal").mkdir()
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
                for path in sorted((*bundle.joinpath("bin").iterdir(), *bundle.joinpath("lib").iterdir(), sbom)):
                    checksum_lines.append(
                        f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(bundle)}"
                    )
                (bundle / "SHA256SUMS").write_text("\n".join(checksum_lines) + "\n", encoding="utf-8")

            fake_bin = archive_root / "fake-bin"
            fake_bin.mkdir()
            docker_log = archive_root / "docker.log"
            fake_docker = fake_bin / "docker"
            fake_docker.write_text(
                "#!/bin/sh\n"
                "set -eu\n"
                "context=\n"
                'for argument in "$@"; do context="$argument"; done\n'
                'if grep -R -F "${CONTEXT_CANARY:?}" "$context" >/dev/null 2>&1; then\n'
                "  echo 'private canary entered the default Docker build context' >&2\n"
                "  exit 91\n"
                "fi\n"
                'printf \'%s\\n\' "$*" >> "${DOCKER_LOG:?}"\n',
                encoding="utf-8",
            )
            fake_docker.chmod(0o755)
            environment = os.environ.copy()
            environment["PATH"] = f"{fake_bin}:{environment['PATH']}"
            environment["DOCKER_LOG"] = str(docker_log)
            environment["CONTEXT_CANARY"] = "CANARY_PRIVATE_TLS_VALUE"

            built = subprocess.run(
                [
                    "bash",
                    "tools/scripts/release/build_backend_images.sh",
                    "--bundle-root",
                    "../backend",
                    "--image-prefix",
                    "memochat",
                    "--tag",
                    "local",
                ],
                cwd=deployment,
                env=environment,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=False,
            )

            self.assertEqual(0, built.returncode, built.stdout)
            docker_commands = docker_log.read_text(encoding="utf-8").splitlines()
            self.assertEqual(15, len(docker_commands))
            for target, slug in TARGET_SLUGS.items():
                with self.subTest(target=target):
                    command = next(line for line in docker_commands if f"TARGET={target}" in line)
                    self.assertIn(f"--tag memochat/{slug}:local", command)
                    self.assertIn(str(deployment / "infra/deploy/images/services/cpp-service.Dockerfile"), command)

    def test_refuses_to_merge_into_an_existing_output(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output = Path(temp_dir) / "deployment"
            output.mkdir()
            marker = output / "keep.txt"
            marker.write_text("preserve\n", encoding="utf-8")

            result = self.run_packager(output)

            self.assertNotEqual(0, result.returncode)
            self.assertEqual("preserve\n", marker.read_text(encoding="utf-8"))

    def test_packaged_compose_model_is_self_contained(self):
        if shutil.which("docker") is None:
            self.skipTest("docker is unavailable")

        probe = subprocess.run(
            ["docker", "compose", "version"],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        if probe.returncode != 0:
            self.skipTest("docker compose is unavailable")

        with tempfile.TemporaryDirectory() as temp_dir:
            output = Path(temp_dir) / "deployment"
            result = self.run_packager(output)
            self.assertEqual(0, result.returncode, result.stdout)

            local_root = output / "infra/deploy/local"
            optional_variables = {
                "MEMOCHAT_CALL_POSTGRES_PASSWORD",
                "MEMOCHAT_LIVEKIT_API_KEY",
                "MEMOCHAT_LIVEKIT_API_SECRET",
                "MEMOCHAT_LIVEKIT_URL",
                "MEMOCHAT_R18_SOURCE_ADMIN_KEY",
                "MEMOCHAT_R18_PICACG_API_KEY",
                "MEMOCHAT_R18_PICACG_HMAC_KEY",
                "MEMOCHAT_R18_CREDENTIAL_MASTER_KEY",
                "MEMOCHAT_INFLUXDB_USERNAME",
                "MEMOCHAT_INFLUXDB_PASSWORD",
                "MEMOCHAT_INFLUXDB_ADMIN_TOKEN",
                "MEMOCHAT_GRAFANA_ADMIN_USER",
                "MEMOCHAT_GRAFANA_ADMIN_PASSWORD",
            }
            source_env_file = local_root / ".env.release.example"
            env_file = output / "base-only.env"
            env_file.write_text(
                "\n".join(
                    line
                    for line in source_env_file.read_text(encoding="utf-8").splitlines()
                    if not line or line.startswith("#") or line.split("=", 1)[0] not in optional_variables
                )
                + "\n",
                encoding="utf-8",
            )
            clean_environment = {
                "HOME": os.environ.get("HOME", "/tmp"),
                "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            }
            compose = subprocess.run(
                [
                    "docker",
                    "compose",
                    "--env-file",
                    str(env_file),
                    "--project-directory",
                    str(local_root),
                    "-f",
                    str(local_root / "docker-compose.yml"),
                    "-f",
                    str(local_root / "compose/backend-services.yml"),
                    "-f",
                    str(local_root / "compose/livekit.yml"),
                    "config",
                    "--quiet",
                ],
                cwd=output,
                env=clean_environment,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=False,
            )
            self.assertEqual(0, compose.returncode, compose.stdout)

    def test_ci_packages_the_explicit_library_root_and_deployment_kit(self):
        workflow = CI_WORKFLOW.read_text(encoding="utf-8")

        self.assertIn("package_backend_deployment_kit.sh", workflow)
        self.assertIn(
            '--library-dir "$VCPKG_ROOT/installed-memochat-gcc16-server-release/x64-linux-memochat-release/lib"',
            workflow,
        )
        self.assertRegex(workflow, r"-czf \"\$archive\" -C \"\$release_parent\" backend deployment")


if __name__ == "__main__":
    unittest.main()
