#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd -P)"
VERIFY_SCRIPT="${SCRIPT_DIR}/verify_release_tree.sh"
LEGAL_VERIFIER="${SCRIPT_DIR}/verify_release_legal.sh"
OUTPUT=""
SOURCE_SHA=""

readonly -a CONFIG_FILES=(
    AccountService/account.ini
    CallService/callgateway.ini
    ChatServer/chatdeliveryworker1.ini
    ChatServer/chatmessageservice1.ini
    ChatServer/chatrelationquery1.ini
    ChatServer/chatrelationservice1.ini
    ChatServer/chatserver1.ini
    LoginService/login.ini
    MediaService/mediagateway.ini
    MomentsService/momentsgateway.ini
    R18Service/r18gateway.ini
    RegisterService/register.ini
    VarifyServer/config.ini
)

usage() {
    cat <<'USAGE'
Usage: package_backend_deployment_kit.sh --output PATH [options]

Create a fresh, repository-shaped backend deployment kit. The kit contains the
fixed release Compose entrypoint, public sanitized INI files, datastore
provisioners, PostgreSQL migrations, and local runtime-image builder inputs. It
contains no private environment file, TLS key, credentials, or mutable data.

Options:
  --output PATH  New output directory. It must not already exist.
  --source-sha SHA
                 Bind packaged legal inputs to this exact clean Git commit.
  -h, --help     Show this help.
USAGE
}

fail() {
    printf '[FAIL] %s\n' "$*" >&2
    exit 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --output)
            [[ $# -ge 2 ]] || fail "--output requires a path"
            OUTPUT="$2"
            shift 2
            ;;
        --source-sha)
            [[ $# -ge 2 ]] || fail "--source-sha requires a value"
            SOURCE_SHA="$2"
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

[[ -n "$OUTPUT" ]] || fail "--output is required"
for command_name in awk find grep install mktemp python3 realpath sha256sum; do
    command -v "$command_name" >/dev/null 2>&1 \
        || fail "required command is missing: ${command_name}"
done
[[ -x "$VERIFY_SCRIPT" ]] || fail "release verifier is missing: $VERIFY_SCRIPT"
[[ -x "$LEGAL_VERIFIER" ]] || fail "legal verifier is missing: $LEGAL_VERIFIER"
declare -a LEGAL_ARGS=(--project-root "$PROJECT_ROOT")
[[ -z "$SOURCE_SHA" ]] || LEGAL_ARGS+=(--source-sha "$SOURCE_SHA")

OUTPUT="$(realpath -m -- "$OUTPUT")"
[[ "$OUTPUT" != "/" && "$OUTPUT" != "$PROJECT_ROOT" ]] \
    || fail "refusing unsafe output path: $OUTPUT"
[[ ! -e "$OUTPUT" ]] || fail "output path already exists; refusing to merge or delete it: $OUTPUT"

OUTPUT_PARENT="$(dirname -- "$OUTPUT")"
mkdir -p -- "$OUTPUT_PARENT"
STAGING="$(mktemp -d -- "${OUTPUT_PARENT}/.memochat-backend-deployment.XXXXXX")"
SCAN_ALLOWLIST="${STAGING}.scan-allowlist"
cleanup() {
    if [[ -n "${STAGING:-}" && -d "$STAGING" ]]; then
        rm -rf -- "$STAGING"
    fi
    if [[ -n "${SCAN_ALLOWLIST:-}" && -f "$SCAN_ALLOWLIST" ]]; then
        rm -f -- "$SCAN_ALLOWLIST"
    fi
}
trap cleanup EXIT

copy_file() {
    local relative_path="$1"
    local mode="$2"
    local source_path="${PROJECT_ROOT}/${relative_path}"
    [[ -f "$source_path" && ! -L "$source_path" ]] \
        || fail "required deployment source is not a regular file: $relative_path"
    install -D -m "$mode" -- "$source_path" "${STAGING}/${relative_path}"
}

copy_directory_files() {
    local relative_source="$1"
    local source_root="${PROJECT_ROOT}/${relative_source}"
    local source_path relative_path mode
    [[ -d "$source_root" && ! -L "$source_root" ]] \
        || fail "required deployment source is not a real directory: $relative_source"
    if find "$source_root" -type l -print -quit | grep -q .; then
        fail "deployment source directory contains a symlink: $relative_source"
    fi
    while IFS= read -r -d '' source_path; do
        relative_path="${source_path#"${PROJECT_ROOT}/"}"
        mode=0444
        [[ "$source_path" == *.sh ]] && mode=0555
        copy_file "$relative_path" "$mode"
    done < <(find "$source_root" -type f ! -name '_TREE.md' -print0)
}

copy_file tools/scripts/release/build_backend_images.sh 0555
copy_file tools/scripts/release/audit_backend_images.sh 0555
copy_file tools/scripts/release/run_release_compose.sh 0555
copy_file tools/scripts/release/verify_release_tree.sh 0555
copy_file infra/deploy/images/services/cpp-service.Dockerfile 0444
copy_file infra/deploy/images/common/entrypoints/server-entrypoint.sh 0555
copy_file infra/deploy/local/.env.release.example 0444
copy_file infra/deploy/local/docker-compose.yml 0444
copy_file infra/deploy/local/compose/backend-services.yml 0444
copy_file infra/deploy/local/compose/livekit.yml 0444
copy_file infra/deploy/local/compose/envoy.yaml 0444
copy_directory_files infra/deploy/local/provision
copy_directory_files infra/deploy/local/observability
copy_directory_files apps/server/migrations/postgresql/business

for relative_config in "${CONFIG_FILES[@]}"; do
    copy_file "apps/server/core/${relative_config}" 0444
done

# The staged Compose file is intentionally read-only in the final kit. Make it
# owner-writable only while pinning the mutable image reference below so this
# also works when the packager runs as an unprivileged CI user.
chmod 0644 "${STAGING}/infra/deploy/local/docker-compose.yml"
python3 - "${STAGING}/infra/deploy/local/docker-compose.yml" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text(encoding="utf-8")
required_tokens = (
    "${MEMOCHAT_REDIS_PASSWORD:?set MEMOCHAT_REDIS_PASSWORD}",
    "${MEMOCHAT_POSTGRES_PASSWORD:?set MEMOCHAT_POSTGRES_PASSWORD}",
    "${MEMOCHAT_MONGO_ROOT_PASSWORD:?set MEMOCHAT_MONGO_ROOT_PASSWORD}",
    "${MEMOCHAT_MONGO_APP_PASSWORD:?set MEMOCHAT_MONGO_APP_PASSWORD}",
    "${MEMOCHAT_MINIO_ROOT_PASSWORD:?set MEMOCHAT_MINIO_ROOT_PASSWORD}",
    "${MEMOCHAT_RABBITMQ_PASSWORD:?set MEMOCHAT_RABBITMQ_PASSWORD}",
    "${MEMOCHAT_INFLUXDB_PASSWORD:-}",
    "${MEMOCHAT_INFLUXDB_ADMIN_TOKEN:-}",
    "${MEMOCHAT_INFLUXDB_GRAFANA_TOKEN:-}",
    "${MEMOCHAT_GRAFANA_ADMIN_PASSWORD:-}",
)
missing = [token for token in required_tokens if token not in text]
if missing:
    raise SystemExit(f"release base Compose has non-externalized credentials: {missing}")
if "minio/minio:latest" not in text:
    raise SystemExit("release base Compose must keep the mutable MinIO tag for packaging pinning")
text = text.replace(
    "minio/minio:latest",
    "minio/minio@sha256:14cea493d9a34af32f524e538b8346cf79f3321eff8e708c1e2960462bd8936e",
)
path.write_text(text, encoding="utf-8")
PY
chmod 0444 "${STAGING}/infra/deploy/local/docker-compose.yml"

for relative_config in "${CONFIG_FILES[@]}"; do
    config_path="${STAGING}/apps/server/core/${relative_config}"
    # Sanitization rewrites the staged copy; keep the final release config
    # read-only after the transformation completes.
    chmod 0644 "$config_path"
    python3 - "$config_path" <<'PY'
from configparser import ConfigParser
from pathlib import Path
import sys

path = Path(sys.argv[1])
config = ConfigParser(interpolation=None, strict=True)
config.optionxform = str
with path.open(encoding="utf-8") as stream:
    config.read_file(stream)
sensitive_keys = {
    "Passwd", "Password", "PfxPassword", "HmacSecret", "JwtSecret",
    "InternalApiKey", "AdminKey", "ApiKey", "ApiSecret", "AccessKey",
    "SecretKey", "SMTPUser", "SMTPPass", "From", "Uri",
}
for section in config.sections():
    for key in sensitive_keys:
        if config.has_option(section, key):
            config.set(section, key, "")
with path.open("w", encoding="utf-8", newline="\n") as stream:
    config.write(stream, space_around_delimiters=False)
PY
    chmod 0444 "$config_path"
    if grep -Eiq 'memochat-dev-|^[[:space:]]*(Passwd|Password|PfxPassword|HmacSecret|JwtSecret|InternalApiKey|AdminKey|ApiKey|ApiSecret|AccessKey|SecretKey|SMTPUser|SMTPPass|From|Uri)[[:space:]]*=[[:space:]]*[^[:space:]]' "$config_path"; then
        fail "sanitized release config still contains a credential: $relative_config"
    fi
done

cat >"${STAGING}/README.md" <<'README'
# MemoChat Backend Deployment Kit

This directory is the self-contained deployment half of the Linux backend
release. The sibling `../backend` directory contains 15 C++ service bundles;
this kit builds their local runtime images and starts the supported 13-service
Compose model. No private environment file, TLS private key, account email,
database content, or runtime log is included here.

From this `deployment` directory, verify every sibling bundle and build all 15
local images before provisioning any datastore:

```bash
tools/scripts/release/build_backend_images.sh \
  --bundle-root ../backend \
  --image-prefix memochat \
  --tag local
```

If the in-image package transaction needs an operator-managed CA, pass that
PEM bundle explicitly with `--builder-ca /absolute/path/to/ca-bundle.pem`.
BuildKit mounts it only for that APT transaction; the bundle is not copied into
the image. This option does not configure the BuildKit daemon's own trust store:
the daemon must already trust the TLS chain used by the earlier remote `ADD`.

The public env template already selects `memochat/*:local`. The release wrapper
checks the complete local image set before its `up` action, so a missing image
fails before database provisioning. To use published registry images instead,
verify and pull every digest from the release manifest first, then set the
matching image prefix/tag in the private env file.

Create a private environment file outside this directory from
`infra/deploy/local/.env.release.example`. Replace all base placeholders and
the placeholders for each profile you enable; inactive profile entries may stay
unchanged or be removed. Set `MEMOCHAT_BACKEND_CONFIG_ROOT` to the absolute path of this kit's
`apps/server/core` directory. Set the TLS certificate/key and Docker data root
to operator-owned absolute paths, then restrict the file to mode 0600.

Validate, provision fresh datastores, and start the stack only through the
bundled wrapper:

```bash
tools/scripts/release/run_release_compose.sh --env-file /absolute/private/release.env check
tools/scripts/release/run_release_compose.sh --env-file /absolute/private/release.env config
tools/scripts/release/run_release_compose.sh --env-file /absolute/private/release.env provision
tools/scripts/release/run_release_compose.sh --env-file /absolute/private/release.env up
```

Optional service groups use the wrapper's repeatable `--profile` option:
`calls`, `r18`, and `observability`. Their credentials, INI mounts, mutable
paths, and the Call database are required/provisioned only when selected. This
C++ backend kit does not contain
AIOrchestrator, Ollama, Qdrant, or Neo4j; `AIGatewayServer` and `AIServer` are
therefore intentionally absent from its Compose model. The bundled public INI
files have all credential fields cleared; runtime secrets are injected from the
private env file. PostgreSQL migrations and the MongoDB/MinIO provisioners are
included at the relative paths used by Compose. The `legal` directory always
records the MIT license, dependency inventory, and machine-readable legal
status. A package marked `third_party_legal_corpus=incomplete` is suitable for
local validation only and is not a formal redistributable release.
README
chmod 0444 "${STAGING}/README.md"

"$LEGAL_VERIFIER" "${LEGAL_ARGS[@]}" --copy-to "${STAGING}/legal"
legal_status="${STAGING}/legal/LEGAL-STATUS.txt"
legal_inventory="$(awk -F= '$1 == "third_party_inventory" { print $2 }' "$legal_status")"
third_party_legal_corpus="$(awk -F= '$1 == "third_party_legal_corpus" { print $2 }' "$legal_status")"
corpus_review_id="$(awk -F= '$1 == "corpus_review_id" { print $2 }' "$legal_status")"
corpus_sha256="$(awk -F= '$1 == "corpus_sha256" { print $2 }' "$legal_status")"
release_source_sha="$(awk -F= '$1 == "release_source_sha" { print $2 }' "$legal_status")"
formal_distribution_ready="$(awk -F= '$1 == "formal_distribution_ready" { print $2 }' "$legal_status")"
legal_status_sha256="$(sha256sum -- "$legal_status" | awk '{print $1}')"
[[ "$legal_inventory" == complete ]] || fail "generated legal inventory status is invalid"
[[ "$third_party_legal_corpus" == complete || "$third_party_legal_corpus" == incomplete ]] \
    || fail "generated third-party legal corpus status is invalid"
[[ "$formal_distribution_ready" == true || "$formal_distribution_ready" == false ]] \
    || fail "generated formal distribution status is invalid"

{
    printf 'format=memochat-backend-deployment-kit-v1\n'
    printf 'compose_root=infra/deploy/local\n'
    printf 'bundle_root=../backend\n'
    printf 'local_image_count=15\n'
    printf 'config_root=apps/server/core\n'
    printf 'migration_root=apps/server/migrations/postgresql/business\n'
    printf 'supported_profiles=calls,r18,observability\n'
    printf 'excluded_components=AIGatewayServer,AIServer,AIOrchestrator,Ollama,Qdrant,Neo4j\n'
    printf 'legal_inventory=%s\n' "$legal_inventory"
    printf 'third_party_legal_corpus=%s\n' "$third_party_legal_corpus"
    printf 'corpus_review_id=%s\n' "$corpus_review_id"
    printf 'corpus_sha256=%s\n' "$corpus_sha256"
    printf 'release_source_sha=%s\n' "$release_source_sha"
    printf 'legal_status_sha256=%s\n' "$legal_status_sha256"
    printf 'formal_distribution_ready=%s\n' "$formal_distribution_ready"
} >"${STAGING}/MANIFEST.txt"
chmod 0444 "${STAGING}/MANIFEST.txt"

printf '%s\n' \
    'infra/deploy/local/.env.release.example' \
    'infra/deploy/local/docker-compose.yml' \
    'infra/deploy/local/compose/backend-services.yml' \
    'infra/deploy/local/compose/livekit.yml' \
    'infra/deploy/local/provision/mongo.js' >"$SCAN_ALLOWLIST"
chmod 0600 "$SCAN_ALLOWLIST"
"$VERIFY_SCRIPT" --allow-file "$SCAN_ALLOWLIST" "$STAGING"

(
    cd -- "$STAGING"
    find . -type f ! -name SHA256SUMS -printf '%P\0' \
        | sort -z \
        | xargs -0 sha256sum >SHA256SUMS
    chmod 0444 SHA256SUMS
    sha256sum --check --strict SHA256SUMS >/dev/null
)
"$VERIFY_SCRIPT" --allow-file "$SCAN_ALLOWLIST" "$STAGING"

[[ ! -e "$OUTPUT" ]] || fail "output path appeared during packaging; refusing to merge: $OUTPUT"
mv -T -- "$STAGING" "$OUTPUT"
STAGING=""
rm -f -- "$SCAN_ALLOWLIST"
SCAN_ALLOWLIST=""
printf '[SUCCESS] backend deployment kit: %s\n' "$OUTPUT"
