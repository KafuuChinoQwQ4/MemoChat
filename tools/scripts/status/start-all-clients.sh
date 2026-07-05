#!/usr/bin/env bash
# start-all-clients.sh — launch Qt desktop client + web frontend together
#
# Workflow:
#   1. Web frontend starts in background (Vite dev server)
#   2. Qt desktop client starts in foreground
#   3. When the Qt window closes, the web frontend is automatically stopped
#
# This script does NOT start backend services — run start-all-services.sh first.
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="${MEMOCHAT_ENV_FILE:-/root/.memochat-linux-env}"

QML_SCRIPT="${SCRIPT_DIR}/start-memochat-qml-wslg.sh"
WEB_SCRIPT="${SCRIPT_DIR}/start-memochat-web.sh"

WEB_PORT="${MEMOCHAT_WEB_PORT:-5173}"
SKIP_WEB=0
SKIP_QML=0
QML_ARGS=()
WEB_PID=""

usage() {
    cat <<USAGE
Usage: $0 [--skip-web] [--skip-qml] [--web-port N] [--exe PATH] [-- qml-args...]

Start both MemoChat clients (Qt desktop + web frontend).
Backend services must already be running (use start-all-services.sh first).

Options:
  --skip-web      Only start the Qt desktop client.
  --skip-qml      Only start the web frontend.
  --web-port N    Vite dev server port (default: 5173).
  --exe PATH      Use a specific MemoChatQml binary.
  --diagnose      Forward to start-memochat-qml-wslg.sh for Qt diagnostics.
  --              Remaining args forwarded to MemoChatQml.
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --skip-web)   SKIP_WEB=1;  shift ;;
        --skip-qml)   SKIP_QML=1;  shift ;;
        --web-port)   WEB_PORT="$2"; shift 2 ;;
        --exe|--diagnose)
            QML_ARGS+=("$1" "${2:-}"); [[ "$1" == "--exe" ]] && shift; shift ;;
        --)           shift; QML_ARGS+=("$@"); break ;;
        -h|--help)    usage; exit 0 ;;
        *)            QML_ARGS+=("$1"); shift ;;
    esac
done

if [[ -f "$ENV_FILE" ]]; then
    # shellcheck disable=SC1090
    source "$ENV_FILE"
fi

# ── Start web frontend in background ─────────────────────────────────────────
if [[ "$SKIP_WEB" -eq 0 ]]; then
    if [[ ! -x "$WEB_SCRIPT" ]]; then
        echo "[WARN] Web script not found: ${WEB_SCRIPT}" >&2
        echo "       Web frontend will not start." >&2
    else
        echo "[INFO] Starting web frontend → http://localhost:${WEB_PORT}" >&2
        MEMOCHAT_WEB_PORT="$WEB_PORT" bash "$WEB_SCRIPT" --port "$WEB_PORT" &
        WEB_PID=$!
        echo "[INFO] Web frontend PID: ${WEB_PID}" >&2
    fi
fi

# Stop web frontend when this script exits (desktop client closed or crash)
cleanup() {
    if [[ -n "$WEB_PID" ]] && kill -0 "$WEB_PID" 2>/dev/null; then
        echo "[INFO] Stopping web frontend (PID ${WEB_PID})…" >&2
        kill "$WEB_PID" 2>/dev/null || true
        wait "$WEB_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT

# ── Start Qt desktop client in foreground ────────────────────────────────────
if [[ "$SKIP_QML" -eq 0 ]]; then
    if [[ ! -x "$QML_SCRIPT" ]]; then
        echo "[FAIL] QML script not found: ${QML_SCRIPT}" >&2
        exit 1
    fi
    echo "[INFO] Starting MemoChatQml…" >&2
    bash "$QML_SCRIPT" "${QML_ARGS[@]}"
else
    echo "[INFO] --skip-qml: Qt desktop client skipped. Web frontend running." >&2
    echo "       Press Ctrl+C to stop." >&2
    wait "$WEB_PID" 2>/dev/null || true
fi
