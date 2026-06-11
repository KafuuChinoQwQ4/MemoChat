#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
TOPOLOGY_FILE="${SCRIPT_DIR}/runtime_topology.sh"
RUNTIME_DIR="${MEMOCHAT_RUNTIME_DIR:-${PROJECT_ROOT}/infra/Memo_ops/runtime/services}"
LOG_DIR="${MEMOCHAT_LOG_DIR:-${PROJECT_ROOT}/infra/Memo_ops/artifacts/logs/services}"
PID_DIR="${MEMOCHAT_PID_DIR:-${PROJECT_ROOT}/infra/Memo_ops/runtime/pids}"
ENV_FILE="${MEMOCHAT_ENV_FILE:-/root/.memochat-linux-env}"
LOCAL_COMPOSE_FILE="${MEMOCHAT_LOCAL_COMPOSE_FILE:-${PROJECT_ROOT}/infra/deploy/local/docker-compose.yml}"
WAIT_SECONDS=16
AUTO_DEPLOY=1
START_CORE_SERVICES_OVERRIDE=""
START_CHAT_DELIVERY_WORKER_OVERRIDE=""
START_RELATION_QUERY_SERVICE_OVERRIDE=""
START_RELATION_SERVICE_WORKER_OVERRIDE=""
START_MESSAGE_SERVICE_OVERRIDE=""
START_ENVOY_OVERRIDE=""
START_DOCKER_DEPS_OVERRIDE=""
START_GPT_SOVITS_OVERRIDE=""
GPT_SOVITS_WAIT_SECONDS_OVERRIDE=""

usage() {
    cat <<USAGE
Usage: $0 [--no-deploy] [--wait-seconds N] [--skip-docker-deps] [--skip-envoy] [--skip-gpt-sovits] [--gpt-sovits-wait-seconds N] [--start-chat-delivery-worker] [--start-relation-query-service] [--skip-relation-query-service] [--start-relation-service-worker] [--skip-relation-service-worker] [--start-message-service] [--skip-message-service] [--skip-core-services]

Start Linux MemoChat backend services from:
  ${RUNTIME_DIR}

Docker Envoy Gateway is started by default from:
  ${LOCAL_COMPOSE_FILE}

Docker dependencies required by login, post-login bootstrap, media, and async
side effects are started by default from the same compose file. Set
MEMOCHAT_START_DOCKER_DEPS=0 or pass --skip-docker-deps only for specialized
runtime debugging where those containers are already managed separately.

GPT-SoVITS is started and required as a local WSL service by default so
AIOrchestrator can reach http://host.docker.internal:9880. Set
MEMOCHAT_START_GPT_SOVITS=0 or pass --skip-gpt-sovits to skip it. Set
MEMOCHAT_REQUIRE_GPT_SOVITS=0 only when text-only pet replies are acceptable.

Observability and AI/RAG containers are not started by this script. Start the
broader Docker stack separately when those dependencies are needed.
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
        --skip-envoy|--no-envoy)
            START_ENVOY_OVERRIDE=0
            shift
            ;;
        --skip-docker-deps|--no-docker-deps)
            START_DOCKER_DEPS_OVERRIDE=0
            shift
            ;;
        --skip-gpt-sovits|--no-gpt-sovits)
            START_GPT_SOVITS_OVERRIDE=0
            shift
            ;;
        --start-chat-delivery-worker)
            START_CHAT_DELIVERY_WORKER_OVERRIDE=1
            shift
            ;;
        --start-relation-query-service)
            START_RELATION_QUERY_SERVICE_OVERRIDE=1
            shift
            ;;
        --skip-relation-query-service)
            START_RELATION_QUERY_SERVICE_OVERRIDE=0
            shift
            ;;
        --start-relation-service-worker)
            START_RELATION_SERVICE_WORKER_OVERRIDE=1
            shift
            ;;
        --skip-relation-service-worker)
            START_RELATION_SERVICE_WORKER_OVERRIDE=0
            shift
            ;;
        --start-message-service)
            START_MESSAGE_SERVICE_OVERRIDE=1
            shift
            ;;
        --skip-message-service)
            START_MESSAGE_SERVICE_OVERRIDE=0
            shift
            ;;
        --skip-core-services)
            START_CORE_SERVICES_OVERRIDE=0
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
export MEMOCHAT_ENABLE_QUIC="${MEMOCHAT_ENABLE_QUIC:-1}"
START_CORE_SERVICES="${START_CORE_SERVICES_OVERRIDE:-${MEMOCHAT_START_CORE_SERVICES:-1}}"
START_CHAT_DELIVERY_WORKER="${START_CHAT_DELIVERY_WORKER_OVERRIDE:-${MEMOCHAT_START_CHAT_DELIVERY_WORKER:-0}}"
START_RELATION_QUERY_SERVICE="${START_RELATION_QUERY_SERVICE_OVERRIDE:-${MEMOCHAT_START_RELATION_QUERY_SERVICE:-1}}"
START_RELATION_SERVICE_WORKER="${START_RELATION_SERVICE_WORKER_OVERRIDE:-${MEMOCHAT_START_RELATION_SERVICE_WORKER:-1}}"
START_MESSAGE_SERVICE="${START_MESSAGE_SERVICE_OVERRIDE:-${MEMOCHAT_START_MESSAGE_SERVICE:-1}}"
START_DOCKER_DEPS="${START_DOCKER_DEPS_OVERRIDE:-${MEMOCHAT_START_DOCKER_DEPS:-1}}"
START_ENVOY="${START_ENVOY_OVERRIDE:-${MEMOCHAT_START_ENVOY:-1}}"
START_GPT_SOVITS="${START_GPT_SOVITS_OVERRIDE:-${MEMOCHAT_START_GPT_SOVITS:-1}}"
REQUIRE_GPT_SOVITS="${MEMOCHAT_REQUIRE_GPT_SOVITS:-1}"
GPT_SOVITS_PORT="${GPT_SOVITS_PORT:-9880}"
GPT_SOVITS_HOST="${GPT_SOVITS_HOST:-0.0.0.0}"
GPT_SOVITS_API_URL="${GPT_SOVITS_API_URL:-http://127.0.0.1:${GPT_SOVITS_PORT}}"
GPT_SOVITS_START_SCRIPT="${GPT_SOVITS_START_SCRIPT:-${PROJECT_ROOT}/tools/scripts/pet/start_gpt_sovits_api_wsl.sh}"
GPT_SOVITS_WAIT_SECONDS="${GPT_SOVITS_WAIT_SECONDS_OVERRIDE:-${MEMOCHAT_GPT_SOVITS_WAIT_SECONDS:-${GPT_SOVITS_WAIT_SECONDS:-180}}}"
GPT_SOVITS_PID_FILE="${PID_DIR}/GPT-SoVITS.pid"
GPT_SOVITS_LOG_FILE="${GPT_SOVITS_LOG_FILE:-${LOG_DIR}/GPT-SoVITS_api.log}"
GPT_SOVITS_LOG_DIR="${GPT_SOVITS_LOG_DIR:-$(dirname -- "$GPT_SOVITS_LOG_FILE")}"

# shellcheck source=tools/scripts/status/runtime_topology.sh
source "$TOPOLOGY_FILE"

mkdir -p -- "$LOG_DIR" "$PID_DIR" "$GPT_SOVITS_LOG_DIR"

is_truthy() {
    case "${1,,}" in
        1|true|yes|on) return 0 ;;
        *) return 1 ;;
    esac
}

wait_for_minio() {
    local waited=0
    while (( waited < WAIT_SECONDS )); do
        if curl -fsS http://127.0.0.1:9000/minio/health/live >/dev/null 2>&1; then
            echo "  [OK] memochat-minio ready at http://127.0.0.1:9000"
            return 0
        fi
        sleep 1
        waited=$((waited + 1))
    done
    echo "  [WARN] memochat-minio did not answer health within ${WAIT_SECONDS}s; check docker compose ps memochat-minio"
    return 0
}

wait_for_redpanda() {
    local waited=0
    while (( waited < WAIT_SECONDS )); do
        if docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092 >/dev/null 2>&1; then
            echo "  [OK] memochat-redpanda ready at 127.0.0.1:19092"
            return 0
        fi
        sleep 1
        waited=$((waited + 1))
    done
    echo "  [WARN] memochat-redpanda did not answer cluster info within ${WAIT_SECONDS}s; check docker compose ps memochat-redpanda"
    return 0
}

ensure_docker_dependencies() {
    if ! is_truthy "$START_DOCKER_DEPS"; then
        echo "  [SKIP] Docker dependency startup disabled"
        return 0
    fi

    if [[ ! -f "$LOCAL_COMPOSE_FILE" ]]; then
        echo "  [FAIL] Local compose file not found: ${LOCAL_COMPOSE_FILE}" >&2
        return 1
    fi

    echo "  [*] Starting Docker dependencies from ${LOCAL_COMPOSE_FILE}"
    docker compose -f "$LOCAL_COMPOSE_FILE" up -d \
        memochat-redis \
        memochat-postgres \
        memochat-mongo \
        memochat-minio \
        memochat-redpanda \
        memochat-rabbitmq

    wait_for_minio
    wait_for_redpanda
}

ensure_envoy_gateway() {
    if ! is_truthy "$START_ENVOY"; then
        echo "  [SKIP] Envoy Gateway startup disabled"
        return 0
    fi

    if [[ ! -f "$LOCAL_COMPOSE_FILE" ]]; then
        echo "  [FAIL] Local compose file not found: ${LOCAL_COMPOSE_FILE}" >&2
        return 1
    fi

    echo "  [*] Starting Docker Envoy Gateway from ${LOCAL_COMPOSE_FILE}"
    docker compose -f "$LOCAL_COMPOSE_FILE" up -d memochat-envoy-gateway

    local waited=0
    while (( waited < WAIT_SECONDS )); do
        if curl -fsS http://127.0.0.1/health >/dev/null 2>&1; then
            echo "  [OK] Envoy Gateway ready at http://127.0.0.1/health"
            echo "  [INFO] Envoy listens on 80 and 8443/tcp+udp; upstream GateServer ports are 8080 and 8084"
            return 0
        fi
        sleep 1
        waited=$((waited + 1))
    done

    echo "  [WARN] Envoy Gateway did not answer /health within ${WAIT_SECONDS}s; check docker compose ps memochat-envoy-gateway"
    return 0
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

udp_port_listening() {
    local port="$1"
    if command -v ss >/dev/null 2>&1; then
        ss -lunH 2>/dev/null | awk -v suffix=":${port}" '
            index($4, suffix) == length($4) - length(suffix) + 1 { found = 1 }
            END { exit found ? 0 : 1 }
        '
        return $?
    fi
    if command -v lsof >/dev/null 2>&1; then
        lsof -nP -iUDP:"${port}" >/dev/null 2>&1
        return $?
    fi
    return 1
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
    local missing=0
    if is_truthy "$START_CORE_SERVICES"; then
        [[ -x "$(runtime_executable_path "GateServer1")" && -x "$(runtime_executable_path "AIServer")" ]] || missing=1
    fi
    if is_truthy "$START_CHAT_DELIVERY_WORKER"; then
        [[ -x "$(runtime_executable_path "ChatDeliveryWorker1")" ]] || missing=1
    fi
    if is_truthy "$START_RELATION_QUERY_SERVICE"; then
        [[ -x "$(runtime_executable_path "ChatRelationQueryService1")" ]] || missing=1
    fi
    if is_truthy "$START_RELATION_SERVICE_WORKER"; then
        [[ -x "$(runtime_executable_path "ChatRelationServiceWorker1")" ]] || missing=1
    fi
    if is_truthy "$START_MESSAGE_SERVICE"; then
        [[ -x "$(runtime_executable_path "ChatMessageService1")" ]] || missing=1
    fi
    if [[ "$missing" -eq 0 ]]; then
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

runtime_executable_path() {
    local wanted_name="$1"
    local row group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace
    for row in "${MEMOCHAT_RUNTIME_SERVICE_TOPOLOGY[@]}"; do
        IFS='|' read -r group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace <<<"$row"
        [[ "$name" == "$wanted_name" ]] || continue
        printf '%s/%s/%s\n' "$RUNTIME_DIR" "$name" "$exe"
        return 0
    done
    printf '%s/%s\n' "$RUNTIME_DIR" "$wanted_name"
}

config_value() {
    local config_file="$1"
    local section="$2"
    local key="$3"
    awk -F= -v section="$section" -v key="$key" '
        BEGIN { in_section = 0 }
        /^[[:space:]]*\[/ {
            current = $0
            sub(/^[[:space:]]*\[/, "", current)
            sub(/\][[:space:]]*$/, "", current)
            in_section = (current == section)
            next
        }
        in_section && $1 == key {
            value = $2
            sub(/^[[:space:]]*/, "", value)
            sub(/[[:space:]]*$/, "", value)
            print value
            exit
        }
    ' "$config_file" 2>/dev/null || true
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

wait_for_udp_port() {
    local service_name="$1"
    local port="$2"
    local waited=0
    while (( waited < WAIT_SECONDS )); do
        if udp_port_listening "$port"; then
            echo "  [OK] ${service_name} listening on UDP port ${port}"
            return 0
        fi
        sleep 1
        waited=$((waited + 1))
    done
    echo "  [WARN] ${service_name} did not bind UDP port ${port} yet; check logs"
    return 0
}

verify_started_pid() {
    local pid="$1"
    local svc_name="$2"
    local pid_file="$3"
    local out_log="$4"
    local err_log="$5"

    sleep 1
    if kill -0 "$pid" >/dev/null 2>&1; then
        return 0
    fi

    echo "  [FAIL] ${svc_name}: process exited early" >&2
    echo "         stdout: ${out_log}" >&2
    sed -n '1,80p' "$out_log" >&2 2>/dev/null || true
    echo "         stderr: ${err_log}" >&2
    sed -n '1,80p' "$err_log" >&2 2>/dev/null || true
    rm -f -- "$pid_file"
    return 1
}

gpt_sovits_ready() {
    curl -fsS "${GPT_SOVITS_API_URL%/}/docs" >/dev/null 2>&1
}

print_gpt_sovits_failure_hint() {
    echo "  [FAIL] GPT-SoVITS did not become ready at ${GPT_SOVITS_API_URL}" >&2
    echo "         API docs probe: ${GPT_SOVITS_API_URL%/}/docs" >&2
    echo "         Logs:" >&2
    echo "           ${GPT_SOVITS_LOG_FILE}" >&2
    echo "           ${LOG_DIR}/GPT-SoVITS_out.log" >&2
    echo "           ${LOG_DIR}/GPT-SoVITS_err.log" >&2
    echo "         Repair commands:" >&2
    echo "           ${PROJECT_ROOT}/tools/scripts/pet/smoke_gpt_sovits_tts_wsl.sh" >&2
    echo "           ${PROJECT_ROOT}/tools/scripts/pet/apply_gpt_sovits_voice_wsl.sh" >&2
    echo "         To intentionally allow text-only pet replies, set MEMOCHAT_REQUIRE_GPT_SOVITS=0 or pass --skip-gpt-sovits." >&2
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
    if is_truthy "$REQUIRE_GPT_SOVITS"; then
        print_gpt_sovits_failure_hint
        return 1
    fi
    echo "  [WARN] GPT-SoVITS did not become ready at ${GPT_SOVITS_API_URL}; continuing because MEMOCHAT_REQUIRE_GPT_SOVITS=0"
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
        if ! stop_gpt_sovits_for_rebind; then
            if is_truthy "$REQUIRE_GPT_SOVITS"; then
                return 1
            fi
            return 0
        fi
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
        echo "  [FAIL] GPT-SoVITS start script missing: ${GPT_SOVITS_START_SCRIPT}" >&2
        if is_truthy "$REQUIRE_GPT_SOVITS"; then
            return 1
        fi
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
    local instance_name="${5:-}"
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

    if [[ -z "$instance_name" ]]; then
        instance_name="$(config_value "${svc_dir}/config.ini" "SelfServer" "Name")"
    fi
    if [[ -z "$instance_name" ]]; then
        instance_name="$safe_name"
    fi

    : > "$out_log"
    : > "$err_log"

    echo "  [*] Starting ${svc_name}..."
    (
        cd "$svc_dir"
        exec nohup env \
            MEMOCHAT_ENABLE_KAFKA="$MEMOCHAT_ENABLE_KAFKA" \
            MEMOCHAT_ENABLE_RABBITMQ="$MEMOCHAT_ENABLE_RABBITMQ" \
            MEMOCHAT_ENABLE_QUIC="${MEMOCHAT_ENABLE_QUIC:-1}" \
            MEMOCHAT_INSTANCE_NAME="$instance_name" \
            "./${exe_name}"
    ) >>"$out_log" 2>>"$err_log" </dev/null &

    local pid=$!
    echo "$pid" > "$pid_file"
    echo "  [OK] ${svc_name} started pid=${pid}"

    if [[ -n "$port" ]]; then
        wait_for_port "$svc_name" "$port"
    fi
    verify_started_pid "$pid" "$svc_name" "$pid_file" "$out_log" "$err_log"
}

launch_topology_group() {
    local wanted_group="$1"
    local row group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace
    for row in "${MEMOCHAT_RUNTIME_SERVICE_TOPOLOGY[@]}"; do
        IFS='|' read -r group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace <<<"$row"
        [[ "$group" == "$wanted_group" ]] || continue
        launch_svc "${RUNTIME_DIR}/${name}" "$exe" "$display_name" "$tcp_wait_port" "$instance_name"
    done
}

ensure_quic_certs_for_group() {
    local wanted_group="$1"
    local row group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace
    for row in "${MEMOCHAT_RUNTIME_SERVICE_TOPOLOGY[@]}"; do
        IFS='|' read -r group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace <<<"$row"
        [[ "$group" == "$wanted_group" ]] || continue
        ensure_quic_cert "${RUNTIME_DIR}/${name}"
    done
}

wait_for_topology_group_udp_ports() {
    local wanted_group="$1"
    local suffix="$2"
    local row group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace
    local port
    for row in "${MEMOCHAT_RUNTIME_SERVICE_TOPOLOGY[@]}"; do
        IFS='|' read -r group name exe source_exe config display_name tcp_wait_port udp_wait_ports instance_name stop_tcp_ports stop_udp_ports log_dir telemetry_service_name telemetry_namespace <<<"$row"
        [[ "$group" == "$wanted_group" ]] || continue
        for port in $udp_wait_ports; do
            wait_for_udp_port "${display_name} ${suffix}" "$port"
        done
    done
}

start_topology_core_group() {
    local group="$1"
    local label
    label="$(memochat_topology_group_label "$group")"

    if [[ "$group" == "$MEMOCHAT_TOPOLOGY_GROUP_CHAT" ]]; then
        echo "[STEP] Prepare ChatServer QUIC certificates"
        ensure_quic_certs_for_group "$MEMOCHAT_TOPOLOGY_GROUP_CHAT"
        echo
    fi

    echo "[STEP] Start ${label}"
    launch_topology_group "$group"

    if [[ "$group" == "$MEMOCHAT_TOPOLOGY_GROUP_CHAT" ]] && is_truthy "$MEMOCHAT_ENABLE_QUIC"; then
        wait_for_topology_group_udp_ports "$MEMOCHAT_TOPOLOGY_GROUP_CHAT" "QUIC"
    fi

    if [[ "$group" == "$MEMOCHAT_TOPOLOGY_GROUP_GATE" ]]; then
        echo "  [INFO] Client HTTP traffic enters through Docker Envoy on 80 and 8443/tcp+udp"
    fi
    echo
}

start_topology_core_groups() {
    local group
    for group in "${MEMOCHAT_CORE_START_GROUPS[@]}"; do
        start_topology_core_group "$group"
    done
}

ensure_quic_cert() {
    local svc_dir="$1"
    local crt="${svc_dir}/server.crt"
    local key="${svc_dir}/server.key"

    if [[ -f "$crt" && -f "$key" ]]; then
        return 0
    fi
    if ! command -v openssl >/dev/null 2>&1; then
        echo "  [WARN] OpenSSL not found; ${svc_dir} QUIC certificate was not generated"
        return 0
    fi

    mkdir -p -- "$svc_dir"
    echo "  [*] Generating QUIC certificate for $(basename -- "$svc_dir")"
    openssl req -x509 -newkey rsa:2048 -nodes \
        -keyout "$key" \
        -out "$crt" \
        -days 3650 \
        -subj "/CN=127.0.0.1" \
        -addext "subjectAltName=DNS:localhost,IP:127.0.0.1" \
        >/dev/null 2>&1 || {
            echo "  [WARN] Failed to generate QUIC certificate for ${svc_dir}"
            return 0
        }
    chmod 600 "$key" || true
}

echo "============================================================"
echo "  MemoChat Linux startup"
echo "  PROJECT_ROOT: ${PROJECT_ROOT}"
echo "  RUNTIME_DIR:  ${RUNTIME_DIR}"
echo "  LOG_DIR:      ${LOG_DIR}"
echo "  PID_DIR:      ${PID_DIR}"
echo "  COMPOSE:      ${LOCAL_COMPOSE_FILE}"
echo "============================================================"
echo

echo "[STEP] Start Docker dependencies"
ensure_docker_dependencies
echo

echo "[STEP] Start Docker Envoy Gateway"
ensure_envoy_gateway
echo

echo "[STEP] Start GPT-SoVITS voice service"
start_gpt_sovits
echo

ensure_runtime

echo "[STEP] Start ChatDeliveryWorker"
if is_truthy "$START_CHAT_DELIVERY_WORKER"; then
    launch_topology_group "$MEMOCHAT_TOPOLOGY_GROUP_WORKER"
else
    echo "  [SKIP] ChatDeliveryWorker startup disabled"
fi
echo

echo "[STEP] Start ChatRelationQueryService"
if is_truthy "$START_RELATION_QUERY_SERVICE"; then
    launch_topology_group "$MEMOCHAT_TOPOLOGY_GROUP_RELATION_QUERY"
else
    echo "  [SKIP] ChatRelationQueryService startup disabled"
fi
echo

echo "[STEP] Start ChatRelationServiceWorker"
if is_truthy "$START_RELATION_SERVICE_WORKER"; then
    launch_topology_group "$MEMOCHAT_TOPOLOGY_GROUP_RELATION_SERVICE"
else
    echo "  [SKIP] ChatRelationServiceWorker startup disabled"
fi
echo

echo "[STEP] Start ChatMessageService"
if is_truthy "$START_MESSAGE_SERVICE"; then
    launch_topology_group "$MEMOCHAT_TOPOLOGY_GROUP_MESSAGE_SERVICE"
else
    echo "  [SKIP] ChatMessageService startup disabled"
fi
echo

if is_truthy "$START_CORE_SERVICES"; then
    start_topology_core_groups
else
    echo "[STEP] Start core services"
    echo "  [SKIP] Core service startup disabled"
    echo
fi

echo "Startup command completed. Logs are under:"
echo "  ${LOG_DIR}"
