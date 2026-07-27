#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
BUILD_BIN="${PROJECT_ROOT}/build-linux-server-release-gcc16/bin"
OUTPUT=""
declare -a REQUESTED_TARGETS=()
declare -a LIBRARY_DIRS=()
declare -a LEGAL_FILES=()
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

for command_name in ldd mktemp patchelf readelf realpath sha256sum strip; do
    command -v "$command_name" >/dev/null 2>&1 || fail "required command is missing: ${command_name}"
done

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

for legal_name in LICENSE THIRD_PARTY_NOTICES.md; do
    legal_path="${PROJECT_ROOT}/${legal_name}"
    if [[ ! -e "$legal_path" ]]; then
        printf '[WARN] legal file is unavailable and will not be included: %s\n' "$legal_name" >&2
        continue
    fi
    [[ -f "$legal_path" && ! -L "$legal_path" && -s "$legal_path" ]] \
        || fail "legal file must be a non-empty regular file: $legal_path"
    LEGAL_FILES+=("$legal_name")
done

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
    local current dependency_name dependency_path resolved_source destination existing_hash incoming_hash legal_name
    local -a dependency_queue=()
    local queue_index=0
    declare -A scanned_paths=()
    declare -A copied_names=()

    [[ -f "$source_binary" && -x "$source_binary" ]] \
        || fail "missing executable release target: $source_binary"
    readelf -h "$source_binary" >/dev/null 2>&1 || fail "target is not an ELF executable: $source_binary"

    mkdir -p -- "${service_dir}/bin" "${service_dir}/lib" "${service_dir}/legal"
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

    {
        printf 'format=memochat-cpp-service-bundle-v1\n'
        printf 'target=%s\n' "$target"
        printf 'entrypoint=bin/%s\n' "$target"
        printf 'runtime_library_path=lib\n'
    } >"${service_dir}/MANIFEST.txt"

    for legal_name in "${LEGAL_FILES[@]}"; do
        install -m 0444 -- "${PROJECT_ROOT}/${legal_name}" "${service_dir}/legal/${legal_name}"
    done

    (
        cd -- "$service_dir"
        {
            find bin lib legal -type f -print0
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

[[ ! -e "$OUTPUT" ]] || fail "output path appeared during packaging; refusing to merge: $OUTPUT"
mv -T -- "$STAGING" "$OUTPUT"
STAGING=""
printf '[SUCCESS] backend release bundles: %s\n' "$OUTPUT"
