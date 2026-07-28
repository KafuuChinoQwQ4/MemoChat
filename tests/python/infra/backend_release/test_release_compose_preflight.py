import os
import re
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
PREFLIGHT = REPO_ROOT / "tools/scripts/release/run_release_compose.sh"
ENV_EXAMPLE = REPO_ROOT / "infra/deploy/local/.env.release.example"
README = REPO_ROOT / "infra/deploy/local/README.md"
BACKEND_COMPOSE = REPO_ROOT / "infra/deploy/local/compose/backend-services.yml"
LIVEKIT_COMPOSE = REPO_ROOT / "infra/deploy/local/compose/livekit.yml"

PROFILE_VARIABLES = {
    "calls": (
        "MEMOCHAT_CALL_POSTGRES_PASSWORD",
        "MEMOCHAT_LIVEKIT_API_KEY",
        "MEMOCHAT_LIVEKIT_API_SECRET",
        "MEMOCHAT_LIVEKIT_URL",
    ),
    "r18": (
        "MEMOCHAT_R18_SOURCE_ADMIN_KEY",
        "MEMOCHAT_R18_PICACG_API_KEY",
        "MEMOCHAT_R18_PICACG_HMAC_KEY",
        "MEMOCHAT_R18_CREDENTIAL_MASTER_KEY",
    ),
    "observability": (
        "MEMOCHAT_INFLUXDB_USERNAME",
        "MEMOCHAT_INFLUXDB_PASSWORD",
        "MEMOCHAT_INFLUXDB_ADMIN_TOKEN",
        "MEMOCHAT_GRAFANA_ADMIN_USER",
        "MEMOCHAT_GRAFANA_ADMIN_PASSWORD",
    ),
}
OPTIONAL_PROFILE_VARIABLES = frozenset(variable_name for names in PROFILE_VARIABLES.values() for variable_name in names)

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

OBSERVABILITY_RUNTIME_DIRECTORIES = (
    "observability/credentials",
    "observability/influxdb",
    "observability/grafana",
    "observability/prometheus",
    "observability/alertmanager",
    "observability/loki",
    "observability/tempo",
)

RELATION_ROLE_TOKEN_VARIABLES = (
    "MEMOCHAT_RELATION_COMMAND_AUTH_TOKEN",
    "MEMOCHAT_RELATION_CHAT_QUERY_AUTH_TOKEN",
    "MEMOCHAT_RELATION_CALL_AUTH_TOKEN",
    "MEMOCHAT_RELATION_MOMENTS_AUTH_TOKEN",
)


def run(*args: str, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=REPO_ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )


class ReleaseComposePreflightTests(unittest.TestCase):
    def make_release_env(self, temp: Path) -> tuple[Path, dict[str, str]]:
        data_root = temp / "data"
        data_root.mkdir()
        data_root.chmod(0o700)
        (data_root / "media/uploads").mkdir(parents=True)
        (data_root / "r18").mkdir()
        (data_root / "envoy/logs").mkdir(parents=True)
        (data_root / "redpanda/data").mkdir(parents=True)
        for relative_directory in OBSERVABILITY_RUNTIME_DIRECTORIES:
            (data_root / relative_directory).mkdir(parents=True)
        runtime_uid = os.getuid() if os.getuid() != 0 else 10001
        runtime_gid = os.getgid() if os.getuid() != 0 else 10001
        for runtime_directory in (
            data_root / "media/uploads",
            data_root / "r18",
            data_root / "envoy/logs",
            data_root / "redpanda/data",
            *(data_root / relative_directory for relative_directory in OBSERVABILITY_RUNTIME_DIRECTORIES),
        ):
            runtime_directory.chmod(0o700)
            if os.getuid() == 0:
                os.chown(runtime_directory, runtime_uid, runtime_gid)
        config_root = temp / "config"
        for relative in CONFIG_FILES:
            config_file = config_root / relative
            config_file.parent.mkdir(parents=True, exist_ok=True)
            config_file.write_text("[App]\nName=MemoChat\n", encoding="utf-8")
            config_file.chmod(0o644)

        key_file = temp / "server.key"
        cert_file = temp / "server.crt"
        subprocess.run(
            [
                "openssl",
                "req",
                "-x509",
                "-nodes",
                "-newkey",
                "rsa:2048",
                "-days",
                "2",
                "-keyout",
                str(key_file),
                "-out",
                str(cert_file),
                "-subj",
                "/CN=localhost",
                "-addext",
                "subjectAltName=DNS:localhost,IP:127.0.0.1",
            ],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        cert_file.chmod(0o644)
        key_file.chmod(0o600)

        replacements = {
            "MEMOCHAT_BACKEND_CONFIG_ROOT": str(config_root),
            "MEMOCHAT_DOCKER_DATA_ROOT": str(data_root),
            "MEMOCHAT_TLS_CERT_FILE": str(cert_file),
            "MEMOCHAT_TLS_KEY_FILE": str(key_file),
            "MEMOCHAT_RUNTIME_UID": str(runtime_uid),
            "MEMOCHAT_RUNTIME_GID": str(runtime_gid),
            "MEMOCHAT_R18_CREDENTIAL_MASTER_KEY": "a1" * 32,
            "MEMOCHAT_EMAIL_SMTPUSER": "fixture-smtp-user",
            "MEMOCHAT_EMAIL_FROM": "noreply@example.invalid",
        }
        env_values: dict[str, str] = {}
        rendered_lines: list[str] = []
        for line in ENV_EXAMPLE.read_text(encoding="utf-8").splitlines():
            if not line or line.startswith("#"):
                rendered_lines.append(line)
                continue
            name, value = line.split("=", 1)
            if name in replacements:
                value = replacements[name]
            elif value.startswith("REPLACE_WITH_"):
                value = f"fixture-{name.lower()}-0123456789abcdef0123456789abcdef"
            env_values[name] = value
            rendered_lines.append(f"{name}={value}")

        env_file = temp / "release.env"
        env_file.write_text("\n".join(rendered_lines) + "\n", encoding="utf-8")
        env_file.chmod(0o600)
        return env_file, env_values

    def make_fake_docker(self, temp: Path) -> tuple[dict[str, str], Path]:
        bin_dir = temp / "bin"
        bin_dir.mkdir()
        capture = temp / "docker.args"
        docker = bin_dir / "docker"
        docker.write_text(
            "#!/usr/bin/env bash\n"
            "set -eu\n"
            "if compgen -A variable | grep -q '^MEMOCHAT_'; then\n"
            "  echo '[fake-docker] MemoChat variables leaked into process environment' >&2\n"
            "  exit 73\n"
            "fi\n"
            'if [[ -n "${FAKE_DOCKER_PROFILE_CAPTURE:-}" ]]; then\n'
            '  printf \'%s\\n\' "${COMPOSE_MEMOCHAT_ENABLE_CALLS:-unset}" >> "$FAKE_DOCKER_PROFILE_CAPTURE"\n'
            "fi\n"
            'printf \'%s\\n\' "---" >> "${FAKE_DOCKER_CAPTURE:?}"\n'
            'printf \'%s\\n\' "$@" >> "${FAKE_DOCKER_CAPTURE:?}"\n'
            'if [[ "${1:-}" == image && "${2:-}" == inspect && "${4:-}" == "${FAKE_DOCKER_MISSING_IMAGE:-}" ]]; then\n'
            "  exit 66\n"
            "fi\n"
            'if [[ "${*: -1}" == memochat-release-provision-influxdb && -n "${FAKE_GRAFANA_TOKEN_FILE:-}" ]]; then\n'
            "  printf '%s\\n' fixture-rotated-grafana-reader-token-0123456789abcdef > \"$FAKE_GRAFANA_TOKEN_FILE\"\n"
            '  chmod 0600 "$FAKE_GRAFANA_TOKEN_FILE"\n'
            "fi\n"
            'if [[ "$*" == *" ps -q memochat-grafana"* && -n "${FAKE_GRAFANA_RUNNING:-}" ]]; then\n'
            "  echo fake-grafana-container-id\n"
            "fi\n",
            encoding="utf-8",
        )
        docker.chmod(docker.stat().st_mode | stat.S_IXUSR)
        environment = os.environ.copy()
        environment["PATH"] = f"{bin_dir}:{environment['PATH']}"
        environment["FAKE_DOCKER_CAPTURE"] = str(capture)
        environment["FAKE_GRAFANA_TOKEN_FILE"] = str(temp / "data/observability/credentials/grafana-reader.token")
        return environment, capture

    @staticmethod
    def omit_env_variables(env_file: Path, names: set[str] | frozenset[str]) -> None:
        retained_lines = []
        for line in env_file.read_text(encoding="utf-8").splitlines():
            if line and not line.startswith("#") and line.split("=", 1)[0] in names:
                continue
            retained_lines.append(line)
        env_file.write_text("\n".join(retained_lines) + "\n", encoding="utf-8")

    def test_base_profile_accepts_env_without_optional_profile_inputs(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, env_values = self.make_release_env(temp)
            self.omit_env_variables(env_file, OPTIONAL_PROFILE_VARIABLES)
            Path(env_values["MEMOCHAT_BACKEND_CONFIG_ROOT"], "CallService/callgateway.ini").unlink()
            Path(env_values["MEMOCHAT_BACKEND_CONFIG_ROOT"], "R18Service/r18gateway.ini").unlink()
            Path(env_values["MEMOCHAT_DOCKER_DATA_ROOT"], "r18").rmdir()
            for relative_directory in reversed(OBSERVABILITY_RUNTIME_DIRECTORIES):
                Path(env_values["MEMOCHAT_DOCKER_DATA_ROOT"], relative_directory).rmdir()
            Path(env_values["MEMOCHAT_DOCKER_DATA_ROOT"], "observability").rmdir()
            environment, capture = self.make_fake_docker(temp)

            checked = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "check", env=environment)
            configured = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "config", env=environment)

            self.assertEqual(checked.returncode, 0, checked.stdout)
            self.assertEqual(configured.returncode, 0, configured.stdout)
            self.assertIn("config\n--quiet", capture.read_text(encoding="utf-8"))

    def test_selected_profiles_require_every_profile_input_before_docker(self):
        for profile, variable_names in PROFILE_VARIABLES.items():
            for variable_name in variable_names:
                with self.subTest(profile=profile, variable=variable_name), tempfile.TemporaryDirectory() as temp_text:
                    temp = Path(temp_text)
                    env_file, _ = self.make_release_env(temp)
                    self.omit_env_variables(env_file, {variable_name})
                    environment, capture = self.make_fake_docker(temp)

                    result = run(
                        "bash",
                        str(PREFLIGHT),
                        "--env-file",
                        str(env_file),
                        "--profile",
                        profile,
                        "check",
                        env=environment,
                    )

                    self.assertNotEqual(result.returncode, 0, result.stdout)
                    self.assertIn(variable_name, result.stdout)
                    self.assertFalse(capture.exists(), result.stdout)

    def test_selected_profiles_accept_complete_profile_inputs(self):
        for profile in PROFILE_VARIABLES:
            with self.subTest(profile=profile), tempfile.TemporaryDirectory() as temp_text:
                temp = Path(temp_text)
                env_file, _ = self.make_release_env(temp)
                environment, capture = self.make_fake_docker(temp)

                result = run(
                    "bash",
                    str(PREFLIGHT),
                    "--env-file",
                    str(env_file),
                    "--profile",
                    profile,
                    "check",
                    env=environment,
                )

                self.assertEqual(result.returncode, 0, result.stdout)
                self.assertFalse(capture.exists(), result.stdout)

    def test_optional_profile_variables_do_not_require_compose_interpolation(self):
        compose_text = "\n".join(
            (
                BACKEND_COMPOSE.read_text(encoding="utf-8"),
                LIVEKIT_COMPOSE.read_text(encoding="utf-8"),
            )
        )
        for variable_name in OPTIONAL_PROFILE_VARIABLES:
            with self.subTest(variable=variable_name):
                self.assertNotIn(f"${{{variable_name}:?", compose_text)

    def test_wrapper_owns_the_call_provisioning_flag(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, env_values = self.make_release_env(temp)
            environment, _ = self.make_fake_docker(temp)
            profile_capture = temp / "profile.flag"
            environment["FAKE_DOCKER_PROFILE_CAPTURE"] = str(profile_capture)

            base = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "config", env=environment)
            calls = run(
                "bash",
                str(PREFLIGHT),
                "--env-file",
                str(env_file),
                "--profile",
                "calls",
                "config",
                env=environment,
            )

            self.assertEqual(base.returncode, 0, base.stdout)
            self.assertEqual(calls.returncode, 0, calls.stdout)
            self.assertEqual(["0", "1"], profile_capture.read_text(encoding="utf-8").splitlines())
            compose = BACKEND_COMPOSE.read_text(encoding="utf-8")
            self.assertIn(
                "MEMOCHAT_PROVISION_CALLS: ${COMPOSE_MEMOCHAT_ENABLE_CALLS:-0}",
                compose,
            )

    def test_check_only_accepts_complete_private_inputs_without_invoking_docker(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, _ = self.make_release_env(temp)
            environment, capture = self.make_fake_docker(temp)

            result = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "check", env=environment)

            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn("preflight passed", result.stdout.lower())
            self.assertFalse(capture.exists(), result.stdout)

    def test_config_invokes_only_the_fixed_release_compose_files_and_clears_ambient_overrides(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, env_values = self.make_release_env(temp)
            environment, capture = self.make_fake_docker(temp)
            environment["MEMOCHAT_REDIS_PASSWORD"] = "123456"
            environment["MEMOCHAT_MINIO_ROOT_PASSWORD"] = "legacy-ambient-value"

            result = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "config", env=environment)

            self.assertEqual(result.returncode, 0, result.stdout)
            arguments = [line for line in capture.read_text(encoding="utf-8").splitlines() if line != "---"]
            self.assertEqual(arguments[0], "compose")
            self.assertIn("--env-file", arguments)
            self.assertIn(str(env_file), arguments)
            self.assertIn(str(REPO_ROOT / "infra/deploy/local/docker-compose.yml"), arguments)
            self.assertIn(str(REPO_ROOT / "infra/deploy/local/compose/backend-services.yml"), arguments)
            self.assertIn(str(REPO_ROOT / "infra/deploy/local/compose/livekit.yml"), arguments)
            self.assertEqual(arguments[-2:], ["config", "--quiet"])
            for secret in env_values.values():
                self.assertNotIn(secret, result.stdout)

    def test_up_is_detached_and_preserves_only_explicit_profile_and_service_arguments(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, _ = self.make_release_env(temp)
            environment, capture = self.make_fake_docker(temp)

            result = run(
                "bash",
                str(PREFLIGHT),
                "--env-file",
                str(env_file),
                "--profile",
                "observability",
                "up",
                "memochat-chat-server",
                env=environment,
            )

            self.assertEqual(result.returncode, 0, result.stdout)
            arguments = [line for line in capture.read_text(encoding="utf-8").splitlines() if line != "---"]
            self.assertEqual(
                arguments[-5:],
                ["--profile", "observability", "up", "-d", "memochat-chat-server"],
            )

    def test_up_cannot_activate_an_unvalidated_profile_service(self):
        cases = (
            ("memochat-call-gateway", "calls"),
            ("memochat-livekit", "calls"),
            ("memochat-r18-gateway", "r18"),
            ("memochat-cadvisor", "observability"),
            ("memochat-grafana", "observability"),
        )
        for service_name, required_profile in cases:
            with self.subTest(service=service_name), tempfile.TemporaryDirectory() as temp_text:
                temp = Path(temp_text)
                env_file, _ = self.make_release_env(temp)
                environment, capture = self.make_fake_docker(temp)

                result = run(
                    "bash",
                    str(PREFLIGHT),
                    "--env-file",
                    str(env_file),
                    "up",
                    service_name,
                    env=environment,
                )

                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn(f"--profile {required_profile}", result.stdout)
                self.assertFalse(capture.exists(), result.stdout)

    def test_up_rejects_direct_provision_job_activation(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, _ = self.make_release_env(temp)
            environment, capture = self.make_fake_docker(temp)

            result = run(
                "bash",
                str(PREFLIGHT),
                "--env-file",
                str(env_file),
                "up",
                "memochat-release-provision-influxdb",
                env=environment,
            )

            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("provision action", result.stdout.lower())
            self.assertFalse(capture.exists(), result.stdout)

    def test_observability_up_runs_the_idempotent_influx_scraper_provisioner(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, env_values = self.make_release_env(temp)
            environment, capture = self.make_fake_docker(temp)

            result = run(
                "bash",
                str(PREFLIGHT),
                "--env-file",
                str(env_file),
                "--profile",
                "observability",
                "up",
                "memochat-prometheus",
                env=environment,
            )

            self.assertEqual(result.returncode, 0, result.stdout)
            credential_root = Path(env_values["MEMOCHAT_DOCKER_DATA_ROOT"]) / "observability/credentials"
            expected_runtime_secrets = {
                "influxdb-username": env_values["MEMOCHAT_INFLUXDB_USERNAME"],
                "influxdb-password": env_values["MEMOCHAT_INFLUXDB_PASSWORD"],
                "influxdb-admin.token": env_values["MEMOCHAT_INFLUXDB_ADMIN_TOKEN"],
            }
            for file_name, expected_value in expected_runtime_secrets.items():
                with self.subTest(runtime_secret=file_name):
                    secret_file = credential_root / file_name
                    self.assertEqual(secret_file.read_text(encoding="utf-8"), f"{expected_value}\n")
                    self.assertEqual(stat.S_IMODE(secret_file.stat().st_mode), 0o400)
                    self.assertEqual(secret_file.stat().st_uid, int(env_values["MEMOCHAT_RUNTIME_UID"]))
                    self.assertEqual(secret_file.stat().st_gid, int(env_values["MEMOCHAT_RUNTIME_GID"]))
                    self.assertNotIn(expected_value, result.stdout)
            self.assertNotIn("fixture-old-grafana-reader-token", result.stdout)
            self.assertNotIn("fixture-rotated-grafana-reader-token", result.stdout)
            calls = capture.read_text(encoding="utf-8")
            self.assertRegex(calls, r"up\n-d\nmemochat-postgres\nmemochat-mongo\nmemochat-minio\nmemochat-influxdb")
            self.assertRegex(
                calls,
                r"--profile\nprovision\nrun\n--rm\n--no-deps\nmemochat-release-provision-influxdb",
            )
            self.assertTrue(calls.rstrip().endswith("up\n-d\nmemochat-prometheus"), calls)

    def test_observability_token_rotation_recreates_only_an_existing_grafana(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, env_values = self.make_release_env(temp)
            environment, capture = self.make_fake_docker(temp)
            token_file = (
                Path(env_values["MEMOCHAT_DOCKER_DATA_ROOT"]) / "observability/credentials/grafana-reader.token"
            )
            token_file.write_text("fixture-old-grafana-reader-token-0123456789abcdef\n", encoding="utf-8")
            token_file.chmod(0o600)
            if os.getuid() == 0:
                os.chown(token_file, int(env_values["MEMOCHAT_RUNTIME_UID"]), int(env_values["MEMOCHAT_RUNTIME_GID"]))
            environment["FAKE_GRAFANA_TOKEN_FILE"] = str(token_file)
            environment["FAKE_GRAFANA_RUNNING"] = "1"

            result = run(
                "bash",
                str(PREFLIGHT),
                "--env-file",
                str(env_file),
                "--profile",
                "observability",
                "provision",
                env=environment,
            )

            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertNotIn("fixture-old-grafana-reader-token", result.stdout)
            self.assertNotIn("fixture-rotated-grafana-reader-token", result.stdout)
            calls = capture.read_text(encoding="utf-8")
            self.assertRegex(calls, r"ps\n-q\nmemochat-grafana")
            self.assertRegex(
                calls,
                r"up\n-d\n--no-deps\n--force-recreate\nmemochat-grafana",
            )

    def test_up_rejects_option_like_service_arguments_before_docker(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, _ = self.make_release_env(temp)
            environment, capture = self.make_fake_docker(temp)

            result = run(
                "bash",
                str(PREFLIGHT),
                "--env-file",
                str(env_file),
                "up",
                "--remove-orphans",
                env=environment,
            )

            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("service", result.stdout.lower())
            self.assertFalse(capture.exists(), result.stdout)

    def test_up_rejects_a_missing_backend_image_before_provisioning(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, _ = self.make_release_env(temp)
            environment, capture = self.make_fake_docker(temp)
            environment["FAKE_DOCKER_MISSING_IMAGE"] = "memochat/login-server:local"

            result = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "up", env=environment)

            self.assertNotEqual(0, result.returncode, result.stdout)
            self.assertIn("memochat/login-server:local", result.stdout)
            calls = capture.read_text(encoding="utf-8")
            self.assertIn("image\ninspect\n--\nmemochat/login-server:local", calls)
            self.assertNotIn("memochat-release-provision-postgres", calls)
            self.assertNotIn("memochat-postgres", calls)

    def test_unavailable_or_unknown_profiles_fail_before_docker(self):
        for profile in ("ai", "unknown"):
            with self.subTest(profile=profile), tempfile.TemporaryDirectory() as temp_text:
                temp = Path(temp_text)
                env_file, _ = self.make_release_env(temp)
                environment, capture = self.make_fake_docker(temp)

                result = run(
                    "bash",
                    str(PREFLIGHT),
                    "--env-file",
                    str(env_file),
                    "--profile",
                    profile,
                    "check",
                    env=environment,
                )

                self.assertNotEqual(0, result.returncode, result.stdout)
                self.assertIn("profile", result.stdout.lower())
                self.assertFalse(capture.exists(), result.stdout)

    def test_placeholder_weak_and_short_secret_values_fail_before_docker(self):
        cases = {
            "placeholder": ("MEMOCHAT_REDIS_PASSWORD", "REPLACE_WITH_SECRET"),
            "weak": ("MEMOCHAT_REDIS_PASSWORD", "123456"),
            "short": ("MEMOCHAT_AUTHTOKEN_JWTSECRET", "not-long-enough"),
            "weak-mongo-app-password": ("MEMOCHAT_MONGO_APP_PASSWORD", "123456"),
            "short-relation-call-token": ("MEMOCHAT_RELATION_CALL_AUTH_TOKEN", "x" * 31),
            "non-ascii-relation-token": ("MEMOCHAT_RELATION_MOMENTS_AUTH_TOKEN", "密" * 32),
        }
        for case, (name, value) in cases.items():
            with self.subTest(case=case), tempfile.TemporaryDirectory() as temp_text:
                temp = Path(temp_text)
                env_file, _ = self.make_release_env(temp)
                original = env_file.read_text(encoding="utf-8")
                env_file.write_text(
                    "\n".join(
                        f"{name}={value}" if line.startswith(f"{name}=") else line for line in original.splitlines()
                    )
                    + "\n",
                    encoding="utf-8",
                )
                environment, capture = self.make_fake_docker(temp)

                result = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "config", env=environment)

                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn(name, result.stdout)
                self.assertNotIn(value, result.stdout)
                self.assertFalse(capture.exists(), result.stdout)

    def test_relation_role_tokens_are_required_and_cannot_be_reused(self):
        for variable_name in RELATION_ROLE_TOKEN_VARIABLES:
            with self.subTest(missing=variable_name), tempfile.TemporaryDirectory() as temp_text:
                temp = Path(temp_text)
                env_file, _ = self.make_release_env(temp)
                retained_lines = [
                    line
                    for line in env_file.read_text(encoding="utf-8").splitlines()
                    if not line.startswith(f"{variable_name}=")
                ]
                env_file.write_text("\n".join(retained_lines) + "\n", encoding="utf-8")
                environment, capture = self.make_fake_docker(temp)

                result = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "check", env=environment)

                self.assertNotEqual(0, result.returncode, result.stdout)
                self.assertIn(variable_name, result.stdout)
                self.assertFalse(capture.exists(), result.stdout)

        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, env_values = self.make_release_env(temp)
            duplicate_value = env_values["MEMOCHAT_RELATION_COMMAND_AUTH_TOKEN"]
            rendered_lines = [
                f"MEMOCHAT_RELATION_CALL_AUTH_TOKEN={duplicate_value}"
                if line.startswith("MEMOCHAT_RELATION_CALL_AUTH_TOKEN=")
                else line
                for line in env_file.read_text(encoding="utf-8").splitlines()
            ]
            env_file.write_text("\n".join(rendered_lines) + "\n", encoding="utf-8")
            environment, capture = self.make_fake_docker(temp)

            result = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "check", env=environment)

            self.assertNotEqual(0, result.returncode, result.stdout)
            self.assertIn("must not be reused", result.stdout)
            self.assertNotIn(duplicate_value, result.stdout)
            self.assertFalse(capture.exists(), result.stdout)

    def test_env_file_must_be_owned_mode_0600_regular_and_non_symlink(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, _ = self.make_release_env(temp)
            environment, _ = self.make_fake_docker(temp)

            env_file.chmod(0o640)
            result = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "check", env=environment)
            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("0600", result.stdout)

            env_file.chmod(0o600)
            link = temp / "release-link.env"
            link.symlink_to(env_file)
            result = run("bash", str(PREFLIGHT), "--env-file", str(link), "check", env=environment)
            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("non-symlink", result.stdout.lower())

    def test_private_key_and_config_mounts_fail_closed_on_links_or_unsafe_permissions(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, env_values = self.make_release_env(temp)
            environment, _ = self.make_fake_docker(temp)
            key_file = Path(env_values["MEMOCHAT_TLS_KEY_FILE"])
            key_file.chmod(0o644)

            result = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "check", env=environment)

            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("MEMOCHAT_TLS_KEY_FILE", result.stdout)
            self.assertNotIn(str(key_file), result.stdout)

        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, env_values = self.make_release_env(temp)
            environment, _ = self.make_fake_docker(temp)
            cert_file = Path(env_values["MEMOCHAT_TLS_CERT_FILE"])
            cert_file.chmod(0o666)

            result = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "check", env=environment)

            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("MEMOCHAT_TLS_CERT_FILE", result.stdout)
            self.assertNotIn(str(cert_file), result.stdout)

        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, env_values = self.make_release_env(temp)
            environment, _ = self.make_fake_docker(temp)
            config_file = Path(env_values["MEMOCHAT_BACKEND_CONFIG_ROOT"]) / CONFIG_FILES[0]
            real_config = config_file.with_suffix(".real.ini")
            config_file.rename(real_config)
            config_file.symlink_to(real_config)

            result = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "check", env=environment)

            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn(CONFIG_FILES[0], result.stdout)
            self.assertIn("non-symlink", result.stdout.lower())

        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file, env_values = self.make_release_env(temp)
            environment, _ = self.make_fake_docker(temp)
            config_file = Path(env_values["MEMOCHAT_BACKEND_CONFIG_ROOT"]) / CONFIG_FILES[0]
            service_dir = config_file.parent
            real_service_dir = service_dir.with_name(f"{service_dir.name}-real")
            service_dir.rename(real_service_dir)
            service_dir.symlink_to(real_service_dir, target_is_directory=True)

            result = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "check", env=environment)

            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn(CONFIG_FILES[0], result.stdout)
            self.assertIn("symlink", result.stdout.lower())

    def test_missing_mutable_runtime_directories_fail_before_docker(self):
        cases = (
            ("media/uploads", None),
            ("envoy/logs", None),
            ("redpanda/data", None),
            ("r18", "r18"),
            *((relative_path, "observability") for relative_path in OBSERVABILITY_RUNTIME_DIRECTORIES),
        )
        for relative_path, profile in cases:
            with self.subTest(relative_path=relative_path, profile=profile), tempfile.TemporaryDirectory() as temp_text:
                temp = Path(temp_text)
                env_file, env_values = self.make_release_env(temp)
                environment, capture = self.make_fake_docker(temp)
                (Path(env_values["MEMOCHAT_DOCKER_DATA_ROOT"]) / relative_path).rmdir()

                arguments = ["bash", str(PREFLIGHT), "--env-file", str(env_file)]
                if profile:
                    arguments.extend(("--profile", profile))
                arguments.append("check")
                result = run(*arguments, env=environment)

                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn(relative_path, result.stdout)
                self.assertFalse(capture.exists(), result.stdout)

    def test_mutable_runtime_directories_require_owner_only_permissions(self):
        cases = (
            ("envoy/logs", None),
            ("observability/grafana", "observability"),
        )
        for relative_path, profile in cases:
            with self.subTest(relative_path=relative_path), tempfile.TemporaryDirectory() as temp_text:
                temp = Path(temp_text)
                env_file, env_values = self.make_release_env(temp)
                environment, capture = self.make_fake_docker(temp)
                runtime_directory = Path(env_values["MEMOCHAT_DOCKER_DATA_ROOT"]) / relative_path
                runtime_directory.chmod(0o755)

                arguments = ["bash", str(PREFLIGHT), "--env-file", str(env_file)]
                if profile:
                    arguments.extend(("--profile", profile))
                arguments.append("check")
                result = run(*arguments, env=environment)

                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn(relative_path, result.stdout)
                self.assertIn("0700", result.stdout)
                self.assertFalse(capture.exists(), result.stdout)

    def test_duplicate_expanding_or_command_like_assignments_are_rejected_without_execution(self):
        cases = {
            "duplicate": "MEMOCHAT_REDIS_PASSWORD=another-strong-fixture-secret-0123456789\n",
            "expansion": "MEMOCHAT_IMAGE_TAG=${UNTRUSTED_TAG}\n",
            "command": "MEMOCHAT_IMAGE_TAG=$(touch should-not-exist)\n",
        }
        for case, extra_line in cases.items():
            with self.subTest(case=case), tempfile.TemporaryDirectory() as temp_text:
                temp = Path(temp_text)
                env_file, _ = self.make_release_env(temp)
                with env_file.open("a", encoding="utf-8") as stream:
                    stream.write(extra_line)
                environment, _ = self.make_fake_docker(temp)

                result = run("bash", str(PREFLIGHT), "--env-file", str(env_file), "check", env=environment)

                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertFalse((REPO_ROOT / "should-not-exist").exists())
                self.assertNotIn(extra_line.split("=", 1)[1].strip(), result.stdout)

    def test_release_documentation_routes_validation_and_start_through_preflight(self):
        readme = README.read_text(encoding="utf-8")
        start_section = readme.split("### 3. Validate and start", 1)[1].split("### 4.", 1)[0]
        self.assertIn("tools/scripts/release/run_release_compose.sh", start_section)
        self.assertNotIn("docker compose --env-file", start_section)

    def test_release_documentation_tracks_profiles_for_status_and_shutdown(self):
        readme = README.read_text(encoding="utf-8")
        health_section = readme.split("### 4. Health checks and stop", 1)[1].split("## Compose Files", 1)[0]

        self.assertIn("RELEASE_PROFILE_ARGS=()", health_section)
        self.assertIn("-f infra/deploy/local/compose/livekit.yml", health_section)
        self.assertIn('"${RELEASE_PROFILE_ARGS[@]}" ps', health_section)
        self.assertIn('"${RELEASE_PROFILE_ARGS[@]}" \\\n  down', health_section)

    def test_release_documentation_states_the_influx_admin_token_file_boundary(self):
        readme = README.read_text(encoding="utf-8")

        self.assertIn("native `_FILE`", readme)
        self.assertIn("docker inspect", readme)
        self.assertIn("mode `0400`", readme)

    def test_preflight_allowlists_track_every_release_compose_variable_and_config_mount(self):
        script = PREFLIGHT.read_text(encoding="utf-8")
        compose = BACKEND_COMPOSE.read_text(encoding="utf-8")
        example_names = {
            line.split("=", 1)[0]
            for line in ENV_EXAMPLE.read_text(encoding="utf-8").splitlines()
            if line and not line.startswith("#")
        }
        variable_blocks = re.findall(
            r"(?:readonly -a|declare -a) [A-Z0-9_]*REQUIRED_VARIABLES=\(\n(?P<body>.*?)\n\)",
            script,
            flags=re.S,
        )
        self.assertGreaterEqual(len(variable_blocks), 3)
        script_names = set(re.findall(r"MEMOCHAT_[A-Z0-9_]+", "\n".join(variable_blocks)))
        compose_names = set(re.findall(r"\$\{(MEMOCHAT_[A-Z0-9_]+)", compose))
        materialized_secret_names = set(re.findall(r"materialize_runtime_secret [^\n]+ (MEMOCHAT_[A-Z0-9_]+)", script))
        self.assertEqual(
            materialized_secret_names,
            {
                "MEMOCHAT_INFLUXDB_USERNAME",
                "MEMOCHAT_INFLUXDB_PASSWORD",
                "MEMOCHAT_INFLUXDB_ADMIN_TOKEN",
            },
        )
        self.assertEqual(script_names, compose_names | materialized_secret_names)
        self.assertEqual(script_names, example_names)

        config_blocks = re.findall(
            r"(?:readonly -a|declare -a) [A-Z0-9_]*CONFIG_FILES=\(\n(?P<body>.*?)\n\)",
            script,
            flags=re.S,
        )
        self.assertGreaterEqual(len(config_blocks), 3)
        script_configs = set(re.findall(r"[A-Za-z0-9_-]+/[A-Za-z0-9_.-]+\.ini", "\n".join(config_blocks)))
        compose_configs = set(
            re.findall(
                r"source:\s+\$\{MEMOCHAT_BACKEND_CONFIG_ROOT[^}]*\}/([A-Za-z0-9_./-]+\.ini)",
                compose,
            )
        )
        self.assertEqual(script_configs, compose_configs)
        self.assertEqual(script_configs, set(CONFIG_FILES))


if __name__ == "__main__":
    unittest.main()
