import hashlib
import json
import os
import re
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path

import yaml

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
DEPLOY_SCRIPT = REPO_ROOT / "tools/scripts/status/deploy_services.sh"
TOPOLOGY_SCRIPT = REPO_ROOT / "tools/scripts/status/runtime_topology.sh"
RELEASE_SCANNER = REPO_ROOT / "tools/scripts/release/verify_release_tree.sh"
BUILD_ENV_LOADER = REPO_ROOT / "tools/scripts/release/load_build_environment.sh"
CLIENT_SCAN_ALLOWLIST = REPO_ROOT / "tools/scripts/release/client_release_scan.allowlist"
VCPKG_SBOM_GENERATOR = REPO_ROOT / "tools/scripts/release/generate_vcpkg_installed_sbom.py"
INI_CONFIG = REPO_ROOT / "apps/server/core/common/runtime/IniConfig.cpp"
CI_WORKFLOW = REPO_ROOT / ".github/workflows/ci.yml"
BACKEND_COMPOSE = REPO_ROOT / "infra/deploy/local/compose/backend-services.yml"
ACTION_PINS = {
    "actions/attest-build-provenance": (
        "e8998f949152b193b063cb0ec769d69d929409be",
        "v2.4.0",
    ),
    "actions/checkout": ("11d5960a326750d5838078e36cf38b85af677262", "v4.4.0"),
    "actions/download-artifact": ("d3f86a106a0bac45b974a628896c90dbdf5c8093", "v4.3.0"),
    "actions/setup-python": ("a26af69be951a213d495a4c3e4e4022e16d87065", "v5.6.0"),
    "actions/upload-artifact": ("ea165f8d65b6e75b540449e92b4886f43607fa02", "v4.6.2"),
    "docker/login-action": ("c94ce9fb468520275223c153574b00df6fe4bcc9", "v3.7.0"),
    "docker/setup-buildx-action": ("8d2750c68a42422c14e847fe6c8ac0403b4cbd6f", "v3.12.0"),
}
TRACKED_VERIFY_LOGS = (
    "apps/server/Memo_ops/artifacts/logs/services/VarifyServer/VarifyServer_20260426.json",
    "apps/server/Memo_ops/artifacts/logs/services/VarifyServer/VarifyServer_20260427.json",
)


def run(*args: str, cwd: Path = REPO_ROOT, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=cwd,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )


def topology_source_executables() -> set[str]:
    source = TOPOLOGY_SCRIPT.read_text(encoding="utf-8")
    match = re.search(r"MEMOCHAT_RUNTIME_SERVICE_TOPOLOGY=\(\n(?P<body>.*?)\n\)", source, flags=re.S)
    if match is None:
        raise AssertionError("runtime topology array is missing")
    return {row.split("|")[3] for row in re.findall(r'^\s*"([^"]+)"', match.group("body"), flags=re.M)}


def workflow_job_body(workflow: str, job_name: str) -> str:
    match = re.search(
        rf"(?ms)^  {re.escape(job_name)}:\n(?P<body>.*?)(?=^  [a-z0-9-]+:\n|\Z)",
        workflow,
    )
    if match is None:
        raise AssertionError(f"workflow job is missing: {job_name}")
    return match.group("body")


def workflow_step_script(workflow_path: Path, job_name: str, step_name: str) -> str:
    workflow = yaml.safe_load(workflow_path.read_text(encoding="utf-8"))
    for step in workflow["jobs"][job_name]["steps"]:
        if step.get("name") == step_name:
            return step["run"]
    raise AssertionError(f"workflow step is missing: {job_name}/{step_name}")


class UniqueKeyLoader(yaml.SafeLoader):
    pass


def construct_unique_mapping(
    loader: UniqueKeyLoader,
    node: yaml.nodes.MappingNode,
    deep: bool = False,
) -> dict[object, object]:
    mapping: dict[object, object] = {}
    for key_node, value_node in node.value:
        key = loader.construct_object(key_node, deep=deep)
        if key in mapping:
            raise yaml.constructor.ConstructorError(
                "while constructing a mapping",
                node.start_mark,
                f"found duplicate key: {key!r}",
                key_node.start_mark,
            )
        mapping[key] = loader.construct_object(value_node, deep=deep)
    return mapping


UniqueKeyLoader.add_constructor(
    yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG,
    construct_unique_mapping,
)


class FreshRuntimeDeployTests(unittest.TestCase):
    def test_deploy_promotes_a_fresh_allowlisted_runtime_and_preserves_old_state_outside_it(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            build_bin = temp / "build-bin"
            runtime = temp / "runtime" / "services"
            backup_root = temp / "private-backups"
            build_bin.mkdir()
            runtime.mkdir(parents=True)

            for executable in topology_source_executables():
                path = build_bin / executable
                path.write_text("#!/usr/bin/env sh\nexit 0\n", encoding="utf-8")
                path.chmod(path.stat().st_mode | stat.S_IXUSR)

            leaked_key = runtime / "chatserver1" / "server.key"
            leaked_key.parent.mkdir()
            leaked_key.write_text("-----BEGIN PRIVATE KEY-----\nsecret\n", encoding="utf-8")
            leaked_log = runtime / "MemoChatQml" / "logs" / "client.log"
            leaked_log.parent.mkdir(parents=True)
            leaked_log.write_text("token=secret", encoding="utf-8")

            env = os.environ.copy()
            env.update(
                {
                    "MEMOCHAT_RUNTIME_BACKUP_ROOT": str(backup_root),
                    "MEMOCHAT_REQUIRE_GPT_SOVITS": "0",
                }
            )
            result = run(
                "bash",
                str(DEPLOY_SCRIPT),
                "--build-bin",
                str(build_bin),
                "--client-build-bin",
                str(build_bin),
                "--runtime-dir",
                str(runtime),
                env=env,
            )

            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertFalse(leaked_key.exists(), result.stdout)
            self.assertFalse(leaked_log.exists(), result.stdout)
            backups = list(backup_root.glob("services-*"))
            self.assertEqual(len(backups), 1, result.stdout)
            self.assertTrue((backups[0] / "chatserver1/server.key").is_file())
            self.assertTrue((backups[0] / "MemoChatQml/logs/client.log").is_file())
            self.assertNotEqual(runtime.resolve(), backups[0].resolve())

    def test_deploy_rejects_runtime_that_overlaps_its_build_input(self):
        with tempfile.TemporaryDirectory() as temp_text:
            build_bin = Path(temp_text) / "build-bin"
            build_bin.mkdir()
            for executable in topology_source_executables():
                path = build_bin / executable
                path.write_text("binary", encoding="utf-8")

            result = run(
                "bash",
                str(DEPLOY_SCRIPT),
                "--build-bin",
                str(build_bin),
                "--client-build-bin",
                str(build_bin),
                "--runtime-dir",
                str(build_bin),
            )

            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("runtime directory", result.stdout.lower())


class ReleaseTreeScannerTests(unittest.TestCase):
    def scan(self, tree: Path, *extra: str) -> subprocess.CompletedProcess[str]:
        return run("bash", str(RELEASE_SCANNER), *extra, str(tree))

    def test_clean_tree_and_environment_placeholders_pass(self):
        with tempfile.TemporaryDirectory() as temp_text:
            tree = Path(temp_text)
            (tree / "config.ini").write_text(
                "[Redis]\nPasswd=${MEMOCHAT_REDIS_PASSWD}\n"
                "[Auth]\nJwtSecret=${MEMOCHAT_AUTH_JWTSECRET}\n"
                "Authorization=${MEMOCHAT_AUTHORIZATION}\n"
                "DatabaseUrl=postgresql://${MEMOCHAT_DB_USER}:${MEMOCHAT_DB_PASSWORD}@postgres/memo\n",
                encoding="utf-8",
            )
            result = self.scan(tree)
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn("release tree verified", result.stdout.lower())

    def test_private_keys_literal_credentials_and_runtime_data_fail_closed(self):
        fixtures = {
            "server.key": "-----BEGIN PRIVATE KEY-----\nsecret\n",
            "credentials.json": '{"access_token":"real-token-value"}\n',
            "client.log": "request token=real-token-value\n",
            "state.sqlite": "database bytes\n",
            "database-url.txt": "postgresql://memo:real-password@postgres/memo\n",
            "authorization.txt": "Authorization: Bearer real-access-token\n",
        }
        for filename, content in fixtures.items():
            with self.subTest(filename=filename), tempfile.TemporaryDirectory() as temp_text:
                tree = Path(temp_text)
                (tree / filename).write_text(content, encoding="utf-8")
                result = self.scan(tree)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn("fail", result.stdout.lower())

    def test_developer_paths_missing_dependencies_and_external_symlinks_fail(self):
        fixtures = {
            "paths.txt": "RUNPATH=/data/vcpkg/installed/lib:/root/code/MemoChat\n",
            "ldd.txt": "libmsquic.so.2 => not found\n",
        }
        for filename, content in fixtures.items():
            with self.subTest(filename=filename), tempfile.TemporaryDirectory() as temp_text:
                tree = Path(temp_text)
                (tree / filename).write_text(content, encoding="utf-8")
                result = self.scan(tree)
                self.assertNotEqual(result.returncode, 0, result.stdout)

        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            tree = temp / "release"
            tree.mkdir()
            (temp / "outside.txt").write_text("outside", encoding="utf-8")
            (tree / "escape").symlink_to(temp / "outside.txt")
            result = self.scan(tree)
            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("symlink", result.stdout.lower())

    def test_empty_runtime_directories_and_sensitive_symlink_names_fail(self):
        with tempfile.TemporaryDirectory() as temp_text:
            tree = Path(temp_text) / "release"
            (tree / "logs").mkdir(parents=True)
            result = self.scan(tree)
            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("runtime data directory", result.stdout.lower())

        with tempfile.TemporaryDirectory() as temp_text:
            tree = Path(temp_text) / "release"
            tree.mkdir()
            (tree / "config.ini").write_text("[App]\nName=MemoChat\n", encoding="utf-8")
            (tree / "server.key").symlink_to("config.ini")
            result = self.scan(tree)
            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("sensitive or mutable file type", result.stdout.lower())

    def test_host_library_path_cannot_mask_a_missing_release_dependency(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            tree = temp / "release"
            ambient = temp / "ambient"
            tree.mkdir()
            ambient.mkdir()
            (temp / "probe.c").write_text("int release_probe(void) { return 7; }\n", encoding="utf-8")
            (temp / "main.c").write_text(
                "extern int release_probe(void);\nint main(void) { return release_probe() == 7 ? 0 : 1; }\n",
                encoding="utf-8",
            )
            library = ambient / "libambientreleaseprobe.so"
            compile_library = run(
                "cc",
                "-shared",
                "-fPIC",
                "probe.c",
                "-Wl,-soname,libambientreleaseprobe.so",
                "-o",
                str(library),
                cwd=temp,
            )
            self.assertEqual(compile_library.returncode, 0, compile_library.stdout)
            compile_binary = run(
                "cc",
                "main.c",
                "-L",
                str(ambient),
                "-lambientreleaseprobe",
                "-o",
                str(tree / "release-probe"),
                cwd=temp,
            )
            self.assertEqual(compile_binary.returncode, 0, compile_binary.stdout)

            env = os.environ.copy()
            env["LD_LIBRARY_PATH"] = str(ambient)
            result = run("bash", str(RELEASE_SCANNER), str(tree), env=env)
            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("unresolved runtime dependencies", result.stdout.lower())

    def test_allowlist_is_exact_relative_and_rejects_unsafe_entries(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            tree = temp / "release"
            tree.mkdir()
            (tree / "known-fixture.txt").write_text("token=fixture-only-value", encoding="utf-8")
            allowlist = temp / "allowlist.txt"
            allowlist.write_text("known-fixture.txt\n", encoding="utf-8")

            result = self.scan(tree, "--allow-file", str(allowlist))
            self.assertEqual(result.returncode, 0, result.stdout)

            allowlist.write_text("../known-fixture.txt\n", encoding="utf-8")
            result = self.scan(tree, "--allow-file", str(allowlist))
            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("allowlist", result.stdout.lower())

    def test_public_env_example_requires_an_exact_content_allowlist(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            tree = temp / "release"
            tree.mkdir()
            example = tree / ".env.release.example"
            example.write_text(
                "MEMOCHAT_REDIS_PASSWORD=REPLACE_WITH_GENERATED_REDIS_SECRET\n",
                encoding="utf-8",
            )

            without_allowlist = self.scan(tree)
            self.assertNotEqual(0, without_allowlist.returncode)

            allowlist = temp / "allowlist.txt"
            allowlist.write_text(".env.release.example\n", encoding="utf-8")
            with_allowlist = self.scan(tree, "--allow-file", str(allowlist))
            self.assertEqual(0, with_allowlist.returncode, with_allowlist.stdout)

            (tree / ".env.production").write_text("PUBLIC_FLAG=true\n", encoding="utf-8")
            production_env = self.scan(tree, "--allow-file", str(allowlist))
            self.assertNotEqual(0, production_env.returncode)
            self.assertIn("sensitive or mutable file type", production_env.stdout.lower())

    def test_source_core_directory_and_schema_migration_names_are_not_runtime_state(self):
        with tempfile.TemporaryDirectory() as temp_text:
            tree = Path(temp_text) / "release"
            migration_root = tree / "apps/server/migrations/postgresql/business"
            config_root = tree / "apps/server/core/ChatServer"
            migration_root.mkdir(parents=True)
            config_root.mkdir(parents=True)
            (config_root / "chatserver.ini").write_text("[Server]\nPort=8080\n", encoding="utf-8")
            (migration_root / "010_password_hash.sql").write_text(
                "ALTER TABLE memo.user ADD COLUMN password_hash text;\n",
                encoding="utf-8",
            )
            (migration_root / "011_auth_refresh_tokens.sql").write_text(
                "CREATE TABLE memo.auth_refresh_token (id bigint PRIMARY KEY);\n",
                encoding="utf-8",
            )

            result = self.scan(tree)

            self.assertEqual(0, result.returncode, result.stdout)

    def test_allowlist_cannot_bypass_mandatory_release_guards(self):
        private_key_fixture = (
            "-----BEGIN " + "PRIVATE" + " KEY-----\n"
            "QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVo=\n"
            "-----END " + "PRIVATE" + " KEY-----\n"
        )
        fixtures = {
            "server.key": "token=fixture-only-value\n",
            "private-material.txt": private_key_fixture,
            "developer-path.txt": "RUNPATH=/root/code/MemoChat/build/lib\n",
        }
        for filename, content in fixtures.items():
            with self.subTest(filename=filename), tempfile.TemporaryDirectory() as temp_text:
                tree = Path(temp_text) / "release"
                tree.mkdir()
                (tree / filename).write_text(content, encoding="utf-8")
                allowlist = Path(temp_text) / "allowlist.txt"
                allowlist.write_text(f"{filename}\n", encoding="utf-8")

                result = self.scan(tree, "--allow-file", str(allowlist))
                self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_client_allowlist_is_exact_and_only_exempts_the_qt_password_label(self):
        entries = [
            line.strip()
            for line in CLIENT_SCAN_ALLOWLIST.read_text(encoding="utf-8").splitlines()
            if line.strip() and not line.lstrip().startswith("#")
        ]
        expected = "qml/QtWebEngine/ControlsDelegates/AuthenticationDialog.qml"
        self.assertEqual(entries, [expected])

        with tempfile.TemporaryDirectory() as temp_text:
            tree = Path(temp_text)
            dialog = tree / expected
            dialog.parent.mkdir(parents=True)
            dialog.write_text('text: qsTr("Password:")\n', encoding="utf-8")
            result = self.scan(tree, "--allow-file", str(CLIENT_SCAN_ALLOWLIST))
            self.assertEqual(result.returncode, 0, result.stdout)

    def test_high_confidence_secrets_are_rejected_in_opaque_binary_files(self):
        private_key_fixture = (
            b"\x00-----BEGIN "
            + b"PRIVATE"
            + b" KEY-----\n"
            + (b"QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVo=\n" * 3)
            + b"-----END "
            + b"PRIVATE"
            + b" KEY-----\n"
        )
        fixtures = {
            "github.bin": b"\x00\x01ghp_" + (b"A" * 40),
            "github-fine-grained.bin": b"\x00github_pat_" + (b"D" * 60),
            "openai.bin": b"\x00sk-proj-" + (b"B" * 40),
            "aws.bin": b"\x00AKIA" + (b"C" * 16),
            "private-key.bin": private_key_fixture,
        }
        for filename, content in fixtures.items():
            with self.subTest(filename=filename), tempfile.TemporaryDirectory() as temp_text:
                tree = Path(temp_text)
                (tree / filename).write_bytes(content)
                result = self.scan(tree)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertNotIn("ghp_", result.stdout)
                self.assertNotIn("sk-proj-", result.stdout)

        with tempfile.TemporaryDirectory() as temp_text:
            tree = Path(temp_text)
            (tree / "format-strings.bin").write_bytes(b"\x00token=%s\x00password=%s\x00")
            result = self.scan(tree)
            self.assertEqual(result.returncode, 0, result.stdout)

        with tempfile.TemporaryDirectory() as temp_text:
            tree = Path(temp_text)
            (tree / "marker-support.c").write_text(
                'volatile const char *markers[] = {"-----BEGIN " "PRIVATE" " KEY-----", '
                '"-----END " "PRIVATE" " KEY-----"};\n'
                "int main(void) { return markers[0][0] == '-' && markers[1][0] == '-' ? 0 : 1; }\n",
                encoding="utf-8",
            )
            compile_marker_binary = run(
                "cc",
                "marker-support.c",
                "-o",
                "marker-support",
                cwd=tree,
            )
            self.assertEqual(compile_marker_binary.returncode, 0, compile_marker_binary.stdout)
            marker_binary = (tree / "marker-support").read_bytes()
            self.assertIn(b"-----BEGIN " + b"PRIVATE" + b" KEY-----", marker_binary)
            self.assertIn(b"-----END " + b"PRIVATE" + b" KEY-----", marker_binary)
            (tree / "marker-support.c").unlink()

            result = self.scan(tree)
            self.assertEqual(result.returncode, 0, result.stdout)


class BuildEnvironmentLoaderTests(unittest.TestCase):
    def test_loader_keeps_toolchain_and_removes_runtime_secrets(self):
        with tempfile.TemporaryDirectory() as temp_text:
            env_file = Path(temp_text) / "build.env"
            env_file.write_text(
                "export VCPKG_ROOT=/opt/test-vcpkg\n"
                "export VCPKG_MAX_CONCURRENCY=99\n"
                "export CMAKE_PREFIX_PATH=/opt/test-qt\n"
                "export CMAKE_BUILD_PARALLEL_LEVEL=99\n"
                "export Qt6_DIR=/opt/test-qt/lib/cmake/Qt6\n"
                "export MEMOCHAT_AUTH_JWTSECRET=fixture-secret\n"
                "export PGPASSWORD=fixture-password\n"
                "export AWS_SECRET_ACCESS_KEY=fixture-aws-secret\n"
                "export SMTP_PASSWORD=fixture-smtp-password\n"
                "export UNCLASSIFIED_RUNTIME_VALUE=fixture-unclassified\n",
                encoding="utf-8",
            )
            env_file.chmod(stat.S_IRUSR | stat.S_IWUSR)

            command = """
                set -Eeuo pipefail
                set -x
                source "$1" "$2"
                [[ "$VCPKG_ROOT" == /opt/test-vcpkg ]]
                [[ -z "${VCPKG_MAX_CONCURRENCY+x}" ]]
                [[ "$CMAKE_PREFIX_PATH" == /opt/test-qt ]]
                [[ -z "${CMAKE_BUILD_PARALLEL_LEVEL+x}" ]]
                [[ "$Qt6_DIR" == /opt/test-qt/lib/cmake/Qt6 ]]
                [[ -z "${MEMOCHAT_AUTH_JWTSECRET+x}" ]]
                [[ -z "${PGPASSWORD+x}" ]]
                [[ -z "${AWS_SECRET_ACCESS_KEY+x}" ]]
                [[ -z "${SMTP_PASSWORD+x}" ]]
                [[ -z "${UNCLASSIFIED_RUNTIME_VALUE+x}" ]]
            """
            result = run("bash", "-c", command, "loader-test", str(BUILD_ENV_LOADER), str(env_file))
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertNotIn("fixture-", result.stdout)

    def test_loader_rejects_a_build_environment_file_that_is_not_mode_0600(self):
        with tempfile.TemporaryDirectory() as temp_text:
            env_file = Path(temp_text) / "build.env"
            env_file.write_text("export VCPKG_ROOT=/opt/test-vcpkg\n", encoding="utf-8")
            env_file.chmod(stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP)

            result = run(
                "bash",
                "-c",
                'source "$1" "$2"',
                "loader-test",
                str(BUILD_ENV_LOADER),
                str(env_file),
            )
            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("0600", result.stdout)

    def test_loader_rejects_commands_appended_to_an_assignment(self):
        with tempfile.TemporaryDirectory() as temp_text:
            temp = Path(temp_text)
            env_file = temp / "build.env"
            marker = temp / "unexpected-command"
            env_file.write_text(
                f"export VCPKG_ROOT=/opt/test-vcpkg; touch {marker}\n",
                encoding="utf-8",
            )
            env_file.chmod(stat.S_IRUSR | stat.S_IWUSR)

            result = run(
                "bash",
                "-c",
                'source "$1" "$2"',
                "loader-test",
                str(BUILD_ENV_LOADER),
                str(env_file),
            )
            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertFalse(marker.exists(), result.stdout)


class ReleaseAssetManifestBindingTests(unittest.TestCase):
    SOURCE_SHA = "a" * 40
    VERSION_TAG = "v3.0.0"

    @staticmethod
    def write_archive_with_checksum(path: Path, content: bytes) -> str:
        path.write_bytes(content)
        digest = hashlib.sha256(content).hexdigest()
        path.with_name(f"{path.name}.sha256").write_text(
            f"{digest}  {path.name}\n",
            encoding="utf-8",
        )
        return digest

    def prepare_release_assets(self, root: Path) -> tuple[Path, Path]:
        client_dir = root / "release-assets/client"
        backend_dir = root / "release-assets/backend"
        audit_dir = root / "release-assets/audit"
        metadata_dir = root / "release-assets/metadata"
        client_dir.mkdir(parents=True)
        backend_dir.mkdir(parents=True)
        audit_dir.mkdir(parents=True)
        metadata_dir.mkdir(parents=True)

        client_name = f"MemoChatQml-{self.SOURCE_SHA[:12]}-linux-x86_64.tar.gz"
        backend_name = f"MemoChat-backend-{self.SOURCE_SHA[:12]}-linux-x86_64.tar.gz"
        audit_name = f"backend-image-audit-{self.SOURCE_SHA[:12]}.tar.gz"
        client_path = client_dir / client_name
        backend_path = backend_dir / backend_name
        audit_path = audit_dir / audit_name
        client_sha256 = self.write_archive_with_checksum(client_path, b"client archive\n")
        backend_sha256 = self.write_archive_with_checksum(backend_path, b"backend archive\n")
        audit_sha256 = self.write_archive_with_checksum(audit_path, b"image audit evidence\n")
        manifest = {
            "schema": "memochat-release-v1",
            "source_sha": self.SOURCE_SHA,
            "client": {"artifact": client_name, "sha256": client_sha256},
            "backend": {"artifact": backend_name, "sha256": backend_sha256},
            "legal": {
                "release_source_sha": self.SOURCE_SHA,
                "formal_distribution_ready": True,
                "third_party_legal_corpus": "complete",
                "corpus_review_id": "memo-legal-review-2026-07-28",
                "corpus_sha256": hashlib.sha256(b"legal corpus\n").hexdigest(),
                "status_sha256": hashlib.sha256(b"legal status\n").hexdigest(),
            },
            "backend_image_audit": {
                "schema": "memochat-backend-image-audit-artifact-v1",
                "source_sha": self.SOURCE_SHA,
                "artifact": audit_name,
                "sha256": audit_sha256,
                "image_manifest_sha256": hashlib.sha256(b"backend-images.json").hexdigest(),
            },
            "images": [
                {
                    "version_tag": self.VERSION_TAG,
                    "bundle_sha256": f"{index:064x}",
                    "vcpkg_sbom_sha256": f"{index + 16:064x}",
                    "legal_status_sha256": hashlib.sha256(b"legal status\n").hexdigest(),
                }
                for index in range(15)
            ],
        }
        (metadata_dir / "manifest.json").write_text(
            json.dumps(manifest),
            encoding="utf-8",
        )
        return client_path, backend_path

    def run_release_asset_verifier(self, root: Path) -> subprocess.CompletedProcess[str]:
        env = os.environ.copy()
        env.update({"GITHUB_SHA": self.SOURCE_SHA, "GITHUB_REF_NAME": self.VERSION_TAG})
        return run(
            "bash",
            "-c",
            workflow_step_script(CI_WORKFLOW, "publish-github-release", "Verify release assets"),
            cwd=root,
            env=env,
        )

    def test_rejects_a_client_archive_and_companion_checksum_that_drift_from_the_manifest(self):
        with tempfile.TemporaryDirectory() as temp_text:
            root = Path(temp_text)
            client_path, _ = self.prepare_release_assets(root)
            baseline = self.run_release_asset_verifier(root)
            self.assertEqual(0, baseline.returncode, baseline.stdout)

            self.write_archive_with_checksum(client_path, b"different but self-consistent client archive\n")
            result = self.run_release_asset_verifier(root)

            self.assertNotEqual(0, result.returncode, result.stdout)
            self.assertIn("client archive digest mismatch", result.stdout)

    def test_rejects_a_missing_manifest_client_even_when_an_unexpected_archive_is_self_consistent(self):
        with tempfile.TemporaryDirectory() as temp_text:
            root = Path(temp_text)
            client_path, _ = self.prepare_release_assets(root)
            baseline = self.run_release_asset_verifier(root)
            self.assertEqual(0, baseline.returncode, baseline.stdout)

            client_path.unlink()
            client_path.with_name(f"{client_path.name}.sha256").unlink()
            self.write_archive_with_checksum(
                client_path.with_name("unexpected-client.tar.gz"),
                b"unexpected client archive\n",
            )
            result = self.run_release_asset_verifier(root)

            self.assertNotEqual(0, result.returncode, result.stdout)
            self.assertIn("release manifest client archive is missing", result.stdout)

    def test_rejects_an_image_audit_archive_and_checksum_that_drift_from_the_manifest(self):
        with tempfile.TemporaryDirectory() as temp_text:
            root = Path(temp_text)
            self.prepare_release_assets(root)
            baseline = self.run_release_asset_verifier(root)
            self.assertEqual(0, baseline.returncode, baseline.stdout)
            audit_path = root / "release-assets/audit" / f"backend-image-audit-{self.SOURCE_SHA[:12]}.tar.gz"

            self.write_archive_with_checksum(audit_path, b"different but self-consistent image audit\n")
            result = self.run_release_asset_verifier(root)

            self.assertNotEqual(0, result.returncode, result.stdout)
            self.assertIn("backend image audit digest mismatch", result.stdout)


class SourceAndWorkflowSecurityContractTests(unittest.TestCase):
    def test_etcd_change_logging_never_streams_raw_values(self):
        source = INI_CONFIG.read_text(encoding="utf-8")
        function = source[source.index("void IniConfig::OnEtcdConfigChange") :]

        self.assertNotRegex(function, r"(?:<<\s*value|value\s*<<)")
        self.assertIn("value_redacted=true", function)
        self.assertRegex(function, r"(?i)(password|passwd|secret|token|cookie|credential|private.?key)")

    def test_historical_verify_logs_are_not_tracked(self):
        for relative_path in TRACKED_VERIFY_LOGS:
            self.assertFalse((REPO_ROOT / relative_path).exists(), relative_path)

    def test_release_sbom_generator_is_an_executable_regular_file(self):
        mode = VCPKG_SBOM_GENERATOR.stat().st_mode
        self.assertTrue(stat.S_ISREG(mode))
        self.assertTrue(mode & stat.S_IXUSR)

    def test_workflows_are_linux_current_and_have_a_real_artifact_handoff(self):
        ci = CI_WORKFLOW.read_text(encoding="utf-8")
        yaml.safe_load(ci)

        combined = ci
        for obsolete in ("windows-latest", "GateServer.exe", "ChatServer.exe", "docker-images"):
            self.assertNotIn(obsolete, combined)
        self.assertNotRegex(combined, r"(?<![A-Za-z])GateServer(?![A-Za-z])")

        for token in (
            "linux-client-release-gcc16",
            "linux-server-release-gcc16",
            "verify_release_tree.sh",
            "client_release_scan.allowlist",
            "actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02",
            "ghcr.io",
            "attest-build-provenance",
        ):
            self.assertIn(token, ci)

        self.assertIn("release-metadata", ci)
        self.assertFalse((REPO_ROOT / ".github/workflows/cd.yml").exists())
        self.assertNotIn("stable", ci)
        self.assertNotIn("newTag: latest", ci)
        self.assertNotIn('sha256sum "$archive" >', ci)
        self.assertIn('sha256sum "$(basename -- "$archive")"', ci)
        self.assertNotIn("git grep -nI", ci)
        self.assertIn("git grep -Il", ci)
        self.assertIn("github_pat_", ci)
        self.assertNotIn("test -x /data/vcpkg/vcpkg/vcpkg", ci)
        self.assertIn('test -x "$VCPKG_ROOT/vcpkg"', ci)
        self.assertNotIn("source /root/.memochat-linux-env", ci)
        self.assertIn("source tools/scripts/release/load_build_environment.sh", ci)
        self.assertRegex(ci, r"(?m)^permissions:\n\s+contents: read$")

    def test_release_workflow_rejects_duplicate_yaml_mapping_keys(self):
        yaml.load(CI_WORKFLOW.read_text(encoding="utf-8"), Loader=UniqueKeyLoader)

    def test_all_remote_actions_are_pinned_to_verified_full_commit_shas(self):
        workflows = CI_WORKFLOW.read_text(encoding="utf-8")
        uses_lines = re.findall(r"(?m)^\s*uses:\s*([^\s#]+)(?:\s+#\s*(\S+))?\s*$", workflows)

        self.assertTrue(uses_lines)
        seen_repositories = set()
        for action, version_comment in uses_lines:
            with self.subTest(action=action):
                repository, separator, revision = action.rpartition("@")
                self.assertEqual(separator, "@")
                self.assertIn(repository, ACTION_PINS)
                expected_revision, expected_version = ACTION_PINS[repository]
                self.assertRegex(revision, r"^[0-9a-f]{40}$")
                self.assertEqual(revision, expected_revision)
                self.assertEqual(version_comment, expected_version)
                seen_repositories.add(repository)

        self.assertEqual(seen_repositories, set(ACTION_PINS))

    def test_version_releases_require_the_verified_distribution_legal_corpus(self):
        ci = CI_WORKFLOW.read_text(encoding="utf-8")
        release_contract_job = workflow_job_body(ci, "release-contracts")

        self.assertIn("Inspect distribution legal status", release_contract_job)
        self.assertIn('--status-file "$status_file"', release_contract_job)
        self.assertIn('>> "$GITHUB_STEP_SUMMARY"', release_contract_job)
        self.assertIn("Require complete distribution legal corpus", release_contract_job)
        self.assertIn("if: startsWith(github.ref, 'refs/tags/v')", release_contract_job)
        self.assertIn("--require-distribution-corpus", release_contract_job)
        self.assertIn('--source-sha "$GITHUB_SHA"', release_contract_job)
        self.assertNotIn("MEMOCHAT_LEGAL_APPROVAL", release_contract_job)
        self.assertNotIn("--approval-public-key", release_contract_job)
        self.assertNotIn("--approval-signature", release_contract_job)
        self.assertIn("bash -n tools/scripts/release/verify_release_legal.sh", release_contract_job)
        self.assertNotIn("Verify release legal notices", release_contract_job)

    def test_version_tags_require_main_provenance_and_full_history_secret_scan(self):
        ci = CI_WORKFLOW.read_text(encoding="utf-8")
        secret_scan_job = workflow_job_body(ci, "secret-scan")
        release_contract_job = workflow_job_body(ci, "release-contracts")

        self.assertIn("Scan the current source tree with Gitleaks", secret_scan_job)
        self.assertIn("gitleaks_${version}_linux_x64.tar.gz", secret_scan_job)
        self.assertIn('gitleaks" dir', secret_scan_job)
        self.assertIn("--config .gitleaks.toml", secret_scan_job)
        self.assertIn("Scan every commit introduced by this event with Gitleaks", secret_scan_job)
        self.assertIn('commit_range="${PULL_REQUEST_BASE_SHA}..${GITHUB_SHA}"', secret_scan_job)
        self.assertIn('--log-opts="$commit_range"', secret_scan_job)
        self.assertIn("--redact", secret_scan_job)
        self.assertIn("fetch-depth: 0", release_contract_job)
        self.assertIn("Release tags must be annotated", release_contract_job)
        self.assertIn(
            "^v(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)$",
            release_contract_job,
        )
        self.assertIn('git merge-base --is-ancestor "$GITHUB_SHA" origin/main', release_contract_job)
        self.assertIn("git log --all --branches --tags --name-only", release_contract_job)
        self.assertIn('comm -23 "${RUNNER_TEMP}/history-paths.txt"', release_contract_job)
        self.assertNotIn("+refs/pull/*/head:refs/remotes/origin/pull/*", release_contract_job)
        self.assertIn("Rewrite all refs and rotate affected credentials", release_contract_job)
        self.assertIn("gitleaks_${version}_linux_x64.tar.gz", release_contract_job)
        self.assertIn('gitleaks" git', release_contract_job)
        self.assertIn("--config .gitleaks.toml", release_contract_job)
        self.assertIn('--log-opts="--all"', release_contract_job)
        self.assertIn("--redact", release_contract_job)

    def test_private_self_hosted_builds_run_only_for_trusted_pushes(self):
        ci = CI_WORKFLOW.read_text(encoding="utf-8")
        for job_name in ("build-linux-client", "build-linux-backend"):
            with self.subTest(job=job_name):
                job = workflow_job_body(ci, job_name)
                self.assertIn("if: github.event_name == 'push'", job)
                self.assertNotIn("pull_request", job)
                self.assertIn("-validation'", job)
                self.assertIn("-release'", job)

    def test_registry_and_release_writes_run_only_for_version_tags_after_strict_legal(self):
        ci = CI_WORKFLOW.read_text(encoding="utf-8")
        tag_condition = "github.event_name == 'push' && startsWith(github.ref, 'refs/tags/v')"
        for job_name in (
            "publish-backend-images",
            "audit-backend-images",
            "attest-release-artifacts",
            "release-metadata",
            "publish-version-image-tags",
            "publish-github-release",
        ):
            with self.subTest(job=job_name):
                self.assertIn(tag_condition, workflow_job_body(ci, job_name))

        publish_job = workflow_job_body(ci, "publish-backend-images")
        self.assertIn("needs: [release-contracts, build-linux-backend]", publish_job)
        self.assertFalse((REPO_ROOT / ".github/workflows/cd.yml").exists())
        self.assertNotIn("stable", ci)

    def test_packagers_and_dependency_sboms_are_explicitly_source_bound(self):
        ci = CI_WORKFLOW.read_text(encoding="utf-8")
        client_job = workflow_job_body(ci, "build-linux-client")
        backend_job = workflow_job_body(ci, "build-linux-backend")
        publish_job = workflow_job_body(ci, "publish-backend-images")
        metadata_job = workflow_job_body(ci, "release-metadata")

        self.assertIn("package_linux_client.sh", client_job)
        self.assertIn('--source-sha "$GITHUB_SHA"', client_job)
        self.assertNotIn("external legal approval", client_job.lower())
        self.assertNotIn("MEMOCHAT_LEGAL_APPROVAL", client_job)
        self.assertNotIn("--approval-public-key", client_job)
        self.assertNotIn("--approval-signature", client_job)
        self.assertIn("package_backend_services.sh", backend_job)
        self.assertIn("package_backend_deployment_kit.sh", backend_job)
        self.assertGreaterEqual(backend_job.count('--source-sha "$GITHUB_SHA"'), 2)
        self.assertNotIn("external legal approval", backend_job.lower())
        self.assertNotIn("MEMOCHAT_LEGAL_APPROVAL", backend_job)
        self.assertNotIn("--approval-public-key", backend_job)
        self.assertNotIn("--approval-signature", backend_job)
        self.assertIn('--vcpkg-installed-root "$VCPKG_ROOT/installed-memochat-gcc16-server-release"', backend_job)
        self.assertIn("--vcpkg-triplet x64-linux-memochat-release", backend_job)
        self.assertIn("'backend-vcpkg-sboms' || 'backend-vcpkg-validation-sboms'", backend_job)

        for binding in (
            "io.memochat.vcpkg.sbom.sha256",
            "io.memochat.legal.status.sha256",
            '"vcpkg_sbom_sha256": vcpkg_sbom_sha256',
            '"legal_status_sha256": image_legal_status_sha256',
            '"formal_distribution_ready": True',
        ):
            self.assertIn(binding, publish_job)
        self.assertIn('"legal": backend_legal', metadata_job)
        self.assertIn('image.get("vcpkg_sbom_sha256"', metadata_job)

    def test_ci_images_cover_all_bundles_while_compose_excludes_incomplete_ai(self):
        ci = CI_WORKFLOW.read_text(encoding="utf-8")
        compose = BACKEND_COMPOSE.read_text(encoding="utf-8")
        ci_mapping = dict(re.findall(r"^\s*\[([A-Za-z0-9]+)\]=([a-z0-9-]+)\s*$", ci, flags=re.M))
        compose_mapping = {
            target: slug
            for slug, target in re.findall(
                r"image:\s+\$\{MEMOCHAT_CPP_IMAGE_PREFIX:-memochat\}/([a-z0-9-]+):[^\n]+"
                r"(?:(?!\n\s{4}image:).)*?\n\s+MEMOCHAT_SERVICE:\s+([A-Za-z0-9]+)",
                compose,
                flags=re.S,
            )
        }

        self.assertEqual(len(ci_mapping), 15)
        self.assertEqual(
            compose_mapping,
            {target: slug for target, slug in ci_mapping.items() if target not in {"AIGatewayServer", "AIServer"}},
        )
        self.assertNotIn("ai", re.findall(r"^\s+-\s+([a-z0-9-]+)\s*$", compose, flags=re.M))
        self.assertNotIn("${target,,}", ci)
        for target, slug in ci_mapping.items():
            self.assertIn(target, ci)
            self.assertIn(slug, ci)

    def test_version_tags_publish_archives_and_alias_existing_image_digests(self):
        ci = CI_WORKFLOW.read_text(encoding="utf-8")
        release_preflight_job = workflow_job_body(ci, "release-version-preflight")
        image_alias_job = workflow_job_body(ci, "publish-version-image-tags")
        github_release_job = workflow_job_body(ci, "publish-github-release")

        self.assertIn("startsWith(github.ref, 'refs/tags/v')", release_preflight_job)
        self.assertIn("gh api --include", release_preflight_job)
        self.assertIn('"/repos/${GH_REPO}/releases/tags/${GITHUB_REF_NAME}"', release_preflight_job)
        self.assertIn("Release already exists; versioned assets are immutable", release_preflight_job)
        self.assertIn("Unable to prove that the GitHub Release is absent", release_preflight_job)

        self.assertIn("startsWith(github.ref, 'refs/tags/v')", image_alias_job)
        self.assertIn("release-version-preflight", image_alias_job)
        self.assertIn("packages: write", image_alias_job)
        self.assertIn("docker buildx imagetools create", image_alias_job)
        self.assertIn("docker buildx imagetools inspect", image_alias_job)
        self.assertIn('"${image}@${digest}"', image_alias_job)
        self.assertIn('"${image}:${version_tag}"', image_alias_job)
        self.assertIn("Version image alias digest mismatch", image_alias_job)
        self.assertIn('elif [[ "$inspect_status" -eq 1 ]]', image_alias_job)
        self.assertIn('done < "$alias_plan"', image_alias_job)
        self.assertNotIn("alias_mode", image_alias_job)
        self.assertLess(
            image_alias_job.index("docker buildx imagetools inspect"),
            image_alias_job.index("docker buildx imagetools create"),
        )
        self.assertNotIn("docker buildx build ", image_alias_job)

        self.assertIn("startsWith(github.ref, 'refs/tags/v')", github_release_job)
        self.assertIn("contents: write", github_release_job)
        for dependency in (
            "build-linux-client",
            "build-linux-backend",
            "release-metadata",
            "attest-release-artifacts",
        ):
            self.assertIn(dependency, github_release_job)
        for artifact in ("linux-client-release", "linux-backend-release", "release-metadata"):
            self.assertIn(artifact, github_release_job)
        self.assertIn('gh release create "$GITHUB_REF_NAME" --verify-tag --generate-notes', github_release_job)
        self.assertIn("Release already exists; versioned assets are immutable", github_release_job)
        self.assertNotIn("--clobber", github_release_job)
        self.assertIn('release_image["version_tag"] = version_tag', ci)

    def test_commit_image_tags_are_inspected_before_any_push_and_never_overwritten(self):
        ci = CI_WORKFLOW.read_text(encoding="utf-8")
        publish_job = workflow_job_body(ci, "publish-backend-images")

        self.assertIn("group: publish-images-${{ github.sha }}", publish_job)
        self.assertIn("cancel-in-progress: false", publish_job)
        self.assertIn("docker buildx imagetools inspect", publish_job)
        self.assertIn('elif [[ "$inspect_status" -eq 1 ]]', publish_job)
        self.assertIn("verify_bundle_binding", publish_job)
        self.assertIn("Commit image digest set is incomplete", publish_job)
        self.assertNotIn("publish_mode", publish_job)
        self.assertIn("Commit image tag changed during publication", publish_job)
        self.assertIn("org.opencontainers.image.revision", publish_job)
        self.assertIn("io.memochat.service.target", publish_job)
        self.assertIn("io.memochat.bundle.sha256", publish_job)
        self.assertIn("io.memochat.vcpkg.sbom.sha256", publish_job)
        self.assertIn("io.memochat.legal.status.sha256", publish_job)
        self.assertIn("Commit image bundle binding mismatch", publish_job)
        self.assertLess(
            publish_job.index("docker buildx imagetools inspect"),
            publish_job.index("docker buildx build"),
        )

    def test_release_manifest_binds_the_backend_archive_and_each_service_bundle(self):
        ci = CI_WORKFLOW.read_text(encoding="utf-8")
        publish_job = workflow_job_body(ci, "publish-backend-images")
        metadata_job = workflow_job_body(ci, "release-metadata")
        github_release_job = workflow_job_body(ci, "publish-github-release")

        self.assertIn("BACKEND_ARCHIVE_SHA256", publish_job)
        self.assertIn('"archive": {', publish_job)
        self.assertIn('"bundle_sha256": bundle_sha256', publish_job)
        self.assertIn('"backend": backend_archive', metadata_job)
        self.assertIn("expected_client_artifact", metadata_job)
        self.assertIn('image["bundle_sha256"]', metadata_job)
        self.assertIn("hashlib.sha256", github_release_job)
        self.assertIn('manifest.get("client", {})', github_release_job)
        self.assertIn("client_hasher", github_release_job)
        self.assertIn('manifest.get("backend", {})', github_release_job)

    def test_backend_images_are_audited_by_digest_before_release_aliases(self):
        ci = CI_WORKFLOW.read_text(encoding="utf-8")
        release_contract_job = workflow_job_body(ci, "release-contracts")
        audit_job = workflow_job_body(ci, "audit-backend-images")
        metadata_job = workflow_job_body(ci, "release-metadata")
        image_alias_job = workflow_job_body(ci, "publish-version-image-tags")

        self.assertIn("bash -n tools/scripts/release/audit_backend_images.sh", release_contract_job)
        self.assertIn("needs: [publish-backend-images]", audit_job)
        self.assertIn("packages: read", audit_job)
        self.assertIn("syft_version=1.44.0", audit_job)
        self.assertIn("grype_version=0.112.0", audit_job)
        self.assertIn(
            "0e91737aee2b5baf1d255b959630194a302335d848ff97bb07921eb6205b5f5a",
            audit_job,
        )
        self.assertIn(
            "acb14a030010fe9bdb9594b4ae108d9d14ef2f926d936aa0916dc62c89c058ea",
            audit_job,
        )
        self.assertIn('"$grype_bin" db update', audit_job)
        self.assertIn('grype_config="${RUNNER_TEMP}/memochat-grype-config.yaml"', audit_job)
        self.assertIn('--config "$grype_config"', audit_job)
        self.assertNotIn("--config /dev/null", audit_job)
        self.assertIn('GRYPE_DB_AUTO_UPDATE: "false"', audit_job)
        self.assertIn("tools/scripts/release/audit_backend_images.sh", audit_job)
        self.assertIn("--image-manifest metadata/backend-images.json", audit_job)
        self.assertIn("--pull", audit_job)
        self.assertIn("--fail-on high", audit_job)
        self.assertIn("--vcpkg-sbom-dir metadata/vcpkg", audit_job)
        self.assertNotIn("--fix-policy", audit_job)
        self.assertNotIn("--only-fixed", audit_job)
        self.assertIn("tar --sort=name --mtime='UTC 1970-01-01'", audit_job)
        self.assertIn("backend-image-audit-${GITHUB_SHA:0:12}.tar.gz", audit_job)
        self.assertIn("name: backend-image-audit", audit_job)
        self.assertIn("if: always()", audit_job)
        self.assertIn("audit-backend-images", metadata_job)
        self.assertIn("backend-image-audit", metadata_job)
        self.assertIn("release-metadata", image_alias_job)

    def test_release_manifest_and_github_release_bind_image_audit_evidence(self):
        ci = CI_WORKFLOW.read_text(encoding="utf-8")
        metadata_job = workflow_job_body(ci, "release-metadata")
        github_release_job = workflow_job_body(ci, "publish-github-release")

        self.assertIn('audit = json.loads(Path("metadata/audit/audit.json")', metadata_job)
        self.assertIn('"backend_image_audit": audit', metadata_job)
        self.assertIn("expected_audit_artifact", metadata_job)
        self.assertIn("audit-backend-images", github_release_job)
        self.assertIn("name: backend-image-audit", github_release_job)
        self.assertIn('manifest.get("backend_image_audit", {})', github_release_job)
        self.assertIn("audit_hasher = hashlib.sha256()", github_release_job)
        self.assertIn('[[ "${#assets[@]}" -eq 7 ]]', github_release_job)

    def test_backend_image_builds_use_only_explicit_named_contexts(self):
        ci = CI_WORKFLOW.read_text(encoding="utf-8")
        build_job = workflow_job_body(ci, "build-linux-backend")
        publish_job = workflow_job_body(ci, "publish-backend-images")

        for job in (build_job, publish_job):
            with self.subTest(job="build" if job is build_job else "publish"):
                self.assertIn("memochat-empty-docker-context", job)
                self.assertIn('--build-context "service_bundle=', job)
                self.assertIn(
                    '--build-context "server_entrypoint=$GITHUB_WORKSPACE/infra/deploy/images/common/entrypoints"',
                    job,
                )
                self.assertNotRegex(job, r"(?m)^\s+--(?:load|push) \.$")


if __name__ == "__main__":
    unittest.main()
