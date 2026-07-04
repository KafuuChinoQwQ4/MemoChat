#!/usr/bin/env bash
set -Eeuo pipefail

PROJECT_ROOT="${MEMOCHAT_PROJECT_ROOT:-/root/code/MemoChat}"
AI_COMPOSE="${MEMOCHAT_AI_COMPOSE_FILE:-$PROJECT_ROOT/apps/server/core/AIOrchestrator/docker-compose.yml}"
SMOKE_SCRIPT="${GPT_SOVITS_SMOKE_SCRIPT:-$PROJECT_ROOT/tools/scripts/pet/smoke_gpt_sovits_tts_wsl.sh}"
HOST_DATA_DIR="${MEMOCHAT_PET_SOVITS_HOST_DATA_DIR:-/data/gpt-sovits}"
REF_AUDIO="${MEMOCHAT_PET_SOVITS_REFERENCE_AUDIO:-$HOST_DATA_DIR/refs/kafuu-chino-ref.wav}"
PROMPT_TEXT="${MEMOCHAT_PET_SOVITS_PROMPT_TEXT:-}"
PROMPT_TEXT_FILE="${MEMOCHAT_PET_SOVITS_PROMPT_TEXT_FILE:-$HOST_DATA_DIR/refs/kafuu-chino-ref.ja.txt}"
CONTAINER_API_URL="${MEMOCHAT_PET_SOVITS_BASE_URL:-http://host.docker.internal:9880}"
HOST_API_URL="${GPT_SOVITS_API_URL:-http://127.0.0.1:9880}"
GPT_SOVITS_PORT="${GPT_SOVITS_PORT:-9880}"

if [ ! -x "$SMOKE_SCRIPT" ]; then
  echo "Missing GPT-SoVITS smoke script: $SMOKE_SCRIPT" >&2
  exit 1
fi

export GPT_SOVITS_API_URL="$HOST_API_URL"
export GPT_SOVITS_OUT_DIR="$HOST_DATA_DIR"
export GPT_SOVITS_HOST="${GPT_SOVITS_HOST:-0.0.0.0}"
export GPT_SOVITS_PORT

if ss -ltnp 2>/dev/null | grep -q "127.0.0.1:$GPT_SOVITS_PORT"; then
  echo "Restarting GPT-SoVITS API on 0.0.0.0:$GPT_SOVITS_PORT so Docker can reach it..."
  pkill -f "api_v2.py.*-p $GPT_SOVITS_PORT" 2>/dev/null || true
  sleep 2
fi

"$SMOKE_SCRIPT"

if [ ! -f "$REF_AUDIO" ]; then
  echo "GPT-SoVITS reference audio is missing: $REF_AUDIO" >&2
  exit 1
fi

if [ -z "$PROMPT_TEXT" ] && [ -f "$PROMPT_TEXT_FILE" ]; then
  PROMPT_TEXT="$(tr '\n' ' ' < "$PROMPT_TEXT_FILE" | sed 's/[[:space:]]\+/ /g; s/^ //; s/ $//')"
fi

export MEMOCHAT_PET_VOICE_PROVIDER="${MEMOCHAT_PET_VOICE_PROVIDER:-gpt-sovits}"
export MEMOCHAT_PET_SOVITS_BASE_URL="$CONTAINER_API_URL"
export MEMOCHAT_PET_SOVITS_REFERENCE_AUDIO="$REF_AUDIO"
export MEMOCHAT_PET_SOVITS_PROMPT_TEXT="$PROMPT_TEXT"
export MEMOCHAT_PET_SOVITS_PROMPT_LANGUAGE="${MEMOCHAT_PET_SOVITS_PROMPT_LANGUAGE:-ja}"
export MEMOCHAT_PET_SOVITS_TEXT_LANGUAGE="${MEMOCHAT_PET_SOVITS_TEXT_LANGUAGE:-zh}"
export MEMOCHAT_PET_SOVITS_TIMEOUT_SEC="${MEMOCHAT_PET_SOVITS_TIMEOUT_SEC:-90}"
export MEMOCHAT_PET_SOVITS_HOST_DATA_DIR="$HOST_DATA_DIR"

echo "Recreating AIOrchestrator with GPT-SoVITS voice:"
echo "  compose: $AI_COMPOSE"
echo "  api:     $MEMOCHAT_PET_SOVITS_BASE_URL"
echo "  ref:     $MEMOCHAT_PET_SOVITS_REFERENCE_AUDIO"
echo "  lang:    prompt=$MEMOCHAT_PET_SOVITS_PROMPT_LANGUAGE text=$MEMOCHAT_PET_SOVITS_TEXT_LANGUAGE"

docker compose -f "$AI_COMPOSE" up -d --force-recreate memochat-ai-orchestrator

for _ in $(seq 1 60); do
  if curl -fsS http://127.0.0.1:8096/health >/dev/null 2>&1; then
    break
  fi
  sleep 2
done

curl -fsS "http://127.0.0.1:8096/pet/diagnostics/voice?probe_endpoint=true"
echo
