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
  --diagnose    Print font, Qt platform, OpenGL, and Vulkan diagnostics.
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

set_wslg_llvmpipe_rendering_defaults() {
    export QSG_RHI_BACKEND="opengl"
    export QT_OPENGL="software"
    export GALLIUM_DRIVER="llvmpipe"
    export MESA_LOADER_DRIVER_OVERRIDE="llvmpipe"
    export LIBGL_ALWAYS_SOFTWARE="1"
    export MEMOCHAT_WSLG_GL_FALLBACK="llvmpipe"
}

can_create_glx_context() {
    if ! command -v glxinfo >/dev/null 2>&1; then
        return 0
    fi
    glxinfo -B >/dev/null 2>&1
}

maybe_fallback_wslg_rendering() {
    if [[ "${MEMOCHAT_DISABLE_WSLG_GL_FALLBACK:-0}" == "1" ]]; then
        return 0
    fi
    if ! grep -qiE 'microsoft|wsl' /proc/sys/kernel/osrelease 2>/dev/null; then
        return 0
    fi
    if [[ "${QSG_RHI_BACKEND:-}" != "opengl" || "${QT_OPENGL:-}" != "desktop" ]]; then
        return 0
    fi
    if [[ "${GALLIUM_DRIVER:-}" != "d3d12" || "${MESA_LOADER_DRIVER_OVERRIDE:-}" != "d3d12" ]]; then
        return 0
    fi
    if can_create_glx_context; then
        return 0
    fi
    echo "[WARN] WSLg d3d12 OpenGL context is unavailable; falling back to llvmpipe software rendering." >&2
    set_wslg_llvmpipe_rendering_defaults
}

select_input_method() {
    local qt_module="$1"
    local gtk_module="$2"
    local xmodifier="$3"
    if [[ "${MEMOCHAT_KEEP_QT_IM_MODULE:-0}" == "1" && -n "${QT_IM_MODULE:-}" ]]; then
        return 0
    fi
    export QT_IM_MODULE="$qt_module"
    export GTK_IM_MODULE="$gtk_module"
    export XMODIFIERS="$xmodifier"
}

start_ibus_daemon() {
    local mode="${1:-}"
    if [[ "$mode" == "replace" ]]; then
        env -u WAYLAND_DISPLAY \
            GDK_BACKEND=x11 \
            QT_QPA_PLATFORM=xcb \
            GTK_IM_MODULE=ibus \
            QT_IM_MODULE=ibus \
            XMODIFIERS=@im=ibus \
            ibus-daemon -drx --replace >/dev/null 2>&1 || true
    else
        env -u WAYLAND_DISPLAY \
            GDK_BACKEND=x11 \
            QT_QPA_PLATFORM=xcb \
            GTK_IM_MODULE=ibus \
            QT_IM_MODULE=ibus \
            XMODIFIERS=@im=ibus \
            ibus-daemon -drx >/dev/null 2>&1 || true
    fi
}

select_ibus_libpinyin() {
    if ! command -v ibus >/dev/null 2>&1; then
        return 1
    fi
    for _ in 1 2 3 4 5 6 7 8 9 10; do
        if env -u WAYLAND_DISPLAY \
            GTK_IM_MODULE=ibus \
            QT_IM_MODULE=ibus \
            XMODIFIERS=@im=ibus \
            ibus engine libpinyin >/dev/null 2>&1; then
            return 0
        fi
        sleep 0.1
    done
    return 1
}

stop_ibus_daemons() {
    if command -v ibus >/dev/null 2>&1; then
        env -u WAYLAND_DISPLAY ibus exit >/dev/null 2>&1 || true
    fi
    if command -v pkill >/dev/null 2>&1; then
        pkill -x ibus-daemon >/dev/null 2>&1 || true
    fi
    sleep 0.2
}

cmake_cache_value() {
    local cache_file="$1"
    local key="$2"
    awk -F= -v key="$key" '$1 ~ "^" key ":" { print $2; exit }' "$cache_file"
}

client_cache_file() {
    local client_exe="$1"
    case "$client_exe" in
        "${PROJECT_ROOT}"/build-*/bin/MemoChatQml)
            printf '%s\n' "${client_exe%/bin/MemoChatQml}/CMakeCache.txt"
            ;;
        *)
            return 1
            ;;
    esac
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

require_live2d_native_client() {
    local client_exe="$1"
    local cache_file=""
    cache_file="$(client_cache_file "$client_exe" 2>/dev/null || true)"

    if [[ -n "$cache_file" && -f "$cache_file" ]]; then
        if live2d_native_cache_is_valid "$cache_file"; then
            return 0
        fi
        echo "[FAIL] MemoChatQml build cache is stale or non-native for Live2D: $cache_file" >&2
        echo "       Run: source ${ENV_FILE} && cmake --preset linux-full-gcc16 && cmake --build --preset linux-full-gcc16 --parallel 12" >&2
        return 1
    fi

    if command -v strings >/dev/null 2>&1 \
        && strings "$client_exe" | grep -Fq "Live2D Cubism Native OpenGL is not enabled in this build"; then
        echo "[FAIL] MemoChatQml binary was built without Live2D Native OpenGL: $client_exe" >&2
        echo "       Redeploy from build-linux-full-gcc16 after reconfiguring linux-full-gcc16." >&2
        return 1
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
set_default QSG_RENDER_LOOP "threaded"
set_default QSG_RHI_BACKEND "opengl"
set_default QT_OPENGL "desktop"
set_default __GL_THREADED_OPTIMIZATIONS "1"
set_default QTWEBENGINE_DISABLE_SANDBOX "1"
set_default NO_AT_BRIDGE "1"
set_default LANG "C.UTF-8"
set_default LC_ALL "C.UTF-8"
case "${QML_DISABLE_DISK_CACHE:-}" in
    0|false|FALSE|False|no|NO|No)
        unset QML_DISABLE_DISK_CACHE
        ;;
esac
if grep -qiE 'microsoft|wsl' /proc/sys/kernel/osrelease 2>/dev/null; then
    set_default GALLIUM_DRIVER "d3d12"
    set_default MESA_LOADER_DRIVER_OVERRIDE "d3d12"
    set_default LIBGL_ALWAYS_SOFTWARE "0"
fi
maybe_fallback_wslg_rendering

if [[ -z "${DBUS_SESSION_BUS_ADDRESS:-}" ]] && command -v dbus-launch >/dev/null 2>&1; then
    eval "$(dbus-launch --sh-syntax)" >/dev/null 2>&1 || true
fi

# Prefer a real WSL-side IME for Chinese composition. Qt Virtual Keyboard is a
# useful fallback, but if it is selected first it can hide fcitx5/ibus Pinyin.
has_qt_fcitx_plugin=0
has_qt_ibus_plugin=0
if [[ -f "${QT_ROOT}/plugins/platforminputcontexts/libfcitx5platforminputcontextplugin.so" ]]; then
    has_qt_fcitx_plugin=1
fi
if [[ -f "${QT_ROOT}/plugins/platforminputcontexts/libibusplatforminputcontextplugin.so" ]]; then
    has_qt_ibus_plugin=1
fi

if [[ "$has_qt_fcitx_plugin" -eq 1 ]] && (command -v fcitx5 >/dev/null 2>&1 || pgrep -x fcitx5 >/dev/null 2>&1); then
    if command -v fcitx5 >/dev/null 2>&1 && ! pgrep -x fcitx5 >/dev/null 2>&1; then
        env -u WAYLAND_DISPLAY fcitx5 -d >/dev/null 2>&1 || true
    fi
    select_input_method "fcitx" "fcitx" "@im=fcitx"
elif [[ "$has_qt_ibus_plugin" -eq 1 ]] && (command -v ibus-daemon >/dev/null 2>&1 || pgrep -x ibus-daemon >/dev/null 2>&1); then
    set_default IBUS_ENABLE_SYNC_MODE "1"
    if command -v ibus-daemon >/dev/null 2>&1; then
        if [[ "${MEMOCHAT_RESTART_IBUS:-1}" == "1" ]]; then
            stop_ibus_daemons
            start_ibus_daemon
        elif pgrep -x ibus-daemon >/dev/null 2>&1; then
            start_ibus_daemon replace
        else
            start_ibus_daemon
        fi
        sleep 0.15
    fi
    if ! select_ibus_libpinyin && command -v ibus-daemon >/dev/null 2>&1; then
        start_ibus_daemon replace
        select_ibus_libpinyin || true
    fi
    unset WAYLAND_DISPLAY
    select_input_method "ibus" "ibus" "@im=ibus"
elif [[ -f "${QT_ROOT}/plugins/platforminputcontexts/libqtvirtualkeyboardplugin.so" ]]; then
    set_default QT_IM_MODULE "qtvirtualkeyboard"
    set_default QT_VIRTUALKEYBOARD_DESKTOP_DISABLE "0"
fi

if [[ -z "$CLIENT_EXE" ]]; then
    for candidate in \
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
    echo "  QSG_RENDER_LOOP:  ${QSG_RENDER_LOOP:-<unset>}"
    echo "  QSG_RHI_BACKEND:  ${QSG_RHI_BACKEND}"
    echo "  QT_OPENGL:        ${QT_OPENGL:-<unset>}"
    echo "  GALLIUM_DRIVER:   ${GALLIUM_DRIVER:-<unset>}"
    echo "  MESA_DRIVER:      ${MESA_LOADER_DRIVER_OVERRIDE:-<unset>}"
    echo "  GL_SOFTWARE:      ${LIBGL_ALWAYS_SOFTWARE:-<unset>}"
    echo "  GL_FALLBACK:      ${MEMOCHAT_WSLG_GL_FALLBACK:-<unset>}"
    echo "  QML_DISK_CACHE:   ${QML_DISABLE_DISK_CACHE:-enabled}"
    echo "  DISPLAY:          ${DISPLAY:-<unset>}"
    echo "  WAYLAND_DISPLAY:  ${WAYLAND_DISPLAY:-<unset>}"
    echo "  QT_IM_MODULE:     ${QT_IM_MODULE:-<unset>}"
    echo "  GTK_IM_MODULE:    ${GTK_IM_MODULE:-<unset>}"
    echo "  XMODIFIERS:       ${XMODIFIERS:-<unset>}"
    echo "  IBUS_SYNC_MODE:   ${IBUS_ENABLE_SYNC_MODE:-<unset>}"
    echo "  GDK_BACKEND:      ${GDK_BACKEND:-<unset>}"
    echo "============================================================"
    echo

    echo "[input method]"
    pgrep -a fcitx5 || true
    pgrep -a ibus-daemon || true
    if command -v ibus >/dev/null 2>&1; then
        env -u WAYLAND_DISPLAY ibus address 2>/dev/null || true
        env -u WAYLAND_DISPLAY ibus engine 2>/dev/null || true
    fi
    echo

    if [[ -n "${CLIENT_EXE:-}" ]]; then
        local cache_file=""
        case "$CLIENT_EXE" in
            "${PROJECT_ROOT}"/build-*/bin/MemoChatQml)
                cache_file="${CLIENT_EXE%/bin/MemoChatQml}/CMakeCache.txt"
                ;;
        esac
        if [[ -f "$cache_file" ]]; then
            echo "[Live2D build flags]"
            grep -E '^(MEMOCHAT_ENABLE_LIVE2D_NATIVE|MEMOCHAT_LIVE2D_SDK_ROOT|Live2DCubism_ROOT):' "$cache_file" || true
            echo
        fi
        if command -v strings >/dev/null 2>&1; then
            echo "[Live2D binary hints]"
            strings "$CLIENT_EXE" | grep -E 'CubismSdkForNative|Native OpenGL is not enabled|official OpenGL renderer ready' | head -8 || true
            echo
        fi
    fi

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
    echo "       Build it with: source ${ENV_FILE} && cmake --build --preset linux-full-gcc16 --parallel 12" >&2
    exit 1
fi

require_live2d_native_client "$CLIENT_EXE"

cd -- "$(dirname -- "$CLIENT_EXE")"
exec "$CLIENT_EXE" "${APP_ARGS[@]}"
