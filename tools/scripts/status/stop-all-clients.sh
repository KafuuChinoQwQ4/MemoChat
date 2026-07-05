#!/usr/bin/env bash
# stop-all-clients.sh — stop Qt desktop client and web frontend processes
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
ENV_FILE="${MEMOCHAT_ENV_FILE:-/root/.memochat-linux-env}"
WEB_PORT="${MEMOCHAT_WEB_PORT:-5173}"

if [[ -f "$ENV_FILE" ]]; then
    # shellcheck disable=SC1090
    source "$ENV_FILE"
fi

usage() {
    cat <<USAGE
Usage: $0

Stop MemoChatQml desktop client and web frontend (Vite dev server).
Does NOT stop backend services — use stop-all-services.sh for that.
USAGE
}

case "${1:-}" in -h|--help) usage; exit 0 ;; esac

stop_by_name() {
    local name="$1"
    local pids
    if pids=$(pgrep -x "$name" 2>/dev/null); then
        echo "[STOP] ${name} (PIDs: ${pids})"
        pkill -TERM -x "$name" 2>/dev/null || true
        sleep 1
        pkill -KILL -x "$name" 2>/dev/null || true
    else
        echo "[    ] ${name} — not running"
    fi
}

stop_vite_on_port() {
    local port="$1"
    local pid
    if pid=$(lsof -ti TCP:"$port" 2>/dev/null | head -1); [[ -n "$pid" ]]; then
        echo "[STOP] Vite dev server on port ${port} (PID: ${pid})"
        kill -TERM "$pid" 2>/dev/null || true
        sleep 1
        kill -KILL "$pid" 2>/dev/null || true
    else
        echo "[    ] Vite dev server on port ${port} — not running"
    fi
}

echo "=== Stopping MemoChat client processes ==="
echo

echo "[STEP] Stop Qt desktop client"
stop_by_name "MemoChatQml"
echo

echo "[STEP] Stop web frontend (Vite dev server on port ${WEB_PORT})"
stop_vite_on_port "$WEB_PORT"
# Also try by process name in case port differs
pkill -f "vite.*memochat-web" 2>/dev/null || true
echo

echo "Client stop complete."
