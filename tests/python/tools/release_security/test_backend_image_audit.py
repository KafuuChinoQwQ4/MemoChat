import hashlib
import json
import os
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
AUDITOR = REPO_ROOT / "tools/scripts/release/audit_backend_images.sh"
DOCKERFILE = REPO_ROOT / "infra/deploy/images/services/cpp-service.Dockerfile"
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
SOURCE_SHA = "0123456789abcdef0123456789abcdef01234567"
VCPKG_SBOM_BYTES = (
    json.dumps(
        {
            "spdxVersion": "SPDX-2.3",
            "documentComment": (f"release_source_sha={SOURCE_SHA}; coverage=installed-closure-overapproximation"),
            "packages": [{"name": "openssl", "versionInfo": "3.6.0"}],
        },
        separators=(",", ":"),
    )
    + "\n"
).encode()
VCPKG_SBOM_SHA256 = hashlib.sha256(VCPKG_SBOM_BYTES).hexdigest()
LEGAL_STATUS_SHA256 = hashlib.sha256(b"formal legal status\n").hexdigest()
LEGAL_CORPUS_SHA256 = hashlib.sha256(b"approved corpus checksums\n").hexdigest()
LEGAL_REVIEW_ID = "memo-legal-review-2026-07-28"
UBUNTU_PROVENANCE_LABELS = {
    "io.memochat.ubuntu.snapshot": "20260727T000000Z",
    "io.memochat.ubuntu.ca-bootstrap.sha256": ("6bac2a01979e210d9eac1d4d56747ec709ea60654744d66705dc3c36e7629e50"),
    "io.memochat.ubuntu.runtime-packages": (
        "ca-certificates=20260601~24.04.1,libturbojpeg=1:2.1.5-2ubuntu2,libwebp7=1.3.2-0.4build3"
    ),
}


def target_digest(target: str) -> str:
    return f"sha256:{hashlib.sha256(target.encode('utf-8')).hexdigest()}"


def bundle_digest(target: str) -> str:
    return hashlib.sha256(f"bundle:{target}".encode()).hexdigest()


def hardened_config(target: str = "ChatServer", **overrides: object) -> dict[str, object]:
    config: dict[str, object] = {
        "User": "10001:10001",
        "WorkingDir": "/run/memochat",
        "Entrypoint": ["/app/entrypoint.sh"],
        "Healthcheck": {
            "Test": ["CMD", "/app/entrypoint.sh", "--healthcheck"],
            "Interval": 20_000_000_000,
            "Timeout": 3_000_000_000,
            "StartPeriod": 10_000_000_000,
            "Retries": 3,
        },
        "Env": [
            "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
            f"MEMOCHAT_SERVICE={target}",
            "MEMOCHAT_RELEASE_MODE=1",
            "MEMOCHAT_ALLOW_DEV_SECRETS=0",
            "CONFIG_PATH=/run/memochat/config.ini",
        ],
        "Labels": {
            "org.opencontainers.image.revision": SOURCE_SHA,
            "io.memochat.service.target": target,
            "io.memochat.bundle.sha256": bundle_digest(target),
            "io.memochat.vcpkg.sbom.sha256": VCPKG_SBOM_SHA256,
            "io.memochat.legal.status.sha256": LEGAL_STATUS_SHA256,
            **UBUNTU_PROVENANCE_LABELS,
        },
    }
    config.update(overrides)
    return config


class BackendImageAuditTests(unittest.TestCase):
    def test_ubuntu_runtime_provenance_contract_matches_the_runtime_dockerfile(self):
        source = AUDITOR.read_text(encoding="utf-8")
        dockerfile = DOCKERFILE.read_text(encoding="utf-8")

        for label, expected_value in UBUNTU_PROVENANCE_LABELS.items():
            with self.subTest(label=label):
                if label == "io.memochat.ubuntu.snapshot":
                    self.assertIn(f"ARG UBUNTU_SNAPSHOT={expected_value}", dockerfile)
                    self.assertIn(f'{label}="${{UBUNTU_SNAPSHOT}}"', dockerfile)
                else:
                    self.assertIn(f'{label}="{expected_value}"', dockerfile)
                self.assertIn(label, source)
                self.assertIn(expected_value, source)

    def test_disables_tool_and_database_auto_updates_while_recording_database_status(self):
        source = AUDITOR.read_text(encoding="utf-8")

        self.assertIn("SYFT_CHECK_FOR_APP_UPDATE=false", source)
        self.assertIn("GRYPE_CHECK_FOR_APP_UPDATE=false", source)
        self.assertIn("GRYPE_DB_AUTO_UPDATE=false", source)
        self.assertIn('SYFT_CONFIG_PATH="${OUTPUT_DIR}/SYFT_CONFIG.yaml"', source)
        self.assertIn('GRYPE_CONFIG_PATH="${OUTPUT_DIR}/GRYPE_CONFIG.yaml"', source)
        self.assertNotIn("--config /dev/null", source)
        self.assertNotIn('"$GRYPE_BIN" db status -o json --config /dev/null', source)
        self.assertIn('"$GRYPE_BIN" db status -o json', source)
        self.assertNotIn('"$GRYPE_BIN" db update', source)

    def write_executable(self, path: Path, source: str) -> None:
        path.write_text(source, encoding="utf-8")
        path.chmod(path.stat().st_mode | stat.S_IXUSR)

    def create_harness(self, root: Path, config: dict[str, object]) -> tuple[dict[str, str], dict[str, Path]]:
        config_path = root / "docker-config.json"
        config_path.write_text(json.dumps(config), encoding="utf-8")
        paths = {
            "docker": root / "docker",
            "syft": root / "syft",
            "grype": root / "grype",
            "docker_log": root / "docker.log",
            "docker_pull_log": root / "docker-pull.log",
            "syft_log": root / "syft.log",
            "grype_log": root / "grype.log",
        }
        self.write_executable(
            paths["docker"],
            """#!/usr/bin/env bash
set -Eeuo pipefail
if [[ "${1:-}" == "pull" && $# -eq 2 ]]; then
    printf '%s\n' "$2" >> "${MOCK_DOCKER_PULL_LOG:?}"
    [[ "${MOCK_DOCKER_PULL_MODE:-ok}" == "ok" ]] || exit 41
    exit 0
fi
if [[ "${1:-}" == "image" && "${2:-}" == "inspect" ]]; then
    reference="${*: -1}"
    printf '%s\n' "$reference" >> "${MOCK_DOCKER_LOG:?}"
    [[ "${MOCK_DOCKER_MODE:-ok}" == "ok" ]] || exit 42
    repository="${reference%@*}"
    repository="${repository%:*}"
    slug="${repository##*/}"
    case "$slug" in
        ai-gateway) target=AIGatewayServer ;;
        ai-server) target=AIServer ;;
        account-server) target=AccountServer ;;
        call-gateway) target=CallGatewayServer ;;
        chat-delivery-worker) target=ChatDeliveryWorker ;;
        chat-message-service) target=ChatMessageService ;;
        chat-relation-query-service) target=ChatRelationQueryService ;;
        chat-relation-service-worker) target=ChatRelationServiceWorker ;;
        chat-server) target=ChatServer ;;
        login-server) target=LoginServer ;;
        media-gateway) target=MediaGatewayServer ;;
        moments-gateway) target=MomentsGatewayServer ;;
        r18-gateway) target=R18GatewayServer ;;
        register-server) target=RegisterServer ;;
        varify-server) target=VarifyServer ;;
        *) exit 64 ;;
    esac
    if [[ -n "${MOCK_DOCKER_INSECURE_SUFFIX:-}" && "$reference" == *"$MOCK_DOCKER_INSECURE_SUFFIX" ]]; then
        config="${MOCK_DOCKER_INSECURE_CONFIG:?}"
    else
        config="${MOCK_DOCKER_CONFIG:?}"
    fi
    if [[ "${MOCK_DOCKER_DYNAMIC_IDENTITY:-1}" == 1 ]]; then
        target_hash="$(printf '%s' "$target" | sha256sum | awk '{print $1}')"
        bundle_hash="$(printf 'bundle:%s' "$target" | sha256sum | awk '{print $1}')"
        config_json="$(jq \
            --arg target "$target" \
            --arg revision "${MOCK_SOURCE_SHA:?}" \
            --arg bundle_hash "$bundle_hash" \
            '(.Env // []) |= map(
                if startswith("MEMOCHAT_SERVICE=") then "MEMOCHAT_SERVICE=" + $target else . end
             )
             | (.Labels // {})["org.opencontainers.image.revision"] = $revision
             | .Labels["io.memochat.service.target"] = $target
             | .Labels["io.memochat.bundle.sha256"] = $bundle_hash' \
            "$config")"
    else
        config_json="$(cat -- "$config")"
    fi
    if [[ "$reference" == *@sha256:* ]]; then
        repo_digest="$reference"
        image_id="${reference##*@}"
    else
        target_hash="$(printf '%s' "$target" | sha256sum | awk '{print $1}')"
        repo_digest="${repository}@sha256:${target_hash}"
        image_id="sha256:${target_hash}"
    fi
    jq -n \
        --argjson config "$config_json" \
        --arg image_id "$image_id" \
        --arg repo_digest "$repo_digest" \
        '{Config: $config, Id: $image_id, RepoDigests: [$repo_digest]}'
    exit 0
fi
echo "unexpected docker invocation" >&2
exit 64
""",
        )
        self.write_executable(
            paths["syft"],
            """#!/usr/bin/env bash
set -Eeuo pipefail
if [[ "${1:-}" == "version" ]]; then
    printf 'syft 1.20.0-test\n'
    exit 0
fi
[[ "${1:-}" == "scan" && $# -ge 4 ]] || exit 64
reference="$2"
shift 2
output=
while [[ $# -gt 0 ]]; do
    case "$1" in
        --output)
            output="${2#spdx-json=}"
            shift 2
            ;;
        --config)
            [[ "$2" == *.yaml && -f "$2" ]] || exit 65
            [[ "$(cat -- "$2")" == "{}" ]] || exit 65
            shift 2
            ;;
        *) exit 64 ;;
    esac
done
printf '%s\n' "$reference" >> "${MOCK_SYFT_LOG:?}"
case "${MOCK_SYFT_MODE:-ok}" in
    fail) exit 43 ;;
    invalid) printf '{}\n' > "$output" ;;
    ok) printf '{"spdxVersion":"SPDX-2.3","packages":[{"name":"mock-package"}]}\n' > "$output" ;;
    *) exit 64 ;;
esac
""",
        )
        self.write_executable(
            paths["grype"],
            """#!/usr/bin/env bash
set -Eeuo pipefail
if [[ "${1:-}" == "version" ]]; then
    printf 'grype 0.100.0-test\n'
    exit 0
fi
if [[ "${1:-}" == "db" && "${2:-}" == "status" && "${3:-}" == "-o" && "${4:-}" == "json" ]]; then
    [[ "${5:-}" == "--config" && "${6:-}" == *.yaml && -f "${6:-}" ]] || exit 65
    printf '%s\n' "$*" >> "${MOCK_GRYPE_LOG:?}"
    case "${MOCK_GRYPE_DB_MODE:-ok}" in
        fail) exit 45 ;;
        invalid) printf '[]\n' ;;
        ok) printf '{"built":"2026-07-28T00:00:00Z","schemaVersion":6,"status":"valid"}\n' ;;
        *) exit 64 ;;
    esac
    exit 0
fi
output=
printf '%s\n' "$*" >> "${MOCK_GRYPE_LOG:?}"
while [[ $# -gt 0 ]]; do
    case "$1" in
        sbom:*) shift ;;
        --config)
            [[ "$2" == *.yaml && -f "$2" ]] || exit 65
            [[ "$(cat -- "$2")" == "{}" ]] || exit 65
            shift 2
            ;;
        --fail-on|--output|--file)
            if [[ "$1" == "--file" ]]; then output="$2"; fi
            shift 2
            ;;
        *) exit 64 ;;
    esac
done
[[ -n "$output" ]] || exit 64
case "${MOCK_GRYPE_MODE:-ok}" in
    fail)
        printf '{"matches":[{"vulnerability":{"severity":"High","fix":{"state":"not-fixed"}}}]}\n' > "$output"
        exit 2
        ;;
    critical)
        printf '{"matches":[{"vulnerability":{"severity":"Critical","fix":{"state":"not-fixed"}}}]}\n' > "$output"
        exit 2
        ;;
    invalid) printf '{}\n' > "$output" ;;
    ok) printf '{"matches":[],"source":{"type":"sbom"}}\n' > "$output" ;;
    *) exit 64 ;;
esac
""",
        )
        env = os.environ.copy()
        env.update(
            {
                "MOCK_DOCKER_CONFIG": str(config_path),
                "MOCK_DOCKER_LOG": str(paths["docker_log"]),
                "MOCK_DOCKER_PULL_LOG": str(paths["docker_pull_log"]),
                "MOCK_SYFT_LOG": str(paths["syft_log"]),
                "MOCK_GRYPE_LOG": str(paths["grype_log"]),
                "MOCK_SOURCE_SHA": SOURCE_SHA,
            }
        )
        return env, paths

    def write_image_manifest(self, root: Path, prefix: str = "registry.example/memochat") -> Path:
        sbom_root = root / "sboms"
        sbom_root.mkdir()
        for slug in TARGET_SLUGS.values():
            sbom_root.joinpath(f"{slug}.vcpkg.spdx.json").write_bytes(VCPKG_SBOM_BYTES)
        manifest = root / "backend-images.json"
        manifest.write_text(
            json.dumps(
                {
                    "schema": "memochat-backend-images-v1",
                    "source_sha": SOURCE_SHA,
                    "archive": {
                        "artifact": f"MemoChat-backend-{SOURCE_SHA[:12]}-linux-x86_64.tar.gz",
                        "sha256": hashlib.sha256(b"backend-archive").hexdigest(),
                    },
                    "legal": {
                        "release_source_sha": SOURCE_SHA,
                        "formal_distribution_ready": True,
                        "third_party_legal_corpus": "complete",
                        "corpus_review_id": LEGAL_REVIEW_ID,
                        "corpus_sha256": LEGAL_CORPUS_SHA256,
                        "status_sha256": LEGAL_STATUS_SHA256,
                    },
                    "images": [
                        {
                            "target": target,
                            "image": f"{prefix}/{slug}",
                            "tag": f"sha-{SOURCE_SHA}",
                            "digest": target_digest(target),
                            "bundle_sha256": bundle_digest(target),
                            "vcpkg_sbom_sha256": VCPKG_SBOM_SHA256,
                            "legal_status_sha256": LEGAL_STATUS_SHA256,
                        }
                        for target, slug in TARGET_SLUGS.items()
                    ],
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )
        return manifest

    def run_auditor(
        self,
        root: Path,
        env: dict[str, str],
        paths: dict[str, Path],
        *extra: str,
    ) -> subprocess.CompletedProcess[str]:
        command = [
            "bash",
            str(AUDITOR),
            "--output-dir",
            str(root / "audit"),
            "--docker",
            str(paths["docker"]),
            "--syft",
            str(paths["syft"]),
            "--grype",
            str(paths["grype"]),
            "--fail-on",
            "high",
            *extra,
        ]
        if (root / "sboms").is_dir():
            command.extend(("--vcpkg-sbom-dir", str(root / "sboms")))
        return subprocess.run(
            command,
            cwd=REPO_ROOT,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )

    def test_audits_fixed_topology_and_writes_complete_spdx_and_grype_evidence(self):
        with tempfile.TemporaryDirectory() as temp_text:
            root = Path(temp_text)
            env, paths = self.create_harness(root, hardened_config())

            result = self.run_auditor(
                root,
                env,
                paths,
                "--image-prefix",
                "registry.example/memochat",
                "--tag",
                "sha-0123456789abcdef",
            )

            self.assertEqual(0, result.returncode, result.stdout)
            output = root / "audit"
            self.assertTrue((output / "AUDIT_COMPLETE").is_file())
            self.assertTrue((output / "AUDIT_MANIFEST.txt").is_file())
            self.assertTrue((output / "SHA256SUMS").is_file())
            self.assertEqual(15, len(list(output.glob("*.spdx.json"))))
            self.assertEqual(15, len(list(output.glob("*.grype.json"))))

            inspected = paths["docker_log"].read_text(encoding="utf-8").splitlines()
            expected = {f"registry.example/memochat/{slug}:sha-0123456789abcdef" for slug in TARGET_SLUGS.values()}
            self.assertEqual(expected, set(inspected))
            self.assertEqual(15, len(inspected))
            self.assertEqual(15, len(paths["syft_log"].read_text(encoding="utf-8").splitlines()))
            grype_calls = paths["grype_log"].read_text(encoding="utf-8").splitlines()
            self.assertTrue(grype_calls[0].startswith("db status -o json --config "))
            self.assertTrue(grype_calls[0].endswith("/audit/GRYPE_CONFIG.yaml"))
            self.assertEqual(15, len(grype_calls[1:]))
            self.assertTrue(all("--fail-on high" in call for call in grype_calls[1:]))
            self.assertTrue(all("--config " in call for call in grype_calls[1:]))
            self.assertTrue(all("/audit/GRYPE_CONFIG.yaml" in call for call in grype_calls[1:]))
            self.assertTrue(all("--only-fixed" not in call for call in grype_calls[1:]))
            self.assertTrue((output / "GRYPE_DB_STATUS.json").is_file())
            self.assertEqual("{}\n", (output / "GRYPE_CONFIG.yaml").read_text(encoding="utf-8"))
            self.assertEqual("{}\n", (output / "SYFT_CONFIG.yaml").read_text(encoding="utf-8"))
            manifest = (output / "AUDIT_MANIFEST.txt").read_text(encoding="utf-8")
            self.assertIn("format=memochat-backend-image-audit-v3", manifest)
            self.assertIn("image_count=15", manifest)
            self.assertIn("fail_on=high", manifest)
            self.assertIn("vulnerability_policy=all-findings", manifest)
            checksum = subprocess.run(
                ["sha256sum", "--check", "--strict", "SHA256SUMS"],
                cwd=output,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=False,
            )
            self.assertEqual(0, checksum.returncode, checksum.stdout)

    def test_manifest_mode_pulls_inspects_and_scans_only_digest_bound_references(self):
        with tempfile.TemporaryDirectory() as temp_text:
            root = Path(temp_text)
            env, paths = self.create_harness(root, hardened_config())
            manifest = self.write_image_manifest(root)

            result = self.run_auditor(
                root,
                env,
                paths,
                "--image-prefix",
                "registry.example/memochat",
                "--image-manifest",
                str(manifest),
                "--pull",
            )

            self.assertEqual(0, result.returncode, result.stdout)
            expected = {
                f"registry.example/memochat/{slug}@{target_digest(target)}" for target, slug in TARGET_SLUGS.items()
            }
            self.assertEqual(expected, set(paths["docker_pull_log"].read_text(encoding="utf-8").splitlines()))
            self.assertEqual(expected, set(paths["docker_log"].read_text(encoding="utf-8").splitlines()))
            self.assertEqual(expected, set(paths["syft_log"].read_text(encoding="utf-8").splitlines()))
            self.assertEqual(30, len(list((root / "audit").glob("*.spdx.json"))))
            self.assertEqual(30, len(list((root / "audit").glob("*.grype.json"))))
            copied_manifest = root / "audit" / "BACKEND_IMAGES.json"
            self.assertTrue(copied_manifest.is_file())
            self.assertEqual(manifest.read_bytes(), copied_manifest.read_bytes())
            audit_manifest = (root / "audit" / "AUDIT_MANIFEST.txt").read_text(encoding="utf-8")
            self.assertIn("format=memochat-backend-image-audit-v3", audit_manifest)
            self.assertIn(f"source_sha={SOURCE_SHA}", audit_manifest)
            for target, slug in TARGET_SLUGS.items():
                self.assertIn(
                    f"image={target}|{slug}|{target_digest(target)}",
                    audit_manifest,
                )

    def test_manifest_mode_rejects_wrong_target_mapping_before_pull_or_scan(self):
        with tempfile.TemporaryDirectory() as temp_text:
            root = Path(temp_text)
            env, paths = self.create_harness(root, hardened_config())
            manifest = self.write_image_manifest(root)
            payload = json.loads(manifest.read_text(encoding="utf-8"))
            payload["images"][0]["image"], payload["images"][1]["image"] = (
                payload["images"][1]["image"],
                payload["images"][0]["image"],
            )
            manifest.write_text(json.dumps(payload), encoding="utf-8")

            result = self.run_auditor(
                root,
                env,
                paths,
                "--image-prefix",
                "registry.example/memochat",
                "--image-manifest",
                str(manifest),
                "--pull",
            )

            self.assertNotEqual(0, result.returncode, result.stdout)
            self.assertIn("image mapping mismatch", result.stdout)
            self.assertFalse(paths["docker_pull_log"].exists())
            self.assertFalse(paths["syft_log"].exists())

    def test_manifest_mode_rejects_legal_and_vcpkg_source_binding_drift(self):
        for mutation, expected in (
            ("legal-source", "legal summary"),
            ("vcpkg-source", "vcpkg dependency SBOM source binding"),
        ):
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as temp_text:
                root = Path(temp_text)
                env, paths = self.create_harness(root, hardened_config())
                manifest = self.write_image_manifest(root)
                if mutation == "legal-source":
                    payload = json.loads(manifest.read_text(encoding="utf-8"))
                    payload["legal"]["release_source_sha"] = "f" * 40
                    manifest.write_text(json.dumps(payload), encoding="utf-8")
                else:
                    sbom = root / "sboms/ai-gateway.vcpkg.spdx.json"
                    payload = json.loads(sbom.read_text(encoding="utf-8"))
                    payload["documentComment"] = (
                        f"release_source_sha={'f' * 40}; coverage=installed-closure-overapproximation"
                    )
                    sbom.write_text(json.dumps(payload), encoding="utf-8")
                    manifest_payload = json.loads(manifest.read_text(encoding="utf-8"))
                    manifest_payload["images"][0]["vcpkg_sbom_sha256"] = hashlib.sha256(sbom.read_bytes()).hexdigest()
                    manifest.write_text(json.dumps(manifest_payload), encoding="utf-8")

                result = self.run_auditor(
                    root,
                    env,
                    paths,
                    "--image-prefix",
                    "registry.example/memochat",
                    "--image-manifest",
                    str(manifest),
                    "--pull",
                )

                self.assertNotEqual(0, result.returncode, result.stdout)
                self.assertIn(expected, result.stdout)
                self.assertFalse(paths["docker_pull_log"].exists())
                self.assertFalse(paths["syft_log"].exists())

    def test_manifest_mode_rejects_modified_ubuntu_runtime_provenance_before_scanning(self):
        with tempfile.TemporaryDirectory() as temp_text:
            root = Path(temp_text)
            config = hardened_config(target="AIGatewayServer")
            labels = dict(config["Labels"])
            label = "io.memochat.ubuntu.runtime-packages"
            labels[label] = f"tampered-{UBUNTU_PROVENANCE_LABELS[label]}"
            config["Labels"] = labels
            env, paths = self.create_harness(root, config)
            manifest = self.write_image_manifest(root)

            result = self.run_auditor(
                root,
                env,
                paths,
                "--image-prefix",
                "registry.example/memochat",
                "--image-manifest",
                str(manifest),
                "--pull",
            )

            self.assertNotEqual(0, result.returncode, result.stdout)
            self.assertIn(
                f"expected {label}={UBUNTU_PROVENANCE_LABELS[label]}",
                result.stdout,
            )
            self.assertEqual(
                15,
                len(paths["docker_pull_log"].read_text(encoding="utf-8").splitlines()),
            )
            self.assertFalse(paths["syft_log"].exists(), result.stdout)
            self.assertFalse(paths["grype_log"].exists(), result.stdout)
            self.assertFalse((root / "audit" / "AUDIT_COMPLETE").exists())

    def test_rejects_missing_tools_unsafe_references_and_implicit_policy(self):
        with tempfile.TemporaryDirectory() as temp_text:
            root = Path(temp_text)
            env, paths = self.create_harness(root, hardened_config())
            base = ["bash", str(AUDITOR), "--output-dir", str(root / "audit")]
            cases = (
                (base, "--syft is required"),
                (
                    [
                        *base,
                        "--docker",
                        str(paths["docker"]),
                        "--syft",
                        str(paths["syft"]),
                        "--grype",
                        str(paths["grype"]),
                    ],
                    "--fail-on is required",
                ),
                (
                    [
                        *base,
                        "--docker",
                        str(paths["docker"]),
                        "--syft",
                        str(paths["syft"]),
                        "--grype",
                        str(paths["grype"]),
                        "--fail-on",
                        "severe",
                    ],
                    "unsafe --fail-on",
                ),
                (
                    [
                        *base,
                        "--docker",
                        str(paths["docker"]),
                        "--syft",
                        str(paths["syft"]),
                        "--grype",
                        str(paths["grype"]),
                        "--fail-on",
                        "high",
                        "--image-prefix",
                        "memochat;touch-pwned",
                    ],
                    "unsafe --image-prefix",
                ),
            )
            for index, (command, expected) in enumerate(cases):
                with self.subTest(index=index):
                    result = subprocess.run(
                        command,
                        cwd=REPO_ROOT,
                        env=env,
                        text=True,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT,
                        check=False,
                    )
                    self.assertNotEqual(0, result.returncode, result.stdout)
                    self.assertIn(expected, result.stdout)
                    if (root / "audit").exists():
                        self.fail("invalid arguments must fail before creating the output directory")

    def test_rejects_insecure_image_metadata_before_running_scanners(self):
        cases = {
            "root user": (hardened_config(User="root"), "expected Config.User=10001:10001"),
            "named user": (hardened_config(User="memochat"), "expected Config.User=10001:10001"),
            "entrypoint": (hardened_config(Entrypoint=["/bin/sh"]), "unexpected entrypoint"),
            "workdir": (hardened_config(WorkingDir="/app"), "unexpected working directory"),
            "healthcheck": (hardened_config(Healthcheck=None), "invalid healthcheck"),
            "interval": (
                hardened_config(
                    Healthcheck={
                        "Test": ["CMD", "/app/entrypoint.sh", "--healthcheck"],
                        "Interval": 1,
                        "Timeout": 3_000_000_000,
                        "StartPeriod": 10_000_000_000,
                        "Retries": 3,
                    }
                ),
                "invalid healthcheck",
            ),
            "secret env": (
                hardened_config(
                    Env=[
                        "PATH=/usr/bin",
                        "MEMOCHAT_SERVICE=ChatServer",
                        "MEMOCHAT_RELEASE_MODE=1",
                        "MEMOCHAT_ALLOW_DEV_SECRETS=0",
                        "MEMOCHAT_API_TOKEN=actual-release-token",
                    ]
                ),
                "secret-like environment variable",
            ),
            "jwt value": (
                hardened_config(Env=["PATH=/usr/bin", "PUBLIC_VALUE=eyJhbGciOiJIUzI1NiJ9.payload.signature"]),
                "secret-like environment value",
            ),
        }
        for name, (config, expected) in cases.items():
            with self.subTest(name=name), tempfile.TemporaryDirectory() as temp_text:
                root = Path(temp_text)
                env, paths = self.create_harness(root, config)
                result = self.run_auditor(root, env, paths)
                self.assertNotEqual(0, result.returncode, result.stdout)
                self.assertIn(expected, result.stdout)
                self.assertFalse(paths["syft_log"].exists(), result.stdout)
                self.assertFalse((root / "audit" / "AUDIT_COMPLETE").exists())

    def test_rejects_missing_or_modified_ubuntu_runtime_provenance_before_scanning(self):
        for label, expected_value in UBUNTU_PROVENANCE_LABELS.items():
            for mutation in ("missing", "modified"):
                with (
                    self.subTest(label=label, mutation=mutation),
                    tempfile.TemporaryDirectory() as temp_text,
                ):
                    root = Path(temp_text)
                    config = hardened_config(target="AIGatewayServer")
                    labels = dict(config["Labels"])
                    if mutation == "missing":
                        labels.pop(label)
                    else:
                        labels[label] = f"tampered-{expected_value}"
                    config["Labels"] = labels
                    env, paths = self.create_harness(root, config)

                    result = self.run_auditor(root, env, paths)

                    self.assertNotEqual(0, result.returncode, result.stdout)
                    self.assertIn(f"expected {label}={expected_value}", result.stdout)
                    self.assertFalse(paths["syft_log"].exists(), result.stdout)
                    self.assertFalse(paths["grype_log"].exists(), result.stdout)
                    self.assertFalse((root / "audit" / "AUDIT_COMPLETE").exists())

    def test_rejects_concatenated_and_compound_secret_environment_names(self):
        sensitive_names = (
            "MEMOCHAT_R18_CREDENTIAL_MASTER_KEY",
            "MEMOCHAT_CHATAUTH_HMACSECRET",
            "MEMOCHAT_AUTH_JWTSECRET",
            "MEMOCHAT_MINIO_ACCESSKEY",
            "MEMOCHAT_CALL_APIKEY",
            "MEMOCHAT_RELEASE_SIGNING_KEY",
            "MEMOCHAT_REGISTRY_AUTHORIZATION",
        )
        for sensitive_name in sensitive_names:
            with self.subTest(sensitive_name=sensitive_name), tempfile.TemporaryDirectory() as temp_text:
                root = Path(temp_text)
                config = hardened_config(target="AIGatewayServer")
                config["Env"] = [*config["Env"], f"{sensitive_name}=embedded-release-value"]
                env, paths = self.create_harness(root, config)

                result = self.run_auditor(root, env, paths)

                self.assertNotEqual(0, result.returncode, result.stdout)
                self.assertIn(f"secret-like environment variable {sensitive_name}", result.stdout)
                self.assertFalse(paths["syft_log"].exists(), result.stdout)

    def test_rejects_service_identity_and_release_mode_mismatches(self):
        cases = (
            ("service", "MEMOCHAT_SERVICE=WrongServer", "expected MEMOCHAT_SERVICE=AIGatewayServer"),
            ("release mode", "MEMOCHAT_RELEASE_MODE=0", "expected MEMOCHAT_RELEASE_MODE=1"),
            ("dev secrets", "MEMOCHAT_ALLOW_DEV_SECRETS=1", "expected MEMOCHAT_ALLOW_DEV_SECRETS=0"),
        )
        for name, replacement, expected in cases:
            with self.subTest(name=name), tempfile.TemporaryDirectory() as temp_text:
                root = Path(temp_text)
                config = hardened_config(target="AIGatewayServer")
                key = replacement.split("=", 1)[0]
                config["Env"] = [entry for entry in config["Env"] if not entry.startswith(f"{key}=")]
                config["Env"].append(replacement)
                env, paths = self.create_harness(root, config)
                env["MOCK_DOCKER_DYNAMIC_IDENTITY"] = "0"

                result = self.run_auditor(root, env, paths)

                self.assertNotEqual(0, result.returncode, result.stdout)
                self.assertIn(expected, result.stdout)
                self.assertFalse(paths["syft_log"].exists(), result.stdout)

    def test_allows_empty_or_environment_injected_sensitive_names(self):
        with tempfile.TemporaryDirectory() as temp_text:
            root = Path(temp_text)
            env, paths = self.create_harness(
                root,
                hardened_config(
                    Env=[
                        "PATH=/usr/bin",
                        "MEMOCHAT_SERVICE=ChatServer",
                        "MEMOCHAT_RELEASE_MODE=1",
                        "MEMOCHAT_ALLOW_DEV_SECRETS=0",
                        "MEMOCHAT_API_TOKEN=",
                        "MEMOCHAT_DB_PASSWORD=${MEMOCHAT_DB_PASSWORD}",
                    ]
                ),
            )
            result = self.run_auditor(root, env, paths)
            self.assertEqual(0, result.returncode, result.stdout)

    def test_preflights_all_image_configs_before_starting_any_scanner(self):
        with tempfile.TemporaryDirectory() as temp_text:
            root = Path(temp_text)
            env, paths = self.create_harness(root, hardened_config())
            insecure_config = root / "insecure-config.json"
            insecure_config.write_text(json.dumps(hardened_config(User="root")), encoding="utf-8")
            env.update(
                {
                    "MOCK_DOCKER_INSECURE_SUFFIX": "/varify-server:local",
                    "MOCK_DOCKER_INSECURE_CONFIG": str(insecure_config),
                }
            )

            result = self.run_auditor(root, env, paths)

            self.assertNotEqual(0, result.returncode, result.stdout)
            self.assertIn("expected Config.User=10001:10001", result.stdout)
            self.assertEqual(15, len(paths["docker_log"].read_text(encoding="utf-8").splitlines()))
            self.assertFalse(paths["syft_log"].exists(), result.stdout)
            self.assertFalse(paths["grype_log"].exists(), result.stdout)
            self.assertFalse((root / "audit" / "AUDIT_COMPLETE").exists())

    def test_syft_and_grype_errors_fail_closed_without_completion_marker(self):
        for tool, mode, expected in (
            ("MOCK_SYFT_MODE", "fail", "Syft failed"),
            ("MOCK_SYFT_MODE", "invalid", "invalid SPDX JSON"),
            ("MOCK_GRYPE_MODE", "fail", "complete reports retained"),
            ("MOCK_GRYPE_MODE", "invalid", "invalid Grype JSON"),
        ):
            with self.subTest(tool=tool, mode=mode), tempfile.TemporaryDirectory() as temp_text:
                root = Path(temp_text)
                env, paths = self.create_harness(root, hardened_config())
                env[tool] = mode
                result = self.run_auditor(root, env, paths)
                self.assertNotEqual(0, result.returncode, result.stdout)
                self.assertIn(expected, result.stdout)
                self.assertFalse((root / "audit" / "AUDIT_COMPLETE").exists())
                if tool == "MOCK_GRYPE_MODE" and mode == "fail":
                    reports = list((root / "audit").glob("*.grype.json"))
                    self.assertEqual(15, len(reports), result.stdout)
                    self.assertTrue((root / "audit/AUDIT_POLICY_FAILED").is_file())
                    self.assertTrue((root / "audit/SHA256SUMS").is_file())
                    self.assertIn(
                        "AUDIT_POLICY_FAILED",
                        (root / "audit/SHA256SUMS").read_text(encoding="utf-8"),
                    )
                    self.assertTrue((root / "audit/AUDIT_MANIFEST.txt").is_file())
                    self.assertTrue((root / "audit/POLICY_FAILURES.tsv").is_file())
                    self.assertTrue(
                        all('"state":"not-fixed"' in report.read_text(encoding="utf-8") for report in reports)
                    )

    def test_grype_database_status_errors_fail_closed_before_scanning(self):
        for mode, expected in (("fail", "Grype database status failed"), ("invalid", "invalid Grype database status")):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as temp_text:
                root = Path(temp_text)
                env, paths = self.create_harness(root, hardened_config())
                env["MOCK_GRYPE_DB_MODE"] = mode

                result = self.run_auditor(root, env, paths)

                self.assertNotEqual(0, result.returncode, result.stdout)
                self.assertIn(expected, result.stdout)
                self.assertFalse(paths["syft_log"].exists(), result.stdout)
                self.assertFalse((root / "audit" / "AUDIT_COMPLETE").exists())

    def test_critical_findings_also_fail_after_all_reports_are_written(self):
        with tempfile.TemporaryDirectory() as temp_text:
            root = Path(temp_text)
            env, paths = self.create_harness(root, hardened_config())
            env["MOCK_GRYPE_MODE"] = "critical"

            result = self.run_auditor(root, env, paths)

            self.assertNotEqual(0, result.returncode, result.stdout)
            self.assertIn("complete reports retained", result.stdout)
            self.assertEqual(15, len(list((root / "audit").glob("*.grype.json"))))
            self.assertFalse((root / "audit/AUDIT_COMPLETE").exists())

    def test_refuses_to_mix_results_with_an_existing_output_directory(self):
        with tempfile.TemporaryDirectory() as temp_text:
            root = Path(temp_text)
            env, paths = self.create_harness(root, hardened_config())
            output = root / "audit"
            output.mkdir()
            (output / "stale.spdx.json").write_text("stale\n", encoding="utf-8")
            result = self.run_auditor(root, env, paths)
            self.assertNotEqual(0, result.returncode, result.stdout)
            self.assertIn("output directory already exists", result.stdout)
            self.assertEqual("stale\n", (output / "stale.spdx.json").read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
