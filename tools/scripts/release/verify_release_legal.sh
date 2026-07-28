#!/usr/bin/env bash
set -Eeuo pipefail

umask 077

readonly SCRIPT_NAME="$(basename -- "$0")"
readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly DEFAULT_PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd -P)"
readonly EXPECTED_LICENSE_SHA256="030cbe3ed1aabdf58b3e882cdcb34fb1a397e6116c8418fa01f44d88072bb181"

PROJECT_ROOT="$DEFAULT_PROJECT_ROOT"
COPY_TO=""
STATUS_FILE=""
SOURCE_SHA=""
REQUIRE_DISTRIBUTION_CORPUS=0
APPROVAL_PUBLIC_KEY=""
APPROVAL_SIGNATURE=""
WRITE_APPROVAL_PAYLOAD=""
APPROVAL_PAYLOAD_TEMPLATE=""
APPROVAL_PAYLOAD_OUTPUT_TEMPLATE=""
STATUS_TEMPLATE=""

usage() {
    cat <<'USAGE'
Usage: verify_release_legal.sh [options]

Validate the MemoChat MIT license and third-party inventory. A missing formal
third-party distribution corpus is reported as an explicit incomplete status
for ordinary CI and local packages. A present but invalid corpus always fails.

Options:
  --project-root PATH
      Repository root to inspect. Defaults to the current MemoChat checkout.
  --require-distribution-corpus
      Fail unless legal/third-party is complete and approved for distribution.
  --source-sha SHA
      Bind a complete corpus to this 40-character checkout commit SHA.
  --approval-public-key PATH
      Public key used to verify the canonical v2 approval payload. PATH must
      be a nonempty regular file outside the repository.
  --approval-signature PATH
      OpenSSL SHA-256 detached signature over the canonical v2 approval
      payload. PATH must be a nonempty regular file outside the repository.
      Formal distribution requires this option and --approval-public-key.
  --write-approval-payload PATH
      Write the canonical v2 approval payload for independent signing. PATH
      must be outside the repository and must not already exist. Requires a
      complete corpus, --source-sha, and an exact clean checkout.
  --copy-to PATH
      Create a new legal directory containing LICENSE, THIRD_PARTY_NOTICES.md,
      LEGAL-STATUS.txt, and the verified third-party corpus when complete.
  --status-file PATH
      Write the machine-readable legal status to PATH.
  -h, --help
      Show this help.
USAGE
}

fail() {
    printf '%s: [FAIL] %s\n' "$SCRIPT_NAME" "$*" >&2
    exit 1
}

cleanup() {
    if [[ -n "${APPROVAL_PAYLOAD_TEMPLATE:-}" && -f "$APPROVAL_PAYLOAD_TEMPLATE" ]]; then
        rm -f -- "$APPROVAL_PAYLOAD_TEMPLATE"
    fi
    if [[ -n "${APPROVAL_PAYLOAD_OUTPUT_TEMPLATE:-}" \
        && -f "$APPROVAL_PAYLOAD_OUTPUT_TEMPLATE" ]]; then
        rm -f -- "$APPROVAL_PAYLOAD_OUTPUT_TEMPLATE"
    fi
    if [[ -n "${STATUS_TEMPLATE:-}" && -f "$STATUS_TEMPLATE" ]]; then
        rm -f -- "$STATUS_TEMPLATE"
    fi
}
trap cleanup EXIT

while [[ $# -gt 0 ]]; do
    case "$1" in
        --project-root)
            [[ $# -ge 2 ]] || fail "--project-root requires a path"
            PROJECT_ROOT="$2"
            shift 2
            ;;
        --require-distribution-corpus)
            REQUIRE_DISTRIBUTION_CORPUS=1
            shift
            ;;
        --source-sha)
            [[ $# -ge 2 ]] || fail "--source-sha requires a value"
            SOURCE_SHA="$2"
            shift 2
            ;;
        --approval-public-key)
            [[ $# -ge 2 ]] || fail "--approval-public-key requires a path"
            APPROVAL_PUBLIC_KEY="$2"
            shift 2
            ;;
        --approval-signature)
            [[ $# -ge 2 ]] || fail "--approval-signature requires a path"
            APPROVAL_SIGNATURE="$2"
            shift 2
            ;;
        --write-approval-payload)
            [[ $# -ge 2 ]] || fail "--write-approval-payload requires a path"
            WRITE_APPROVAL_PAYLOAD="$2"
            shift 2
            ;;
        --copy-to)
            [[ $# -ge 2 ]] || fail "--copy-to requires a path"
            COPY_TO="$2"
            shift 2
            ;;
        --status-file)
            [[ $# -ge 2 ]] || fail "--status-file requires a path"
            STATUS_FILE="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            fail "unknown argument: $1"
            ;;
    esac
done

for command_name in awk basename cat chmod cp dirname find grep install ln mkdir mktemp python3 realpath rm sha256sum stat wc; do
    command -v "$command_name" >/dev/null 2>&1 \
        || fail "required command is missing: ${command_name}"
done

[[ -z "$SOURCE_SHA" || "$SOURCE_SHA" =~ ^[0-9a-f]{40}$ ]] \
    || fail "--source-sha must be a lowercase 40-character Git commit SHA"

PROJECT_ROOT="$(realpath -e -- "$PROJECT_ROOT")" \
    || fail "project root does not exist: $PROJECT_ROOT"
[[ -d "$PROJECT_ROOT" ]] || fail "project root is not a directory: $PROJECT_ROOT"

license_path="${PROJECT_ROOT}/LICENSE"
notice_path="${PROJECT_ROOT}/THIRD_PARTY_NOTICES.md"
[[ -f "$license_path" && ! -L "$license_path" ]] \
    || fail "LICENSE must be a regular non-symlink file"
[[ -f "$notice_path" && ! -L "$notice_path" ]] \
    || fail "THIRD_PARTY_NOTICES.md must be a regular non-symlink file"

actual_license_sha256="$(sha256sum -- "$license_path" | awk '{print $1}')"
[[ "$actual_license_sha256" == "$EXPECTED_LICENSE_SHA256" ]] \
    || fail "LICENSE does not match the approved MemoChat MIT license text"
actual_notice_sha256="$(sha256sum -- "$notice_path" | awk '{print $1}')"
[[ "$actual_notice_sha256" =~ ^[0-9a-f]{64}$ ]] \
    || fail "THIRD_PARTY_NOTICES.md digest is invalid"
[[ "$(wc -c < "$notice_path")" -ge 10000 ]] \
    || fail "third-party inventory is not substantive"

readonly -a REQUIRED_NOTICE_MARKERS=(
    '## Linux Client Distribution'
    '## C++ Backend Distribution'
    '## MemoChat Service Container Images'
    '## Separately Obtained Deployment Dependencies'
    '## Required Materials Before Formal Release'
    'Qt WebEngine'
    'FFmpeg'
    'MsQuic'
    'GCC runtime libraries'
)
for marker in "${REQUIRED_NOTICE_MARKERS[@]}"; do
    grep -Fq -- "$marker" "$notice_path" \
        || fail "third-party inventory is missing required marker: ${marker}"
done

corpus_root="${PROJECT_ROOT}/legal/third-party"
corpus_status=incomplete
corpus_review_id=unavailable
corpus_sha256=unavailable
approval_payload_sha256=unavailable
approval_signature_status=unavailable
approval_signature_sha256=unavailable
approval_public_key_fingerprint_sha256=unavailable
release_source_sha=unbound
release_source_tree=unbound
corpus_reason="legal/third-party is absent"

if [[ -e "$corpus_root" ]]; then
    [[ -d "$corpus_root" && ! -L "$corpus_root" ]] \
        || fail "legal/third-party must be a real directory"
    if find "$corpus_root" -type l -print -quit | grep -q .; then
        fail "third-party legal corpus contains a symlink"
    fi
    unsupported_entry="$(find "$corpus_root" -mindepth 1 ! -type d ! -type f -print -quit)"
    [[ -z "$unsupported_entry" ]] \
        || fail "third-party legal corpus contains an unsupported filesystem entry: $unsupported_entry"

    corpus_review_id="$({
        python3 - "$corpus_root" <<'PY'
import hashlib
import json
import re
import sys
from pathlib import Path, PurePosixPath

corpus_root = Path(sys.argv[1])
required_scopes = {
    "backend-vcpkg",
    "client-assets",
    "container-ubuntu",
    "ffmpeg",
    "gcc-runtime",
    "icu",
    "qt",
    "qtwebengine-chromium",
}


def fail(message: str) -> None:
    raise SystemExit(f"third-party legal corpus {message}")


manifest_path = corpus_root / "CORPUS.json"
checksums_path = corpus_root / "SHA256SUMS"
for path in (manifest_path, checksums_path):
    if not path.is_file() or path.is_symlink() or path.stat().st_size == 0:
        fail(f"requires a nonempty regular {path.name}")

try:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
except (OSError, UnicodeError, json.JSONDecodeError) as error:
    fail(f"CORPUS.json is invalid: {error}")
if not isinstance(manifest, dict):
    fail("CORPUS.json must be an object")
allowed_manifest_keys = {"schema", "review_status", "review_id", "scopes"}
unexpected_manifest_keys = sorted(set(manifest) - allowed_manifest_keys)
if unexpected_manifest_keys:
    fail(f"CORPUS.json contains unexpected key(s): {', '.join(unexpected_manifest_keys)}")
if manifest.get("schema") != "memochat-third-party-corpus-v1":
    fail("CORPUS.json schema is not memochat-third-party-corpus-v1")
if manifest.get("review_status") != "approved-for-distribution":
    fail("CORPUS.json review_status is not approved-for-distribution")

review_id = manifest.get("review_id")
if not isinstance(review_id, str) or re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]{7,127}", review_id) is None:
    fail("CORPUS.json review_id must be an 8-128 character stable identifier")

scopes = manifest.get("scopes")
if not isinstance(scopes, dict):
    fail("CORPUS.json scopes must be an object")
missing_scopes = sorted(required_scopes - set(scopes))
if missing_scopes:
    fail(f"CORPUS.json is missing required scope(s): {', '.join(missing_scopes)}")

payload_paths: set[str] = set()
for scope, paths in scopes.items():
    if not isinstance(scope, str) or re.fullmatch(r"[a-z0-9][a-z0-9-]*", scope) is None:
        fail(f"CORPUS.json has an invalid scope name: {scope!r}")
    if not isinstance(paths, list) or not paths:
        fail(f"CORPUS.json scope {scope!r} must contain at least one file")
    for raw_path in paths:
        if not isinstance(raw_path, str) or not raw_path or "\\" in raw_path:
            fail(f"CORPUS.json scope {scope!r} contains an invalid path")
        relative = PurePosixPath(raw_path)
        if relative.is_absolute() or ".." in relative.parts or relative.as_posix() != raw_path:
            fail(f"CORPUS.json scope {scope!r} contains an unsafe path: {raw_path}")
        if raw_path in {"CORPUS.json", "CORPUS.sig", "SHA256SUMS"}:
            fail(f"CORPUS.json scope {scope!r} references reserved metadata: {raw_path}")
        if raw_path in payload_paths:
            fail(f"CORPUS.json references a file more than once: {raw_path}")
        payload_paths.add(raw_path)

checksum_rows: dict[str, str] = {}
try:
    checksum_lines = checksums_path.read_text(encoding="utf-8").splitlines()
except (OSError, UnicodeError) as error:
    fail(f"SHA256SUMS is unreadable: {error}")
for line_number, line in enumerate(checksum_lines, start=1):
    match = re.fullmatch(r"([0-9a-f]{64})  ([^\r\n]+)", line)
    if match is None:
        fail(f"SHA256SUMS line {line_number} is malformed")
    digest, raw_path = match.groups()
    relative = PurePosixPath(raw_path)
    if "\\" in raw_path or relative.is_absolute() or ".." in relative.parts or relative.as_posix() != raw_path:
        fail(f"SHA256SUMS line {line_number} contains an unsafe path")
    if raw_path in checksum_rows:
        fail(f"SHA256SUMS repeats path: {raw_path}")
    checksum_rows[raw_path] = digest

expected_files = {"CORPUS.json", *payload_paths}
actual_files = {
    path.relative_to(corpus_root).as_posix()
    for path in corpus_root.rglob("*")
    if path.is_file()
}
required_files = {"SHA256SUMS", *expected_files}
undeclared = sorted(actual_files - required_files)
missing = sorted(required_files - actual_files)
if undeclared:
    fail(f"contains undeclared file(s): {', '.join(undeclared)}")
if missing:
    fail(f"is missing declared file(s): {', '.join(missing)}")
if set(checksum_rows) != expected_files:
    missing_checksums = sorted(expected_files - set(checksum_rows))
    extra_checksums = sorted(set(checksum_rows) - expected_files)
    fail(
        "SHA256SUMS path set mismatch"
        f"; missing={','.join(missing_checksums) or 'none'}"
        f"; extra={','.join(extra_checksums) or 'none'}"
    )

for raw_path, expected_digest in checksum_rows.items():
    path = corpus_root / raw_path
    if not path.is_file() or path.is_symlink() or path.stat().st_size == 0:
        fail(f"declared file is not a nonempty regular file: {raw_path}")
    actual_digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual_digest != expected_digest:
        fail(f"checksum mismatch for {raw_path}")

print(review_id)
PY
    } 2>&1)" || fail "$corpus_review_id"
    [[ "$corpus_review_id" =~ ^[A-Za-z0-9][A-Za-z0-9._-]{7,127}$ ]] \
        || fail "third-party legal corpus verifier returned an invalid review_id"
    corpus_sha256="$(sha256sum -- "${corpus_root}/SHA256SUMS" | awk '{print $1}')"
    [[ "$corpus_sha256" =~ ^[0-9a-f]{64}$ ]] \
        || fail "third-party legal corpus verifier returned an invalid corpus digest"
    corpus_status=complete
    corpus_reason="contents verified; external release approval is unavailable"
fi

if [[ -n "$APPROVAL_PUBLIC_KEY" && -z "$APPROVAL_SIGNATURE" ]]; then
    fail "--approval-public-key requires --approval-signature"
fi
if [[ -n "$APPROVAL_SIGNATURE" && -z "$APPROVAL_PUBLIC_KEY" ]]; then
    fail "--approval-signature requires --approval-public-key"
fi
if [[ "$REQUIRE_DISTRIBUTION_CORPUS" -eq 1 ]]; then
    [[ -n "$SOURCE_SHA" ]] \
        || fail "formal distribution is blocked: --require-distribution-corpus requires --source-sha"
    [[ "$corpus_status" == complete ]] \
        || fail "formal distribution is blocked: third-party distribution corpus is incomplete (${corpus_reason})"
    [[ -n "$APPROVAL_PUBLIC_KEY" ]] \
        || fail "formal distribution is blocked: --require-distribution-corpus requires --approval-public-key"
    [[ -n "$APPROVAL_SIGNATURE" ]] \
        || fail "formal distribution is blocked: --require-distribution-corpus requires --approval-signature"
fi
if [[ -n "$WRITE_APPROVAL_PAYLOAD" ]]; then
    [[ -n "$SOURCE_SHA" ]] || fail "--write-approval-payload requires --source-sha"
    [[ "$corpus_status" == complete ]] \
        || fail "--write-approval-payload requires a complete third-party distribution corpus"
fi

if [[ -n "$SOURCE_SHA" ]]; then
    command -v git >/dev/null 2>&1 || fail "git is required to bind the legal corpus to --source-sha"
    git_root="$(git -C "$PROJECT_ROOT" rev-parse --show-toplevel 2>/dev/null)" \
        || fail "--source-sha requires project-root to be a Git checkout"
    git_root="$(realpath -e -- "$git_root")"
    [[ "$git_root" == "$PROJECT_ROOT" ]] \
        || fail "project-root must equal the Git checkout root for source binding"
    checkout_head="$(git -C "$PROJECT_ROOT" rev-parse --verify HEAD 2>/dev/null)" \
        || fail "could not resolve checkout HEAD for legal corpus binding"
    [[ "$checkout_head" == "$SOURCE_SHA" ]] \
        || fail "release source SHA does not match checkout HEAD (${checkout_head})"

    git -C "$PROJECT_ROOT" cat-file -e "${SOURCE_SHA}^{commit}" 2>/dev/null \
        || fail "release source SHA does not resolve to a Git commit"
    checkout_drift="$(
        git -C "$PROJECT_ROOT" status --porcelain=v1 --untracked-files=all 2>/dev/null
    )" || fail "could not inspect checkout drift for legal source binding"
    [[ -z "$checkout_drift" ]] \
        || fail "release source binding rejected working tree drift"

    actual_legal_files="$({
        printf '%s\n' LICENSE THIRD_PARTY_NOTICES.md
        if [[ "$corpus_status" == complete ]]; then
            (
                cd -- "$PROJECT_ROOT"
                find legal/third-party -type f -printf '%p\n'
            )
        fi
    } | LC_ALL=C sort)"
    tree_legal_files="$(
        git -C "$PROJECT_ROOT" ls-tree -r --name-only "$SOURCE_SHA" -- \
            LICENSE THIRD_PARTY_NOTICES.md legal/third-party \
            | LC_ALL=C sort
    )" || fail "could not inspect release legal inputs in the source commit"
    [[ "$actual_legal_files" == "$tree_legal_files" ]] \
        || fail "release legal inputs are not exactly represented by the source commit"
    release_source_sha="$SOURCE_SHA"
    release_source_tree="$(
        git -C "$PROJECT_ROOT" rev-parse --verify "${SOURCE_SHA}^{tree}" 2>/dev/null
    )" || fail "could not resolve the release source tree"
    [[ "$release_source_tree" =~ ^([0-9a-f]{40}|[0-9a-f]{64})$ ]] \
        || fail "release source tree has an invalid Git object ID"
fi

if [[ "$corpus_status" == complete && "$release_source_sha" != unbound ]]; then
    APPROVAL_PAYLOAD_TEMPLATE="$(mktemp)"
    cat >"$APPROVAL_PAYLOAD_TEMPLATE" <<EOF
format=memochat-release-legal-approval-v2
release_source_sha=${release_source_sha}
release_source_tree=${release_source_tree}
license_sha256=${actual_license_sha256}
third_party_notices_sha256=${actual_notice_sha256}
corpus_review_id=${corpus_review_id}
corpus_sha256=${corpus_sha256}
EOF
    chmod 0444 "$APPROVAL_PAYLOAD_TEMPLATE"
    approval_payload_sha256="$(sha256sum -- "$APPROVAL_PAYLOAD_TEMPLATE" | awk '{print $1}')"
    [[ "$approval_payload_sha256" =~ ^[0-9a-f]{64}$ ]] \
        || fail "canonical legal approval payload digest is invalid"
fi

if [[ -n "$WRITE_APPROVAL_PAYLOAD" ]]; then
    approval_payload_output_input="$WRITE_APPROVAL_PAYLOAD"
    [[ ! -e "$approval_payload_output_input" && ! -L "$approval_payload_output_input" ]] \
        || fail "approval payload output already exists"
    approval_payload_output_parent="$(
        realpath -e -- "$(dirname -- "$approval_payload_output_input")"
    )" || fail "approval payload output parent does not exist"
    [[ -d "$approval_payload_output_parent" ]] \
        || fail "approval payload output parent is not a directory"
    WRITE_APPROVAL_PAYLOAD="$(
        realpath -m -- "${approval_payload_output_parent}/$(basename -- "$approval_payload_output_input")"
    )"
    [[ "$WRITE_APPROVAL_PAYLOAD" != "$PROJECT_ROOT" \
        && "$WRITE_APPROVAL_PAYLOAD" != "$PROJECT_ROOT/"* ]] \
        || fail "approval payload output must be outside the repository trust boundary"
    [[ ! -e "$WRITE_APPROVAL_PAYLOAD" && ! -L "$WRITE_APPROVAL_PAYLOAD" ]] \
        || fail "approval payload output already exists"
fi

if [[ -n "$APPROVAL_PUBLIC_KEY" ]]; then
    [[ -n "$APPROVAL_PAYLOAD_TEMPLATE" && -f "$APPROVAL_PAYLOAD_TEMPLATE" ]] \
        || fail "external legal approval requires a complete source-bound approval payload"
    command -v openssl >/dev/null 2>&1 \
        || fail "openssl is required to verify the external legal approval signature"

    approval_public_key_input="$APPROVAL_PUBLIC_KEY"
    [[ ! -L "$approval_public_key_input" ]] \
        || fail "approval public key must not be a symlink"
    APPROVAL_PUBLIC_KEY="$(realpath -e -- "$approval_public_key_input")" \
        || fail "approval public key does not exist: $approval_public_key_input"
    [[ -f "$APPROVAL_PUBLIC_KEY" && -s "$APPROVAL_PUBLIC_KEY" ]] \
        || fail "approval public key must be a nonempty regular non-symlink file"
    [[ "$APPROVAL_PUBLIC_KEY" != "$PROJECT_ROOT" && "$APPROVAL_PUBLIC_KEY" != "$PROJECT_ROOT/"* ]] \
        || fail "approval public key must be provided from outside the repository trust boundary"

    approval_signature_input="$APPROVAL_SIGNATURE"
    [[ ! -L "$approval_signature_input" ]] \
        || fail "approval signature must not be a symlink"
    APPROVAL_SIGNATURE="$(realpath -e -- "$approval_signature_input")" \
        || fail "approval signature does not exist: $approval_signature_input"
    [[ -f "$APPROVAL_SIGNATURE" && -s "$APPROVAL_SIGNATURE" ]] \
        || fail "approval signature must be a nonempty regular non-symlink file"
    [[ "$APPROVAL_SIGNATURE" != "$PROJECT_ROOT" && "$APPROVAL_SIGNATURE" != "$PROJECT_ROOT/"* ]] \
        || fail "approval signature must be provided from outside the repository trust boundary"

    openssl pkey -pubin -in "$APPROVAL_PUBLIC_KEY" -noout >/dev/null 2>&1 \
        || fail "approval public key is not a valid OpenSSL public key"
    approval_public_key_fingerprint_sha256="$(
        openssl pkey -pubin -in "$APPROVAL_PUBLIC_KEY" -outform DER 2>/dev/null \
            | sha256sum | awk '{print $1}'
    )" || fail "could not fingerprint the approval public key"
    [[ "$approval_public_key_fingerprint_sha256" =~ ^[0-9a-f]{64}$ ]] \
        || fail "approval public key fingerprint is invalid"
    approval_signature_sha256="$(sha256sum -- "$APPROVAL_SIGNATURE" | awk '{print $1}')"
    [[ "$approval_signature_sha256" =~ ^[0-9a-f]{64}$ ]] \
        || fail "approval signature digest is invalid"
    openssl dgst -sha256 \
        -verify "$APPROVAL_PUBLIC_KEY" \
        -signature "$APPROVAL_SIGNATURE" \
        "$APPROVAL_PAYLOAD_TEMPLATE" >/dev/null 2>&1 \
        || fail "external legal approval signature verification failed"
    approval_signature_status=verified
    corpus_reason="contents verified and externally approved for the exact source tree"
fi

formal_ready=false
if [[ "$corpus_status" == complete \
    && "$approval_signature_status" == verified \
    && "$release_source_sha" != unbound \
    && "$release_source_tree" != unbound ]]; then
    formal_ready=true
fi

if [[ -n "$STATUS_FILE" ]]; then
    STATUS_FILE="$(realpath -m -- "$STATUS_FILE")"
    status_parent="$(dirname -- "$STATUS_FILE")"
    [[ -d "$status_parent" ]] || fail "status-file parent does not exist: $status_parent"
    [[ ! -e "$STATUS_FILE" && ! -L "$STATUS_FILE" ]] \
        || fail "status-file path already exists: $STATUS_FILE"
    [[ "$STATUS_FILE" != "$corpus_root" && "$STATUS_FILE" != "$corpus_root/"* ]] \
        || fail "status-file cannot be written inside the verified corpus"
fi

if [[ -n "$COPY_TO" ]]; then
    COPY_TO="$(realpath -m -- "$COPY_TO")"
    copy_parent="$(dirname -- "$COPY_TO")"
    [[ "$COPY_TO" != "/" && "$COPY_TO" != "$PROJECT_ROOT" ]] \
        || fail "refusing unsafe legal copy destination: $COPY_TO"
    [[ -d "$copy_parent" ]] || fail "legal copy destination parent does not exist: $copy_parent"
    [[ ! -e "$COPY_TO" && ! -L "$COPY_TO" ]] \
        || fail "legal copy destination already exists: $COPY_TO"
    [[ "$COPY_TO" != "$corpus_root" && "$COPY_TO" != "$corpus_root/"* ]] \
        || fail "legal copy destination cannot be inside the verified corpus"
fi

if [[ "$REQUIRE_DISTRIBUTION_CORPUS" -eq 1 ]]; then
    [[ "$release_source_sha" == "$SOURCE_SHA" ]] \
        || fail "formal distribution is blocked: legal approval is not bound to the checkout commit"
    [[ "$formal_ready" == true ]] \
        || fail "formal distribution is blocked: exact-source external legal approval is not verified"
fi

STATUS_TEMPLATE="$(mktemp)"
cat >"$STATUS_TEMPLATE" <<EOF
format=memochat-distribution-legal-status-v2
project_license=complete
third_party_inventory=complete
third_party_legal_corpus=${corpus_status}
corpus_review_id=${corpus_review_id}
corpus_sha256=${corpus_sha256}
approval_payload_sha256=${approval_payload_sha256}
approval_signature_status=${approval_signature_status}
approval_signature_sha256=${approval_signature_sha256}
approval_public_key_fingerprint_sha256=${approval_public_key_fingerprint_sha256}
release_source_sha=${release_source_sha}
release_source_tree=${release_source_tree}
formal_distribution_ready=${formal_ready}
EOF
chmod 0444 "$STATUS_TEMPLATE"

if [[ -n "$WRITE_APPROVAL_PAYLOAD" ]]; then
    approval_payload_output_parent="$(dirname -- "$WRITE_APPROVAL_PAYLOAD")"
    APPROVAL_PAYLOAD_OUTPUT_TEMPLATE="$(
        mktemp "${approval_payload_output_parent}/.memochat-legal-approval.XXXXXX"
    )" || fail "could not create approval payload output staging file"
    cat -- "$APPROVAL_PAYLOAD_TEMPLATE" >"$APPROVAL_PAYLOAD_OUTPUT_TEMPLATE" \
        || fail "could not stage canonical legal approval payload"
    chmod 0444 "$APPROVAL_PAYLOAD_OUTPUT_TEMPLATE"
    if ! ln -- "$APPROVAL_PAYLOAD_OUTPUT_TEMPLATE" "$WRITE_APPROVAL_PAYLOAD"; then
        if [[ -e "$WRITE_APPROVAL_PAYLOAD" || -L "$WRITE_APPROVAL_PAYLOAD" ]]; then
            fail "approval payload output already exists"
        fi
        fail "could not atomically create approval payload output"
    fi
    rm -f -- "$APPROVAL_PAYLOAD_OUTPUT_TEMPLATE"
    APPROVAL_PAYLOAD_OUTPUT_TEMPLATE=""
fi

if [[ -n "$STATUS_FILE" ]]; then
    install -m 0444 -- "$STATUS_TEMPLATE" "$STATUS_FILE"
fi

if [[ -n "$COPY_TO" ]]; then
    mkdir -m 0700 -- "$COPY_TO"
    install -m 0444 -- "$license_path" "${COPY_TO}/LICENSE"
    install -m 0444 -- "$notice_path" "${COPY_TO}/THIRD_PARTY_NOTICES.md"
    install -m 0444 -- "$STATUS_TEMPLATE" "${COPY_TO}/LEGAL-STATUS.txt"
    if [[ "$corpus_status" == complete ]]; then
        cp -a -- "$corpus_root" "${COPY_TO}/third-party"
        find "${COPY_TO}/third-party" -type d -exec chmod 0555 {} +
        find "${COPY_TO}/third-party" -type f -exec chmod 0444 {} +
    fi
    if [[ -n "$APPROVAL_PAYLOAD_TEMPLATE" ]]; then
        mkdir -m 0700 -- "${COPY_TO}/approval"
        install -m 0444 -- "$APPROVAL_PAYLOAD_TEMPLATE" \
            "${COPY_TO}/approval/APPROVAL-PAYLOAD.txt"
        if [[ "$approval_signature_status" == verified ]]; then
            install -m 0444 -- "$APPROVAL_SIGNATURE" \
                "${COPY_TO}/approval/APPROVAL.sig"
        fi
        chmod 0555 "${COPY_TO}/approval"
    fi
    chmod 0555 "$COPY_TO"
fi

printf '[OK] project license: complete\n'
printf '[OK] third-party inventory: complete\n'
if [[ "$corpus_status" == complete ]]; then
    printf '[OK] third-party distribution corpus: complete (%s)\n' "$corpus_review_id"
    printf '[%s] external legal approval signature: %s\n' \
        "$([[ "$approval_signature_status" == verified ]] && printf 'OK' || printf 'WARN')" \
        "$approval_signature_status"
    printf '[%s] release source binding: %s\n' \
        "$([[ "$release_source_sha" != unbound ]] && printf 'OK' || printf 'WARN')" \
        "$release_source_sha"
else
    printf '[WARN] third-party distribution corpus: incomplete (%s)\n' "$corpus_reason"
fi
if [[ -n "$WRITE_APPROVAL_PAYLOAD" ]]; then
    printf '[OK] canonical legal approval payload: %s\n' "$WRITE_APPROVAL_PAYLOAD"
fi
