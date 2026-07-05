#!/usr/bin/env bash
# start-memochat-web.sh — start the MemoChat React/TypeScript web client (apps/web/)
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
WEB_DIR="${PROJECT_ROOT}/apps/web"
ENV_FILE="${MEMOCHAT_ENV_FILE:-/root/.memochat-linux-env}"

PORT="${MEMOCHAT_WEB_PORT:-5173}"
MODE="dev"          # dev | build | preview
OPEN_BROWSER=0
DIAGNOSE=0
MOCK_TRANSPORT=""   # "" = use .env.local value; "1" = force mock; "0" = force real

usage() {
    cat <<USAGE
Usage: $0 [--port N] [--build] [--preview] [--open] [--mock] [--real] [--diagnose]

Start the MemoChat web client from:
  ${WEB_DIR}

Options:
  --port N      Vite dev server port (default: 5173).
  --build       Run production build (dist/) instead of dev server.
  --preview     Serve the production build with vite preview.
  --open        Open the browser automatically after starting.
  --mock        Force VITE_USE_MOCK_TRANSPORT=1 (no WS bridge required).
  --real        Force VITE_USE_MOCK_TRANSPORT=0 (requires WS↔TCP bridge).
  --diagnose    Print environment diagnostics and exit.
  -h, --help    Show this help.

Environment variables:
  MEMOCHAT_ENV_FILE   Path to the Linux env file (default: /root/.memochat-linux-env).
  MEMOCHAT_WEB_PORT   Vite dev server port override.
USAGE
}

# ── Argument parsing ─────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --port)
            PORT="$2"
            shift 2
            ;;
        --build)
            MODE="build"
            shift
            ;;
        --preview)
            MODE="preview"
            shift
            ;;
        --open)
            OPEN_BROWSER=1
            shift
            ;;
        --mock)
            MOCK_TRANSPORT="1"
            shift
            ;;
        --real)
            MOCK_TRANSPORT="0"
            shift
            ;;
        --diagnose)
            DIAGNOSE=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "[FAIL] Unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

# ── Load Linux env file ───────────────────────────────────────────────────────
if [[ -f "$ENV_FILE" ]]; then
    # shellcheck disable=SC1090
    source "$ENV_FILE"
fi

# ── Prerequisite checks ───────────────────────────────────────────────────────
check_prereqs() {
    local ok=1

    if ! command -v node >/dev/null 2>&1; then
        echo "[FAIL] node not found in PATH." >&2
        echo "       Install Node.js >= 20: https://nodejs.org/" >&2
        ok=0
    fi

    if ! command -v npm >/dev/null 2>&1; then
        echo "[FAIL] npm not found in PATH." >&2
        ok=0
    fi

    if [[ ! -d "${WEB_DIR}/node_modules" ]]; then
        echo "[WARN] node_modules not found. Running npm install..." >&2
        (cd "$WEB_DIR" && npm install --legacy-peer-deps) || {
            echo "[FAIL] npm install failed." >&2
            ok=0
        }
    fi

    [[ "$ok" -eq 1 ]]
}

# ── .env.local setup ─────────────────────────────────────────────────────────
ensure_env_local() {
    local env_local="${WEB_DIR}/.env.local"
    local env_example="${WEB_DIR}/.env.example"

    if [[ ! -f "$env_local" ]]; then
        if [[ -f "$env_example" ]]; then
            cp "$env_example" "$env_local"
            echo "[INFO] Created ${env_local} from .env.example." >&2
            echo "       Edit it to point VITE_GATE_BASE_URL / VITE_WS_BRIDGE_URL at your backend." >&2
        else
            echo "[WARN] No .env.example found; skipping .env.local creation." >&2
        fi
    fi

    # Apply --mock / --real overrides at runtime via VITE_ env vars
    if [[ -n "$MOCK_TRANSPORT" ]]; then
        export VITE_USE_MOCK_TRANSPORT="$MOCK_TRANSPORT"
    fi
}

# ── Diagnostics ───────────────────────────────────────────────────────────────
print_diagnostics() {
    local node_ver npm_ver npx_vite_ver
    node_ver="$(node --version 2>/dev/null || echo '<not found>')"
    npm_ver="$(npm --version 2>/dev/null || echo '<not found>')"
    npx_vite_ver="$(cd "$WEB_DIR" && npx vite --version 2>/dev/null || echo '<not found>')"

    echo "============================================================"
    echo "  MemoChat Web diagnostics"
    echo "  PROJECT_ROOT:          ${PROJECT_ROOT}"
    echo "  WEB_DIR:               ${WEB_DIR}"
    echo "  ENV_FILE:              ${ENV_FILE}"
    echo "  MODE:                  ${MODE}"
    echo "  PORT:                  ${PORT}"
    echo "  MOCK_TRANSPORT flag:   ${MOCK_TRANSPORT:-<from .env.local>}"
    echo "  VITE_USE_MOCK:         ${VITE_USE_MOCK_TRANSPORT:-<from .env.local>}"
    echo "  VITE_GATE_BASE_URL:    ${VITE_GATE_BASE_URL:-<from .env.local>}"
    echo "  VITE_WS_BRIDGE_URL:    ${VITE_WS_BRIDGE_URL:-<from .env.local>}"
    echo "  node:                  ${node_ver}"
    echo "  npm:                   ${npm_ver}"
    echo "  vite:                  ${npx_vite_ver}"
    echo "  node_modules present:  $([[ -d "${WEB_DIR}/node_modules" ]] && echo yes || echo NO)"
    echo "  .env.local present:    $([[ -f "${WEB_DIR}/.env.local" ]] && echo yes || echo NO)"
    echo "============================================================"
}

# ── Open browser helper ───────────────────────────────────────────────────────
maybe_open_browser() {
    local url="$1"
    # Small delay to let Vite bind the port
    sleep 2
    if command -v xdg-open >/dev/null 2>&1; then
        xdg-open "$url" >/dev/null 2>&1 &
    elif command -v wslview >/dev/null 2>&1; then
        wslview "$url" >/dev/null 2>&1 &
    elif command -v open >/dev/null 2>&1; then
        open "$url" &
    fi
}

# ── Main ──────────────────────────────────────────────────────────────────────
if [[ "$DIAGNOSE" -eq 1 ]]; then
    print_diagnostics
    exit 0
fi

check_prereqs || exit 1
ensure_env_local

cd "$WEB_DIR"

case "$MODE" in
    dev)
        echo "[INFO] Starting MemoChat web dev server on http://localhost:${PORT}" >&2
        if [[ "$OPEN_BROWSER" -eq 1 ]]; then
            maybe_open_browser "http://localhost:${PORT}" &
        fi
        exec npx vite --port "$PORT" --host 0.0.0.0
        ;;
    build)
        echo "[INFO] Building MemoChat web for production..." >&2
        exec npx vite build
        ;;
    preview)
        echo "[INFO] Serving production build on http://localhost:${PORT}" >&2
        if [[ "$OPEN_BROWSER" -eq 1 ]]; then
            maybe_open_browser "http://localhost:${PORT}" &
        fi
        exec npx vite preview --port "$PORT" --host 0.0.0.0
        ;;
esac
