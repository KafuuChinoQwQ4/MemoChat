#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
ENV_FILE="${MEMOCHAT_ENV_FILE:-/root/.memochat-linux-env}"
CLIENT_EXE="${MEMOCHAT_CLIENT_EXE:-}"
DIAGNOSE=0
APP_ARGS=()

usage() {
    cat <<USAGE
Usage: $0 [--exe PATH] [--diagnose] [-- app-args...]

Start MemoChatQml on Arch Linux under WSLg with the Qt/X11 runtime
environment that matches the /data Qt 6.8.3 Linux kit.

Options:
  --exe PATH    Run a specific MemoChatQml executable.
  --diagnose   Print font, Qt platform, OpenGL, and Vulkan diagnostics.
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --exe)
            CLIENT_EXE="$2"
            shift 2
            ;;
        --diagnose)
            DIAGNOSE=1
            shift
            ;;
        --)
            shift
            APP_ARGS=("$@")
            break
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            APP_ARGS+=("$1")
            shift
            ;;
    esac
done

if [[ -f "$ENV_FILE" ]]; then
    # shellcheck disable=SC1090
    source "$ENV_FILE"
fi

set_default() {
    local name="$1"
    local value="$2"
    if [[ -z "${!name:-}" ]]; then
        export "${name}=${value}"
    fi
}

QT_ROOT="${QT_ROOT:-/data/Qt/6.8.3/gcc_64}"
if [[ -d "$QT_ROOT" ]]; then
    export PATH="${QT_ROOT}/bin:${PATH}"
    export LD_LIBRARY_PATH="${QT_ROOT}/lib:${LD_LIBRARY_PATH:-}"
    export QT_PLUGIN_PATH="${QT_ROOT}/plugins"
fi

# The Qt 6.8.3 kit installed under /data currently includes XCB, not the
# matching Qt Wayland platform plugin, so XWayland/XCB is the compatible path.
set_default QT_QPA_PLATFORM "xcb"
set_default QSG_RHI_BACKEND "opengl"
set_default QT_OPENGL "desktop"
set_default QTWEBENGINE_DISABLE_SANDBOX "1"
set_default QML_DISABLE_DISK_CACHE "1"
set_default NO_AT_BRIDGE "1"
set_default LANG "C.UTF-8"
set_default LC_ALL "C.UTF-8"

if [[ -z "$CLIENT_EXE" ]]; then
    for candidate in \
        "${PROJECT_ROOT}/build-linux-client-gcc16/bin/MemoChatQml" \
        "${PROJECT_ROOT}/build-linux-full-gcc16/bin/MemoChatQml" \
        "${PROJECT_ROOT}/infra/Memo_ops/runtime/services/MemoChatQml/MemoChatQml"; do
        if [[ -x "$candidate" ]]; then
            CLIENT_EXE="$candidate"
            break
        fi
    done
fi

print_diagnostics() {
    echo "============================================================"
    echo "  MemoChatQml WSLg diagnostics"
    echo "  PROJECT_ROOT:     ${PROJECT_ROOT}"
    echo "  QT_ROOT:          ${QT_ROOT}"
    echo "  CLIENT_EXE:       ${CLIENT_EXE:-<not found>}"
    echo "  QT_QPA_PLATFORM:  ${QT_QPA_PLATFORM}"
    echo "  QSG_RHI_BACKEND:  ${QSG_RHI_BACKEND}"
    echo "  DISPLAY:          ${DISPLAY:-<unset>}"
    echo "  WAYLAND_DISPLAY:  ${WAYLAND_DISPLAY:-<unset>}"
    echo "============================================================"
    echo

    if command -v fc-match >/dev/null 2>&1; then
        echo "[fontconfig]"
        fc-match sans:lang=zh-cn || true
        fc-match "Noto Sans CJK SC" || true
        echo
    fi

    if command -v glxinfo >/dev/null 2>&1; then
        echo "[OpenGL/XWayland]"
        glxinfo -B 2>&1 | sed -n '1,38p' || true
        echo
    fi

    if command -v vulkaninfo >/dev/null 2>&1; then
        echo "[Vulkan/D3D12]"
        MESA_VK_DEVICE_SELECT=list vulkaninfo --summary 2>&1 | sed -n '1,80p' || true
        echo
    fi
}

if [[ "$DIAGNOSE" -eq 1 ]]; then
    print_diagnostics
    exit 0
fi

if [[ -z "$CLIENT_EXE" || ! -x "$CLIENT_EXE" ]]; then
    echo "[FAIL] MemoChatQml executable not found." >&2
    echo "       Build it with: source ${ENV_FILE} && cmake --build --preset linux-client-gcc16 --parallel 12" >&2
    exit 1
fi

cd -- "$(dirname -- "$CLIENT_EXE")"
exec "$CLIENT_EXE" "${APP_ARGS[@]}"
