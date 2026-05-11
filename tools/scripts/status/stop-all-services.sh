#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
RUNTIME_DIR="${MEMOCHAT_RUNTIME_DIR:-${PROJECT_ROOT}/infra/Memo_ops/runtime/services}"
PID_DIR="${MEMOCHAT_PID_DIR:-${PROJECT_ROOT}/infra/Memo_ops/runtime/pids}"
KILL_TIMEOUT="${MEMOCHAT_STOP_TIMEOUT:-8}"

usage() {
    cat <<USAGE
Usage: $0

Stop Linux MemoChat backend service processes started by start-all-services.sh.
Docker containers are intentionally left running.
USAGE
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

port_pids() {
    local port="$1"
    if command -v ss >/dev/null 2>&1; then
        ss -ltnpH 2>/dev/null |
            awk -v suffix=":${port}" '
                index($4, suffix) == length($4) - length(suffix) + 1 { print }
            ' |
            sed -n 's/.*pid=\([0-9][0-9]*\).*/\1/p' |
            sort -u
        return 0
    fi
    if command -v lsof >/dev/null 2>&1; then
        lsof -tiTCP:"${port}" -sTCP:LISTEN 2>/dev/null | sort -u
        return 0
    fi
    return 0
}

wait_pid_exit() {
    local pid="$1"
    local waited=0
    while kill -0 "$pid" >/dev/null 2>&1; do
        if (( waited >= KILL_TIMEOUT )); then
            return 1
        fi
        sleep 1
        waited=$((waited + 1))
    done
    return 0
}

stop_pid() {
    local pid="$1"
    local label="$2"
    [[ -n "$pid" ]] || return 0
    if ! kill -0 "$pid" >/dev/null 2>&1; then
        return 0
    fi

    echo "  [*] Stopping ${label} pid=${pid}"
    kill "$pid" >/dev/null 2>&1 || true
    if ! wait_pid_exit "$pid"; then
        echo "  [WARN] ${label} pid=${pid} did not exit; sending SIGKILL"
        kill -9 "$pid" >/dev/null 2>&1 || true
    fi
}

is_runtime_pid() {
    local pid="$1"
    local cwd=""
    cwd="$(readlink "/proc/${pid}/cwd" 2>/dev/null || true)"
    [[ "$cwd" == "${RUNTIME_DIR}"/* ]]
}

stop_pid_file() {
    local pid_file="$1"
    local label="$2"
    if [[ ! -f "$pid_file" ]]; then
        return 0
    fi
    local pid
    pid="$(cat "$pid_file" 2>/dev/null || true)"
    if [[ -n "$pid" ]] && kill -0 "$pid" >/dev/null 2>&1 && ! is_runtime_pid "$pid"; then
        echo "  [WARN] ${label}: pid ${pid} is not under MemoChat runtime; removing stale pid file"
        rm -f -- "$pid_file"
        return 0
    fi
    stop_pid "$pid" "$label"
    rm -f -- "$pid_file"
}

stop_by_name_under_runtime() {
    local exe_name="$1"
    local label="$2"
    if ! command -v pgrep >/dev/null 2>&1; then
        return 0
    fi

    while IFS= read -r pid; do
        [[ -n "$pid" ]] || continue
        local cwd=""
        cwd="$(readlink "/proc/${pid}/cwd" 2>/dev/null || true)"
        if [[ "$cwd" == "${RUNTIME_DIR}"/* ]]; then
            stop_pid "$pid" "$label"
        fi
    done < <(pgrep -x "$exe_name" 2>/dev/null || true)
}

stop_by_ports() {
    local label="$1"
    shift
    local seen=" "
    local port pid
    for port in "$@"; do
        while IFS= read -r pid; do
            [[ -n "$pid" ]] || continue
            if [[ "$seen" == *" ${pid} "* ]]; then
                continue
            fi
            if ! is_runtime_pid "$pid"; then
                echo "  [WARN] ${label}:${port} pid=${pid} is not under MemoChat runtime; skipped"
                continue
            fi
            seen+="${pid} "
            stop_pid "$pid" "${label}:${port}"
        done < <(port_pids "$port")
    done
}

echo
echo "============================================================"
echo "  MemoChat Linux stop"
echo "  RUNTIME_DIR: ${RUNTIME_DIR}"
echo "  PID_DIR:     ${PID_DIR}"
echo "============================================================"
echo

echo "[STEP] Stop services by PID files"
for name in \
    GateServer-2 GateServer-1 \
    AIServer \
    ChatServer-6 ChatServer-5 ChatServer-4 ChatServer-3 ChatServer-2 ChatServer-1 \
    StatusServer-2 StatusServer-1 \
    VarifyServer-2 VarifyServer-1
do
    stop_pid_file "${PID_DIR}/${name}.pid" "$name"
done

echo
echo "[STEP] Stop remaining listeners on MemoChat service ports"
stop_by_ports "GateServer" 8080 8082 8084 8085
stop_by_ports "AIServer" 8095
stop_by_ports "ChatServer" 8090 8091 8092 8093 8094 8097 50055 50056 50057 50058 50059 50581
stop_by_ports "StatusServer" 50052 50582
stop_by_ports "VarifyServer" 50051 48083 8083 8087

echo
echo "[STEP] Stop remaining runtime processes by executable name"
stop_by_name_under_runtime "GateServer" "GateServer"
stop_by_name_under_runtime "AIServer" "AIServer"
stop_by_name_under_runtime "ChatServer" "ChatServer"
stop_by_name_under_runtime "StatusServer" "StatusServer"
stop_by_name_under_runtime "VarifyServer" "VarifyServer"

echo
echo "Stop command completed. Docker containers were left running."
