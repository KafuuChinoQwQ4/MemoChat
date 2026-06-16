#!/usr/bin/env bash
set -Eeuo pipefail

# Runtime smoke for the gateserver microservice split Envoy edge. Starts the
# real local Envoy (compose/envoy.yaml) and proves each domain prefix has an
# explicit per-service route, while unknown paths return Envoy 404 instead of
# falling back to the retired GateServer monolith.
#
# This does not require the gateways or GateServer to be running; it validates
# the EDGE routing table, which is the risky part of Phase 6. Safe + reversible:
# it only starts/stops the Envoy container.

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
COMPOSE_FILE="${MEMOCHAT_LOCAL_COMPOSE_FILE:-${PROJECT_ROOT}/infra/deploy/local/docker-compose.yml}"
ENVOY_SERVICE="memochat-envoy-gateway"
KEEP_ENVOY=0

usage() { echo "Usage: $0 [--keep-envoy]"; }
while [[ $# -gt 0 ]]; do
    case "$1" in
        --keep-envoy) KEEP_ENVOY=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "[FAIL] Unknown argument: $1" >&2; usage >&2; exit 2 ;;
    esac
done

fail() { echo "[FAIL] $1" >&2; exit 1; }

cleanup() {
    if [[ "$KEEP_ENVOY" -eq 0 ]]; then
        docker compose -f "$COMPOSE_FILE" stop "$ENVOY_SERVICE" >/dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

# Each cut domain prefix -> a probe path + method. Backends may be up or down in
# a developer environment; the invariant here is that the edge route exists
# (not 404). Unknown paths must return the direct-response 404 catch-all.
declare -a PROBES=(
    "GET:/ai/model/list"
    "GET:/upload_media_status"
    "POST:/api/moments/list"
    "GET:/api/call/token"
    "GET:/api/r18/sources"
    "POST:/user_login"
    "POST:/get_varifycode"
    "POST:/user_register"
    "POST:/reset_pwd"
    "POST:/user_update_profile"
)

echo "[STEP] Start local Envoy edge from ${COMPOSE_FILE}"
docker compose -f "$COMPOSE_FILE" up -d "$ENVOY_SERVICE" >/dev/null 2>&1 \
    || fail "could not start ${ENVOY_SERVICE}"

up=0
for _ in $(seq 1 30); do
    if curl -fsS -m 2 http://127.0.0.1/health >/dev/null 2>&1; then up=1; break; fi
    sleep 1
done
[[ "$up" -eq 1 ]] || fail "Envoy did not answer /health"
echo "  [OK] Envoy healthy on :80"

echo "[STEP] Probe each cut domain prefix (expect any routed status except 404)"
for probe in "${PROBES[@]}"; do
    method="${probe%%:*}"
    path="${probe#*:}"
    code="$(curl -s -m 6 -o /dev/null -w "%{http_code}" -X "$method" "http://127.0.0.1${path}" -d '{}')"
    # 503 = routed to a down per-service backend (cut-over working). 200/4xx are
    # fine too IF a backend happens to be up. The failure we guard against is a
    # route that does not resolve to any cluster (404 from Envoy = no route).
    if [[ "$code" == "404" ]]; then
        fail "${method} ${path} -> 404 (no Envoy route — cut-over route missing)"
    fi
    echo "  [OK] ${method} ${path} -> ${code} (has a route)"
done

echo "[STEP] Verify unknown paths return Envoy catch-all 404"
unknown_code="$(curl -s -m 5 -o /dev/null -w "%{http_code}" http://127.0.0.1/__memochat_unknown_route__)"
[[ "$unknown_code" == "404" ]] || fail "expected 404 for unknown route, got ${unknown_code}"
echo "  [OK] unknown route -> 404"

echo "[STEP] Verify GateServer is retired (legacy debug routes no longer proxied)"
# After Gate dissolution the catch-all returns a direct 404 instead of proxying
# unmapped paths (incl. the consumerless /get_test, /test_procedure debug stubs)
# to the GateServer monolith.
for retired in "/get_test" "/test_procedure"; do
    code="$(curl -s -m 5 -o /dev/null -w "%{http_code}" "http://127.0.0.1${retired}")"
    [[ "$code" == "404" ]] || fail "catch-all ${retired} -> ${code}, expected 404 (Gate not retired?)"
    echo "  [OK] ${retired} -> 404 (not proxied to monolith)"
done

echo "[STEP] Verify misdirected-host protection still active"
host_code="$(curl -s -m 5 -o /dev/null -w "%{http_code}" http://127.0.0.1/ai/model/list -H 'Host: evil.example')"
[[ "$host_code" == "421" ]] || fail "expected 421 for unknown Host, got ${host_code}"
echo "  [OK] unknown Host -> 421"

echo "[SUCCESS] Envoy microservice routing smoke passed"
