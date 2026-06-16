#!/usr/bin/env bash
set -Eeuo pipefail

# Opt-in runtime smoke for the AIGatewayServer (gateserver microservice split,
# Phase 3). Starts a temp AIGatewayServer on an isolated port and proves:
#   1. it comes up and serves /healthz + /readyz,
#   2. it serves /ai/* (the AIGateway route profile registered the AI module),
#   3. it does NOT serve the excluded domains (/user_login, /upload_media,
#      /api/moments/list, /api/r18/sources, /api/call/token -> 404), proving the
#      LogicSystem::RouteProfile::AIGateway seam isolates the service.
# It then stops the process and verifies the port is released.
#
# This does NOT require AIServer/AIOrchestrator to be up: /ai/* returning a
# proxy/unavailable response still proves the route is registered (we only
# assert the route is NOT a 404 "route not found"). It changes no deployed
# default startup; this smoke still uses an isolated temporary runtime.

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
ENV_FILE="${MEMOCHAT_ENV_FILE:-/root/.memochat-linux-env}"
BUILD_BIN="${MEMOCHAT_BUILD_BIN:-${PROJECT_ROOT}/build-linux-full-gcc16/bin}"
AIGATEWAY_BIN="${BUILD_BIN}/AIGatewayServer"
AIGATEWAY_PORT="${MEMOCHAT_AIGATEWAY_SMOKE_PORT:-18093}"
WAIT_SECONDS="${MEMOCHAT_AIGATEWAY_SMOKE_WAIT_SECONDS:-15}"
KEEP_RUNTIME=0
TMP_ROOT=""
SERVER_PID=""

usage() {
    cat <<USAGE
Usage: $0 [--keep-runtime]

Run an opt-in HTTP runtime smoke for the standalone AIGatewayServer.

Required built artifact:
  ${AIGATEWAY_BIN}
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

fail() {
    echo "[FAIL] $1" >&2
    exit 1
}

cleanup() {
    if [[ -n "$SERVER_PID" ]] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null || true
        for _ in $(seq 1 20); do
            kill -0 "$SERVER_PID" 2>/dev/null || break
            sleep 0.2
        done
        kill -9 "$SERVER_PID" 2>/dev/null || true
    fi
    if [[ "$KEEP_RUNTIME" -eq 0 && -n "$TMP_ROOT" && -d "$TMP_ROOT" ]]; then
        rm -rf "$TMP_ROOT"
    fi
}
trap cleanup EXIT

[[ -x "$AIGATEWAY_BIN" ]] || fail "Missing built AIGatewayServer: $AIGATEWAY_BIN (build linux-full-gcc16 first)"

TMP_ROOT="$(mktemp -d -t aigateway-smoke-XXXXXX)"
RUN_DIR="${TMP_ROOT}/AIGatewayService1"
mkdir -p "$RUN_DIR"

# Temp config on an isolated port; logs to console off so we can read failures.
cat >"${RUN_DIR}/config.ini" <<INI
[AIGateway]
Port=${AIGATEWAY_PORT}
WorkerThreads=4

[AIOrchestrator]
Host=127.0.0.1
Port=8096
TimeoutSec=30

[Log]
Level=warn
Dir=${RUN_DIR}/logs
Format=json
ToConsole=true
Env=local

[Telemetry]
Enabled=false
INI
mkdir -p "${RUN_DIR}/logs"

cp -f "$AIGATEWAY_BIN" "${RUN_DIR}/AIGatewayServer"

echo "[STEP] Start temp AIGatewayServer on 127.0.0.1:${AIGATEWAY_PORT}"
(
    cd "$RUN_DIR"
    exec ./AIGatewayServer
) >"${RUN_DIR}/stdout.log" 2>&1 &
SERVER_PID=$!

# Wait for the port to accept connections.
up=0
for _ in $(seq 1 "$((WAIT_SECONDS * 5))"); do
    if curl -s -m 2 -o /dev/null "http://127.0.0.1:${AIGATEWAY_PORT}/healthz"; then
        up=1
        break
    fi
    kill -0 "$SERVER_PID" 2>/dev/null || fail "AIGatewayServer exited early; see ${RUN_DIR}/stdout.log"
    sleep 0.2
done
[[ "$up" -eq 1 ]] || fail "AIGatewayServer did not come up on ${AIGATEWAY_PORT} within ${WAIT_SECONDS}s"

echo "[STEP] Probe health endpoints"
curl -sf -m 3 "http://127.0.0.1:${AIGATEWAY_PORT}/healthz" >/dev/null || fail "/healthz not OK"
curl -sf -m 3 "http://127.0.0.1:${AIGATEWAY_PORT}/readyz" >/dev/null || fail "/readyz not OK"
echo "  [OK] /healthz + /readyz"

echo "[STEP] Assert /ai/* routes ARE registered (not 404)"
for ai_route in "/ai/model/list" "/ai/history"; do
    code="$(curl -s -m 5 -o /dev/null -w "%{http_code}" "http://127.0.0.1:${AIGATEWAY_PORT}${ai_route}")"
    [[ "$code" != "404" ]] || fail "AI route ${ai_route} returned 404 (not registered in AIGateway profile)"
    echo "  [OK] GET ${ai_route} -> ${code} (registered)"
done

echo "[STEP] Assert excluded domains are NOT registered (404)"
for excluded in "/user_login" "/upload_media" "/api/moments/list" "/api/r18/sources" "/api/call/token"; do
    code="$(curl -s -m 5 -o /dev/null -w "%{http_code}" -X POST "http://127.0.0.1:${AIGATEWAY_PORT}${excluded}" -d '{}')"
    [[ "$code" == "404" ]] || fail "Excluded route ${excluded} returned ${code}, expected 404 (route profile leak)"
    echo "  [OK] POST ${excluded} -> 404 (correctly excluded)"
done

echo "[STEP] Stop AIGatewayServer and verify port released"
kill "$SERVER_PID" 2>/dev/null || true
for _ in $(seq 1 25); do
    kill -0 "$SERVER_PID" 2>/dev/null || break
    sleep 0.2
done
kill -9 "$SERVER_PID" 2>/dev/null || true
SERVER_PID=""
sleep 0.5
if curl -s -m 2 -o /dev/null "http://127.0.0.1:${AIGATEWAY_PORT}/healthz"; then
    fail "Port ${AIGATEWAY_PORT} still serving after stop"
fi
echo "  [OK] port ${AIGATEWAY_PORT} released"

echo "[SUCCESS] AIGatewayServer runtime smoke passed"
