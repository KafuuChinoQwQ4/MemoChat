#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
RUNTIME_DIR="${MEMOCHAT_RUNTIME_DIR:-${PROJECT_ROOT}/infra/Memo_ops/runtime/services}"
LOG_DIR="${MEMOCHAT_LOG_DIR:-${PROJECT_ROOT}/infra/Memo_ops/artifacts/logs/services}"
PID_DIR="${MEMOCHAT_PID_DIR:-${PROJECT_ROOT}/infra/Memo_ops/runtime/pids}"
ENV_FILE="${MEMOCHAT_ENV_FILE:-/root/.memochat-linux-env}"
WAIT_SECONDS=16
AUTO_DEPLOY=1
START_GPT_SOVITS_OVERRIDE=""
GPT_SOVITS_WAIT_SECONDS_OVERRIDE=""

usage() {
    cat <<USAGE
Usage: $0 [--no-deploy] [--wait-seconds N] [--skip-gpt-sovits] [--gpt-sovits-wait-seconds N]

Start Linux MemoChat backend services from:
  ${RUNTIME_DIR}

GPT-SoVITS is started as a local WSL service by default so AIOrchestrator can
reach http://host.docker.internal:9880. Set MEMOCHAT_START_GPT_SOVITS=0 or pass
--skip-gpt-sovits to skip it.

Docker dependencies are not started by this script. Start them separately first.
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-deploy)
            AUTO_DEPLOY=0
            shift
            ;;
        --wait-seconds)
            WAIT_SECONDS="$2"
            shift 2
            ;;
        --skip-gpt-sovits|--no-gpt-sovits)
            START_GPT_SOVITS_OVERRIDE=0
            shift
            ;;
        --gpt-sovits-wait-seconds)
            GPT_SOVITS_WAIT_SECONDS_OVERRIDE="$2"
            shift 2
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

if [[ -f "$ENV_FILE" ]]; then
    # shellcheck disable=SC1090
    source "$ENV_FILE"
fi

export MEMOCHAT_ENABLE_KAFKA="${MEMOCHAT_ENABLE_KAFKA:-1}"
export MEMOCHAT_ENABLE_RABBITMQ="${MEMOCHAT_ENABLE_RABBITMQ:-1}"
START_GPT_SOVITS="${START_GPT_SOVITS_OVERRIDE:-${MEMOCHAT_START_GPT_SOVITS:-1}}"
GPT_SOVITS_PORT="${GPT_SOVITS_PORT:-9880}"
GPT_SOVITS_HOST="${GPT_SOVITS_HOST:-0.0.0.0}"
GPT_SOVITS_API_URL="${GPT_SOVITS_API_URL:-http://127.0.0.1:${GPT_SOVITS_PORT}}"
GPT_SOVITS_START_SCRIPT="${GPT_SOVITS_START_SCRIPT:-${PROJECT_ROOT}/tools/scripts/pet/start_gpt_sovits_api_wsl.sh}"
GPT_SOVITS_WAIT_SECONDS="${GPT_SOVITS_WAIT_SECONDS_OVERRIDE:-${MEMOCHAT_GPT_SOVITS_WAIT_SECONDS:-${GPT_SOVITS_WAIT_SECONDS:-180}}}"
GPT_SOVITS_PID_FILE="${PID_DIR}/GPT-SoVITS.pid"
GPT_SOVITS_LOG_FILE="${GPT_SOVITS_LOG_FILE:-${LOG_DIR}/GPT-SoVITS_api.log}"
GPT_SOVITS_LOG_DIR="${GPT_SOVITS_LOG_DIR:-$(dirname -- "$GPT_SOVITS_LOG_FILE")}"

mkdir -p -- "$LOG_DIR" "$PID_DIR" "$GPT_SOVITS_LOG_DIR"

is_truthy() {
    case "${1,,}" in
        1|true|yes|on) return 0 ;;
        *) return 1 ;;
    esac
}

port_listening() {
    local port="$1"
    if command -v ss >/dev/null 2>&1; then
        ss -ltnH 2>/dev/null | awk -v suffix=":${port}" '
            index($4, suffix) == length($4) - length(suffix) + 1 { found = 1 }
            END { exit found ? 0 : 1 }
        '
        return $?
    fi
    if command -v lsof >/dev/null 2>&1; then
        lsof -nP -iTCP:"${port}" -sTCP:LISTEN >/dev/null 2>&1
        return $?
    fi
    return 1
}

port_pid() {
    local port="$1"
    if command -v ss >/dev/null 2>&1; then
        ss -ltnpH 2>/dev/null |
            awk -v suffix=":${port}" '
                index($4, suffix) == length($4) - length(suffix) + 1 { print }
            ' |
            sed -n 's/.*pid=\([0-9][0-9]*\).*/\1/p' |
            head -n 1
        return 0
    fi
    if command -v lsof >/dev/null 2>&1; then
        lsof -tiTCP:"${port}" -sTCP:LISTEN 2>/dev/null | head -n 1
        return 0
    fi
    return 0
}

port_listen_addresses() {
    local port="$1"
    if command -v ss >/dev/null 2>&1; then
        ss -ltnH 2>/dev/null |
            awk -v suffix=":${port}" '
                index($4, suffix) == length($4) - length(suffix) + 1 { print $4 }
            '
    fi
}

pid_cmdline() {
    local pid="$1"
    tr '\0' ' ' <"/proc/${pid}/cmdline" 2>/dev/null || true
}

is_gpt_sovits_pid() {
    local pid="$1"
    local cmdline=""
    cmdline="$(pid_cmdline "$pid")"
    [[ "$cmdline" == *"api_v2.py"* ]]
}

gpt_sovits_loopback_only() {
    local has_loopback=0
    local has_wildcard=0
    local address
    while IFS= read -r address; do
        case "$address" in
            127.0.0.1:${GPT_SOVITS_PORT}|\[::1\]:${GPT_SOVITS_PORT})
                has_loopback=1
                ;;
            0.0.0.0:${GPT_SOVITS_PORT}|\[::\]:${GPT_SOVITS_PORT}|\*:${GPT_SOVITS_PORT})
                has_wildcard=1
                ;;
        esac
    done < <(port_listen_addresses "$GPT_SOVITS_PORT")
    [[ "$has_loopback" -eq 1 && "$has_wildcard" -eq 0 ]]
}

stop_gpt_sovits_for_rebind() {
    local pid
    pid="$(port_pid "$GPT_SOVITS_PORT")"
    if [[ -z "$pid" ]]; then
        return 0
    fi
    if ! is_gpt_sovits_pid "$pid"; then
        echo "  [WARN] Port ${GPT_SOVITS_PORT} is loopback-only but pid=${pid} is not GPT-SoVITS; skipped restart"
        return 1
    fi

    echo "  [*] Restarting GPT-SoVITS because it is bound only to loopback"
    kill "$pid" >/dev/null 2>&1 || true
    local waited=0
    while kill -0 "$pid" >/dev/null 2>&1 && (( waited < 8 )); do
        sleep 1
        waited=$((waited + 1))
    done
    if kill -0 "$pid" >/dev/null 2>&1; then
        kill -9 "$pid" >/dev/null 2>&1 || true
    fi
    rm -f -- "$GPT_SOVITS_PID_FILE"
}

sanitize_name() {
    local value="$1"
    value="${value//:/_}"
    value="${value// /_}"
    printf '%s' "$value"
}

pid_running() {
    local pid_file="$1"
    [[ -f "$pid_file" ]] || return 1
    local pid
    pid="$(cat "$pid_file" 2>/dev/null || true)"
    [[ -n "$pid" ]] || return 1
    kill -0 "$pid" >/dev/null 2>&1
}

ensure_runtime() {
    if [[ -x "${RUNTIME_DIR}/GateServer1/GateServer" && -x "${RUNTIME_DIR}/AIServer/AIServer" ]]; then
        return 0
    fi
    if [[ "$AUTO_DEPLOY" -eq 0 ]]; then
        echo "[FAIL] Runtime files missing under ${RUNTIME_DIR}" >&2
        echo "       Run ${SCRIPT_DIR}/deploy_services.sh first." >&2
        exit 1
    fi
    echo "[WARN] C++ runtime missing, deploying services..."
    "${SCRIPT_DIR}/deploy_services.sh"
}

wait_for_port() {
    local service_name="$1"
    local port="$2"
    local waited=0
    while (( waited < WAIT_SECONDS )); do
        if port_listening "$port"; then
            echo "  [OK] ${service_name} listening on port ${port}"
            return 0
        fi
        sleep 1
        waited=$((waited + 1))
    done
    echo "  [WARN] ${service_name} did not bind port ${port} yet; check logs"
    return 0
}

gpt_sovits_ready() {
    curl -fsS "${GPT_SOVITS_API_URL%/}/docs" >/dev/null 2>&1
}

wait_for_gpt_sovits() {
    local waited=0
    while (( waited < GPT_SOVITS_WAIT_SECONDS )); do
        if gpt_sovits_ready; then
            echo "  [OK] GPT-SoVITS ready at ${GPT_SOVITS_API_URL}"
            return 0
        fi
        sleep 1
        waited=$((waited + 1))
    done
    echo "  [WARN] GPT-SoVITS did not become ready at ${GPT_SOVITS_API_URL}; check logs"
    return 0
}

remember_gpt_sovits_pid_from_port() {
    local pid
    pid="$(port_pid "$GPT_SOVITS_PORT")"
    if [[ -n "$pid" ]]; then
        echo "$pid" > "$GPT_SOVITS_PID_FILE"
    fi
}

start_gpt_sovits() {
    if ! is_truthy "$START_GPT_SOVITS"; then
        echo "  [SKIP] GPT-SoVITS startup disabled"
        return 0
    fi

    if gpt_sovits_loopback_only; then
        stop_gpt_sovits_for_rebind || return 0
    fi

    if gpt_sovits_ready; then
        remember_gpt_sovits_pid_from_port
        echo "  [OK] GPT-SoVITS already ready at ${GPT_SOVITS_API_URL}"
        return 0
    fi

    if pid_running "$GPT_SOVITS_PID_FILE"; then
        echo "  [WARN] GPT-SoVITS already running with pid $(cat "$GPT_SOVITS_PID_FILE"); waiting for readiness"
        wait_for_gpt_sovits
        return 0
    fi

    if port_listening "$GPT_SOVITS_PORT"; then
        remember_gpt_sovits_pid_from_port
        echo "  [WARN] GPT-SoVITS port ${GPT_SOVITS_PORT} is listening but ${GPT_SOVITS_API_URL}/docs is not ready"
        wait_for_gpt_sovits
        return 0
    fi

    if [[ ! -f "$GPT_SOVITS_START_SCRIPT" ]]; then
        echo "  [WARN] GPT-SoVITS start script missing: ${GPT_SOVITS_START_SCRIPT}"
        return 0
    fi

    local out_log="${LOG_DIR}/GPT-SoVITS_out.log"
    local err_log="${LOG_DIR}/GPT-SoVITS_err.log"
    : > "$out_log"
    : > "$err_log"

    echo "  [*] Starting GPT-SoVITS on ${GPT_SOVITS_HOST}:${GPT_SOVITS_PORT}..."
    (
        cd "$PROJECT_ROOT"
        exec nohup env \
            GPT_SOVITS_HOST="$GPT_SOVITS_HOST" \
            GPT_SOVITS_PORT="$GPT_SOVITS_PORT" \
            GPT_SOVITS_LOG_DIR="$GPT_SOVITS_LOG_DIR" \
            GPT_SOVITS_LOG_FILE="$GPT_SOVITS_LOG_FILE" \
            bash "$GPT_SOVITS_START_SCRIPT"
    ) >>"$out_log" 2>>"$err_log" </dev/null &

    local pid=$!
    echo "$pid" > "$GPT_SOVITS_PID_FILE"
    echo "  [OK] GPT-SoVITS started pid=${pid}"
    wait_for_gpt_sovits
}

launch_svc() {
    local svc_dir="$1"
    local exe_name="$2"
    local svc_name="$3"
    local port="$4"
    local exe_path="${svc_dir}/${exe_name}"
    local safe_name
    safe_name="$(sanitize_name "$svc_name")"
    local pid_file="${PID_DIR}/${safe_name}.pid"
    local out_log="${LOG_DIR}/${safe_name}_out.log"
    local err_log="${LOG_DIR}/${safe_name}_err.log"

    if [[ ! -x "$exe_path" ]]; then
        echo "  [X] ${svc_name}: not found or not executable: ${exe_path}"
        return 0
    fi

    if pid_running "$pid_file"; then
        echo "  [WARN] ${svc_name}: already running with pid $(cat "$pid_file")"
        return 0
    fi

    if [[ -n "$port" ]] && port_listening "$port"; then
        echo "  [WARN] ${svc_name}: port ${port} is already listening"
        return 0
    fi

    : > "$out_log"
    : > "$err_log"

    echo "  [*] Starting ${svc_name}..."
    (
        cd "$svc_dir"
        exec nohup env \
            MEMOCHAT_ENABLE_KAFKA="$MEMOCHAT_ENABLE_KAFKA" \
            MEMOCHAT_ENABLE_RABBITMQ="$MEMOCHAT_ENABLE_RABBITMQ" \
            "./${exe_name}"
    ) >>"$out_log" 2>>"$err_log" </dev/null &

    local pid=$!
    echo "$pid" > "$pid_file"
    echo "  [OK] ${svc_name} started pid=${pid}"

    if [[ -n "$port" ]]; then
        wait_for_port "$svc_name" "$port"
    fi
}

echo "============================================================"
echo "  MemoChat Linux startup"
echo "  PROJECT_ROOT: ${PROJECT_ROOT}"
echo "  RUNTIME_DIR:  ${RUNTIME_DIR}"
echo "  LOG_DIR:      ${LOG_DIR}"
echo "  PID_DIR:      ${PID_DIR}"
echo "============================================================"
echo

echo "[STEP] Start GPT-SoVITS voice service"
start_gpt_sovits
echo

ensure_runtime

echo "[STEP] Start VarifyServer"
launch_svc "${RUNTIME_DIR}/VarifyServer1" "VarifyServer" "VarifyServer-1" "50051"
launch_svc "${RUNTIME_DIR}/VarifyServer2" "VarifyServer" "VarifyServer-2" "48083"
echo

echo "[STEP] Start StatusServer"
launch_svc "${RUNTIME_DIR}/StatusServer1" "StatusServer" "StatusServer-1" "50052"
launch_svc "${RUNTIME_DIR}/StatusServer2" "StatusServer" "StatusServer-2" "50582"
echo

echo "[STEP] Start ChatServer"
launch_svc "${RUNTIME_DIR}/chatserver1" "ChatServer" "ChatServer-1" "8090"
launch_svc "${RUNTIME_DIR}/chatserver2" "ChatServer" "ChatServer-2" "8091"
launch_svc "${RUNTIME_DIR}/chatserver3" "ChatServer" "ChatServer-3" "8092"
launch_svc "${RUNTIME_DIR}/chatserver4" "ChatServer" "ChatServer-4" "8093"
launch_svc "${RUNTIME_DIR}/chatserver5" "ChatServer" "ChatServer-5" "8094"
launch_svc "${RUNTIME_DIR}/chatserver6" "ChatServer" "ChatServer-6" "8097"
echo

echo "[STEP] Start AIServer"
launch_svc "${RUNTIME_DIR}/AIServer" "AIServer" "AIServer" "8095"
echo

echo "[STEP] Start GateServer"
launch_svc "${RUNTIME_DIR}/GateServer1" "GateServer" "GateServer-1" "8080"
launch_svc "${RUNTIME_DIR}/GateServer2" "GateServer" "GateServer-2" "8084"
echo

echo "Startup command completed. Logs are under:"
echo "  ${LOG_DIR}"
