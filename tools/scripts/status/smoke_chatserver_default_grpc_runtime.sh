#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
ENV_FILE="${MEMOCHAT_ENV_FILE:-/root/.memochat-linux-env}"
BUILD_BIN="${MEMOCHAT_BUILD_BIN:-${PROJECT_ROOT}/build-linux-full-gcc16/bin}"
LOCAL_COMPOSE_FILE="${MEMOCHAT_LOCAL_COMPOSE_FILE:-${PROJECT_ROOT}/infra/deploy/local/docker-compose.yml}"
RELATION_QUERY_TEST_BIN="${MEMOCHAT_RELATION_QUERY_SMOKE_BIN:-${PROJECT_ROOT}/build-linux-full-gcc16/tests/chatserver/chatserver_relation_query_remote_smoke_gtest}"
RELATION_SERVICE_TEST_BIN="${MEMOCHAT_RELATION_SERVICE_SMOKE_BIN:-${PROJECT_ROOT}/build-linux-full-gcc16/tests/chatserver/chatserver_relation_grpc_client_gtest}"
MESSAGE_SERVICE_TEST_BIN="${MEMOCHAT_MESSAGE_SERVICE_SMOKE_BIN:-${PROJECT_ROOT}/build-linux-full-gcc16/tests/chatserver/chatserver_message_remote_smoke_gtest}"
CHATSERVER_TCP_PORT="${MEMOCHAT_DEFAULT_GRPC_SMOKE_CHAT_TCP_PORT:-18094}"
CHATSERVER_QUIC_PORT="${MEMOCHAT_DEFAULT_GRPC_SMOKE_CHAT_QUIC_PORT:-18194}"
CHATSERVER_RPC_PORT="${MEMOCHAT_DEFAULT_GRPC_SMOKE_CHAT_RPC_PORT:-15061}"
WAIT_SECONDS="${MEMOCHAT_DEFAULT_GRPC_SMOKE_WAIT_SECONDS:-20}"
KEEP_RUNTIME=0
TMP_ROOT=""
RUNTIME_DIR=""
LOG_DIR=""
PID_DIR=""

usage() {
    cat <<USAGE
Usage: $0 [--keep-runtime]

Run a runtime smoke for the default ChatServer microservice shape:
  ChatServer default config
    -> RelationQueryService Backend=grpc 127.0.0.1:50090
    -> RelationService Backend=grpc 127.0.0.1:50091
    -> MessageService Backend=grpc 127.0.0.1:50092

The worker service configs remain Backend=inprocess because they host the
service implementation behind their gRPC listeners.
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --keep-runtime)
            KEEP_RUNTIME=1
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

if [[ -f "$ENV_FILE" ]]; then
    # shellcheck disable=SC1090
    source "$ENV_FILE"
fi

require_executable() {
    local path="$1"
    if [[ ! -x "$path" ]]; then
        echo "[FAIL] Required executable missing: ${path}" >&2
        exit 1
    fi
}

set_ini_value() {
    local file="$1"
    local section="$2"
    local key="$3"
    local value="$4"
    local tmp="${file}.tmp"
    awk -v section="$section" -v key="$key" -v value="$value" '
        BEGIN { in_section = 0; seen_section = 0; wrote_key = 0 }
        /^[[:space:]]*\[/ {
            if (in_section && !wrote_key) {
                print key "=" value
                wrote_key = 1
            }
            current = $0
            sub(/^[[:space:]]*\[/, "", current)
            sub(/\][[:space:]]*$/, "", current)
            in_section = (current == section)
            if (in_section) {
                seen_section = 1
                wrote_key = 0
            }
            print
            next
        }
        in_section && $0 ~ "^[[:space:]]*" key "[[:space:]]*=" {
            print key "=" value
            wrote_key = 1
            next
        }
        { print }
        END {
            if (in_section && !wrote_key) {
                print key "=" value
            }
            if (!seen_section) {
                print ""
                print "[" section "]"
                print key "=" value
            }
        }
    ' "$file" > "$tmp"
    mv -f -- "$tmp" "$file"
}

tcp_port_listening() {
    local port="$1"
    if command -v ss >/dev/null 2>&1; then
        ss -ltnH 2>/dev/null | awk -v suffix=":${port}" '
            index($4, suffix) == length($4) - length(suffix) + 1 { found = 1 }
            END { exit found ? 0 : 1 }
        '
        return $?
    fi
    if command -v lsof >/dev/null 2>&1; then
        lsof -tiTCP:"${port}" -sTCP:LISTEN >/dev/null 2>&1
        return $?
    fi
    return 1
}

wait_for_tcp_port() {
    local label="$1"
    local port="$2"
    local waited=0
    while (( waited < WAIT_SECONDS )); do
        if tcp_port_listening "$port"; then
            echo "  [OK] ${label} listening on ${port}"
            return 0
        fi
        sleep 1
        waited=$((waited + 1))
    done
    echo "[FAIL] ${label} did not listen on ${port} within ${WAIT_SECONDS}s" >&2
    return 1
}

wait_for_storage_deps() {
    local waited=0
    while (( waited < WAIT_SECONDS )); do
        if docker exec memochat-redis redis-cli -a 123456 ping >/dev/null 2>&1 \
            && docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;" >/dev/null 2>&1 \
            && docker exec memochat-mongo mongosh -u root -p 123456 --authenticationDatabase admin --quiet --eval "db.adminCommand({ ping: 1 })" >/dev/null 2>&1; then
            echo "  [OK] Redis/Postgres/Mongo ready for default gRPC smoke"
            return 0
        fi
        sleep 1
        waited=$((waited + 1))
    done
    echo "[FAIL] Redis/Postgres/Mongo did not become ready for default gRPC smoke" >&2
    return 1
}

cleanup() {
    local status=$?
    if [[ -n "$RUNTIME_DIR" && -d "$RUNTIME_DIR" ]]; then
        MEMOCHAT_RUNTIME_DIR="$RUNTIME_DIR" \
            MEMOCHAT_LOG_DIR="$LOG_DIR" \
            MEMOCHAT_PID_DIR="$PID_DIR" \
            MEMOCHAT_LOCAL_COMPOSE_FILE="$LOCAL_COMPOSE_FILE" \
            "${SCRIPT_DIR}/stop-all-services.sh" --skip-envoy --skip-gpt-sovits >/dev/null 2>&1 || true
    fi
    if [[ "$KEEP_RUNTIME" -eq 0 && -n "$TMP_ROOT" && -d "$TMP_ROOT" ]]; then
        rm -rf -- "$TMP_ROOT"
    elif [[ -n "$TMP_ROOT" ]]; then
        echo "  [INFO] Kept smoke runtime: ${TMP_ROOT}"
    fi
    exit "$status"
}
trap cleanup EXIT

for exe in ChatRelationQueryService ChatRelationServiceWorker ChatMessageService ChatServer; do
    require_executable "${BUILD_BIN}/${exe}"
done
for test_bin in "$RELATION_QUERY_TEST_BIN" "$RELATION_SERVICE_TEST_BIN" "$MESSAGE_SERVICE_TEST_BIN"; do
    require_executable "$test_bin"
done

for port in 50090 50091 50092 "$CHATSERVER_TCP_PORT" "$CHATSERVER_RPC_PORT"; do
    if tcp_port_listening "$port"; then
        echo "[FAIL] Default gRPC smoke port is already in use: ${port}" >&2
        exit 1
    fi
done

TMP_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/memochat-default-grpc-smoke.XXXXXX")"
RUNTIME_DIR="${TMP_ROOT}/services"
LOG_DIR="${TMP_ROOT}/logs"
PID_DIR="${TMP_ROOT}/pids"
mkdir -p -- \
    "${RUNTIME_DIR}/ChatRelationQueryService1" \
    "${RUNTIME_DIR}/ChatRelationServiceWorker1" \
    "${RUNTIME_DIR}/ChatMessageService1" \
    "${RUNTIME_DIR}/chatserver1" \
    "$LOG_DIR" \
    "$PID_DIR"

cp -f -- "${BUILD_BIN}/ChatRelationQueryService" \
    "${RUNTIME_DIR}/ChatRelationQueryService1/ChatRelationQueryService"
cp -f -- "${PROJECT_ROOT}/apps/server/core/ChatServer/chatrelationquery1.ini" \
    "${RUNTIME_DIR}/ChatRelationQueryService1/config.ini"
cp -f -- "${BUILD_BIN}/ChatRelationServiceWorker" \
    "${RUNTIME_DIR}/ChatRelationServiceWorker1/ChatRelationServiceWorker"
cp -f -- "${PROJECT_ROOT}/apps/server/core/ChatServer/chatrelationservice1.ini" \
    "${RUNTIME_DIR}/ChatRelationServiceWorker1/config.ini"
cp -f -- "${BUILD_BIN}/ChatMessageService" \
    "${RUNTIME_DIR}/ChatMessageService1/ChatMessageService"
cp -f -- "${PROJECT_ROOT}/apps/server/core/ChatServer/chatmessageservice1.ini" \
    "${RUNTIME_DIR}/ChatMessageService1/config.ini"
cp -f -- "${BUILD_BIN}/ChatServer" "${RUNTIME_DIR}/chatserver1/ChatServer"
cp -f -- "${PROJECT_ROOT}/apps/server/core/ChatServer/chatserver1.ini" \
    "${RUNTIME_DIR}/chatserver1/config.ini"
chmod +x -- \
    "${RUNTIME_DIR}/ChatRelationQueryService1/ChatRelationQueryService" \
    "${RUNTIME_DIR}/ChatRelationServiceWorker1/ChatRelationServiceWorker" \
    "${RUNTIME_DIR}/ChatMessageService1/ChatMessageService" \
    "${RUNTIME_DIR}/chatserver1/ChatServer"

chat_config="${RUNTIME_DIR}/chatserver1/config.ini"
set_ini_value "$chat_config" "Runtime" "Role" "ingress"
set_ini_value "$chat_config" "Runtime" "AsyncEventBus" "redis"
set_ini_value "$chat_config" "Runtime" "TaskBus" "inline"
set_ini_value "$chat_config" "chatserver1" "TcpPort" "$CHATSERVER_TCP_PORT"
set_ini_value "$chat_config" "chatserver1" "QuicPort" "$CHATSERVER_QUIC_PORT"
set_ini_value "$chat_config" "chatserver1" "RpcPort" "$CHATSERVER_RPC_PORT"
set_ini_value "$chat_config" "Log" "Dir" "${LOG_DIR}/chatserver1-runtime"
set_ini_value "${RUNTIME_DIR}/ChatRelationQueryService1/config.ini" "Log" "Dir" "${LOG_DIR}/chatrelationquery1-runtime"
set_ini_value "${RUNTIME_DIR}/ChatRelationServiceWorker1/config.ini" "Log" "Dir" "${LOG_DIR}/chatrelationservice1-runtime"
set_ini_value "${RUNTIME_DIR}/ChatMessageService1/config.ini" "Log" "Dir" "${LOG_DIR}/chatmessageservice1-runtime"

grep -q '^Backend=grpc$' "$chat_config"
grep -q '^Endpoint=127.0.0.1:50090$' "$chat_config"
grep -q '^Endpoint=127.0.0.1:50091$' "$chat_config"
grep -q '^Endpoint=127.0.0.1:50092$' "$chat_config"

echo "============================================================"
echo "  ChatServer default gRPC runtime smoke"
echo "  TMP_ROOT:    ${TMP_ROOT}"
echo "  RUNTIME_DIR: ${RUNTIME_DIR}"
echo "  CONFIG:      ${chat_config}"
echo "============================================================"

MEMOCHAT_RUNTIME_DIR="$RUNTIME_DIR" \
    MEMOCHAT_LOG_DIR="$LOG_DIR" \
    MEMOCHAT_PID_DIR="$PID_DIR" \
    MEMOCHAT_LOCAL_COMPOSE_FILE="$LOCAL_COMPOSE_FILE" \
    MEMOCHAT_REQUIRE_GPT_SOVITS=0 \
    "${SCRIPT_DIR}/start-all-services.sh" \
    --no-deploy \
    --skip-core-services \
    --skip-envoy \
    --skip-gpt-sovits \
    --wait-seconds "$WAIT_SECONDS"

wait_for_storage_deps
wait_for_tcp_port "ChatRelationQueryService" "50090"
wait_for_tcp_port "ChatRelationServiceWorker" "50091"
wait_for_tcp_port "ChatMessageService" "50092"

chat_out="${LOG_DIR}/ChatServer-default-grpc_out.log"
chat_err="${LOG_DIR}/ChatServer-default-grpc_err.log"
chat_pid_file="${PID_DIR}/ChatServer-1.pid"
: > "$chat_out"
: > "$chat_err"
(
    cd "${RUNTIME_DIR}/chatserver1"
    MEMOCHAT_INSTANCE_NAME=chatserver1 \
        MEMOCHAT_ENABLE_QUIC=0 \
        MEMOCHAT_ENABLE_KAFKA=0 \
        MEMOCHAT_ENABLE_RABBITMQ=0 \
        ./ChatServer --config config.ini >"$chat_out" 2>"$chat_err" &
    echo $! > "$chat_pid_file"
)

wait_for_tcp_port "ChatServer TCP" "$CHATSERVER_TCP_PORT"
wait_for_tcp_port "ChatServer gRPC" "$CHATSERVER_RPC_PORT"

echo "[STEP] Run default gRPC relation query probe"
MEMOCHAT_RELATION_QUERY_SMOKE_CONFIG="$chat_config" \
    MEMOCHAT_RELATION_QUERY_SMOKE_ENDPOINT="127.0.0.1:50090" \
    "$RELATION_QUERY_TEST_BIN" --gtest_filter='RelationQueryRemoteSmokeTest.*'

echo "[STEP] Run default gRPC relation command probe"
MEMOCHAT_RELATION_SERVICE_SMOKE_ENDPOINT="127.0.0.1:50091" \
    "$RELATION_SERVICE_TEST_BIN" --gtest_filter='RelationGrpcClientTest.RuntimeSmokeSearchUserUsesRealRelationServiceWorker'

echo "[STEP] Run default gRPC message probe"
MEMOCHAT_MESSAGE_SERVICE_SMOKE_CONFIG="$chat_config" \
    MEMOCHAT_MESSAGE_SERVICE_SMOKE_ENDPOINT="127.0.0.1:50092" \
    "$MESSAGE_SERVICE_TEST_BIN" --gtest_filter='MessageRemoteSmokeTest.*'

echo "[STEP] Stop temp default gRPC runtime services"
MEMOCHAT_RUNTIME_DIR="$RUNTIME_DIR" \
    MEMOCHAT_LOG_DIR="$LOG_DIR" \
    MEMOCHAT_PID_DIR="$PID_DIR" \
    MEMOCHAT_LOCAL_COMPOSE_FILE="$LOCAL_COMPOSE_FILE" \
    "${SCRIPT_DIR}/stop-all-services.sh" --skip-envoy --skip-gpt-sovits

echo "[SUCCESS] ChatServer default gRPC runtime smoke passed"
