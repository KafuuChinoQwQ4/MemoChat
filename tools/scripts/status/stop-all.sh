#!/usr/bin/env bash
# stop-all.sh — stop ALL MemoChat services: backend + frontend clients
#
# Equivalent to running stop-all-services.sh + stop-all-clients.sh in sequence.
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="${MEMOCHAT_ENV_FILE:-/root/.memochat-linux-env}"

STOP_SERVICES_SCRIPT="${SCRIPT_DIR}/stop-all-services.sh"
STOP_CLIENTS_SCRIPT="${SCRIPT_DIR}/stop-all-clients.sh"

STOP_SERVICES_ARGS=()
STOP_CLIENTS=1

usage() {
    cat <<USAGE
Usage: $0 [--skip-envoy] [--skip-clients] [stop-all-services options…]

Stop ALL MemoChat processes: backend services AND client frontends.

  Phase 1: stop-all-clients.sh  — kills MemoChatQml + Vite dev server
  Phase 2: stop-all-services.sh — kills C++ services + Docker Envoy

Options:
  --skip-clients   Skip Phase 1 (don't kill client processes).
  --skip-envoy     Passed through to stop-all-services.sh.
  Any other flag from stop-all-services.sh is forwarded transparently.
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --skip-clients) STOP_CLIENTS=0; shift ;;
        -h|--help)      usage; exit 0 ;;
        *)              STOP_SERVICES_ARGS+=("$1"); shift ;;
    esac
done

if [[ -f "$ENV_FILE" ]]; then
    # shellcheck disable=SC1090
    source "$ENV_FILE"
fi

echo "=============================="
echo "  MemoChat — stop everything"
echo "=============================="
echo

# Phase 1: stop clients (Qt + web)
if [[ "$STOP_CLIENTS" -eq 1 ]]; then
    if [[ -x "$STOP_CLIENTS_SCRIPT" ]]; then
        bash "$STOP_CLIENTS_SCRIPT"
    else
        echo "[WARN] stop-all-clients.sh not found: ${STOP_CLIENTS_SCRIPT}" >&2
    fi
    echo
fi

# Phase 2: stop backend services
if [[ -x "$STOP_SERVICES_SCRIPT" ]]; then
    bash "$STOP_SERVICES_SCRIPT" "${STOP_SERVICES_ARGS[@]}"
else
    echo "[FAIL] stop-all-services.sh not found: ${STOP_SERVICES_SCRIPT}" >&2
    exit 1
fi

echo
echo "All MemoChat processes stopped."
