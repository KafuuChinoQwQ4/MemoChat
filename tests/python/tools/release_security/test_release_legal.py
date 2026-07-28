import hashlib
import json
import os
import subprocess
from pathlib import Path

import pytest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
LEGAL_VERIFIER = REPO_ROOT / "tools/scripts/release/verify_release_legal.sh"
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
REVIEW_ID = "memo-legal-review-2026-07-28"


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


def make_approval_key(key_root: Path) -> tuple[Path, Path]:
    key_root.mkdir(parents=True, exist_ok=True)
    private_key = key_root / "legal-approval-private.pem"
    public_key = key_root / "legal-approval-public.pem"
    subprocess.run(
        [
            "openssl",
            "genpkey",
            "-algorithm",
            "RSA",
            "-pkeyopt",
            "rsa_keygen_bits:2048",
            "-out",
            str(private_key),
        ],
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    subprocess.run(
        [
            "openssl",
            "pkey",
            "-in",
            str(private_key),
            "-pubout",
            "-out",
            str(public_key),
        ],
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    return private_key, public_key


def make_complete_corpus(project_root: Path) -> Path:
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
        "schema": "memochat-third-party-corpus-v1",
        "review_status": "approved-for-distribution",
        "review_id": REVIEW_ID,
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


def write_approval_payload(project_root: Path, source_sha: str, output: Path) -> str:
    result = run_verifier(
        project_root,
        "--source-sha",
        source_sha,
        "--write-approval-payload",
        str(output),
    )
    assert result.returncode == 0, result.stdout
    return result.stdout


def sign_approval_payload(private_key: Path, payload: Path, signature: Path) -> None:
    subprocess.run(
        [
            "openssl",
            "dgst",
            "-sha256",
            "-sign",
            str(private_key),
            "-out",
            str(signature),
            str(payload),
        ],
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def public_key_fingerprint(public_key: Path) -> str:
    der = subprocess.check_output(
        ["openssl", "pkey", "-pubin", "-in", str(public_key), "-outform", "DER"],
        stderr=subprocess.DEVNULL,
    )
    return hashlib.sha256(der).hexdigest()


def parse_status(path: Path) -> dict[str, str]:
    return dict(line.split("=", 1) for line in path.read_text(encoding="utf-8").splitlines())


def test_missing_corpus_is_visible_in_status_and_blocks_formal_distribution(tmp_path: Path) -> None:
    make_inventory(tmp_path)
    status = tmp_path / "status.txt"

    inspected = run_verifier(tmp_path, "--status-file", str(status))

    assert inspected.returncode == 0, inspected.stdout
    assert parse_status(status) == {
        "format": "memochat-distribution-legal-status-v2",
        "project_license": "complete",
        "third_party_inventory": "complete",
        "third_party_legal_corpus": "incomplete",
        "corpus_review_id": "unavailable",
        "corpus_sha256": "unavailable",
        "approval_payload_sha256": "unavailable",
        "approval_signature_status": "unavailable",
        "approval_signature_sha256": "unavailable",
        "approval_public_key_fingerprint_sha256": "unavailable",
        "release_source_sha": "unbound",
        "release_source_tree": "unbound",
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
        ("undeclared-file", "undeclared"),
        ("checksum-mismatch", "checksum"),
        ("unapproved", "review_status"),
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
    elif mutation == "undeclared-file":
        corpus.joinpath("unreviewed.txt").write_text("not in manifest\n", encoding="utf-8")
    elif mutation == "checksum-mismatch":
        corpus.joinpath("materials/qt/LICENSE.txt").write_text("changed after review\n", encoding="utf-8")
    elif mutation == "unapproved":
        manifest["review_status"] = "draft"
        manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
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
    make_inventory(project_root)
    private_key, public_key = make_approval_key(tmp_path / "external-approval")
    corpus = make_complete_corpus(project_root)
    source_sha = commit_fixture(project_root, "legal fixture")
    source_tree = subprocess.check_output(
        ["git", "-C", str(project_root), "rev-parse", "HEAD^{tree}"],
        text=True,
    ).strip()

    unbound_status = tmp_path / "unbound-status.txt"
    unbound_result = run_verifier(project_root, "--status-file", str(unbound_status))
    assert unbound_result.returncode == 0, unbound_result.stdout
    assert parse_status(unbound_status)["formal_distribution_ready"] == "false"
    assert parse_status(unbound_status)["release_source_sha"] == "unbound"
    assert parse_status(unbound_status)["approval_signature_status"] == "unavailable"
    assert parse_status(unbound_status)["approval_payload_sha256"] == "unavailable"

    approval_payload = tmp_path / "external-approval/approval-v2.txt"
    approval_signature = tmp_path / "external-approval/approval-v2.sig"
    write_approval_payload(project_root, source_sha, approval_payload)
    sign_approval_payload(private_key, approval_payload, approval_signature)
    payload = parse_status(approval_payload)
    assert payload == {
        "format": "memochat-release-legal-approval-v2",
        "release_source_sha": source_sha,
        "release_source_tree": source_tree,
        "license_sha256": hashlib.sha256(project_root.joinpath("LICENSE").read_bytes()).hexdigest(),
        "third_party_notices_sha256": hashlib.sha256(
            project_root.joinpath("THIRD_PARTY_NOTICES.md").read_bytes()
        ).hexdigest(),
        "corpus_review_id": REVIEW_ID,
        "corpus_sha256": hashlib.sha256(corpus.joinpath("SHA256SUMS").read_bytes()).hexdigest(),
    }

    result = run_verifier(
        project_root,
        "--require-distribution-corpus",
        "--source-sha",
        source_sha,
        "--approval-public-key",
        str(public_key),
        "--approval-signature",
        str(approval_signature),
        "--copy-to",
        str(destination),
        "--status-file",
        str(status),
    )

    assert result.returncode == 0, result.stdout
    status_data = parse_status(status)
    assert status_data["format"] == "memochat-distribution-legal-status-v2"
    assert status_data["formal_distribution_ready"] == "true"
    assert status_data["third_party_legal_corpus"] == "complete"
    assert status_data["corpus_review_id"] == REVIEW_ID
    assert status_data["approval_signature_status"] == "verified"
    assert status_data["approval_payload_sha256"] == hashlib.sha256(approval_payload.read_bytes()).hexdigest()
    assert status_data["approval_signature_sha256"] == hashlib.sha256(approval_signature.read_bytes()).hexdigest()
    assert status_data["approval_public_key_fingerprint_sha256"] == public_key_fingerprint(public_key)
    assert status_data["corpus_sha256"] == hashlib.sha256(corpus.joinpath("SHA256SUMS").read_bytes()).hexdigest()
    assert status_data["release_source_sha"] == source_sha
    assert status_data["release_source_tree"] == source_tree
    assert destination.joinpath("LICENSE").read_bytes() == project_root.joinpath("LICENSE").read_bytes()
    assert (
        destination.joinpath("THIRD_PARTY_NOTICES.md").read_bytes()
        == project_root.joinpath("THIRD_PARTY_NOTICES.md").read_bytes()
    )
    assert destination.joinpath("third-party/CORPUS.json").read_bytes() == corpus.joinpath("CORPUS.json").read_bytes()
    assert not destination.joinpath("third-party/CORPUS.sig").exists()
    assert destination.joinpath("approval/APPROVAL-PAYLOAD.txt").read_bytes() == approval_payload.read_bytes()
    assert destination.joinpath("approval/APPROVAL.sig").read_bytes() == approval_signature.read_bytes()
    assert destination.joinpath("LEGAL-STATUS.txt").read_bytes() == status.read_bytes()

    mismatch = run_verifier(
        project_root,
        "--require-distribution-corpus",
        "--source-sha",
        "f" * 40,
        "--approval-public-key",
        str(public_key),
        "--approval-signature",
        str(approval_signature),
    )
    assert mismatch.returncode != 0, mismatch.stdout
    assert "checkout head" in mismatch.stdout.lower()

    unbound = run_verifier(
        project_root,
        "--require-distribution-corpus",
        "--approval-public-key",
        str(public_key),
        "--approval-signature",
        str(approval_signature),
    )
    assert unbound.returncode != 0, unbound.stdout
    assert "--source-sha" in unbound.stdout

    unapproved = run_verifier(
        project_root,
        "--require-distribution-corpus",
        "--source-sha",
        source_sha,
    )
    assert unapproved.returncode != 0, unapproved.stdout
    assert "--approval-public-key" in unapproved.stdout


def test_external_approval_rejects_wrong_signature_and_repo_owned_trust_root(tmp_path: Path) -> None:
    project_root = tmp_path / "repo"
    make_inventory(project_root)
    private_key, public_key = make_approval_key(tmp_path / "external-approval")
    wrong_private_key, wrong_public_key = make_approval_key(tmp_path / "wrong-approval")
    make_complete_corpus(project_root)
    repo_private_key, repo_public_key = make_approval_key(project_root / "repo-owned-approval")
    source_sha = commit_fixture(project_root)
    approval_payload = tmp_path / "external-approval/approval-v2.txt"
    approval_signature = tmp_path / "external-approval/approval-v2.sig"
    wrong_signature = tmp_path / "wrong-approval/wrong-approval.sig"
    repo_key_signature = tmp_path / "external-approval/repo-key-approval.sig"
    write_approval_payload(project_root, source_sha, approval_payload)
    sign_approval_payload(private_key, approval_payload, approval_signature)
    sign_approval_payload(wrong_private_key, approval_payload, wrong_signature)
    sign_approval_payload(repo_private_key, approval_payload, repo_key_signature)

    wrong_key = run_verifier(
        project_root,
        "--source-sha",
        source_sha,
        "--approval-public-key",
        str(wrong_public_key),
        "--approval-signature",
        str(approval_signature),
    )
    assert wrong_key.returncode != 0, wrong_key.stdout
    assert "signature verification failed" in wrong_key.stdout.lower()

    tampered = run_verifier(
        project_root,
        "--source-sha",
        source_sha,
        "--approval-public-key",
        str(public_key),
        "--approval-signature",
        str(wrong_signature),
    )
    assert tampered.returncode != 0, tampered.stdout
    assert "signature verification failed" in tampered.stdout.lower()

    linked_public_key = tmp_path / "linked-public-key.pem"
    linked_public_key.symlink_to(public_key)
    linked_key = run_verifier(
        project_root,
        "--source-sha",
        source_sha,
        "--approval-public-key",
        str(linked_public_key),
        "--approval-signature",
        str(approval_signature),
    )
    assert linked_key.returncode != 0, linked_key.stdout
    assert "must not be a symlink" in linked_key.stdout.lower()

    repo_trust_root = run_verifier(
        project_root,
        "--source-sha",
        source_sha,
        "--approval-public-key",
        str(repo_public_key),
        "--approval-signature",
        str(repo_key_signature),
    )
    assert repo_trust_root.returncode != 0, repo_trust_root.stdout
    assert "outside the repository trust boundary" in repo_trust_root.stdout.lower()

    linked_signature = tmp_path / "linked-approval.sig"
    linked_signature.symlink_to(approval_signature)
    linked_signature_result = run_verifier(
        project_root,
        "--source-sha",
        source_sha,
        "--approval-public-key",
        str(public_key),
        "--approval-signature",
        str(linked_signature),
    )
    assert linked_signature_result.returncode != 0, linked_signature_result.stdout
    assert "signature must not be a symlink" in linked_signature_result.stdout.lower()

    missing_signature = run_verifier(
        project_root,
        "--source-sha",
        source_sha,
        "--approval-public-key",
        str(public_key),
    )
    assert missing_signature.returncode != 0, missing_signature.stdout
    assert "requires --approval-signature" in missing_signature.stdout.lower()

    missing_key = run_verifier(
        project_root,
        "--source-sha",
        source_sha,
        "--approval-signature",
        str(approval_signature),
    )
    assert missing_key.returncode != 0, missing_key.stdout
    assert "requires --approval-public-key" in missing_key.stdout.lower()

    repo_signature = project_root / "repo-owned-approval.sig"
    repo_signature.write_bytes(approval_signature.read_bytes())
    changed_source_sha = commit_fixture(project_root, "add repository-owned approval signature")
    repo_signature_result = run_verifier(
        project_root,
        "--source-sha",
        changed_source_sha,
        "--approval-public-key",
        str(public_key),
        "--approval-signature",
        str(repo_signature),
    )
    assert repo_signature_result.returncode != 0, repo_signature_result.stdout
    assert "signature must be provided from outside" in repo_signature_result.stdout.lower()


def test_external_approval_cannot_be_replayed_after_the_source_tree_changes(tmp_path: Path) -> None:
    project_root = tmp_path / "repo"
    make_inventory(project_root)
    make_complete_corpus(project_root)
    private_key, public_key = make_approval_key(tmp_path / "external-approval")
    original_sha = commit_fixture(project_root, "approved source")
    original_payload = tmp_path / "external-approval/original-approval.txt"
    original_signature = tmp_path / "external-approval/original-approval.sig"
    write_approval_payload(project_root, original_sha, original_payload)
    sign_approval_payload(private_key, original_payload, original_signature)

    with project_root.joinpath("THIRD_PARTY_NOTICES.md").open("a", encoding="utf-8") as notice:
        notice.write("\nApproval replay regression fixture.\n")
    changed_sha = commit_fixture(project_root, "change approved release inputs")

    replay = run_verifier(
        project_root,
        "--require-distribution-corpus",
        "--source-sha",
        changed_sha,
        "--approval-public-key",
        str(public_key),
        "--approval-signature",
        str(original_signature),
    )

    assert replay.returncode != 0, replay.stdout
    assert "signature verification failed" in replay.stdout.lower()


@pytest.mark.parametrize("drift_name", ("tracked", "untracked"))
def test_source_binding_rejects_any_checkout_drift(tmp_path: Path, drift_name: str) -> None:
    project_root = tmp_path / "repo"
    make_inventory(project_root)
    make_complete_corpus(project_root)
    project_root.joinpath("README.md").write_text("committed input\n", encoding="utf-8")
    subprocess.run(["git", "init", "-q", str(project_root)], check=True)
    subprocess.run(["git", "-C", str(project_root), "config", "user.name", "MemoChat Test"], check=True)
    subprocess.run(
        ["git", "-C", str(project_root), "config", "user.email", "test@localhost.invalid"],
        check=True,
    )
    subprocess.run(["git", "-C", str(project_root), "add", "."], check=True)
    subprocess.run(["git", "-C", str(project_root), "commit", "-qm", "release inputs"], check=True)
    source_sha = subprocess.check_output(["git", "-C", str(project_root), "rev-parse", "HEAD"], text=True).strip()
    if drift_name == "tracked":
        project_root.joinpath("README.md").write_text("drifted input\n", encoding="utf-8")
    else:
        project_root.joinpath("untracked-release-input.txt").write_text("drift\n", encoding="utf-8")

    result = run_verifier(project_root, "--source-sha", source_sha)

    assert result.returncode != 0, result.stdout
    assert "working tree drift" in result.stdout.lower()


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


def test_approval_payload_output_must_be_new_and_outside_the_repository(tmp_path: Path) -> None:
    project_root = tmp_path / "repo"
    make_inventory(project_root)
    make_complete_corpus(project_root)
    source_sha = commit_fixture(project_root)

    inside_repository = project_root / "approval-v2.txt"
    inside_result = run_verifier(
        project_root,
        "--source-sha",
        source_sha,
        "--write-approval-payload",
        str(inside_repository),
    )
    assert inside_result.returncode != 0, inside_result.stdout
    assert "outside the repository" in inside_result.stdout.lower()
    assert not inside_repository.exists()

    existing = tmp_path / "existing-approval.txt"
    existing.write_text("preserve\n", encoding="utf-8")
    existing_result = run_verifier(
        project_root,
        "--source-sha",
        source_sha,
        "--write-approval-payload",
        str(existing),
    )
    assert existing_result.returncode != 0, existing_result.stdout
    assert "already exists" in existing_result.stdout.lower()
    assert existing.read_text(encoding="utf-8") == "preserve\n"

    linked_output = tmp_path / "linked-approval.txt"
    linked_output.symlink_to(tmp_path / "missing-target")
    linked_result = run_verifier(
        project_root,
        "--source-sha",
        source_sha,
        "--write-approval-payload",
        str(linked_output),
    )
    assert linked_result.returncode != 0, linked_result.stdout
    assert "already exists" in linked_result.stdout.lower()
