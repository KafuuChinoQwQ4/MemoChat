#!/usr/bin/env bash
set -Eeuo pipefail

# Generic opt-in runtime smoke for a focused GateServer domain gateway peeled off
# in the gateserver microservice split (Phase 4): MediaGatewayServer,
# MomentsGatewayServer, CallGatewayServer, R18GatewayServer (and AIGatewayServer).
# It starts the chosen binary on an isolated temp port and proves:
#   1. it comes up and serves /healthz + /readyz,
#   2. it serves its own domain route (the route profile registered the module),
#   3. it does NOT serve a route from another domain (404),
# then stops it and verifies the port is released. Changes no deployed default.
#
# Usage:
#   smoke_domain_gateway_runtime.sh --binary NAME --port N \
#       --owned-route PATH --foreign-route PATH [--owned-method M] [--keep-runtime]
#
# Example (media):
#   smoke_domain_gateway_runtime.sh --binary MediaGatewayServer --port 18094 \
#       --owned-route /upload_media_status --foreign-route /api/r18/sources

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
ENV_FILE="${MEMOCHAT_ENV_FILE:-/root/.memochat-linux-env}"
BUILD_BIN="${MEMOCHAT_BUILD_BIN:-${PROJECT_ROOT}/build-linux-full-gcc16/bin}"

BINARY=""
PORT=""
OWNED_ROUTE=""
OWNED_METHOD="GET"
FOREIGN_ROUTE=""
CONFIG_SECTION=""
KEEP_RUNTIME=0
TMP_ROOT=""
SERVER_PID=""

usage() {
    cat <<USAGE
Usage: $0 --binary NAME --port N --owned-route PATH --foreign-route PATH \\
          [--owned-method GET|POST] [--config-section NAME] [--keep-runtime]
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --binary) BINARY="$2"; shift 2 ;;
        --port) PORT="$2"; shift 2 ;;
        --owned-route) OWNED_ROUTE="$2"; shift 2 ;;
        --owned-method) OWNED_METHOD="$2"; shift 2 ;;
        --foreign-route) FOREIGN_ROUTE="$2"; shift 2 ;;
        --config-section) CONFIG_SECTION="$2"; shift 2 ;;
        --keep-runtime) KEEP_RUNTIME=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "[FAIL] Unknown argument: $1" >&2; usage >&2; exit 2 ;;
    esac
done

[[ -n "$BINARY" && -n "$PORT" && -n "$OWNED_ROUTE" && -n "$FOREIGN_ROUTE" ]] \
    || { echo "[FAIL] --binary, --port, --owned-route, --foreign-route are required" >&2; usage >&2; exit 2; }

# Default config section is the binary name minus "Server" + "Gateway" style;
# callers can override. The standalone mains read [<Section>]Port.
if [[ -z "$CONFIG_SECTION" ]]; then
    case "$BINARY" in
        MediaGatewayServer) CONFIG_SECTION="MediaGateway" ;;
        MomentsGatewayServer) CONFIG_SECTION="MomentsGateway" ;;
        CallGatewayServer) CONFIG_SECTION="CallGateway" ;;
        R18GatewayServer) CONFIG_SECTION="R18Gateway" ;;
        AIGatewayServer) CONFIG_SECTION="AIGateway" ;;
        *) CONFIG_SECTION="Gateway" ;;
    esac
fi

if [[ -f "$ENV_FILE" ]]; then
    # shellcheck disable=SC1090
    source "$ENV_FILE"
fi

BIN_PATH="${BUILD_BIN}/${BINARY}"

fail() { echo "[FAIL] $1" >&2; exit 1; }

first_env()
{
    local name
    for name in "$@"; do
        if [[ -n "${!name:-}" ]]; then
            printf '%s' "${!name}"
            return 0
        fi
    done
    return 1
}

require_value()
{
    local label="$1"
    local value="$2"
    if [[ -z "${value}" ]]; then
        fail "Missing ${label}; export the matching MEMOCHAT_* environment variable before running this smoke"
    fi
}

cleanup() {
    if [[ -n "$SERVER_PID" ]] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null || true
        for _ in $(seq 1 20); do kill -0 "$SERVER_PID" 2>/dev/null || break; sleep 0.2; done
        kill -9 "$SERVER_PID" 2>/dev/null || true
    fi
    if [[ "$KEEP_RUNTIME" -eq 0 && -n "$TMP_ROOT" && -d "$TMP_ROOT" ]]; then
        rm -rf "$TMP_ROOT"
    fi
}
trap cleanup EXIT

[[ -x "$BIN_PATH" ]] || fail "Missing built ${BINARY}: ${BIN_PATH} (build linux-full-gcc16 first)"

TMP_ROOT="$(mktemp -d -t domain-gateway-smoke-XXXXXX)"
RUN_DIR="${TMP_ROOT}/${BINARY}"
mkdir -p "${RUN_DIR}/logs"

POSTGRES_HOST="${MEMOCHAT_POSTGRES_HOST:-127.0.0.1}"
POSTGRES_PORT="${MEMOCHAT_POSTGRES_PORT:-15432}"
POSTGRES_USER="${MEMOCHAT_POSTGRES_USER:-memochat}"
POSTGRES_PASSWORD="$(first_env MEMOCHAT_POSTGRES_PASSWD MEMOCHAT_POSTGRES_PASSWORD || true)"
POSTGRES_DB="${MEMOCHAT_POSTGRES_DB:-memo_pg}"
REDIS_HOST="${MEMOCHAT_REDIS_HOST:-127.0.0.1}"
REDIS_PORT="${MEMOCHAT_REDIS_PORT:-6379}"
REDIS_PASSWORD="$(first_env MEMOCHAT_REDIS_PASSWD MEMOCHAT_REDIS_PASSWORD || true)"
CHAT_AUTH_HMAC_SECRET="${MEMOCHAT_CHATAUTH_HMACSECRET:-}"
MONGO_URI="$(first_env MEMOCHAT_MONGODB_URI MEMOCHAT_MONGO_URI || true)"
MINIO_ENDPOINT="${MEMOCHAT_MINIO_ENDPOINT:-127.0.0.1:9000}"
MINIO_ACCESS_KEY="$(first_env MEMOCHAT_MINIO_ROOT_USER MEMOCHAT_MINIO_ACCESSKEY MEMOCHAT_MINIO_ACCESS_KEY MINIO_ROOT_USER MINIO_ACCESS_KEY || true)"
MINIO_SECRET_KEY="$(first_env MEMOCHAT_MINIO_ROOT_PASSWORD MEMOCHAT_MINIO_SECRETKEY MEMOCHAT_MINIO_SECRET_KEY MINIO_ROOT_PASSWORD MINIO_SECRET_KEY || true)"
CALL_API_KEY="$(first_env MEMOCHAT_CALL_APIKEY MEMOCHAT_LIVEKIT_API_KEY LIVEKIT_API_KEY || true)"
CALL_API_SECRET="$(first_env MEMOCHAT_CALL_APISECRET MEMOCHAT_LIVEKIT_API_SECRET LIVEKIT_API_SECRET || true)"

require_value "Postgres password" "${POSTGRES_PASSWORD}"
require_value "Redis password" "${REDIS_PASSWORD}"
require_value "ChatAuth HMAC secret" "${CHAT_AUTH_HMAC_SECRET}"
require_value "MongoDB URI" "${MONGO_URI}"
require_value "MinIO access key" "${MINIO_ACCESS_KEY}"
require_value "MinIO secret key" "${MINIO_SECRET_KEY}"
require_value "Call/LiveKit API key" "${CALL_API_KEY}"
require_value "Call/LiveKit API secret" "${CALL_API_SECRET}"

# Minimal config: the listen port + DB/redis/mongo/minio so service singletons
# can open lazily. We point at the live local Docker infra (stable dev ports).
cat >"${RUN_DIR}/config.ini" <<INI
[${CONFIG_SECTION}]
Port=${PORT}
WorkerThreads=4

[Snowflake]
DatacenterId=0
WorkerId=9

[Postgres]
Host=${POSTGRES_HOST}
Port=${POSTGRES_PORT}
User=${POSTGRES_USER}
Passwd=${POSTGRES_PASSWORD}
Database=${POSTGRES_DB}
Schema=memo
SslMode=disable
PoolSize=4

[Redis]
Host=${REDIS_HOST}
Port=${REDIS_PORT}
Passwd=${REDIS_PASSWORD}
PoolSize=8

[ChatAuth]
HmacSecret=${CHAT_AUTH_HMAC_SECRET}
TicketTtlSec=20

[Mongo]
Enabled=true
Uri=${MONGO_URI}
Database=memochat
MomentsCollection=moments_content

[Media]
StorageProvider=s3

[MinIO]
Enabled=true
Endpoint=${MINIO_ENDPOINT}
Region=us-east-1
AccessKey=${MINIO_ACCESS_KEY}
SecretKey=${MINIO_SECRET_KEY}
BucketAvatar=memochat-avatar
BucketFile=memochat-file
BucketImage=memochat-image
BucketVideo=memochat-video
BucketMoments=memochat-moments
PublicUrl=http://127.0.0.1:9000

[Call]
Enabled=true
BaseUrl=http://127.0.0.1:${PORT}
LiveKitUrl=ws://127.0.0.1:7880
ApiKey=${CALL_API_KEY}
ApiSecret=${CALL_API_SECRET}
RoomPrefix=memochat
TokenTtlSec=3600

[Log]
Level=warn
Dir=${RUN_DIR}/logs
Format=json
ToConsole=true
Env=local

[Telemetry]
Enabled=false
INI

cp -f "$BIN_PATH" "${RUN_DIR}/${BINARY}"

echo "[STEP] Start temp ${BINARY} on 127.0.0.1:${PORT}"
(
    cd "$RUN_DIR"
    exec "./${BINARY}"
) >"${RUN_DIR}/stdout.log" 2>&1 &
SERVER_PID=$!

up=0
for _ in $(seq 1 75); do
    if curl -s -m 2 -o /dev/null "http://127.0.0.1:${PORT}/healthz"; then up=1; break; fi
    kill -0 "$SERVER_PID" 2>/dev/null || fail "${BINARY} exited early; see ${RUN_DIR}/stdout.log"
    sleep 0.2
done
[[ "$up" -eq 1 ]] || fail "${BINARY} did not come up on ${PORT}"

echo "[STEP] Probe health"
curl -sf -m 3 "http://127.0.0.1:${PORT}/healthz" >/dev/null || fail "/healthz not OK"
curl -sf -m 3 "http://127.0.0.1:${PORT}/readyz" >/dev/null || fail "/readyz not OK"
echo "  [OK] /healthz + /readyz"

echo "[STEP] Assert owned route ${OWNED_METHOD} ${OWNED_ROUTE} IS registered (not 404)"
owned_code="$(curl -s -m 8 -o /dev/null -w "%{http_code}" -X "$OWNED_METHOD" "http://127.0.0.1:${PORT}${OWNED_ROUTE}" -d '{}')"
[[ "$owned_code" != "404" ]] || fail "owned route ${OWNED_ROUTE} returned 404 (profile did not register it)"
echo "  [OK] ${OWNED_METHOD} ${OWNED_ROUTE} -> ${owned_code} (registered)"

echo "[STEP] Assert foreign route ${FOREIGN_ROUTE} is NOT registered (404)"
foreign_code="$(curl -s -m 8 -o /dev/null -w "%{http_code}" -X POST "http://127.0.0.1:${PORT}${FOREIGN_ROUTE}" -d '{}')"
[[ "$foreign_code" == "404" ]] || fail "foreign route ${FOREIGN_ROUTE} returned ${foreign_code}, expected 404 (profile leak)"
echo "  [OK] POST ${FOREIGN_ROUTE} -> 404 (correctly excluded)"

echo "[STEP] Stop ${BINARY} and verify port released"
kill "$SERVER_PID" 2>/dev/null || true
for _ in $(seq 1 25); do kill -0 "$SERVER_PID" 2>/dev/null || break; sleep 0.2; done
kill -9 "$SERVER_PID" 2>/dev/null || true
SERVER_PID=""
sleep 0.5
if curl -s -m 2 -o /dev/null "http://127.0.0.1:${PORT}/healthz"; then
    fail "Port ${PORT} still serving after stop"
fi
echo "  [OK] port ${PORT} released"

echo "[SUCCESS] ${BINARY} runtime smoke passed"
