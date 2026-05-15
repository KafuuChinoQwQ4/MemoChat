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
CLIENT_EXE="${MEMOCHAT_CLIENT_EXE:-${BUILD_BIN}/MemoChatQml}"
RUN_CLIENT=1
RUN_BUILD=1
RUN_AI_BUILD=1
RUN_DEPLOY=1
RUN_BACKEND=1
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
  --no-client        Do not launch MemoChatQml after backend startup.
  --client-diagnose  Print WSLg/Qt/client diagnostics instead of launching.
  --client-exe PATH  Launch a specific MemoChatQml executable.
  --parallel N       Build parallelism, default: ${BUILD_PARALLEL}.
  -h, --help         Show this help.

Environment overrides:
  MEMOCHAT_ENV_FILE, MEMOCHAT_CMAKE_PRESET, MEMOCHAT_BUILD_DIR,
  MEMOCHAT_BUILD_BIN, MEMOCHAT_BUILD_PARALLEL, MEMOCHAT_AI_COMPOSE_FILE,
  MEMOCHAT_CLIENT_EXE.
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

cd -- "$PROJECT_ROOT"
CLIENT_EXE="$(project_path "$CLIENT_EXE")"

echo "============================================================"
echo "  MemoChat Linux full-stack launcher"
echo "  PROJECT_ROOT:     ${PROJECT_ROOT}"
echo "  PRESET:           ${PRESET}"
echo "  BUILD_BIN:        ${BUILD_BIN}"
echo "  BUILD_PARALLEL:   ${BUILD_PARALLEL}"
echo "  AI_COMPOSE_FILE:  ${AI_COMPOSE_FILE}"
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
    run_step "Start backend services" "${SCRIPT_DIR}/start-all-services.sh" --no-deploy
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
