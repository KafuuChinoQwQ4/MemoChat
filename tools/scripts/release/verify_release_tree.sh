#!/usr/bin/env bash
set -Eeuo pipefail

ALLOW_FILE=""
ROOT=""
FAILURES=0
declare -A ALLOWED_PATHS=()

usage() {
    cat <<'USAGE'
Usage: verify_release_tree.sh [--allow-file PATH] RELEASE_ROOT

Fail closed when a release tree contains private keys, literal credentials,
runtime data/logs, developer paths, unresolved dependencies, unsafe file types,
or symlinks that escape the release root. The optional allow file contains exact
root-relative paths whose low-confidence text credential checks may be skipped,
one per line. File type, secret material, developer path, dependency, and ELF
checks remain mandatory; glob and parent traversal entries are rejected.
USAGE
}

fail() {
    echo "[FAIL] $*" >&2
    FAILURES=$((FAILURES + 1))
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --allow-file)
            [[ $# -ge 2 ]] || { echo "[FAIL] --allow-file requires a path" >&2; exit 2; }
            ALLOW_FILE="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --*)
            echo "[FAIL] Unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
        *)
            if [[ -n "$ROOT" ]]; then
                echo "[FAIL] Exactly one release root is required" >&2
                exit 2
            fi
            ROOT="$1"
            shift
            ;;
    esac
done

if [[ -z "$ROOT" || ! -d "$ROOT" || -L "$ROOT" ]]; then
    echo "[FAIL] Release root must be a real directory: ${ROOT:-<empty>}" >&2
    exit 2
fi
for command_name in realpath find grep file readelf ldd env awk; do
    if ! command -v "$command_name" >/dev/null 2>&1; then
        echo "[FAIL] Required release verification command is unavailable: ${command_name}" >&2
        exit 2
    fi
done

ROOT="$(realpath -e -- "$ROOT")"
if [[ "$ROOT" == "/" ]]; then
    echo "[FAIL] Refusing to scan the filesystem root" >&2
    exit 2
fi

if [[ -n "$ALLOW_FILE" ]]; then
    if [[ ! -f "$ALLOW_FILE" || -L "$ALLOW_FILE" ]]; then
        echo "[FAIL] Allowlist must be a regular file: ${ALLOW_FILE}" >&2
        exit 2
    fi
    while IFS= read -r entry || [[ -n "$entry" ]]; do
        entry="${entry%$'\r'}"
        [[ -z "$entry" || "$entry" == \#* ]] && continue
        case "$entry" in
            /*|..|../*|*/../*|*/..|*/|*'*'*|*'?'*|*'['*)
                echo "[FAIL] Unsafe allowlist entry: ${entry}" >&2
                exit 2
                ;;
        esac
        if [[ ! -e "${ROOT}/${entry}" && ! -L "${ROOT}/${entry}" ]]; then
            echo "[FAIL] Allowlist entry does not exist in release tree: ${entry}" >&2
            exit 2
        fi
        ALLOWED_PATHS["$entry"]=1
    done < "$ALLOW_FILE"
fi

is_allowed() {
    [[ -n "${ALLOWED_PATHS[$1]:-}" ]]
}

is_elf() {
    readelf -h -- "$1" >/dev/null 2>&1
}

check_symlinks_and_file_types() {
    local entry relative resolved
    while IFS= read -r -d '' entry; do
        relative="${entry#"${ROOT}/"}"
        check_path_name "$relative" "$entry"
        if [[ -L "$entry" ]]; then
            if ! resolved="$(realpath -e -- "$entry" 2>/dev/null)"; then
                fail "Broken symlink: ${relative}"
                continue
            fi
            if [[ "$resolved" != "$ROOT" && "$resolved" != "$ROOT"/* ]]; then
                fail "Symlink escapes release root: ${relative} -> ${resolved}"
            fi
        elif [[ ! -f "$entry" && ! -d "$entry" ]]; then
            fail "Unsupported filesystem entry: ${relative}"
        fi
    done < <(find "$ROOT" -mindepth 1 -print0)
}

check_path_name() {
    local relative="$1"
    local entry="$2"
    local lower="${relative,,}"
    local base="${lower##*/}"

    if [[ "/${lower}/" =~ /(log|logs|data|credentials|sessions|cookies)/ ]]; then
        fail "Runtime data directory is forbidden: ${relative}"
    fi
    case "$base" in
        .env.example|.env.*.example)
            ;;
        core|core.*)
            if [[ ! -d "$entry" || -L "$entry" ]]; then
                fail "Sensitive or mutable file type is forbidden: ${relative}"
            fi
            ;;
        *.key|*.p12|*.pfx|*.jks|*.log|*.db|*.sqlite|*.sqlite3|*.dump|.env|.env.*|id_rsa|id_ed25519)
            fail "Sensitive or mutable file type is forbidden: ${relative}"
            ;;
    esac
    if [[ "$base" =~ (^|[._-])(credentials?|sessions?|cookies?|tokens?|passwords?)([._-]|$) ]]; then
        case "$lower" in
            apps/server/migrations/postgresql/business/*.sql) ;;
            *) fail "Credential-bearing filename is forbidden: ${relative}" ;;
        esac
    fi
}

check_text_content() {
    local path="$1"
    local relative="$2"
    local allow_generic_credentials="$3"
    local mime_type
    local secret_lines=""
    local developer_path_pattern unresolved_dependency_pattern
    local credential_pattern='(password|passwd|secret|token|cookie|session|credential|authorization|bearer|private[_-]?key|access[_-]?key|api[_-]?key)[[:space:]_]*[:=][[:space:]]*[^[:space:]${}<>]'
    local credential_uri_pattern='[A-Za-z][A-Za-z0-9+.-]*://[^/@[:space:]${}:]+:[^/@[:space:]${}]+@'

    developer_path_pattern='(/root'
    developer_path_pattern="${developer_path_pattern}/code/|/data/(Qt|vcpkg|third_party)/|/opt/(Qt|vcpkg|third_party)/|/home/[^/]+/(code|src|build)/|[A-Za-z]:\\\\Users\\\\)"
    if grep -aEin -- "$developer_path_pattern" "$path" >/dev/null 2>&1; then
        fail "Developer-only absolute path found: ${relative}"
    fi

    mime_type="$(file -b --mime-type -- "$path")"
    if [[ "$mime_type" == text/* || "$mime_type" == application/json || "$mime_type" == application/xml ]]; then
        unresolved_dependency_pattern='(=>[[:space:]]*not found|library[[:space:]]+not found|cannot open shared'
        unresolved_dependency_pattern="${unresolved_dependency_pattern} object file)"
        if grep -aEin -- "$unresolved_dependency_pattern" "$path" >/dev/null 2>&1; then
            fail "Unresolved dependency marker found: ${relative}"
        fi
        if [[ "$allow_generic_credentials" -eq 0 ]]; then
            secret_lines="$(LC_ALL=C tr -d '\042\047' < "$path" | grep -aEin -- "$credential_pattern" 2>/dev/null || true)"
            if [[ -n "$secret_lines" ]]; then
                fail "Literal credential assignment found: ${relative}"
            fi
            if grep -aEin -- "$credential_uri_pattern" "$path" >/dev/null 2>&1; then
                fail "Literal credential embedded in URI: ${relative}"
            fi
        fi
    fi
}

check_high_confidence_secret_material() {
    local path="$1"
    local relative="$2"
    local mime_type
    local token_pattern='AKIA[0-9A-Z]{16}|gh[pousr]_[A-Za-z0-9]{36,}|github_pat_[A-Za-z0-9_]{50,}|sk-[A-Za-z0-9_-]{32,}'

    if grep -aEq -- "$token_pattern" "$path"; then
        fail "High-confidence API credential found: ${relative}"
    fi
    mime_type="$(file -b --mime-type -- "$path")"
    if [[ "$mime_type" == text/* || "$mime_type" == application/json || "$mime_type" == application/xml ]]; then
        if contains_paired_private_key_markers "$path"; then
            fail "Private key material found: ${relative}"
        fi
    elif contains_private_key_pem "$path"; then
        fail "Private key material found: ${relative}"
    fi
}

contains_paired_private_key_markers() {
    LC_ALL=C awk '
        {
            line = $0
            sub(/\r$/, "", line)
            if (match(line, /-----BEGIN ([A-Z0-9]+ )*PRIVATE KEY-----/)) {
                expected_end = substr(line, RSTART, RLENGTH)
                sub(/BEGIN/, "END", expected_end)
                remainder = substr(line, RSTART + RLENGTH)
                if (index(remainder, expected_end) != 0) {
                    found = 1
                    exit
                }
                in_key = 1
                next
            }
            if (in_key && index(line, expected_end) != 0) {
                found = 1
                exit
            }
        }
        END { exit(found ? 0 : 1) }
    ' "$1"
}

contains_private_key_pem() {
    LC_ALL=C awk '
        {
            line = $0
            sub(/\r$/, "", line)
            if (match(line, /-----BEGIN ([A-Z0-9]+ )*PRIVATE KEY-----/)) {
                expected_end = substr(line, RSTART, RLENGTH)
                sub(/BEGIN/, "END", expected_end)
                body_size = 0
                in_key = 1
                next
            }
            if (!in_key) {
                next
            }
            if (index(line, expected_end) != 0) {
                if (body_size >= 64) {
                    found = 1
                    exit
                }
                in_key = 0
                next
            }
            if (line == "" || line ~ /^(Proc-Type|DEK-Info):/) {
                next
            }
            if (line ~ /^[A-Za-z0-9+\/]+={0,2}$/) {
                body_size += length(line)
                next
            }
            in_key = 0
        }
        END { exit(found ? 0 : 1) }
    ' "$1"
}

check_elf_dependencies() {
    local path="$1"
    local relative="$2"
    local dynamic_info ldd_output
    dynamic_info="$(readelf -d -- "$path" 2>&1 || true)"
    if grep -E '\((RPATH|RUNPATH)\)' <<<"$dynamic_info" \
        | grep -Ein -- '(/root/|/home/|/data/|[A-Za-z]:\\)' >/dev/null 2>&1; then
        fail "ELF contains an absolute developer RPATH/RUNPATH: ${relative}"
    fi

    ldd_output="$(/usr/bin/env -i \
        PATH=/usr/bin:/bin \
        LC_ALL=C \
        LANG=C \
        LD_LIBRARY_PATH="${ROOT}/lib:${ROOT}/lib64" \
        /usr/bin/ldd "$path" 2>&1 || true)"
    if grep -Ein -- '=>[[:space:]]*not found' <<<"$ldd_output" >/dev/null 2>&1; then
        fail "ELF has unresolved runtime dependencies: ${relative}"
    fi
}

check_symlinks_and_file_types

while IFS= read -r -d '' path; do
    relative="${path#"${ROOT}/"}"
    allow_generic_credentials=0
    if is_allowed "$relative"; then
        allow_generic_credentials=1
        echo "[ALLOW] Generic text credential checks only: ${relative}"
    fi
    check_high_confidence_secret_material "$path" "$relative"
    check_text_content "$path" "$relative" "$allow_generic_credentials"
    if is_elf "$path"; then
        check_elf_dependencies "$path" "$relative"
    fi
done < <(find "$ROOT" -type f -print0)

if [[ "$FAILURES" -ne 0 ]]; then
    echo "[FAIL] Release tree rejected with ${FAILURES} finding(s): ${ROOT}" >&2
    exit 1
fi

echo "[SUCCESS] Release tree verified: ${ROOT}"
