#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
BUILD_BIN="${MEMOCHAT_BUILD_BIN:-${PROJECT_ROOT}/build-linux-full-gcc16/bin}"
if [[ -n "${MEMOCHAT_CLIENT_BUILD_BIN:-}" ]]; then
    CLIENT_BUILD_BIN="${MEMOCHAT_CLIENT_BUILD_BIN}"
else
    CLIENT_BUILD_BIN="${PROJECT_ROOT}/build-linux-full-gcc16/bin"
fi
RUNTIME_DIR="${MEMOCHAT_RUNTIME_DIR:-${PROJECT_ROOT}/infra/Memo_ops/runtime/services}"
SOURCE_ROOT="${PROJECT_ROOT}/apps/server/core"
CHECK_ONLY=0

usage() {
    cat <<USAGE
Usage: $0 [--build-bin PATH] [--client-build-bin PATH] [--runtime-dir PATH] [--check-only]

Deploy Linux MemoChat service binaries and config.ini files into:
  ${RUNTIME_DIR}

The script mirrors deploy_services.ps1 but uses Linux binaries from:
  ${BUILD_BIN}

The QML client is copied from:
  ${CLIENT_BUILD_BIN}
USAGE
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

echo "============================================================"
echo "  Deploy Linux services to runtime directory"
echo "  PROJECT_ROOT: ${PROJECT_ROOT}"
echo "  BUILD_BIN:    ${BUILD_BIN}"
echo "  CLIENT_BUILD: ${CLIENT_BUILD_BIN}"
echo "  RUNTIME_DIR:  ${RUNTIME_DIR}"
echo "============================================================"

require_file "${BUILD_BIN}/GateServer"
require_file "${BUILD_BIN}/StatusServer"
require_file "${BUILD_BIN}/ChatServer"
require_file "${BUILD_BIN}/AIServer"
require_file "${BUILD_BIN}/VarifyServer"

[[ "$CHECK_ONLY" -eq 0 ]] && mkdir -p -- "$RUNTIME_DIR"

deploy_service "GateServer1" "GateServer" "GateServer" "GateServer/config.ini"
deploy_service "GateServer2" "GateServer" "GateServer" "GateServer/gate2.ini"
deploy_service "StatusServer1" "StatusServer" "StatusServer" "StatusServer/config.ini"
deploy_service "StatusServer2" "StatusServer" "StatusServer" "StatusServer/status2.ini"
deploy_service "chatserver1" "ChatServer" "ChatServer" "ChatServer/chatserver1.ini"
deploy_service "chatserver2" "ChatServer" "ChatServer" "ChatServer/chatserver2.ini"
deploy_service "chatserver3" "ChatServer" "ChatServer" "ChatServer/chatserver3.ini"
deploy_service "chatserver4" "ChatServer" "ChatServer" "ChatServer/chatserver4.ini"
deploy_service "chatserver5" "ChatServer" "ChatServer" "ChatServer/chatserver5.ini"
deploy_service "chatserver6" "ChatServer" "ChatServer" "ChatServer/chatserver6.ini"
deploy_service "AIServer" "AIServer" "AIServer" "AIServer/config.ini"
deploy_service "VarifyServer1" "VarifyServer" "VarifyServer" "VarifyServer/config.ini"
deploy_service "VarifyServer2" "VarifyServer" "VarifyServer" "VarifyServer/varify2.ini"

copy_memochat_qml_from_candidates "${RUNTIME_DIR}/MemoChatQml/MemoChatQml" \
    "${CLIENT_BUILD_BIN}/MemoChatQml" \
    "${BUILD_BIN}/MemoChatQml"
copy_optional_from_candidates "${RUNTIME_DIR}/MemoChatQml/config.ini" \
    "${CLIENT_BUILD_BIN}/config.ini" \
    "${BUILD_BIN}/config.ini"
copy_optional "${BUILD_BIN}/MemoOpsQml" "${RUNTIME_DIR}/MemoOpsQml/MemoOpsQml"
copy_optional "${BUILD_BIN}/memoops-qml.ini" "${RUNTIME_DIR}/MemoOpsQml/memoops-qml.ini"

if [[ -d "${BUILD_BIN}/r18-plugins" ]]; then
    for gate_dir in "${RUNTIME_DIR}/GateServer1" "${RUNTIME_DIR}/GateServer2"; do
        if [[ "$CHECK_ONLY" -eq 0 ]]; then
            mkdir -p -- "$gate_dir"
            cp -a -- "${BUILD_BIN}/r18-plugins" "$gate_dir/"
            copy_optional "${BUILD_BIN}/R18PluginHost" "${gate_dir}/R18PluginHost"
        else
            echo "[OK] r18-plugins"
        fi
    done
fi

missing=0
required_runtime=(
    "GateServer1/GateServer"
    "GateServer2/GateServer"
    "StatusServer1/StatusServer"
    "StatusServer2/StatusServer"
    "VarifyServer1/VarifyServer"
    "VarifyServer2/VarifyServer"
    "chatserver1/ChatServer"
    "chatserver2/ChatServer"
    "chatserver3/ChatServer"
    "chatserver4/ChatServer"
    "chatserver5/ChatServer"
    "chatserver6/ChatServer"
    "AIServer/AIServer"
    "GateServer1/config.ini"
    "GateServer2/config.ini"
    "StatusServer1/config.ini"
    "StatusServer2/config.ini"
    "VarifyServer1/config.ini"
    "VarifyServer2/config.ini"
    "chatserver1/config.ini"
    "chatserver2/config.ini"
    "chatserver3/config.ini"
    "chatserver4/config.ini"
    "chatserver5/config.ini"
    "chatserver6/config.ini"
    "AIServer/config.ini"
)

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

if [[ "$CHECK_ONLY" -eq 1 ]]; then
    echo "[SUCCESS] Source files are available for Linux runtime deploy"
else
    echo "[SUCCESS] Runtime services deployed to ${RUNTIME_DIR}"
fi
