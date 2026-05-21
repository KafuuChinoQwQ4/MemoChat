#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
ENV_FILE="${MEMOCHAT_ENV_FILE:-/root/.memochat-linux-env}"
PRESET="${MEMOCHAT_CMAKE_PRESET:-linux-full-gcc16}"
BUILD_DIR="${MEMOCHAT_BUILD_DIR:-build-linux-full-gcc16}"
BUILD_BIN="${MEMOCHAT_BUILD_BIN:-${BUILD_DIR}/bin}"
BUILD_PARALLEL="${MEMOCHAT_BUILD_PARALLEL:-12}"
AI_COMPOSE_FILE="${MEMOCHAT_AI_COMPOSE_FILE:-apps/server/core/AIOrchestrator/docker-compose.yml}"
LOCAL_COMPOSE_FILE="${MEMOCHAT_LOCAL_COMPOSE_FILE:-infra/deploy/local/docker-compose.yml}"
CLIENT_EXE="${MEMOCHAT_CLIENT_EXE:-${BUILD_BIN}/MemoChatQml}"
RUN_CLIENT=1
RUN_BUILD=1
RUN_AI_BUILD=1
RUN_DEPLOY=1
RUN_BACKEND=1
RUN_ENVOY=1
RUN_GPT_SOVITS=1
AI_BASE_URL="${MEMOCHAT_AI_BASE_URL:-http://127.0.0.1:8096}"
AI_VOICE_WAIT_SECONDS="${MEMOCHAT_AI_VOICE_WAIT_SECONDS:-60}"
CLIENT_DIAGNOSE=0
APP_ARGS=()

usage() {
    cat <<USAGE
Usage: $0 [options] [-- app-args...]

Build, deploy, start the Linux MemoChat backend stack, then launch MemoChatQml.

Options:
  --skip-build       Skip cmake configure/build.
  --skip-ai-build    Skip AI Orchestrator docker compose rebuild.
  --skip-deploy      Skip service deploy.
  --skip-backend     Skip backend service startup.
  --skip-envoy       Do not start/check the local Docker Envoy gateway.
  --skip-gpt-sovits  Do not start the local GPT-SoVITS voice service.
  --no-client        Do not launch MemoChatQml after backend startup.
  --client-diagnose  Print WSLg/Qt/client diagnostics instead of launching.
  --client-exe PATH  Launch a specific MemoChatQml executable.
  --parallel N       Build parallelism, default: ${BUILD_PARALLEL}.
  -h, --help         Show this help.

Environment overrides:
  MEMOCHAT_ENV_FILE, MEMOCHAT_CMAKE_PRESET, MEMOCHAT_BUILD_DIR,
  MEMOCHAT_BUILD_BIN, MEMOCHAT_BUILD_PARALLEL, MEMOCHAT_AI_COMPOSE_FILE,
  MEMOCHAT_LOCAL_COMPOSE_FILE, MEMOCHAT_CLIENT_EXE, MEMOCHAT_START_ENVOY,
  MEMOCHAT_START_GPT_SOVITS, MEMOCHAT_REQUIRE_GPT_SOVITS,
  MEMOCHAT_AI_BASE_URL, MEMOCHAT_AI_VOICE_WAIT_SECONDS.
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --skip-build)
            RUN_BUILD=0
            shift
            ;;
        --skip-ai-build)
            RUN_AI_BUILD=0
            shift
            ;;
        --skip-deploy)
            RUN_DEPLOY=0
            shift
            ;;
        --skip-backend)
            RUN_BACKEND=0
            shift
            ;;
        --skip-envoy|--no-envoy)
            RUN_ENVOY=0
            shift
            ;;
        --skip-gpt-sovits|--no-gpt-sovits)
            RUN_GPT_SOVITS=0
            shift
            ;;
        --no-client)
            RUN_CLIENT=0
            shift
            ;;
        --client-diagnose)
            CLIENT_DIAGNOSE=1
            shift
            ;;
        --client-exe)
            CLIENT_EXE="$2"
            shift 2
            ;;
        --parallel)
            BUILD_PARALLEL="$2"
            shift 2
            ;;
        --)
            shift
            APP_ARGS=("$@")
            break
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

run_step() {
    local title="$1"
    shift
    echo
    echo "[STEP] ${title}"
    echo "       $*"
    "$@"
}

is_truthy() {
    case "${1,,}" in
        1|true|yes|on) return 0 ;;
        *) return 1 ;;
    esac
}

ai_voice_ready() {
    local response_file="$1"
    local url="${AI_BASE_URL%/}/pet/diagnostics/voice?probe_endpoint=true"
    curl -fsS "$url" -o "$response_file" >/dev/null 2>&1 || return 1
    python - "$response_file" <<'PY'
import json
import sys

path = sys.argv[1]
with open(path, "r", encoding="utf-8") as fh:
    payload = json.load(fh)

diagnostics = payload.get("diagnostics") or {}
provider = diagnostics.get("provider") or {}
ready = bool(payload.get("code") == 0 and diagnostics.get("ready") and provider.get("endpoint_reachable"))
if not ready:
    message = diagnostics.get("message") or provider.get("message") or "AI voice diagnostics are not ready"
    status = provider.get("status") or "unknown"
    print(f"status={status} message={message}")
    sys.exit(1)
print("ready")
PY
}

ensure_ai_voice_ready() {
    if [[ "$RUN_GPT_SOVITS" -eq 0 ]] || [[ "${MEMOCHAT_START_GPT_SOVITS:-}" == "0" ]]; then
        echo
        echo "[SKIP] AI voice diagnostics skipped with GPT-SoVITS startup disabled"
        return 0
    fi
    if ! is_truthy "${MEMOCHAT_REQUIRE_GPT_SOVITS:-1}"; then
        echo
        echo "[SKIP] AI voice diagnostics skipped because MEMOCHAT_REQUIRE_GPT_SOVITS=0"
        return 0
    fi

    local response_file
    response_file="$(mktemp)"
    local waited=0
    local last_error=""
    local url="${AI_BASE_URL%/}/pet/diagnostics/voice?probe_endpoint=true"

    echo
    echo "[STEP] Verify AIOrchestrator GPT-SoVITS voice diagnostics"
    echo "       ${url}"
    while (( waited < AI_VOICE_WAIT_SECONDS )); do
        if last_error="$(ai_voice_ready "$response_file" 2>&1)"; then
            echo "       ready"
            rm -f -- "$response_file"
            return 0
        fi
        sleep 1
        waited=$((waited + 1))
    done

    echo "[FAIL] AIOrchestrator voice diagnostics did not become ready within ${AI_VOICE_WAIT_SECONDS}s" >&2
    if [[ -n "$last_error" ]]; then
        echo "       ${last_error}" >&2
    fi
    if [[ -s "$response_file" ]]; then
        echo "       Last response: $(tr -d '\n' < "$response_file")" >&2
    fi
    echo "       Run: ${PROJECT_ROOT}/tools/scripts/pet/apply_gpt_sovits_voice_wsl.sh" >&2
    rm -f -- "$response_file"
    return 1
}

project_path() {
    local path="$1"
    case "$path" in
        /*) printf '%s\n' "$path" ;;
        *) printf '%s\n' "${PROJECT_ROOT}/${path#./}" ;;
    esac
}

if [[ -f "$ENV_FILE" ]]; then
    # shellcheck disable=SC1090
    source "$ENV_FILE"
fi
AI_BASE_URL="${MEMOCHAT_AI_BASE_URL:-$AI_BASE_URL}"
AI_VOICE_WAIT_SECONDS="${MEMOCHAT_AI_VOICE_WAIT_SECONDS:-$AI_VOICE_WAIT_SECONDS}"

cd -- "$PROJECT_ROOT"
CLIENT_EXE="$(project_path "$CLIENT_EXE")"
DISPLAY_GPT_SOVITS="$RUN_GPT_SOVITS"
if [[ "$RUN_GPT_SOVITS" -eq 1 && "${MEMOCHAT_START_GPT_SOVITS:-}" == "0" ]]; then
    DISPLAY_GPT_SOVITS=0
fi
DISPLAY_ENVOY="$RUN_ENVOY"
if [[ "$RUN_ENVOY" -eq 1 && "${MEMOCHAT_START_ENVOY:-}" == "0" ]]; then
    DISPLAY_ENVOY=0
fi

echo "============================================================"
echo "  MemoChat Linux full-stack launcher"
echo "  PROJECT_ROOT:     ${PROJECT_ROOT}"
echo "  PRESET:           ${PRESET}"
echo "  BUILD_BIN:        ${BUILD_BIN}"
echo "  BUILD_PARALLEL:   ${BUILD_PARALLEL}"
echo "  AI_COMPOSE_FILE:  ${AI_COMPOSE_FILE}"
echo "  LOCAL_COMPOSE:    ${LOCAL_COMPOSE_FILE}"
echo "  ENVOY_GATEWAY:    ${DISPLAY_ENVOY}"
echo "  GPT_SOVITS:       ${DISPLAY_GPT_SOVITS}"
echo "  AI_BASE_URL:      ${AI_BASE_URL}"
echo "  CLIENT_EXE:       ${CLIENT_EXE}"
echo "============================================================"

if [[ "$RUN_BUILD" -eq 1 ]]; then
    run_step "Configure CMake" cmake --preset "$PRESET"
    run_step "Build CMake preset" cmake --build --preset "$PRESET" --parallel "$BUILD_PARALLEL"
else
    echo
    echo "[SKIP] CMake configure/build"
fi

if [[ "$RUN_AI_BUILD" -eq 1 ]]; then
    run_step "Rebuild AI Docker stack" docker compose -f "$AI_COMPOSE_FILE" up -d --build
else
    echo
    echo "[SKIP] AI Docker rebuild"
fi

if [[ "$RUN_DEPLOY" -eq 1 ]]; then
    run_step "Deploy backend services" "${SCRIPT_DIR}/deploy_services.sh" --build-bin "$BUILD_BIN"
else
    echo
    echo "[SKIP] Backend deploy"
fi

if [[ "$RUN_BACKEND" -eq 1 ]]; then
    start_backend_args=("${SCRIPT_DIR}/start-all-services.sh" --no-deploy)
    if [[ "$RUN_ENVOY" -eq 0 ]]; then
        start_backend_args+=(--skip-envoy)
    fi
    if [[ "$RUN_GPT_SOVITS" -eq 0 ]]; then
        start_backend_args+=(--skip-gpt-sovits)
    fi
    run_step "Start backend services" "${start_backend_args[@]}"
    ensure_ai_voice_ready
else
    echo
    echo "[SKIP] Backend startup"
fi

if [[ "$RUN_CLIENT" -eq 0 ]]; then
    echo
    echo "[DONE] Backend flow completed; client launch skipped."
    exit 0
fi

client_args=("${SCRIPT_DIR}/start-memochat-qml-wslg.sh" --exe "$CLIENT_EXE")
if [[ "$CLIENT_DIAGNOSE" -eq 1 ]]; then
    client_args+=(--diagnose)
fi
if [[ "${#APP_ARGS[@]}" -gt 0 ]]; then
    client_args+=(-- "${APP_ARGS[@]}")
fi

echo
echo "[STEP] Launch MemoChatQml"
echo "       ${client_args[*]}"
exec "${client_args[@]}"
