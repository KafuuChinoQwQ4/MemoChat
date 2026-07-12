#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
TOPOLOGY_FILE="${SCRIPT_DIR}/runtime_topology.sh"
BUILD_BIN="${MEMOCHAT_BUILD_BIN:-${PROJECT_ROOT}/build-linux-full-gcc16/bin}"
if [[ -n "${MEMOCHAT_CLIENT_BUILD_BIN:-}" ]]; then
    CLIENT_BUILD_BIN="${MEMOCHAT_CLIENT_BUILD_BIN}"
else
    CLIENT_BUILD_BIN="${PROJECT_ROOT}/build-linux-full-gcc16/bin"
fi
RUNTIME_DIR="${MEMOCHAT_RUNTIME_DIR:-${PROJECT_ROOT}/infra/Memo_ops/runtime/services}"
SOURCE_ROOT="${PROJECT_ROOT}/apps/server/core"
CHECK_ONLY=0
GPT_SOVITS_REQUIRED="${MEMOCHAT_REQUIRE_GPT_SOVITS:-0}"
GPT_SOVITS_START_SCRIPT="${GPT_SOVITS_START_SCRIPT:-${PROJECT_ROOT}/tools/scripts/pet/start_gpt_sovits_api_wsl.sh}"
GPT_SOVITS_ROOT="${GPT_SOVITS_ROOT:-/data/third_party/GPT-SoVITS}"
GPT_SOVITS_ENV="${GPT_SOVITS_ENV:-/data/micromamba/envs/gpt-sovits}"
GPT_SOVITS_HOST_DATA_DIR="${MEMOCHAT_PET_SOVITS_HOST_DATA_DIR:-/data/gpt-sovits}"
GPT_SOVITS_REF_AUDIO="${MEMOCHAT_PET_SOVITS_REFERENCE_AUDIO:-${GPT_SOVITS_HOST_DATA_DIR}/refs/kafuu-chino-ref.wav}"
GPT_SOVITS_PROMPT_TEXT_FILE="${MEMOCHAT_PET_SOVITS_PROMPT_TEXT_FILE:-${GPT_SOVITS_HOST_DATA_DIR}/refs/kafuu-chino-ref.ja.txt}"

# shellcheck source=tools/scripts/status/runtime_topology.sh
source "$TOPOLOGY_FILE"

usage() {
    cat <<USAGE
Usage: $0 [--build-bin PATH] [--client-build-bin PATH] [--runtime-dir PATH] [--check-only]

Deploy Linux MemoChat service binaries and config.ini files into:
  ${RUNTIME_DIR}

The script mirrors deploy_services.ps1 but uses Linux binaries from:
  ${BUILD_BIN}

The QML client is copied from:
  ${CLIENT_BUILD_BIN}

GPT-SoVITS is not deployed as a C++ runtime artifact, but this script checks the
local WSL service prerequisites used by start-all-services.sh. Set
MEMOCHAT_REQUIRE_GPT_SOVITS=1 to make missing GPT-SoVITS prerequisites fail.
USAGE
}

is_truthy() {
    case "${1,,}" in
        1|true|yes|on) return 0 ;;
        *) return 1 ;;
    esac
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-bin)
            BUILD_BIN="$2"
            shift 2
            ;;
        --client-build-bin)
            CLIENT_BUILD_BIN="$2"
            shift 2
            ;;
        --runtime-dir)
            RUNTIME_DIR="$2"
            shift 2
            ;;
        --check-only)
            CHECK_ONLY=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "[FAIL] Unknown argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

require_file() {
    local path="$1"
    if [[ ! -f "$path" ]]; then
        echo "[FAIL] Missing required file: $path" >&2
        exit 1
    fi
}

copy_required_file() {
    local src="$1"
    local dst="$2"
    require_file "$src"
    if [[ "$CHECK_ONLY" -eq 0 ]]; then
        mkdir -p -- "$(dirname -- "$dst")"
        cp -f -- "$src" "$dst"
    fi
    echo "[OK] $(basename -- "$dst")"
}

copy_required_executable() {
    local src="$1"
    local dst="$2"
    copy_required_file "$src" "$dst"
    if [[ "$CHECK_ONLY" -eq 0 ]]; then
        chmod +x -- "$dst"
    fi
}

copy_optional() {
    local src="$1"
    local dst="$2"
    if [[ -f "$src" ]]; then
        if [[ "$CHECK_ONLY" -eq 0 ]]; then
            mkdir -p -- "$(dirname -- "$dst")"
            cp -f -- "$src" "$dst"
            [[ -x "$src" ]] && chmod +x -- "$dst" || true
        fi
        echo "[OK] $(basename -- "$dst")"
    else
        echo "[WARN] Optional source not found: $src"
    fi
}

copy_optional_from_candidates() {
    local dst="$1"
    shift
    local src
    for src in "$@"; do
        if [[ -f "$src" ]]; then
            copy_optional "$src" "$dst"
            return 0
        fi
    done
    echo "[WARN] Optional source not found for $(basename -- "$dst")"
}

gpt_sovits_warn_or_fail() {
    local message="$1"
    if is_truthy "$GPT_SOVITS_REQUIRED"; then
        echo "[FAIL] ${message}" >&2
        exit 1
    fi
    echo "[WARN] ${message}"
}

check_gpt_sovits_path() {
    local label="$1"
    local path="$2"
    local kind="$3"

    case "$kind" in
        file)
            if [[ -f "$path" ]]; then
                echo "[OK] ${label}: ${path}"
                return 0
            fi
            ;;
        executable)
            if [[ -x "$path" ]]; then
                echo "[OK] ${label}: ${path}"
                return 0
            fi
            ;;
        directory)
            if [[ -d "$path" ]]; then
                echo "[OK] ${label}: ${path}"
                return 0
            fi
            ;;
    esac

    gpt_sovits_warn_or_fail "${label} missing: ${path}"
}

check_gpt_sovits_prerequisites() {
    echo
    echo "[STEP] Check GPT-SoVITS voice service prerequisites"
    check_gpt_sovits_path "GPT-SoVITS start script" "$GPT_SOVITS_START_SCRIPT" "file"
    check_gpt_sovits_path "GPT-SoVITS root" "$GPT_SOVITS_ROOT" "directory"
    check_gpt_sovits_path "GPT-SoVITS Python" "${GPT_SOVITS_ENV}/bin/python" "executable"
    check_gpt_sovits_path "GPT-SoVITS reference audio" "$GPT_SOVITS_REF_AUDIO" "file"
    if [[ -f "$GPT_SOVITS_PROMPT_TEXT_FILE" ]]; then
        echo "[OK] GPT-SoVITS prompt text: ${GPT_SOVITS_PROMPT_TEXT_FILE}"
    else
        echo "[WARN] GPT-SoVITS prompt text not found: ${GPT_SOVITS_PROMPT_TEXT_FILE}"
    fi
}

cmake_cache_value() {
    local cache_file="$1"
    local key="$2"
    awk -F= -v key="$key" '$1 ~ "^" key ":" { print $2; exit }' "$cache_file"
}

live2d_native_cache_is_valid() {
    local cache_file="$1"
    local native_flag
    local sdk_root
    native_flag="$(cmake_cache_value "$cache_file" MEMOCHAT_ENABLE_LIVE2D_NATIVE)"
    sdk_root="$(cmake_cache_value "$cache_file" MEMOCHAT_LIVE2D_SDK_ROOT)"

    case "$native_flag" in
        ON|TRUE|1) ;;
        *) return 1 ;;
    esac

    [[ -n "$sdk_root" && -d "$sdk_root/Core" && -d "$sdk_root/Framework" ]]
}

require_live2d_native_memo_chat_qml() {
    local src="$1"
    local src_dir
    local cache_file
    src_dir="$(dirname -- "$src")"
    cache_file="${src_dir%/bin}/CMakeCache.txt"

    if [[ -f "$cache_file" ]]; then
        if live2d_native_cache_is_valid "$cache_file"; then
            return 0
        fi
        echo "[FAIL] Refusing to deploy non-native/stale Live2D MemoChatQml build: $cache_file" >&2
        echo "       Run: source /root/.memochat-linux-env && cmake --preset linux-full-gcc16 && cmake --build --preset linux-full-gcc16 --parallel 12" >&2
        exit 1
    fi

    if command -v strings >/dev/null 2>&1 \
        && strings "$src" | grep -Fq "Live2D Cubism Native OpenGL is not enabled in this build"; then
        echo "[FAIL] Refusing to deploy MemoChatQml built without Live2D Native OpenGL: $src" >&2
        exit 1
    fi
}

copy_memochat_qml_from_candidates() {
    local dst="$1"
    shift
    local src
    for src in "$@"; do
        if [[ -f "$src" ]]; then
            require_live2d_native_memo_chat_qml "$src"
            copy_optional "$src" "$dst"
            return 0
        fi
    done
    echo "[WARN] Optional source not found for $(basename -- "$dst")"
}

deploy_service() {
    local name="$1"
    local exe="$2"
    local source_exe="$3"
    local config="$4"
    local svc_dir="${RUNTIME_DIR}/${name}"

    [[ "$CHECK_ONLY" -eq 0 ]] && mkdir -p -- "$svc_dir"
    copy_required_executable "${BUILD_BIN}/${source_exe}" "${svc_dir}/${exe}"
    copy_required_file "${SOURCE_ROOT}/${config}" "${svc_dir}/config.ini"
}

require_service_binaries() {
    local seen=" "
    local row group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace
    for row in "${MEMOCHAT_RUNTIME_SERVICE_TOPOLOGY[@]}"; do
        IFS='|' read -r group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace <<<"$row"
        if [[ "$seen" == *" ${source_exe} "* ]]; then
            continue
        fi
        seen+="${source_exe} "
        require_file "${BUILD_BIN}/${source_exe}"
    done
}

deploy_topology_services() {
    local row group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace
    for row in "${MEMOCHAT_RUNTIME_SERVICE_TOPOLOGY[@]}"; do
        IFS='|' read -r group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace <<<"$row"
        deploy_service "$name" "$exe" "$source_exe" "$config"
    done
}

required_runtime_files() {
    local row group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace
    for row in "${MEMOCHAT_RUNTIME_SERVICE_TOPOLOGY[@]}"; do
        IFS='|' read -r group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace <<<"$row"
        printf '%s\n' "${name}/${exe}" "${name}/config.ini"
    done
}

echo "============================================================"
echo "  Deploy Linux services to runtime directory"
echo "  PROJECT_ROOT: ${PROJECT_ROOT}"
echo "  BUILD_BIN:    ${BUILD_BIN}"
echo "  CLIENT_BUILD: ${CLIENT_BUILD_BIN}"
echo "  RUNTIME_DIR:  ${RUNTIME_DIR}"
echo "============================================================"

require_service_binaries

[[ "$CHECK_ONLY" -eq 0 ]] && mkdir -p -- "$RUNTIME_DIR"

deploy_topology_services

copy_memochat_qml_from_candidates "${RUNTIME_DIR}/MemoChatQml/MemoChatQml" \
    "${CLIENT_BUILD_BIN}/MemoChatQml" \
    "${BUILD_BIN}/MemoChatQml"
copy_optional_from_candidates "${RUNTIME_DIR}/MemoChatQml/config.ini" \
    "${CLIENT_BUILD_BIN}/config.ini" \
    "${BUILD_BIN}/config.ini"
copy_optional "${BUILD_BIN}/MemoOpsQml" "${RUNTIME_DIR}/MemoOpsQml/MemoOpsQml"
copy_optional "${BUILD_BIN}/memoops-qml.ini" "${RUNTIME_DIR}/MemoOpsQml/memoops-qml.ini"

missing=0
mapfile -t required_runtime < <(required_runtime_files)

if [[ "$CHECK_ONLY" -eq 0 ]]; then
    for relative in "${required_runtime[@]}"; do
        if [[ ! -f "${RUNTIME_DIR}/${relative}" ]]; then
            echo "[FAIL] Missing runtime file: ${relative}" >&2
            missing=1
        fi
    done
fi

if [[ "$missing" -ne 0 ]]; then
    exit 1
fi

check_gpt_sovits_prerequisites

if [[ "$CHECK_ONLY" -eq 1 ]]; then
    echo "[SUCCESS] Source files are available for Linux runtime deploy"
else
    echo "[SUCCESS] Runtime services deployed to ${RUNTIME_DIR}"
fi
