#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
DOCKERFILE="${PROJECT_ROOT}/infra/deploy/images/services/cpp-service.Dockerfile"
ENTRYPOINT_SOURCE="${PROJECT_ROOT}/infra/deploy/images/common/entrypoints/server-entrypoint.sh"
VERIFY_SCRIPT="${SCRIPT_DIR}/verify_release_tree.sh"
BUNDLE_ROOT=""
IMAGE_PREFIX="memochat"
IMAGE_TAG="local"
BUILD_CONTEXT_ROOT=""
PULL_RUNTIME_IMAGE=0

readonly RUNTIME_IMAGE="ubuntu@sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90"

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

usage() {
    cat <<'USAGE'
Usage: build_backend_images.sh --bundle-root PATH [options]

Verify all current C++ service bundles, then build the 15 local runtime images.

Options:
  --bundle-root PATH   Root produced by package_backend_services.sh (required).
  --image-prefix NAME  Registry/repository prefix. Default: memochat
  --tag TAG            Image tag. Default: local
  --pull               Pull the fixed runtime digest before building. The
                       default is --pull=false so an already-loaded digest
                       can be built without Docker Hub access.
  -h, --help           Show this help.
USAGE
}

fail() {
    printf '[FAIL] %s\n' "$*" >&2
    exit 1
}

cleanup() {
    if [[ -n "$BUILD_CONTEXT_ROOT" && -d "$BUILD_CONTEXT_ROOT" ]]; then
        rm -rf -- "$BUILD_CONTEXT_ROOT"
    fi
}

trap cleanup EXIT

while [[ $# -gt 0 ]]; do
    case "$1" in
        --bundle-root)
            [[ $# -ge 2 ]] || fail "--bundle-root requires a path"
            BUNDLE_ROOT="$2"
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
            shift 2
            ;;
        --pull)
            PULL_RUNTIME_IMAGE=1
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

[[ -n "$BUNDLE_ROOT" ]] || fail "--bundle-root is required"
[[ "$IMAGE_PREFIX" =~ ^[a-z0-9]+([._/-][a-z0-9]+)*$ ]] \
    || fail "unsafe --image-prefix: $IMAGE_PREFIX"
[[ "$IMAGE_TAG" =~ ^[A-Za-z0-9_][A-Za-z0-9_.-]*$ ]] \
    || fail "unsafe --tag: $IMAGE_TAG"
command -v docker >/dev/null 2>&1 || fail "docker is required"
command -v install >/dev/null 2>&1 || fail "install is required"
command -v mktemp >/dev/null 2>&1 || fail "mktemp is required"
command -v realpath >/dev/null 2>&1 || fail "realpath is required"
[[ -x "$VERIFY_SCRIPT" ]] || fail "release verifier is missing: $VERIFY_SCRIPT"
[[ -f "$DOCKERFILE" ]] || fail "Dockerfile is missing: $DOCKERFILE"
[[ -f "$ENTRYPOINT_SOURCE" && ! -L "$ENTRYPOINT_SOURCE" ]] \
    || fail "server entrypoint is missing or unsafe: $ENTRYPOINT_SOURCE"

BUNDLE_ROOT="$(realpath -e -- "$BUNDLE_ROOT")" || fail "bundle root does not exist"
[[ -d "$BUNDLE_ROOT" ]] || fail "bundle root is not a directory: $BUNDLE_ROOT"

for row in "${TARGET_ROWS[@]}"; do
    IFS='|' read -r target slug <<<"$row"
    bundle="${BUNDLE_ROOT}/${target}"
    [[ -d "$bundle" && ! -L "$bundle" ]] || fail "missing service bundle: $bundle"
    [[ -x "${bundle}/bin/${target}" ]] || fail "missing executable: ${bundle}/bin/${target}"
    for library in libmsquic.so.2 libstdc++.so.6 libgcc_s.so.1 libatomic.so.1; do
        [[ -f "${bundle}/lib/${library}" ]] || fail "${target} is missing ${library}"
    done
    sbom_path="${bundle}/sbom/vcpkg-build-dependencies.spdx.json"
    [[ -f "$sbom_path" && ! -L "$sbom_path" && -s "$sbom_path" ]] \
        || fail "${target} is missing the validated vcpkg dependency SBOM; package with --vcpkg-installed-root and --vcpkg-triplet"
    grep -Fx "format=memochat-cpp-service-bundle-v1" "${bundle}/MANIFEST.txt" >/dev/null \
        || fail "invalid bundle format: $target"
    grep -Fx "target=${target}" "${bundle}/MANIFEST.txt" >/dev/null \
        || fail "bundle target mismatch: $target"
    grep -Fx "vcpkg_sbom_coverage=installed-closure-overapproximation" "${bundle}/MANIFEST.txt" >/dev/null \
        || fail "${target} does not carry an installed-closure vcpkg SBOM"
    expected_sbom_sha256="$(sed -n 's/^vcpkg_sbom_sha256=//p' "${bundle}/MANIFEST.txt")"
    [[ "$expected_sbom_sha256" =~ ^[0-9a-f]{64}$ ]] \
        || fail "${target} has an invalid vcpkg SBOM digest"
    printf '%s  %s\n' "$expected_sbom_sha256" "$sbom_path" \
        | sha256sum --check --strict >/dev/null \
        || fail "${target} vcpkg SBOM digest verification failed"
    (cd -- "$bundle" && sha256sum --check --strict SHA256SUMS >/dev/null) \
        || fail "bundle checksum verification failed: $target"
    "$VERIFY_SCRIPT" "$bundle"
    printf '[OK] verified %s -> %s/%s:%s\n' "$target" "$IMAGE_PREFIX" "$slug" "$IMAGE_TAG"
done

if [[ "$PULL_RUNTIME_IMAGE" -eq 1 ]]; then
    docker pull "$RUNTIME_IMAGE" >/dev/null \
        || fail "could not pull the fixed runtime image: $RUNTIME_IMAGE"
    RUNTIME_PULL_FLAG=(--pull)
else
    RUNTIME_PULL_FLAG=(--pull=false)
fi

BUILD_CONTEXT_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/memochat-backend-image-context.XXXXXX")" \
    || fail "could not create an isolated Docker build context"
mkdir -p -- "${BUILD_CONTEXT_ROOT}/empty" "${BUILD_CONTEXT_ROOT}/server_entrypoint"
install -m 0555 -- "$ENTRYPOINT_SOURCE" \
    "${BUILD_CONTEXT_ROOT}/server_entrypoint/server-entrypoint.sh"

for row in "${TARGET_ROWS[@]}"; do
    IFS='|' read -r target slug <<<"$row"
    bundle="${BUNDLE_ROOT}/${target}"
    image="${IMAGE_PREFIX}/${slug}:${IMAGE_TAG}"
    docker buildx build \
        --load \
        "${RUNTIME_PULL_FLAG[@]}" \
        --build-context "service_bundle=${bundle}" \
        --build-context "server_entrypoint=${BUILD_CONTEXT_ROOT}/server_entrypoint" \
        --build-arg "RUNTIME_IMAGE=${RUNTIME_IMAGE}" \
        --build-arg "TARGET=${target}" \
        --file "$DOCKERFILE" \
        --tag "$image" \
        "${BUILD_CONTEXT_ROOT}/empty"
    printf '[OK] built %s\n' "$image"
done

printf '[SUCCESS] built %d backend images with tag %s\n' "${#TARGET_ROWS[@]}" "$IMAGE_TAG"
