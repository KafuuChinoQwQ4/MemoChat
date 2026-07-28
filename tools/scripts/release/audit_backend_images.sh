#!/usr/bin/env bash
set -Eeuo pipefail

umask 077

IMAGE_PREFIX="memochat"
IMAGE_TAG="local"
IMAGE_MANIFEST=""
OUTPUT_DIR=""
DOCKER_BIN="docker"
SYFT_BIN=""
SYFT_CONFIG_PATH=""
GRYPE_BIN=""
GRYPE_CONFIG_PATH=""
FAIL_ON=""
VCPKG_SBOM_DIR=""
SOURCE_SHA=""
MANIFEST_SHA256=""
LEGAL_STATUS_SHA256=""
LEGAL_CORPUS_SHA256=""
LEGAL_CORPUS_REVIEW_ID=""
PULL_IMAGES=0
TAG_SET=0
OUTPUT_CREATED=0
VULNERABILITY_POLICY_FAILURES=0

readonly EXPECTED_UBUNTU_SNAPSHOT="20260727T000000Z"
readonly EXPECTED_UBUNTU_CA_BOOTSTRAP_SHA256="6bac2a01979e210d9eac1d4d56747ec709ea60654744d66705dc3c36e7629e50"
readonly EXPECTED_UBUNTU_RUNTIME_PACKAGES="ca-certificates=20260601~24.04.1,libturbojpeg=1:2.1.5-2ubuntu2,libwebp7=1.3.2-0.4build3"

readonly -a TARGET_ROWS=(
    "AIGatewayServer|ai-gateway"
    "AIServer|ai-server"
    "AccountServer|account-server"
    "CallGatewayServer|call-gateway"
    "ChatDeliveryWorker|chat-delivery-worker"
    "ChatMessageService|chat-message-service"
    "ChatRelationQueryService|chat-relation-query-service"
    "ChatRelationServiceWorker|chat-relation-service-worker"
    "ChatServer|chat-server"
    "LoginServer|login-server"
    "MediaGatewayServer|media-gateway"
    "MomentsGatewayServer|moments-gateway"
    "R18GatewayServer|r18-gateway"
    "RegisterServer|register-server"
    "VarifyServer|varify-server"
)

declare -A IMAGE_BY_TARGET=()
declare -A REFERENCE_BY_TARGET=()
declare -A DIGEST_BY_TARGET=()
declare -A BUNDLE_SHA_BY_TARGET=()
declare -A VCPKG_SBOM_SHA_BY_TARGET=()
declare -A LEGAL_STATUS_SHA_BY_TARGET=()

usage() {
    cat <<'USAGE'
Usage: audit_backend_images.sh --output-dir PATH --syft PATH --grype PATH \
    --fail-on SEVERITY [options]

Audit the fixed 15-image MemoChat C++ backend release topology, generate SPDX
JSON SBOMs with caller-supplied Syft, and enforce the selected Grype severity
threshold against every finding, including vulnerabilities without a known fix.
The caller owns scanner installation and the explicit Grype database update.
This script disables scanner auto-updates and records the database status used.

Required:
  --output-dir PATH       New directory for SBOMs, reports, and checksums.
  --syft PATH             Syft executable selected and versioned by the caller.
  --grype PATH            Grype executable selected and versioned by the caller.
  --fail-on SEVERITY      Grype threshold: negligible, low, medium, high, critical.

Image selection:
  --image-manifest PATH   backend-images.json with immutable image digests.
  --vcpkg-sbom-dir PATH   Directory containing one commit-bound
                          SLUG.vcpkg.spdx.json per manifest image. Required with
                          --image-manifest.
  --pull                  Pull every digest reference before inspection; requires
                          --image-manifest.
  --image-prefix NAME     Registry/repository prefix. Default: memochat
  --tag TAG               Local image tag when no manifest is supplied. Default:
                          local. Cannot be combined with --image-manifest.

Other:
  --docker PATH           Docker executable. Default: docker from PATH.
  -h, --help              Show this help.

Release CI must use --image-manifest and --pull. Tag mode exists for local image
validation only. A failed audit leaves partial evidence without AUDIT_COMPLETE.
USAGE
}

fail() {
    printf '[FAIL] %s\n' "$*" >&2
    if [[ "$OUTPUT_CREATED" -eq 1 ]]; then
        printf '[INFO] incomplete audit evidence retained at %s\n' "$OUTPUT_DIR" >&2
    fi
    exit 1
}

resolve_executable() {
    local requested="$1"
    local label="$2"
    local resolved=""

    [[ -n "$requested" && "$requested" != -* && "$requested" != *$'\n'* ]] \
        || fail "unsafe ${label} executable: ${requested}"
    if [[ "$requested" == */* ]]; then
        resolved="$(realpath -e -- "$requested" 2>/dev/null)" \
            || fail "${label} executable does not exist: ${requested}"
    else
        resolved="$(command -v "$requested" 2>/dev/null)" \
            || fail "${label} executable is unavailable: ${requested}"
        resolved="$(realpath -e -- "$resolved" 2>/dev/null)" \
            || fail "${label} executable could not be resolved: ${requested}"
    fi
    [[ -f "$resolved" && -x "$resolved" ]] \
        || fail "${label} executable is not a regular executable file: ${resolved}"
    printf '%s\n' "$resolved"
}

tool_version() {
    local executable="$1"
    local label="$2"
    local version=""

    version="$("$executable" version 2>&1)" \
        || fail "${label} version probe failed"
    version="${version//$'\r'/ }"
    version="${version//$'\n'/ }"
    version="${version//=/-}"
    [[ -n "$version" ]] || fail "${label} version probe returned no version"
    printf '%.256s\n' "$version"
}

validate_spdx_json() {
    local sbom_path="$1"
    local label="$2"

    [[ -f "$sbom_path" && ! -L "$sbom_path" && -s "$sbom_path" ]] \
        || fail "${label} is not a regular non-empty SPDX JSON file"
    "$JQ_BIN" -e '
        (.spdxVersion | type == "string" and startswith("SPDX-")) and
        (.packages | type == "array" and length > 0)
    ' "$sbom_path" >/dev/null 2>&1 \
        || fail "invalid SPDX JSON for ${label}"
}

scan_sbom_with_grype() {
    local sbom_path="$1"
    local grype_path="$2"
    local label="$3"
    local scan_status=0
    local violation_count=0

    env GRYPE_CHECK_FOR_APP_UPDATE=false GRYPE_DB_AUTO_UPDATE=false \
        "$GRYPE_BIN" \
            "sbom:${sbom_path}" \
            --config "$GRYPE_CONFIG_PATH" \
            --fail-on "$FAIL_ON" \
            --output json \
            --file "$grype_path" \
        || scan_status=$?
    [[ -f "$grype_path" && ! -L "$grype_path" && -s "$grype_path" ]] \
        || fail "Grype did not produce a regular non-empty JSON report for ${label}"
    "$JQ_BIN" -e '.matches | type == "array"' "$grype_path" >/dev/null 2>&1 \
        || fail "invalid Grype JSON for ${label}"
    violation_count="$(
        "$JQ_BIN" --arg threshold "$FAIL_ON" '
            def rank:
                ascii_downcase as $severity
                | {negligible: 0, unknown: 0, low: 1, medium: 2, high: 3, critical: 4}[$severity] // 0;
            ({negligible: 0, low: 1, medium: 2, high: 3, critical: 4}[$threshold]) as $minimum
            | [.matches[] | select((.vulnerability.severity // "unknown" | rank) >= $minimum)]
            | length
        ' "$grype_path"
    )"
    [[ "$violation_count" =~ ^[0-9]+$ ]] \
        || fail "could not evaluate Grype severity policy for ${label}"
    if [[ "$scan_status" -ne 0 && "$scan_status" -ne 2 ]]; then
        fail "Grype execution failed for ${label} (exit=${scan_status})"
    fi
    if [[ "$violation_count" -gt 0 ]]; then
        VULNERABILITY_POLICY_FAILURES=$((VULNERABILITY_POLICY_FAILURES + violation_count))
        printf '%s\t%s\t%s\n' "$label" "$FAIL_ON" "$violation_count" \
            >> "${OUTPUT_DIR}/POLICY_FAILURES.tsv"
    elif [[ "$scan_status" -ne 0 ]]; then
        fail "Grype returned a policy failure without matching findings for ${label}"
    fi
}

is_injected_value() {
    local value="$1"
    [[ "$value" =~ ^\$\{[A-Za-z_][A-Za-z0-9_]*\}$ ]]
}

is_sensitive_name() {
    local upper_name="$1"
    [[ "$upper_name" =~ (^|_)(PASSWORD|PASSWD|TOKEN|COOKIE|CREDENTIALS?|SECRET|AUTHORIZATION|API_?KEY|ACCESS_?KEY|PRIVATE_?KEY|SIGNING_?(KEY|SECRET)|MASTER_?(KEY|SECRET)|ENCRYPTION_?(KEY|SECRET)|SESSION_?KEY|TLS_?KEY|SSH_?KEY|HMAC_?(KEY|SECRET)|JWT_?(KEY|SECRET)|CLIENT_?SECRET|API_?SECRET|SECRET_?KEY)(_|$) ]]
}

validate_manifest() {
    local requested_manifest="$IMAGE_MANIFEST"
    local target=""
    local slug=""
    local count=""
    local image=""
    local tag=""
    local digest=""
    local bundle_sha256=""
    local vcpkg_sbom_sha256=""
    local legal_status_sha256=""
    local expected_image=""

    [[ -f "$requested_manifest" && ! -L "$requested_manifest" ]] \
        || fail "image manifest is missing or unsafe: ${requested_manifest}"
    IMAGE_MANIFEST="$(realpath -e -- "$requested_manifest" 2>/dev/null)" \
        || fail "image manifest cannot be resolved: ${requested_manifest}"
    [[ -f "$IMAGE_MANIFEST" ]] || fail "image manifest is not a file: ${IMAGE_MANIFEST}"

    "$JQ_BIN" -e '
        type == "object" and
        .schema == "memochat-backend-images-v1" and
        (.source_sha | type == "string") and
        (.archive | type == "object") and
        (.archive.artifact | type == "string") and
        (.archive.sha256 | type == "string") and
        (.legal | type == "object") and
        (.legal.release_source_sha | type == "string") and
        (.legal.formal_distribution_ready == true) and
        (.legal.third_party_legal_corpus == "complete") and
        (.legal.corpus_review_id | type == "string") and
        (.legal.corpus_sha256 | type == "string") and
        (.legal.status_sha256 | type == "string") and
        (.images | type == "array" and length == 15) and
        all(.images[];
            type == "object" and
            (.target | type == "string") and
            (.image | type == "string") and
            (.tag | type == "string") and
            (.digest | type == "string") and
            (.bundle_sha256 | type == "string") and
            (.vcpkg_sbom_sha256 | type == "string") and
            (.legal_status_sha256 | type == "string")
        )
    ' "$IMAGE_MANIFEST" >/dev/null 2>&1 \
        || fail "invalid backend image manifest structure"

    SOURCE_SHA="$("$JQ_BIN" -r '.source_sha' "$IMAGE_MANIFEST")"
    [[ "$SOURCE_SHA" =~ ^[0-9a-f]{40}$ ]] \
        || fail "invalid source SHA in backend image manifest"
    "$JQ_BIN" -e --arg source_sha "$SOURCE_SHA" '
        .legal.release_source_sha == $source_sha and
        (.legal.corpus_review_id | test("^[A-Za-z0-9][A-Za-z0-9._-]{7,127}$")) and
        (.legal.corpus_sha256 | test("^[0-9a-f]{64}$")) and
        (.legal.status_sha256 | test("^[0-9a-f]{64}$"))
    ' "$IMAGE_MANIFEST" >/dev/null \
        || fail "backend image manifest legal summary is not formal or source-bound"
    LEGAL_STATUS_SHA256="$("$JQ_BIN" -r '.legal.status_sha256' "$IMAGE_MANIFEST")"
    LEGAL_CORPUS_SHA256="$("$JQ_BIN" -r '.legal.corpus_sha256' "$IMAGE_MANIFEST")"
    LEGAL_CORPUS_REVIEW_ID="$("$JQ_BIN" -r '.legal.corpus_review_id' "$IMAGE_MANIFEST")"
    "$JQ_BIN" -e \
        --arg expected_artifact "MemoChat-backend-${SOURCE_SHA:0:12}-linux-x86_64.tar.gz" \
        '(.archive.artifact == $expected_artifact)
         and (.archive.sha256 | test("^[0-9a-f]{64}$"))
         and ([.images[].target] | unique | length == 15)
         and ([.images[].image] | unique | length == 15)
         and ([.images[].digest] | unique | length == 15)' \
        "$IMAGE_MANIFEST" >/dev/null \
        || fail "backend image manifest contains duplicate or invalid bindings"

    for row in "${TARGET_ROWS[@]}"; do
        IFS='|' read -r target slug <<< "$row"
        count="$("$JQ_BIN" --arg target "$target" \
            '[.images[] | select(.target == $target)] | length' "$IMAGE_MANIFEST")"
        [[ "$count" == 1 ]] || fail "manifest target set mismatch: ${target}"
        image="$("$JQ_BIN" -r --arg target "$target" \
            '.images[] | select(.target == $target) | .image' "$IMAGE_MANIFEST")"
        tag="$("$JQ_BIN" -r --arg target "$target" \
            '.images[] | select(.target == $target) | .tag' "$IMAGE_MANIFEST")"
        digest="$("$JQ_BIN" -r --arg target "$target" \
            '.images[] | select(.target == $target) | .digest' "$IMAGE_MANIFEST")"
        bundle_sha256="$("$JQ_BIN" -r --arg target "$target" \
            '.images[] | select(.target == $target) | .bundle_sha256' "$IMAGE_MANIFEST")"
        vcpkg_sbom_sha256="$("$JQ_BIN" -r --arg target "$target" \
            '.images[] | select(.target == $target) | .vcpkg_sbom_sha256' "$IMAGE_MANIFEST")"
        legal_status_sha256="$("$JQ_BIN" -r --arg target "$target" \
            '.images[] | select(.target == $target) | .legal_status_sha256' "$IMAGE_MANIFEST")"
        expected_image="${IMAGE_PREFIX}/${slug}"
        [[ "$image" == "$expected_image" ]] \
            || fail "image mapping mismatch for ${target}: expected ${expected_image}"
        [[ "$tag" == "sha-${SOURCE_SHA}" ]] \
            || fail "immutable tag binding mismatch for ${target}"
        [[ "$digest" =~ ^sha256:[0-9a-f]{64}$ ]] \
            || fail "invalid image digest for ${target}"
        [[ "$bundle_sha256" =~ ^[0-9a-f]{64}$ ]] \
            || fail "invalid bundle digest for ${target}"
        [[ "$vcpkg_sbom_sha256" =~ ^[0-9a-f]{64}$ ]] \
            || fail "invalid vcpkg SBOM digest for ${target}"
        [[ "$legal_status_sha256" == "$LEGAL_STATUS_SHA256" ]] \
            || fail "legal status digest mismatch for ${target}"
        IMAGE_BY_TARGET["$target"]="$image"
        REFERENCE_BY_TARGET["$target"]="${image}@${digest}"
        DIGEST_BY_TARGET["$target"]="$digest"
        BUNDLE_SHA_BY_TARGET["$target"]="$bundle_sha256"
        VCPKG_SBOM_SHA_BY_TARGET["$target"]="$vcpkg_sbom_sha256"
        LEGAL_STATUS_SHA_BY_TARGET["$target"]="$legal_status_sha256"
    done
    MANIFEST_SHA256="$("$SHA256SUM_BIN" "$IMAGE_MANIFEST" | awk '{print $1}')"
}

validate_vcpkg_sboms() {
    local requested_root="$VCPKG_SBOM_DIR"
    local entry_count=""
    local target=""
    local slug=""
    local source_path=""
    local expected_sha256=""
    local actual_sha256=""

    [[ -d "$requested_root" && ! -L "$requested_root" ]] \
        || fail "vcpkg SBOM directory is missing or unsafe: ${requested_root}"
    VCPKG_SBOM_DIR="$(realpath -e -- "$requested_root" 2>/dev/null)" \
        || fail "vcpkg SBOM directory cannot be resolved: ${requested_root}"
    if find "$VCPKG_SBOM_DIR" -mindepth 1 -type l -print -quit | grep -q .; then
        fail "vcpkg SBOM directory contains a symlink"
    fi
    entry_count="$(find "$VCPKG_SBOM_DIR" -mindepth 1 -maxdepth 1 -printf . | wc -c)"
    [[ "$entry_count" -eq "${#TARGET_ROWS[@]}" ]] \
        || fail "vcpkg SBOM directory must contain exactly ${#TARGET_ROWS[@]} service SBOMs"

    for row in "${TARGET_ROWS[@]}"; do
        IFS='|' read -r target slug <<< "$row"
        source_path="${VCPKG_SBOM_DIR}/${slug}.vcpkg.spdx.json"
        validate_spdx_json "$source_path" "vcpkg dependency SBOM for ${target}"
        "$JQ_BIN" -e --arg source_sha "$SOURCE_SHA" '
            .documentComment == (
                "release_source_sha=" + $source_sha +
                "; coverage=installed-closure-overapproximation"
            )
        ' "$source_path" >/dev/null \
            || fail "vcpkg dependency SBOM source binding mismatch for ${target}"
        expected_sha256="${VCPKG_SBOM_SHA_BY_TARGET[$target]}"
        actual_sha256="$("$SHA256SUM_BIN" "$source_path" | awk '{print $1}')"
        [[ "$actual_sha256" == "$expected_sha256" ]] \
            || fail "vcpkg SBOM digest mismatch for ${target}"
    done
}

configure_tag_references() {
    local target=""
    local slug=""
    local image=""

    for row in "${TARGET_ROWS[@]}"; do
        IFS='|' read -r target slug <<< "$row"
        image="${IMAGE_PREFIX}/${slug}"
        IMAGE_BY_TARGET["$target"]="$image"
        REFERENCE_BY_TARGET["$target"]="${image}:${IMAGE_TAG}"
        DIGEST_BY_TARGET["$target"]=""
        BUNDLE_SHA_BY_TARGET["$target"]=""
        VCPKG_SBOM_SHA_BY_TARGET["$target"]=""
        LEGAL_STATUS_SHA_BY_TARGET["$target"]=""
    done
}

inspect_image_config() {
    local target="$1"
    local slug="$2"
    local reference="$3"
    local expected_digest="$4"
    local expected_bundle_sha256="$5"
    local expected_vcpkg_sbom_sha256="$6"
    local expected_legal_status_sha256="$7"
    local inspect_json=""
    local config_json=""
    local actual_user=""
    local expected_repo_digest=""
    local env_entry=""
    local env_name=""
    local env_value=""
    local upper_name=""
    local upper_value=""
    local service_value=""
    local release_mode_value=""
    local allow_dev_secrets_value=""
    local actual_ubuntu_snapshot=""
    local actual_ubuntu_ca_bootstrap_sha256=""
    local actual_ubuntu_runtime_packages=""
    local service_count=0
    local release_mode_count=0
    local allow_dev_secrets_count=0
    local -a image_env=()

    if ! inspect_json="$(
        "$DOCKER_BIN" image inspect --format '{{json .}}' "$reference" 2>/dev/null
    )"; then
        fail "image is missing or cannot be inspected: ${reference}"
    fi
    "$JQ_BIN" -e '
        type == "object" and
        (.Config | type == "object") and
        (.Id | type == "string" and test("^sha256:[0-9a-f]{64}$")) and
        ((.RepoDigests // []) | type == "array" and all(.[]; type == "string"))
    ' <<< "$inspect_json" >/dev/null 2>&1 \
        || fail "Docker returned invalid image inspect JSON: ${reference}"
    config_json="$("$JQ_BIN" -c '.Config' <<< "$inspect_json")"

    if [[ -n "$expected_digest" ]]; then
        expected_repo_digest="${IMAGE_PREFIX}/${slug}@${expected_digest}"
        "$JQ_BIN" -e --arg expected "$expected_repo_digest" \
            '(.RepoDigests // []) | index($expected) != null' \
            <<< "$inspect_json" >/dev/null \
            || fail "RepoDigest binding mismatch for ${target}: ${expected_repo_digest}"
        "$JQ_BIN" -e \
            --arg source_sha "$SOURCE_SHA" \
            --arg target "$target" \
            --arg bundle_sha256 "$expected_bundle_sha256" \
            --arg vcpkg_sbom_sha256 "$expected_vcpkg_sbom_sha256" \
            --arg legal_status_sha256 "$expected_legal_status_sha256" \
            '(.Config.Labels // {}) as $labels |
             $labels["org.opencontainers.image.revision"] == $source_sha and
             $labels["io.memochat.service.target"] == $target and
             $labels["io.memochat.bundle.sha256"] == $bundle_sha256 and
             $labels["io.memochat.vcpkg.sbom.sha256"] == $vcpkg_sbom_sha256 and
             $labels["io.memochat.legal.status.sha256"] == $legal_status_sha256' \
            <<< "$inspect_json" >/dev/null \
            || fail "image provenance label binding mismatch for ${target}"
    fi

    "$JQ_BIN" -e '(.Labels // {}) | type == "object"' \
        <<< "$config_json" >/dev/null \
        || fail "invalid Config.Labels for ${reference}"
    actual_ubuntu_snapshot="$(
        "$JQ_BIN" -r '(.Labels // {})["io.memochat.ubuntu.snapshot"] // ""' <<< "$config_json"
    )"
    [[ "$actual_ubuntu_snapshot" == "$EXPECTED_UBUNTU_SNAPSHOT" ]] \
        || fail "expected io.memochat.ubuntu.snapshot=${EXPECTED_UBUNTU_SNAPSHOT} for ${reference}"
    actual_ubuntu_ca_bootstrap_sha256="$(
        "$JQ_BIN" -r '(.Labels // {})["io.memochat.ubuntu.ca-bootstrap.sha256"] // ""' <<< "$config_json"
    )"
    [[ "$actual_ubuntu_ca_bootstrap_sha256" == "$EXPECTED_UBUNTU_CA_BOOTSTRAP_SHA256" ]] \
        || fail "expected io.memochat.ubuntu.ca-bootstrap.sha256=${EXPECTED_UBUNTU_CA_BOOTSTRAP_SHA256} for ${reference}"
    actual_ubuntu_runtime_packages="$(
        "$JQ_BIN" -r '(.Labels // {})["io.memochat.ubuntu.runtime-packages"] // ""' <<< "$config_json"
    )"
    [[ "$actual_ubuntu_runtime_packages" == "$EXPECTED_UBUNTU_RUNTIME_PACKAGES" ]] \
        || fail "expected io.memochat.ubuntu.runtime-packages=${EXPECTED_UBUNTU_RUNTIME_PACKAGES} for ${reference}"

    actual_user="$("$JQ_BIN" -r '.User // ""' <<< "$config_json")"
    [[ "$actual_user" == "10001:10001" ]] \
        || fail "expected Config.User=10001:10001 for ${reference}"
    "$JQ_BIN" -e '.Entrypoint == ["/app/entrypoint.sh"]' \
        <<< "$config_json" >/dev/null \
        || fail "unexpected entrypoint for ${reference}"
    "$JQ_BIN" -e '.WorkingDir == "/run/memochat"' \
        <<< "$config_json" >/dev/null \
        || fail "unexpected working directory for ${reference}"
    "$JQ_BIN" -e '
        (.Healthcheck // {}) as $health |
        ($health | type == "object") and
        ($health.Test == ["CMD", "/app/entrypoint.sh", "--healthcheck"]) and
        (($health.Interval | type) == "number") and
        ($health.Interval >= 5000000000 and $health.Interval <= 300000000000) and
        (($health.Timeout | type) == "number") and
        ($health.Timeout >= 1000000000 and $health.Timeout <= $health.Interval) and
        (($health.StartPeriod | type) == "number") and
        ($health.StartPeriod >= 0 and $health.StartPeriod <= 300000000000) and
        (($health.Retries | type) == "number") and
        ($health.Retries >= 1 and $health.Retries <= 10)
    ' <<< "$config_json" >/dev/null \
        || fail "invalid healthcheck for ${reference}"
    "$JQ_BIN" -e '(.Env // []) | type == "array" and all(.[]; type == "string")' \
        <<< "$config_json" >/dev/null \
        || fail "invalid Config.Env for ${reference}"

    mapfile -t image_env < <("$JQ_BIN" -r '(.Env // [])[]' <<< "$config_json")
    for env_entry in "${image_env[@]}"; do
        [[ "$env_entry" == *=* ]] || fail "malformed Config.Env entry for ${reference}"
        env_name="${env_entry%%=*}"
        env_value="${env_entry#*=}"
        [[ "$env_name" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]] \
            || fail "malformed Config.Env name for ${reference}"
        upper_name="${env_name^^}"
        upper_value="${env_value^^}"

        case "$env_name" in
            MEMOCHAT_SERVICE)
                service_count=$((service_count + 1))
                service_value="$env_value"
                ;;
            MEMOCHAT_RELEASE_MODE)
                release_mode_count=$((release_mode_count + 1))
                release_mode_value="$env_value"
                ;;
            MEMOCHAT_ALLOW_DEV_SECRETS)
                allow_dev_secrets_count=$((allow_dev_secrets_count + 1))
                allow_dev_secrets_value="$env_value"
                ;;
        esac

        if [[ -n "$env_value" ]] \
            && ! is_injected_value "$env_value" \
            && is_sensitive_name "$upper_name"; then
            fail "secret-like environment variable ${env_name} is embedded in ${reference}"
        fi
        if is_injected_value "$env_value"; then
            continue
        fi
        if [[ "$upper_value" == *"-----BEGIN "*"PRIVATE KEY-----"* ]] \
            || [[ "$upper_value" == BEARER\ * ]] \
            || [[ "$env_value" =~ ^eyJ[A-Za-z0-9_-]{8,}\.[A-Za-z0-9_-]{3,}\.[A-Za-z0-9_-]{3,}$ ]] \
            || [[ "$env_value" =~ ^(gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|sk-[A-Za-z0-9_-]{20,})$ ]] \
            || [[ "$env_value" =~ ^[A-Za-z][A-Za-z0-9+.-]*://[^/@[:space:]:]+:[^/@[:space:]]+@ ]]; then
            fail "secret-like environment value is embedded in ${env_name} for ${reference}"
        fi
    done

    [[ "$service_count" -eq 1 && "$service_value" == "$target" ]] \
        || fail "expected MEMOCHAT_SERVICE=${target} for ${reference}"
    [[ "$release_mode_count" -eq 1 && "$release_mode_value" == 1 ]] \
        || fail "expected MEMOCHAT_RELEASE_MODE=1 for ${reference}"
    [[ "$allow_dev_secrets_count" -eq 1 && "$allow_dev_secrets_value" == 0 ]] \
        || fail "expected MEMOCHAT_ALLOW_DEV_SECRETS=0 for ${reference}"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --output-dir)
            [[ $# -ge 2 ]] || fail "--output-dir requires a path"
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --docker)
            [[ $# -ge 2 ]] || fail "--docker requires a path"
            DOCKER_BIN="$2"
            shift 2
            ;;
        --syft)
            [[ $# -ge 2 ]] || fail "--syft requires a path"
            SYFT_BIN="$2"
            shift 2
            ;;
        --grype)
            [[ $# -ge 2 ]] || fail "--grype requires a path"
            GRYPE_BIN="$2"
            shift 2
            ;;
        --fail-on)
            [[ $# -ge 2 ]] || fail "--fail-on requires a severity"
            FAIL_ON="$2"
            shift 2
            ;;
        --vcpkg-sbom-dir)
            [[ $# -ge 2 ]] || fail "--vcpkg-sbom-dir requires a path"
            VCPKG_SBOM_DIR="$2"
            shift 2
            ;;
        --image-prefix)
            [[ $# -ge 2 ]] || fail "--image-prefix requires a value"
            IMAGE_PREFIX="$2"
            shift 2
            ;;
        --tag)
            [[ $# -ge 2 ]] || fail "--tag requires a value"
            IMAGE_TAG="$2"
            TAG_SET=1
            shift 2
            ;;
        --image-manifest)
            [[ $# -ge 2 ]] || fail "--image-manifest requires a path"
            IMAGE_MANIFEST="$2"
            shift 2
            ;;
        --pull)
            PULL_IMAGES=1
            shift
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

[[ -n "$OUTPUT_DIR" ]] || fail "--output-dir is required"
[[ -n "$SYFT_BIN" ]] || fail "--syft is required"
[[ -n "$GRYPE_BIN" ]] || fail "--grype is required"
[[ -n "$FAIL_ON" ]] || fail "--fail-on is required"
[[ "$IMAGE_PREFIX" =~ ^[a-z0-9]+([._/-][a-z0-9]+)*$ ]] \
    || fail "unsafe --image-prefix: $IMAGE_PREFIX"
[[ "$IMAGE_TAG" =~ ^[A-Za-z0-9_][A-Za-z0-9_.-]*$ ]] \
    || fail "unsafe --tag: $IMAGE_TAG"
[[ "$FAIL_ON" =~ ^(negligible|low|medium|high|critical)$ ]] \
    || fail "unsafe --fail-on: $FAIL_ON"
[[ "$OUTPUT_DIR" != *$'\n'* ]] || fail "unsafe --output-dir"
[[ ! -e "$OUTPUT_DIR" && ! -L "$OUTPUT_DIR" ]] \
    || fail "output directory already exists: $OUTPUT_DIR"
if [[ -n "$IMAGE_MANIFEST" && "$TAG_SET" -eq 1 ]]; then
    fail "--tag cannot be combined with --image-manifest"
fi
if [[ "$PULL_IMAGES" -eq 1 && -z "$IMAGE_MANIFEST" ]]; then
    fail "--pull requires --image-manifest"
fi
if [[ -n "$IMAGE_MANIFEST" && -z "$VCPKG_SBOM_DIR" ]]; then
    fail "--vcpkg-sbom-dir is required with --image-manifest"
fi
if [[ -z "$IMAGE_MANIFEST" && -n "$VCPKG_SBOM_DIR" ]]; then
    fail "--vcpkg-sbom-dir requires --image-manifest"
fi

output_parent="$(dirname -- "$OUTPUT_DIR")"
output_name="$(basename -- "$OUTPUT_DIR")"
[[ "$output_name" != "." && "$output_name" != ".." && -n "$output_name" ]] \
    || fail "unsafe --output-dir: $OUTPUT_DIR"
output_parent="$(realpath -e -- "$output_parent" 2>/dev/null)" \
    || fail "output directory parent does not exist: $OUTPUT_DIR"
[[ -d "$output_parent" ]] || fail "output directory parent is not a directory: $output_parent"
OUTPUT_DIR="${output_parent}/${output_name}"

DOCKER_BIN="$(resolve_executable "$DOCKER_BIN" Docker)"
SYFT_BIN="$(resolve_executable "$SYFT_BIN" Syft)"
GRYPE_BIN="$(resolve_executable "$GRYPE_BIN" Grype)"
JQ_BIN="$(resolve_executable jq jq)"
SHA256SUM_BIN="$(resolve_executable sha256sum sha256sum)"
SYFT_VERSION="$(tool_version "$SYFT_BIN" Syft)"
GRYPE_VERSION="$(tool_version "$GRYPE_BIN" Grype)"

if [[ -n "$IMAGE_MANIFEST" ]]; then
    validate_manifest
    validate_vcpkg_sboms
else
    configure_tag_references
fi

mkdir -m 0700 -- "$OUTPUT_DIR" || fail "could not create output directory: $OUTPUT_DIR"
OUTPUT_CREATED=1
GRYPE_CONFIG_PATH="${OUTPUT_DIR}/GRYPE_CONFIG.yaml"
SYFT_CONFIG_PATH="${OUTPUT_DIR}/SYFT_CONFIG.yaml"
printf '{}\n' > "$GRYPE_CONFIG_PATH"
printf '{}\n' > "$SYFT_CONFIG_PATH"
chmod 0400 -- "$GRYPE_CONFIG_PATH" "$SYFT_CONFIG_PATH"

if [[ "$PULL_IMAGES" -eq 1 ]]; then
    for row in "${TARGET_ROWS[@]}"; do
        IFS='|' read -r target _slug <<< "$row"
        reference="${REFERENCE_BY_TARGET[$target]}"
        "$DOCKER_BIN" pull "$reference" >/dev/null \
            || fail "Docker pull failed for immutable reference: ${reference}"
        printf '[OK] pulled immutable image: %s\n' "$reference"
    done
fi

for row in "${TARGET_ROWS[@]}"; do
    IFS='|' read -r target slug <<< "$row"
    reference="${REFERENCE_BY_TARGET[$target]}"
    inspect_image_config \
        "$target" \
        "$slug" \
        "$reference" \
        "${DIGEST_BY_TARGET[$target]}" \
        "${BUNDLE_SHA_BY_TARGET[$target]}" \
        "${VCPKG_SBOM_SHA_BY_TARGET[$target]}" \
        "${LEGAL_STATUS_SHA_BY_TARGET[$target]}"
    printf '[OK] hardened image config: %s (%s)\n' "$reference" "$target"
done

declare -a evidence_files=("GRYPE_CONFIG.yaml" "SYFT_CONFIG.yaml")
printf 'label\tthreshold\tfinding_count\n' > "${OUTPUT_DIR}/POLICY_FAILURES.tsv"
if [[ -n "$IMAGE_MANIFEST" ]]; then
    cp -- "$IMAGE_MANIFEST" "${OUTPUT_DIR}/BACKEND_IMAGES.json"
    copied_manifest_sha256="$("$SHA256SUM_BIN" "${OUTPUT_DIR}/BACKEND_IMAGES.json" | awk '{print $1}')"
    [[ "$copied_manifest_sha256" == "$MANIFEST_SHA256" ]] \
        || fail "backend image manifest changed during audit"
    chmod 0400 -- "${OUTPUT_DIR}/BACKEND_IMAGES.json"
    evidence_files+=("BACKEND_IMAGES.json")
fi
grype_db_status="${OUTPUT_DIR}/GRYPE_DB_STATUS.json"
if ! env GRYPE_CHECK_FOR_APP_UPDATE=false GRYPE_DB_AUTO_UPDATE=false \
    "$GRYPE_BIN" db status -o json --config "$GRYPE_CONFIG_PATH" > "$grype_db_status"; then
    fail "Grype database status failed"
fi
[[ -f "$grype_db_status" && ! -L "$grype_db_status" && -s "$grype_db_status" ]] \
    || fail "Grype database status did not produce evidence"
"$JQ_BIN" -e 'type == "object"' "$grype_db_status" >/dev/null 2>&1 \
    || fail "invalid Grype database status JSON"
chmod 0400 -- "$grype_db_status"
evidence_files+=("GRYPE_DB_STATUS.json")

for row in "${TARGET_ROWS[@]}"; do
    IFS='|' read -r target slug <<< "$row"
    reference="${REFERENCE_BY_TARGET[$target]}"
    sbom_name="${slug}.image.spdx.json"
    grype_name="${slug}.image.grype.json"
    sbom_path="${OUTPUT_DIR}/${sbom_name}"
    grype_path="${OUTPUT_DIR}/${grype_name}"

    if ! env SYFT_CHECK_FOR_APP_UPDATE=false \
        "$SYFT_BIN" scan "$reference" --config "$SYFT_CONFIG_PATH" --output "spdx-json=${sbom_path}"; then
        fail "Syft failed for ${reference}"
    fi
    validate_spdx_json "$sbom_path" "$reference"
    scan_sbom_with_grype "$sbom_path" "$grype_path" "$reference image SBOM"

    chmod 0400 -- "$sbom_path" "$grype_path"
    evidence_files+=("$sbom_name" "$grype_name")
    printf '[OK] image SBOM and vulnerability report generated: %s\n' "$reference"

    if [[ -n "$IMAGE_MANIFEST" ]]; then
        vcpkg_sbom_name="${slug}.vcpkg.spdx.json"
        vcpkg_grype_name="${slug}.vcpkg.grype.json"
        vcpkg_source_path="${VCPKG_SBOM_DIR}/${vcpkg_sbom_name}"
        vcpkg_sbom_path="${OUTPUT_DIR}/${vcpkg_sbom_name}"
        vcpkg_grype_path="${OUTPUT_DIR}/${vcpkg_grype_name}"
        cp -- "$vcpkg_source_path" "$vcpkg_sbom_path"
        copied_vcpkg_sha256="$("$SHA256SUM_BIN" "$vcpkg_sbom_path" | awk '{print $1}')"
        [[ "$copied_vcpkg_sha256" == "${VCPKG_SBOM_SHA_BY_TARGET[$target]}" ]] \
            || fail "vcpkg SBOM changed during audit for ${target}"
        validate_spdx_json "$vcpkg_sbom_path" "copied vcpkg dependency SBOM for ${target}"
        scan_sbom_with_grype \
            "$vcpkg_sbom_path" \
            "$vcpkg_grype_path" \
            "${target} vcpkg installed dependency closure"
        chmod 0400 -- "$vcpkg_sbom_path" "$vcpkg_grype_path"
        evidence_files+=("$vcpkg_sbom_name" "$vcpkg_grype_name")
        printf '[OK] vcpkg dependency SBOM and vulnerability report generated: %s\n' "$target"
    fi
done

{
    printf 'format=memochat-backend-image-audit-v3\n'
    if [[ -n "$IMAGE_MANIFEST" ]]; then
        printf 'selection=manifest\n'
        printf 'source_sha=%s\n' "$SOURCE_SHA"
        printf 'source_manifest_sha256=%s\n' "$MANIFEST_SHA256"
        printf 'vcpkg_sbom_coverage=installed-closure-overapproximation\n'
        printf 'legal_status_sha256=%s\n' "$LEGAL_STATUS_SHA256"
        printf 'legal_corpus_sha256=%s\n' "$LEGAL_CORPUS_SHA256"
        printf 'legal_corpus_review_id=%s\n' "$LEGAL_CORPUS_REVIEW_ID"
        printf 'formal_distribution_ready=true\n'
    else
        printf 'selection=local-tag\n'
        printf 'image_tag=%s\n' "$IMAGE_TAG"
    fi
    printf 'image_prefix=%s\n' "$IMAGE_PREFIX"
    printf 'image_count=%d\n' "${#TARGET_ROWS[@]}"
    printf 'expected_user=10001:10001\n'
    printf 'expected_release_mode=1\n'
    printf 'expected_allow_dev_secrets=0\n'
    printf 'fail_on=%s\n' "$FAIL_ON"
    printf 'vulnerability_policy=all-findings\n'
    printf 'vulnerability_policy_failures=%s\n' "$VULNERABILITY_POLICY_FAILURES"
    printf 'syft_version=%s\n' "$SYFT_VERSION"
    printf 'grype_version=%s\n' "$GRYPE_VERSION"
    for row in "${TARGET_ROWS[@]}"; do
        IFS='|' read -r target slug <<< "$row"
        if [[ -n "${DIGEST_BY_TARGET[$target]}" ]]; then
            printf 'image=%s|%s|%s\n' "$target" "$slug" "${DIGEST_BY_TARGET[$target]}"
            printf 'vcpkg_sbom=%s|%s|%s\n' \
                "$target" "$slug" "${VCPKG_SBOM_SHA_BY_TARGET[$target]}"
        else
            printf 'image=%s|%s|tag:%s\n' "$target" "$slug" "$IMAGE_TAG"
        fi
    done
} > "${OUTPUT_DIR}/AUDIT_MANIFEST.txt"
chmod 0400 -- "${OUTPUT_DIR}/AUDIT_MANIFEST.txt"
evidence_files+=("AUDIT_MANIFEST.txt")

if [[ "$VULNERABILITY_POLICY_FAILURES" -gt 0 ]]; then
    chmod 0400 -- "${OUTPUT_DIR}/POLICY_FAILURES.tsv"
    evidence_files+=("POLICY_FAILURES.tsv")
    printf 'failed\n' > "${OUTPUT_DIR}/AUDIT_POLICY_FAILED"
    chmod 0400 -- "${OUTPUT_DIR}/AUDIT_POLICY_FAILED"
    evidence_files+=("AUDIT_POLICY_FAILED")
else
    rm -f -- "${OUTPUT_DIR}/POLICY_FAILURES.tsv"
fi

(
    cd -- "$OUTPUT_DIR"
    "$SHA256SUM_BIN" "${evidence_files[@]}"
) > "${OUTPUT_DIR}/SHA256SUMS"
chmod 0400 -- "${OUTPUT_DIR}/SHA256SUMS"
if [[ "$VULNERABILITY_POLICY_FAILURES" -gt 0 ]]; then
    fail "Grype policy found ${VULNERABILITY_POLICY_FAILURES} finding(s) at or above ${FAIL_ON}; complete reports retained"
fi
printf 'complete\n' > "${OUTPUT_DIR}/AUDIT_COMPLETE"
chmod 0400 -- "${OUTPUT_DIR}/AUDIT_COMPLETE"

printf '[SUCCESS] audited %d backend images; evidence: %s\n' \
    "${#TARGET_ROWS[@]}" "$OUTPUT_DIR"
