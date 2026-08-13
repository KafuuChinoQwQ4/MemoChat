import hashlib
import json
import os
import stat
import subprocess
from pathlib import Path

import pytest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
LEGAL_VERIFIER = REPO_ROOT / "tools/scripts/release/verify_release_legal.sh"
SOURCE_SNAPSHOT_TOOL = REPO_ROOT / "tools/scripts/release/compute_release_source_snapshot.py"
REQUIRED_SCOPES = (
    "backend-vcpkg",
    "client-assets",
    "container-ubuntu",
    "ffmpeg",
    "gcc-runtime",
    "icu",
    "qt",
    "qtwebengine-chromium",
)
SOURCE_SHA = "0123456789abcdef0123456789abcdef01234567"
REVIEW_ID = "memo-distribution-materials-2026-07-29"


def run_verifier(project_root: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["bash", str(LEGAL_VERIFIER), "--project-root", str(project_root), *args],
        cwd=REPO_ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )


def make_inventory(project_root: Path) -> None:
    project_root.mkdir(parents=True, exist_ok=True)
    for name in ("LICENSE", "THIRD_PARTY_NOTICES.md"):
        project_root.joinpath(name).write_bytes(REPO_ROOT.joinpath(name).read_bytes())


def make_complete_corpus(
    project_root: Path,
    reviewed_source_snapshot_sha256: str = "0" * 64,
) -> Path:
    corpus = project_root / "legal/third-party"
    corpus.mkdir(parents=True)
    scopes: dict[str, list[str]] = {}
    for scope in REQUIRED_SCOPES:
        relative = f"materials/{scope}/LICENSE.txt"
        payload = corpus / relative
        payload.parent.mkdir(parents=True)
        payload.write_text(f"verbatim fixture for {scope}\n", encoding="utf-8")
        scopes[scope] = [relative]

    manifest = {
        "schema": "memochat-third-party-corpus-v2",
        "review_status": "distribution-materials-complete",
        "review_id": REVIEW_ID,
        "reviewed_source_snapshot_sha256": reviewed_source_snapshot_sha256,
        "scopes": scopes,
    }
    corpus.joinpath("CORPUS.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    checksum_paths = [Path("CORPUS.json")]
    checksum_paths.extend(Path(path) for files in scopes.values() for path in files)
    corpus.joinpath("SHA256SUMS").write_text(
        "".join(
            f"{hashlib.sha256(corpus.joinpath(path).read_bytes()).hexdigest()}  {path.as_posix()}\n"
            for path in sorted(checksum_paths)
        ),
        encoding="utf-8",
    )
    return corpus


def commit_fixture(project_root: Path, message: str = "release fixture") -> str:
    if not project_root.joinpath(".git").is_dir():
        subprocess.run(["git", "init", "-q", str(project_root)], check=True)
        subprocess.run(["git", "-C", str(project_root), "config", "user.name", "MemoChat Test"], check=True)
        subprocess.run(
            ["git", "-C", str(project_root), "config", "user.email", "test@localhost.invalid"],
            check=True,
        )
    subprocess.run(["git", "-C", str(project_root), "add", "."], check=True)
    subprocess.run(["git", "-C", str(project_root), "commit", "-qm", message], check=True)
    return subprocess.check_output(
        ["git", "-C", str(project_root), "rev-parse", "HEAD"],
        text=True,
    ).strip()


def source_snapshot_sha256(project_root: Path, source_sha: str) -> str:
    return subprocess.check_output(
        [
            "python3",
            str(SOURCE_SNAPSHOT_TOOL),
            "--project-root",
            str(project_root),
            "--source-sha",
            source_sha,
        ],
        text=True,
    ).strip()


def make_source_bound_corpus(project_root: Path) -> tuple[Path, str, str]:
    make_inventory(project_root)
    project_root.joinpath("README.md").write_text("reviewed source input\n", encoding="utf-8")
    source_without_corpus = commit_fixture(project_root, "reviewed source snapshot")
    reviewed_snapshot = source_snapshot_sha256(project_root, source_without_corpus)
    corpus = make_complete_corpus(project_root, reviewed_snapshot)
    source_sha = commit_fixture(project_root, "bind distribution materials")
    assert source_snapshot_sha256(project_root, source_sha) == reviewed_snapshot
    return corpus, source_sha, reviewed_snapshot


def parse_status(path: Path) -> dict[str, str]:
    return dict(line.split("=", 1) for line in path.read_text(encoding="utf-8").splitlines())


def test_legal_verifier_is_an_executable_regular_file() -> None:
    mode = LEGAL_VERIFIER.stat().st_mode

    assert stat.S_ISREG(mode)
    assert mode & stat.S_IXUSR


def test_missing_corpus_is_visible_in_status_and_blocks_formal_distribution(tmp_path: Path) -> None:
    make_inventory(tmp_path)
    status = tmp_path / "status.txt"

    inspected = run_verifier(tmp_path, "--status-file", str(status))

    assert inspected.returncode == 0, inspected.stdout
    assert parse_status(status) == {
        "format": "memochat-distribution-legal-status-v3",
        "project_license": "complete",
        "third_party_inventory": "complete",
        "third_party_legal_corpus": "incomplete",
        "corpus_review_id": "unavailable",
        "corpus_sha256": "unavailable",
        "reviewed_source_snapshot_sha256": "unavailable",
        "release_source_sha": "unbound",
        "release_source_tree": "unbound",
        "release_source_snapshot_sha256": "unbound",
        "formal_distribution_ready": "false",
    }
    assert "third-party distribution corpus: incomplete" in inspected.stdout.lower()

    required = run_verifier(
        tmp_path,
        "--require-distribution-corpus",
        "--source-sha",
        SOURCE_SHA,
    )
    assert required.returncode != 0, required.stdout
    assert "formal distribution is blocked" in required.stdout.lower()


@pytest.mark.parametrize(
    "mutation, expected_message",
    (
        ("missing-scope", "scope"),
        ("empty-payload", "nonempty"),
        ("undeclared-file", "undeclared"),
        ("checksum-mismatch", "checksum"),
        ("incomplete-status", "review_status"),
        ("duplicate-key", "duplicate key"),
        ("legacy-source-sha", "unexpected"),
        ("legacy-corpus-signature", "undeclared"),
        ("symlink", "symlink"),
        ("fifo", "unsupported filesystem"),
    ),
)
def test_present_but_incomplete_corpus_fails_even_status_mode(
    tmp_path: Path,
    mutation: str,
    expected_message: str,
) -> None:
    make_inventory(tmp_path)
    corpus = make_complete_corpus(tmp_path)
    manifest_path = corpus / "CORPUS.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if mutation == "missing-scope":
        del manifest["scopes"][REQUIRED_SCOPES[0]]
        manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    elif mutation == "empty-payload":
        corpus.joinpath("materials/qt/LICENSE.txt").write_bytes(b"")
    elif mutation == "undeclared-file":
        corpus.joinpath("unreviewed.txt").write_text("not in manifest\n", encoding="utf-8")
    elif mutation == "checksum-mismatch":
        corpus.joinpath("materials/qt/LICENSE.txt").write_text("changed after review\n", encoding="utf-8")
    elif mutation == "incomplete-status":
        manifest["review_status"] = "draft"
        manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    elif mutation == "duplicate-key":
        original = manifest_path.read_text(encoding="utf-8")
        manifest_path.write_text(
            original.replace(
                '  "review_status": "distribution-materials-complete",',
                '  "review_status": "draft",\n'
                '  "review_status": "distribution-materials-complete",',
            ),
            encoding="utf-8",
        )
        checksum_path = corpus / "SHA256SUMS"
        checksum_lines = checksum_path.read_text(encoding="utf-8").splitlines()
        checksum_path.write_text(
            "\n".join(
                (
                    f"{hashlib.sha256(manifest_path.read_bytes()).hexdigest()}  CORPUS.json"
                    if line.endswith("  CORPUS.json")
                    else line
                )
                for line in checksum_lines
            )
            + "\n",
            encoding="utf-8",
        )
    elif mutation == "legacy-source-sha":
        manifest["source_sha"] = SOURCE_SHA
        manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    elif mutation == "legacy-corpus-signature":
        corpus.joinpath("CORPUS.sig").write_bytes(b"legacy repository-owned signature\n")
    elif mutation == "symlink":
        corpus.joinpath("linked-license.txt").symlink_to("materials/qt/LICENSE.txt")
    elif mutation == "fifo":
        os.mkfifo(corpus / "review.pipe")

    result = run_verifier(tmp_path)

    assert result.returncode != 0, result.stdout
    assert expected_message in result.stdout.lower()


def test_complete_commit_bound_corpus_is_copied_and_marked_ready(tmp_path: Path) -> None:
    project_root = tmp_path / "repo"
    destination = tmp_path / "artifact-legal"
    status = tmp_path / "artifact-status.txt"
    corpus, source_sha, reviewed_snapshot = make_source_bound_corpus(project_root)
    source_tree = subprocess.check_output(
        ["git", "-C", str(project_root), "rev-parse", "HEAD^{tree}"],
        text=True,
    ).strip()

    unbound_status = tmp_path / "unbound-status.txt"
    unbound_result = run_verifier(project_root, "--status-file", str(unbound_status))
    assert unbound_result.returncode == 0, unbound_result.stdout
    assert parse_status(unbound_status)["formal_distribution_ready"] == "false"
    assert parse_status(unbound_status)["release_source_sha"] == "unbound"

    result = run_verifier(
        project_root,
        "--require-distribution-corpus",
        "--source-sha",
        source_sha,
        "--copy-to",
        str(destination),
        "--status-file",
        str(status),
    )

    assert result.returncode == 0, result.stdout
    assert parse_status(status) == {
        "format": "memochat-distribution-legal-status-v3",
        "project_license": "complete",
        "third_party_inventory": "complete",
        "third_party_legal_corpus": "complete",
        "corpus_review_id": REVIEW_ID,
        "corpus_sha256": hashlib.sha256(corpus.joinpath("SHA256SUMS").read_bytes()).hexdigest(),
        "reviewed_source_snapshot_sha256": reviewed_snapshot,
        "release_source_sha": source_sha,
        "release_source_tree": source_tree,
        "release_source_snapshot_sha256": reviewed_snapshot,
        "formal_distribution_ready": "true",
    }
    assert destination.joinpath("LICENSE").read_bytes() == project_root.joinpath("LICENSE").read_bytes()
    assert (
        destination.joinpath("THIRD_PARTY_NOTICES.md").read_bytes()
        == project_root.joinpath("THIRD_PARTY_NOTICES.md").read_bytes()
    )
    assert destination.joinpath("third-party/CORPUS.json").read_bytes() == corpus.joinpath("CORPUS.json").read_bytes()
    assert not destination.joinpath("approval").exists()
    assert destination.joinpath("LEGAL-STATUS.txt").read_bytes() == status.read_bytes()

    mismatch = run_verifier(
        project_root,
        "--require-distribution-corpus",
        "--source-sha",
        "f" * 40,
    )
    assert mismatch.returncode != 0, mismatch.stdout
    assert "checkout head" in mismatch.stdout.lower()

    unbound = run_verifier(project_root, "--require-distribution-corpus")
    assert unbound.returncode != 0, unbound.stdout
    assert "--source-sha" in unbound.stdout


@pytest.mark.parametrize("drift_name", ("tracked", "untracked"))
def test_source_binding_rejects_any_checkout_drift(tmp_path: Path, drift_name: str) -> None:
    project_root = tmp_path / "repo"
    _, source_sha, _ = make_source_bound_corpus(project_root)
    if drift_name == "tracked":
        project_root.joinpath("README.md").write_text("drifted input\n", encoding="utf-8")
    else:
        project_root.joinpath("untracked-release-input.txt").write_text("drift\n", encoding="utf-8")

    result = run_verifier(project_root, "--source-sha", source_sha)

    assert result.returncode != 0, result.stdout
    assert "working tree drift" in result.stdout.lower()


def test_committed_source_change_invalidates_an_unchanged_corpus(tmp_path: Path) -> None:
    project_root = tmp_path / "repo"
    _, _, reviewed_snapshot = make_source_bound_corpus(project_root)
    project_root.joinpath("README.md").write_text("dependency inputs changed\n", encoding="utf-8")
    changed_sha = commit_fixture(project_root, "change reviewed source inputs")

    assert source_snapshot_sha256(project_root, changed_sha) != reviewed_snapshot
    result = run_verifier(
        project_root,
        "--require-distribution-corpus",
        "--source-sha",
        changed_sha,
    )

    assert result.returncode != 0, result.stdout
    assert "source snapshot does not match" in result.stdout.lower()


def test_inventory_rejects_wrong_license_and_placeholder_notice(tmp_path: Path) -> None:
    make_inventory(tmp_path)
    tmp_path.joinpath("LICENSE").write_text("MIT\n", encoding="utf-8")

    wrong_license = run_verifier(tmp_path)
    assert wrong_license.returncode != 0, wrong_license.stdout
    assert "license" in wrong_license.stdout.lower()

    make_inventory(tmp_path)
    tmp_path.joinpath("THIRD_PARTY_NOTICES.md").write_text("placeholder\n", encoding="utf-8")

    placeholder = run_verifier(tmp_path)
    assert placeholder.returncode != 0, placeholder.stdout
    assert "inventory" in placeholder.stdout.lower()


def test_output_paths_cannot_overwrite_inputs_or_mutate_the_verified_corpus(tmp_path: Path) -> None:
    make_inventory(tmp_path)
    original_license = tmp_path.joinpath("LICENSE").read_bytes()

    overwrite = run_verifier(tmp_path, "--status-file", str(tmp_path / "LICENSE"))
    assert overwrite.returncode != 0, overwrite.stdout
    assert "already exists" in overwrite.stdout.lower()
    assert tmp_path.joinpath("LICENSE").read_bytes() == original_license

    corpus = make_complete_corpus(tmp_path)
    status_in_corpus = run_verifier(tmp_path, "--status-file", str(corpus / "generated-status.txt"))
    copy_into_corpus = run_verifier(tmp_path, "--copy-to", str(corpus / "artifact-legal"))

    assert status_in_corpus.returncode != 0, status_in_corpus.stdout
    assert "inside the verified corpus" in status_in_corpus.stdout.lower()
    assert copy_into_corpus.returncode != 0, copy_into_corpus.stdout
    assert "inside the verified corpus" in copy_into_corpus.stdout.lower()
    assert not corpus.joinpath("generated-status.txt").exists()
    assert not corpus.joinpath("artifact-legal").exists()


@pytest.mark.parametrize(
    "legacy_option",
    ("--approval-public-key", "--approval-signature", "--write-approval-payload"),
)
def test_removed_external_approval_options_are_rejected(tmp_path: Path, legacy_option: str) -> None:
    make_inventory(tmp_path)

    result = run_verifier(tmp_path, legacy_option, str(tmp_path / "legacy-input"))

    assert result.returncode != 0, result.stdout
    assert "unknown argument" in result.stdout.lower()
