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

usage() {
    cat <<USAGE
Usage: $0 [--no-deploy] [--wait-seconds N]

Start Linux MemoChat backend services from:
  ${RUNTIME_DIR}

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

mkdir -p -- "$LOG_DIR" "$PID_DIR"

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
