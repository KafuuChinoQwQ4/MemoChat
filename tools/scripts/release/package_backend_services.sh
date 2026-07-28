#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
LEGAL_VERIFIER="${SCRIPT_DIR}/verify_release_legal.sh"
VCPKG_SBOM_GENERATOR="${SCRIPT_DIR}/generate_vcpkg_installed_sbom.py"
BUILD_BIN="${PROJECT_ROOT}/build-linux-server-release-gcc16/bin"
OUTPUT=""
SOURCE_SHA=""
APPROVAL_PUBLIC_KEY=""
APPROVAL_SIGNATURE=""
VCPKG_INSTALLED_ROOT=""
VCPKG_TRIPLET=""
SBOM_TEMPLATE=""
declare -a REQUESTED_TARGETS=()
declare -a LIBRARY_DIRS=()
readonly SAFE_LOADER_PATH="/usr/bin:/bin"

readonly -a SUPPORTED_TARGETS=(
    AIGatewayServer
    AIServer
    AccountServer
    CallGatewayServer
    ChatDeliveryWorker
    ChatMessageService
    ChatRelationQueryService
    ChatRelationServiceWorker
    ChatServer
    LoginServer
    MediaGatewayServer
    MomentsGatewayServer
    R18GatewayServer
    RegisterServer
    VarifyServer
)

usage() {
    cat <<'USAGE'
Usage: package_backend_services.sh --output PATH [options]

Create fresh, relocatable C++ service bundles for the runtime Docker image.

Options:
  --build-bin PATH  Directory containing release service executables.
                    Default: build-linux-server-release-gcc16/bin
  --output PATH     New output root. It must not already exist.
  --library-dir PATH
                    Explicit trusted root for non-system ELF dependencies.
                    May be repeated. At least one directory is required.
  --vcpkg-installed-root PATH
                    vcpkg installed tree containing vcpkg/status and TRIPLET/share.
                    Must be paired with --vcpkg-triplet for formal dependency SBOMs.
  --vcpkg-triplet NAME
                    Target triplet whose complete installed closure is recorded.
  --source-sha SHA  Bind packaged legal inputs to this exact clean Git commit.
  --approval-public-key PATH
                    External legal approval public key; never discovered implicitly.
  --approval-signature PATH
                    External exact-source approval signature; never discovered implicitly.
  --target NAME     Package one supported target. May be repeated.
                    Without --target, all current service targets are packaged.
  -h, --help        Show this help.

Each service is written as NAME/{bin/NAME,lib/,MANIFEST.txt,SHA256SUMS}.
Only the named executable, non-system ELF dependencies, and redistributable GCC
runtime libraries are copied. glibc remains provided by the runtime image.
USAGE
}

fail() {
    printf '[FAIL] %s\n' "$*" >&2
    exit 1
}

is_supported_target() {
    local requested="$1"
    local candidate
    for candidate in "${SUPPORTED_TARGETS[@]}"; do
        [[ "$candidate" == "$requested" ]] && return 0
    done
    return 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-bin)
            [[ $# -ge 2 ]] || fail "--build-bin requires a path"
            BUILD_BIN="$2"
            shift 2
            ;;
        --output)
            [[ $# -ge 2 ]] || fail "--output requires a path"
            OUTPUT="$2"
            shift 2
            ;;
        --library-dir)
            [[ $# -ge 2 ]] || fail "--library-dir requires a path"
            LIBRARY_DIRS+=("$2")
            shift 2
            ;;
        --target)
            [[ $# -ge 2 ]] || fail "--target requires a service name"
            REQUESTED_TARGETS+=("$2")
            shift 2
            ;;
        --vcpkg-installed-root)
            [[ $# -ge 2 ]] || fail "--vcpkg-installed-root requires a path"
            VCPKG_INSTALLED_ROOT="$2"
            shift 2
            ;;
        --vcpkg-triplet)
            [[ $# -ge 2 ]] || fail "--vcpkg-triplet requires a value"
            VCPKG_TRIPLET="$2"
            shift 2
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
[[ ${#LIBRARY_DIRS[@]} -gt 0 ]] || fail "--library-dir is required for non-system dependencies"
if [[ -n "$VCPKG_INSTALLED_ROOT" || -n "$VCPKG_TRIPLET" ]]; then
    [[ -n "$VCPKG_INSTALLED_ROOT" && -n "$VCPKG_TRIPLET" ]] \
        || fail "--vcpkg-installed-root and --vcpkg-triplet must be provided together"
fi

for command_name in ldd mktemp patchelf python3 readelf realpath sha256sum strip; do
    command -v "$command_name" >/dev/null 2>&1 || fail "required command is missing: ${command_name}"
done
[[ -x "$LEGAL_VERIFIER" ]] || fail "legal verifier is missing or not executable: $LEGAL_VERIFIER"
if [[ -n "$VCPKG_INSTALLED_ROOT" ]]; then
    [[ -f "$VCPKG_SBOM_GENERATOR" && ! -L "$VCPKG_SBOM_GENERATOR" ]] \
        || fail "vcpkg SBOM generator is missing or unsafe: $VCPKG_SBOM_GENERATOR"
fi
declare -a LEGAL_ARGS=(--project-root "$PROJECT_ROOT")
[[ -z "$SOURCE_SHA" ]] || LEGAL_ARGS+=(--source-sha "$SOURCE_SHA")
[[ -z "$APPROVAL_PUBLIC_KEY" ]] \
    || LEGAL_ARGS+=(--approval-public-key "$APPROVAL_PUBLIC_KEY")
[[ -z "$APPROVAL_SIGNATURE" ]] \
    || LEGAL_ARGS+=(--approval-signature "$APPROVAL_SIGNATURE")
"$LEGAL_VERIFIER" "${LEGAL_ARGS[@]}"

BUILD_BIN="$(realpath -e -- "$BUILD_BIN")" || fail "build directory does not exist: $BUILD_BIN"
[[ -d "$BUILD_BIN" ]] || fail "build path is not a directory: $BUILD_BIN"

for library_index in "${!LIBRARY_DIRS[@]}"; do
    library_dir="${LIBRARY_DIRS[$library_index]}"
    [[ "$library_dir" != *:* && "$library_dir" != *$'\n'* ]] \
        || fail "--library-dir cannot contain ':' or a newline: $library_dir"
    library_dir="$(realpath -e -- "$library_dir")" \
        || fail "library directory does not exist: $library_dir"
    [[ -d "$library_dir" && ! -L "$library_dir" ]] \
        || fail "library root must be a real directory: $library_dir"
    LIBRARY_DIRS[$library_index]="$library_dir"
done
LIBRARY_SEARCH_PATH="$(IFS=:; printf '%s' "${LIBRARY_DIRS[*]}")"

OUTPUT="$(realpath -m -- "$OUTPUT")"
[[ "$OUTPUT" != "/" && "$OUTPUT" != "$PROJECT_ROOT" ]] \
    || fail "refusing unsafe output path: $OUTPUT"
[[ ! -e "$OUTPUT" ]] || fail "output path already exists; refusing to merge or delete it: $OUTPUT"

if [[ ${#REQUESTED_TARGETS[@]} -eq 0 ]]; then
    REQUESTED_TARGETS=("${SUPPORTED_TARGETS[@]}")
fi

for target in "${REQUESTED_TARGETS[@]}"; do
    is_supported_target "$target" || fail "unsupported service target: $target"
done

OUTPUT_PARENT="$(dirname -- "$OUTPUT")"
mkdir -p -- "$OUTPUT_PARENT"
STAGING="$(mktemp -d -- "${OUTPUT_PARENT}/.memochat-backend-release.XXXXXX")"
cleanup() {
    if [[ -n "${STAGING:-}" && -d "$STAGING" ]]; then
        rm -rf -- "$STAGING"
    fi
}
trap cleanup EXIT

if [[ -n "$VCPKG_INSTALLED_ROOT" ]]; then
    SBOM_TEMPLATE="${STAGING}/.vcpkg-build-dependencies.spdx.json"
    python3 "$VCPKG_SBOM_GENERATOR" \
        --installed-root "$VCPKG_INSTALLED_ROOT" \
        --triplet "$VCPKG_TRIPLET" \
        --source-sha "${SOURCE_SHA:-unbound}" \
        --output "$SBOM_TEMPLATE"
fi

is_system_library() {
    case "$1" in
        /lib/*|/lib64/*|/usr/lib/*|/usr/lib64/*) return 0 ;;
        *) return 1 ;;
    esac
}

is_redistributable_compiler_runtime() {
    case "$1" in
        libstdc++.so.6|libgcc_s.so.1|libatomic.so.1) return 0 ;;
        *) return 1 ;;
    esac
}

is_allowed_library_path() {
    local dependency_path="$1"
    local allowed_root
    for allowed_root in "${LIBRARY_DIRS[@]}"; do
        if [[ "$dependency_path" == "$allowed_root/"* ]]; then
            return 0
        fi
    done
    return 1
}

clean_ldd() {
    local elf="$1"
    env -i \
        PATH="$SAFE_LOADER_PATH" \
        LC_ALL=C \
        LANG=C \
        LD_LIBRARY_PATH="$LIBRARY_SEARCH_PATH" \
        ldd "$elf"
}

dependency_rows() {
    local elf="$1"
    local output
    if ! output="$(clean_ldd "$elf" 2>&1)"; then
        printf '%s\n' "$output" >&2
        fail "ldd failed for $elf"
    fi
    if grep -Eq '=>[[:space:]]+not found|not a dynamic executable' <<<"$output"; then
        printf '%s\n' "$output" >&2
        fail "unresolved dependency in $elf"
    fi

    awk '
        $2 == "=>" && $3 ~ /^\// { print $1 "|" $3 }
        $1 ~ /^\// { name=$1; sub(/^.*\//, "", name); print name "|" $1 }
    ' <<<"$output"
}

package_target() {
    local target="$1"
    local source_binary="${BUILD_BIN}/${target}"
    local service_dir="${STAGING}/${target}"
    local output_binary="${service_dir}/bin/${target}"
    local current dependency_name dependency_path resolved_source destination existing_hash incoming_hash
    local legal_status legal_inventory third_party_legal_corpus corpus_review_id corpus_sha256
    local release_source_sha formal_distribution_ready legal_status_sha256
    local vcpkg_sbom_coverage vcpkg_sbom_sha256 vcpkg_sbom_source_sha
    local -a dependency_queue=()
    local queue_index=0
    declare -A scanned_paths=()
    declare -A copied_names=()

    [[ -f "$source_binary" && -x "$source_binary" ]] \
        || fail "missing executable release target: $source_binary"
    readelf -h "$source_binary" >/dev/null 2>&1 || fail "target is not an ELF executable: $source_binary"

    mkdir -p -- "${service_dir}/bin" "${service_dir}/lib"
    install -m 0755 -- "$source_binary" "$output_binary"
    dependency_queue+=("$source_binary")

    while (( queue_index < ${#dependency_queue[@]} )); do
        current="${dependency_queue[$queue_index]}"
        ((queue_index += 1))
        [[ -z "${scanned_paths[$current]:-}" ]] || continue
        scanned_paths["$current"]=1

        while IFS='|' read -r dependency_name dependency_path; do
            [[ -n "$dependency_name" && -n "$dependency_path" ]] || continue
            if is_system_library "$dependency_path" \
                && ! is_redistributable_compiler_runtime "$dependency_name"; then
                continue
            fi
            [[ "$dependency_name" =~ ^[A-Za-z0-9._+-]+$ ]] \
                || fail "unsafe dependency name '${dependency_name}' from $current"
            [[ -f "$dependency_path" ]] || fail "dependency path is not a file: $dependency_path"
            resolved_source="$(realpath -e -- "$dependency_path")" \
                || fail "dependency cannot be resolved: $dependency_path"
            if ! is_system_library "$resolved_source" \
                && ! is_allowed_library_path "$resolved_source"; then
                fail "dependency is outside explicit --library-dir roots: ${dependency_name} -> ${resolved_source}"
            fi

            destination="${service_dir}/lib/${dependency_name}"
            if [[ -n "${copied_names[$dependency_name]:-}" ]]; then
                existing_hash="$(sha256sum "$destination" | awk '{print $1}')"
                incoming_hash="$(sha256sum "$resolved_source" | awk '{print $1}')"
                [[ "$existing_hash" == "$incoming_hash" ]] \
                    || fail "dependency name collision for ${dependency_name}"
                continue
            fi

            install -m 0755 -- "$resolved_source" "$destination"
            copied_names["$dependency_name"]=1
            dependency_queue+=("$resolved_source")
        done < <(dependency_rows "$current")
    done

    [[ -f "${service_dir}/lib/libmsquic.so.2" ]] \
        || fail "${target} bundle is missing required libmsquic.so.2"

    patchelf --shrink-rpath --allowed-rpath-prefixes /__memochat_no_external_rpath__ "$output_binary"
    patchelf --set-rpath '$ORIGIN/../lib' "$output_binary"
    while IFS= read -r -d '' dependency; do
        patchelf --shrink-rpath --allowed-rpath-prefixes /__memochat_no_external_rpath__ "$dependency"
        patchelf --set-rpath '$ORIGIN' "$dependency"
        strip --strip-unneeded "$dependency"
    done < <(find "${service_dir}/lib" -maxdepth 1 -type f -print0)
    strip --strip-all "$output_binary"

    local verification service_lib_root resolved_dependency
    if ! verification="$(env -i PATH="$SAFE_LOADER_PATH" LC_ALL=C LANG=C LD_LIBRARY_PATH="${service_dir}/lib" ldd "$output_binary" 2>&1)"; then
        printf '%s\n' "$verification" >&2
        fail "packaged dependency verification failed for $target"
    fi
    if grep -Eq '=>[[:space:]]+not found|not a dynamic executable' <<<"$verification"; then
        printf '%s\n' "$verification" >&2
        fail "packaged target still has an unresolved dependency: $target"
    fi
    service_lib_root="$(realpath -e -- "${service_dir}/lib")"
    while IFS='|' read -r dependency_name dependency_path; do
        [[ -n "$dependency_name" && -n "$dependency_path" ]] || continue
        resolved_dependency="$(realpath -e -- "$dependency_path")" \
            || fail "packaged dependency cannot be resolved: ${dependency_name}"
        if [[ "$resolved_dependency" == "$service_lib_root/"* ]] \
            || is_system_library "$resolved_dependency"; then
            continue
        fi
        printf '%s\n' "$verification" >&2
        fail "packaged target resolves a dependency outside its bundle: $target"
    done < <(awk '
        $2 == "=>" && $3 ~ /^\// { print $1 "|" $3 }
        $1 ~ /^\// { name=$1; sub(/^.*\//, "", name); print name "|" $1 }
    ' <<<"$verification")
    if [[ "$(patchelf --print-rpath "$output_binary")" != '$ORIGIN/../lib' ]]; then
        fail "packaged target has an unexpected RUNPATH: $target"
    fi

    if [[ -n "$SBOM_TEMPLATE" ]]; then
        mkdir -p -- "${service_dir}/sbom"
        install -m 0444 -- "$SBOM_TEMPLATE" \
            "${service_dir}/sbom/vcpkg-build-dependencies.spdx.json"
        vcpkg_sbom_coverage=installed-closure-overapproximation
        vcpkg_sbom_sha256="$(
            sha256sum -- "${service_dir}/sbom/vcpkg-build-dependencies.spdx.json" | awk '{print $1}'
        )"
        vcpkg_sbom_source_sha="${SOURCE_SHA:-unbound}"
    else
        vcpkg_sbom_coverage=unavailable
        vcpkg_sbom_sha256=unavailable
        vcpkg_sbom_source_sha=unbound
    fi

    "$LEGAL_VERIFIER" "${LEGAL_ARGS[@]}" --copy-to "${service_dir}/legal" >/dev/null
    legal_status="${service_dir}/legal/LEGAL-STATUS.txt"
    legal_inventory="$(awk -F= '$1 == "third_party_inventory" { print $2 }' "$legal_status")"
    third_party_legal_corpus="$(awk -F= '$1 == "third_party_legal_corpus" { print $2 }' "$legal_status")"
    corpus_review_id="$(awk -F= '$1 == "corpus_review_id" { print $2 }' "$legal_status")"
    corpus_sha256="$(awk -F= '$1 == "corpus_sha256" { print $2 }' "$legal_status")"
    release_source_sha="$(awk -F= '$1 == "release_source_sha" { print $2 }' "$legal_status")"
    formal_distribution_ready="$(awk -F= '$1 == "formal_distribution_ready" { print $2 }' "$legal_status")"
    legal_status_sha256="$(sha256sum -- "$legal_status" | awk '{print $1}')"
    [[ "$legal_inventory" == complete ]] || fail "generated legal inventory status is invalid for $target"
    [[ "$third_party_legal_corpus" == complete || "$third_party_legal_corpus" == incomplete ]] \
        || fail "generated third-party legal corpus status is invalid for $target"
    [[ "$formal_distribution_ready" == true || "$formal_distribution_ready" == false ]] \
        || fail "generated formal distribution status is invalid for $target"
    if [[ "$formal_distribution_ready" == true && "$vcpkg_sbom_coverage" == unavailable ]]; then
        fail "formal distribution requires a validated vcpkg installed closure SBOM for $target"
    fi
    if [[ "$formal_distribution_ready" == true && "$vcpkg_sbom_source_sha" != "$release_source_sha" ]]; then
        fail "formal distribution vcpkg SBOM is not bound to the legal source SHA for $target"
    fi

    {
        printf 'format=memochat-cpp-service-bundle-v1\n'
        printf 'target=%s\n' "$target"
        printf 'entrypoint=bin/%s\n' "$target"
        printf 'runtime_library_path=lib\n'
        printf 'legal_inventory=%s\n' "$legal_inventory"
        printf 'third_party_legal_corpus=%s\n' "$third_party_legal_corpus"
        printf 'corpus_review_id=%s\n' "$corpus_review_id"
        printf 'corpus_sha256=%s\n' "$corpus_sha256"
        printf 'release_source_sha=%s\n' "$release_source_sha"
        printf 'legal_status_sha256=%s\n' "$legal_status_sha256"
        printf 'vcpkg_sbom_coverage=%s\n' "$vcpkg_sbom_coverage"
        printf 'vcpkg_sbom_sha256=%s\n' "$vcpkg_sbom_sha256"
        printf 'vcpkg_sbom_source_sha=%s\n' "$vcpkg_sbom_source_sha"
        printf 'formal_distribution_ready=%s\n' "$formal_distribution_ready"
    } >"${service_dir}/MANIFEST.txt"

    (
        cd -- "$service_dir"
        {
            find bin lib legal -type f -print0
            [[ ! -d sbom ]] || find sbom -type f -print0
        } \
            | sort -z \
            | xargs -0 sha256sum >SHA256SUMS
        sha256sum --check --strict SHA256SUMS >/dev/null
    )
    printf '[OK] packaged %s\n' "$target"
}

for target in "${REQUESTED_TARGETS[@]}"; do
    package_target "$target"
done

if [[ -n "$SBOM_TEMPLATE" ]]; then
    rm -f -- "$SBOM_TEMPLATE"
    SBOM_TEMPLATE=""
fi

[[ ! -e "$OUTPUT" ]] || fail "output path appeared during packaging; refusing to merge: $OUTPUT"
mv -T -- "$STAGING" "$OUTPUT"
STAGING=""
printf '[SUCCESS] backend release bundles: %s\n' "$OUTPUT"
