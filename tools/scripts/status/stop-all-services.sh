#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
ENV_FILE="${MEMOCHAT_ENV_FILE:-/root/.memochat-linux-env}"
TOPOLOGY_FILE="${SCRIPT_DIR}/runtime_topology.sh"

if [[ -f "$ENV_FILE" ]]; then
    # shellcheck disable=SC1090
    source "$ENV_FILE"
fi

RUNTIME_DIR="${MEMOCHAT_RUNTIME_DIR:-${PROJECT_ROOT}/infra/Memo_ops/runtime/services}"
PID_DIR="${MEMOCHAT_PID_DIR:-${PROJECT_ROOT}/infra/Memo_ops/runtime/pids}"
LOCAL_COMPOSE_FILE="${MEMOCHAT_LOCAL_COMPOSE_FILE:-${PROJECT_ROOT}/infra/deploy/local/docker-compose.yml}"
KILL_TIMEOUT="${MEMOCHAT_STOP_TIMEOUT:-8}"
STOP_ENVOY_OVERRIDE=""
STOP_GPT_SOVITS_OVERRIDE=""
GPT_SOVITS_PORT="${GPT_SOVITS_PORT:-9880}"
GPT_SOVITS_ROOT="${GPT_SOVITS_ROOT:-/data/third_party/GPT-SoVITS}"
GPT_SOVITS_PID_FILE="${PID_DIR}/GPT-SoVITS.pid"

usage() {
    cat <<USAGE
Usage: $0 [--skip-envoy] [--skip-gpt-sovits]

Stop Linux MemoChat backend service processes started by start-all-services.sh.
Docker Envoy Gateway is stopped by default because start-all-services.sh starts
it as the local edge. Pass --skip-envoy or set MEMOCHAT_STOP_ENVOY=0 to leave it
running.
GPT-SoVITS is stopped by default because start-all-services.sh starts it as a
local WSL service. Pass --skip-gpt-sovits or set MEMOCHAT_STOP_GPT_SOVITS=0 to
leave it running.
Database, queue, object storage, AI/RAG, and observability Docker containers are
intentionally left running.
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --skip-envoy|--no-envoy)
            STOP_ENVOY_OVERRIDE=0
            shift
            ;;
        --skip-gpt-sovits|--no-gpt-sovits)
            STOP_GPT_SOVITS_OVERRIDE=0
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

STOP_ENVOY="${STOP_ENVOY_OVERRIDE:-${MEMOCHAT_STOP_ENVOY:-1}}"
STOP_GPT_SOVITS="${STOP_GPT_SOVITS_OVERRIDE:-${MEMOCHAT_STOP_GPT_SOVITS:-1}}"

# shellcheck source=tools/scripts/status/runtime_topology.sh
source "$TOPOLOGY_FILE"

is_truthy() {
    case "${1,,}" in
        1|true|yes|on) return 0 ;;
        *) return 1 ;;
    esac
}

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

udp_port_pids() {
    local port="$1"
    if command -v ss >/dev/null 2>&1; then
        ss -lunpH 2>/dev/null |
            awk -v suffix=":${port}" '
                index($4, suffix) == length($4) - length(suffix) + 1 { print }
            ' |
            sed -n 's/.*pid=\([0-9][0-9]*\).*/\1/p' |
            sort -u
        return 0
    fi
    if command -v lsof >/dev/null 2>&1; then
        lsof -tiUDP:"${port}" 2>/dev/null | sort -u
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

pid_cmdline() {
    local pid="$1"
    tr '\0' ' ' <"/proc/${pid}/cmdline" 2>/dev/null || true
}

is_gpt_sovits_pid() {
    local pid="$1"
    local cmdline=""
    local cwd=""
    cmdline="$(pid_cmdline "$pid")"
    cwd="$(readlink "/proc/${pid}/cwd" 2>/dev/null || true)"
    [[ "$cmdline" == *"api_v2.py"* ]] || return 1
    if [[ "$cmdline" == *" -p ${GPT_SOVITS_PORT}"* || "$cmdline" == *" --port ${GPT_SOVITS_PORT}"* ]]; then
        return 0
    fi
    [[ -n "$GPT_SOVITS_ROOT" && "$cwd" == "$GPT_SOVITS_ROOT"* ]]
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

stop_gpt_sovits_pid_file() {
    local pid_file="$1"
    local label="$2"
    if [[ ! -f "$pid_file" ]]; then
        return 0
    fi
    local pid
    pid="$(cat "$pid_file" 2>/dev/null || true)"
    if [[ -n "$pid" ]] && kill -0 "$pid" >/dev/null 2>&1 && ! is_gpt_sovits_pid "$pid"; then
        echo "  [WARN] ${label}: pid ${pid} is not GPT-SoVITS; removing stale pid file"
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
    done < <(
        {
            pgrep -x "$exe_name" 2>/dev/null || true
            pgrep -f "(^|[[:space:]/])${exe_name}([[:space:]]|$)" 2>/dev/null || true
        } | sort -u
    )
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

stop_by_udp_ports() {
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
                echo "  [WARN] ${label}:udp/${port} pid=${pid} is not under MemoChat runtime; skipped"
                continue
            fi
            seen+="${pid} "
            stop_pid "$pid" "${label}:udp/${port}"
        done < <(udp_port_pids "$port")
    done
}

collect_group_stop_ports() {
    local wanted_group="$1"
    local kind="$2"
    local seen=" "
    local row group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace
    local ports port
    for row in "${MEMOCHAT_RUNTIME_SERVICE_TOPOLOGY[@]}"; do
        IFS='|' read -r group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace <<<"$row"
        [[ "$group" == "$wanted_group" ]] || continue
        if [[ "$kind" == "udp" ]]; then
            ports="$stop_udp_ports"
        else
            ports="$stop_tcp_ports"
        fi
        for port in $ports; do
            if [[ "$seen" == *" ${port} "* ]]; then
                continue
            fi
            seen+="${port} "
            printf '%s\n' "$port"
        done
    done
}

stop_group_tcp_ports() {
    local label="$1"
    local group="$2"
    local ports=()
    mapfile -t ports < <(collect_group_stop_ports "$group" "tcp")
    if ((${#ports[@]})); then
        stop_by_ports "$label" "${ports[@]}"
    fi
}

stop_group_udp_ports() {
    local label="$1"
    local group="$2"
    local ports=()
    mapfile -t ports < <(collect_group_stop_ports "$group" "udp")
    if ((${#ports[@]})); then
        stop_by_udp_ports "$label" "${ports[@]}"
    fi
}

stop_topology_port_groups() {
    local row group label protocols protocol
    for row in "${MEMOCHAT_STOP_PORT_GROUP_ORDER[@]}"; do
        IFS='|' read -r group label protocols <<<"$row"
        for protocol in $protocols; do
            case "$protocol" in
                tcp)
                    stop_group_tcp_ports "$label" "$group"
                    ;;
                udp)
                    stop_group_udp_ports "$label" "$group"
                    ;;
                *)
                    echo "  [WARN] Unknown stop protocol '${protocol}' for topology group '${group}'"
                    ;;
            esac
        done
    done
}

stop_gpt_sovits() {
    if ! is_truthy "$STOP_GPT_SOVITS"; then
        echo "  [SKIP] GPT-SoVITS stop disabled"
        return 0
    fi

    stop_gpt_sovits_pid_file "$GPT_SOVITS_PID_FILE" "GPT-SoVITS"

    local seen=" "
    local pid
    while IFS= read -r pid; do
        [[ -n "$pid" ]] || continue
        if [[ "$seen" == *" ${pid} "* ]]; then
            continue
        fi
        seen+="${pid} "
        if is_gpt_sovits_pid "$pid"; then
            stop_pid "$pid" "GPT-SoVITS:${GPT_SOVITS_PORT}"
        else
            echo "  [WARN] GPT-SoVITS:${GPT_SOVITS_PORT} pid=${pid} is not a GPT-SoVITS API process; skipped"
        fi
    done < <(port_pids "$GPT_SOVITS_PORT")

    if command -v pgrep >/dev/null 2>&1; then
        while IFS= read -r pid; do
            [[ -n "$pid" ]] || continue
            if [[ "$seen" == *" ${pid} "* ]]; then
                continue
            fi
            seen+="${pid} "
            if is_gpt_sovits_pid "$pid"; then
                stop_pid "$pid" "GPT-SoVITS"
            fi
        done < <(pgrep -f "api_v2.py.*-p ${GPT_SOVITS_PORT}" 2>/dev/null || true)
    fi
}

stop_envoy_gateway() {
    if ! is_truthy "$STOP_ENVOY"; then
        echo "  [SKIP] Envoy Gateway stop disabled"
        return 0
    fi

    if [[ ! -f "$LOCAL_COMPOSE_FILE" ]]; then
        echo "  [WARN] Local compose file not found: ${LOCAL_COMPOSE_FILE}; Envoy Gateway was not stopped"
        return 0
    fi

    echo "  [*] Stopping Docker Envoy Gateway from ${LOCAL_COMPOSE_FILE}"
    if ! docker compose -f "$LOCAL_COMPOSE_FILE" stop memochat-envoy-gateway; then
        echo "  [WARN] Envoy Gateway stop failed; Docker may be unavailable"
    fi
}

echo
echo "============================================================"
echo "  MemoChat Linux stop"
echo "  RUNTIME_DIR: ${RUNTIME_DIR}"
echo "  PID_DIR:     ${PID_DIR}"
echo "  COMPOSE:     ${LOCAL_COMPOSE_FILE}"
echo "============================================================"
echo

echo "[STEP] Stop GPT-SoVITS voice service"
stop_gpt_sovits
echo

echo "[STEP] Stop services by PID files"
for name in "${MEMOCHAT_STOP_PID_ORDER[@]}"; do
    stop_pid_file "${PID_DIR}/${name}.pid" "$name"
done

echo
echo "[STEP] Stop remaining listeners on MemoChat service ports"
stop_topology_port_groups

echo
echo "[STEP] Stop remaining runtime processes by executable name"
for exe_name in "${MEMOCHAT_STOP_EXECUTABLE_ORDER[@]}"; do
    stop_by_name_under_runtime "$exe_name" "$exe_name"
done

echo
echo "[STEP] Stop Docker Envoy Gateway"
stop_envoy_gateway

echo
echo "Stop command completed. Non-Envoy Docker dependencies were left running."
