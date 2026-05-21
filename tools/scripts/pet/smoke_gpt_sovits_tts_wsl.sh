#!/usr/bin/env bash
set -Eeuo pipefail

API_URL="${GPT_SOVITS_API_URL:-http://127.0.0.1:9880}"
START_SCRIPT="${GPT_SOVITS_START_SCRIPT:-/root/code/MemoChat-Qml-Drogon-linux/tools/scripts/pet/start_gpt_sovits_api_wsl.sh}"
VOICE_DIR="${GPT_SOVITS_VOICE_DIR:-/root/code/MemoChat-Qml-Drogon-linux/apps/client/desktop/MemoChat-qml/src/KafuuChino/香风智乃voice}"
REF_AUDIO="${GPT_SOVITS_REF_AUDIO:-}"
PROMPT_TEXT="${GPT_SOVITS_PROMPT_TEXT:-}"
PROMPT_TEXT_FILE="${GPT_SOVITS_PROMPT_TEXT_FILE:-/data/gpt-sovits/refs/kafuu-chino-ref.ja.txt}"
PROMPT_LANG="${GPT_SOVITS_PROMPT_LANG:-ja}"
TEXT="${GPT_SOVITS_TEXT:-欢迎回来，今天也一起努力吧。}"
TEXT_LANG="${GPT_SOVITS_TEXT_LANG:-zh}"
OUT_DIR="${GPT_SOVITS_OUT_DIR:-/data/gpt-sovits}"

mkdir -p "$OUT_DIR/refs" "$OUT_DIR/out" /data/logs/gpt-sovits

if [ -z "$PROMPT_TEXT" ] && [ -f "$PROMPT_TEXT_FILE" ]; then
  PROMPT_TEXT="$(tr '\n' ' ' < "$PROMPT_TEXT_FILE" | sed 's/[[:space:]]\+/ /g; s/^ //; s/ $//')"
fi

wait_for_api() {
  for _ in $(seq 1 120); do
    if curl -fsS "$API_URL/docs" >/dev/null 2>&1; then
      return 0
    fi
    sleep 2
  done
  return 1
}

if ! curl -fsS "$API_URL/docs" >/dev/null 2>&1; then
  echo "GPT-SoVITS API is not listening; starting it..."
  "$START_SCRIPT"
  wait_for_api
fi

if [ -z "$REF_AUDIO" ]; then
  REF_AUDIO="$(
    find "$VOICE_DIR" -maxdepth 1 -type f \( -iname '*.mp3' -o -iname '*.wav' -o -iname '*.flac' -o -iname '*.m4a' \) -print0 |
      while IFS= read -r -d '' file; do
        duration="$(ffprobe -v error -show_entries format=duration -of default=nw=1:nk=1 "$file" 2>/dev/null || echo 0)"
        awk -v duration="$duration" -v file="$file" 'BEGIN {
          if (duration >= 6 && duration <= 9.8) {
            score = duration >= 8 ? duration - 8 : 8 - duration
            printf "%010.3f\t%010.3f\t%s\n", score, duration, file
          }
        }'
      done |
      sort -n |
      head -1 |
      cut -f3-
  )"
fi

if [ -z "$REF_AUDIO" ] || [ ! -f "$REF_AUDIO" ]; then
  echo "No suitable reference audio found under: $VOICE_DIR" >&2
  exit 1
fi

REF_WAV="$OUT_DIR/refs/kafuu-chino-ref.wav"
OUT_WAV="$OUT_DIR/out/kafuu-chino-smoke.wav"
REQ_JSON="$OUT_DIR/tts-smoke.json"

echo "Reference source: $REF_AUDIO"
ffmpeg -hide_banner -loglevel error -y -i "$REF_AUDIO" -ac 1 -ar 32000 "$REF_WAV"
echo "Reference wav: $REF_WAV"
ffprobe -hide_banner -loglevel error -show_entries format=duration -of default=nw=1:nk=1 "$REF_WAV"

python - "$REQ_JSON" "$REF_WAV" "$TEXT" "$TEXT_LANG" "$PROMPT_TEXT" "$PROMPT_LANG" <<'PY'
import json
import sys

out, ref_audio, text, text_lang, prompt_text, prompt_lang = sys.argv[1:]
payload = {
    "text": text,
    "text_lang": text_lang,
    "ref_audio_path": ref_audio,
    "prompt_text": prompt_text,
    "prompt_lang": prompt_lang,
    "text_split_method": "cut5",
    "batch_size": 1,
    "media_type": "wav",
    "streaming_mode": False,
    "speed_factor": 1.0,
    "top_k": 15,
    "top_p": 1,
    "temperature": 1,
}
with open(out, "w", encoding="utf-8") as fh:
    json.dump(payload, fh, ensure_ascii=False)
PY

echo "Request: $REQ_JSON"
status="$(curl -sS -X POST "$API_URL/tts" -H 'Content-Type: application/json' --data-binary "@$REQ_JSON" -o "$OUT_WAV" -w '%{http_code}')"
echo "HTTP status: $status"

if [ "$status" != "200" ]; then
  echo "TTS request failed. Response body:"
  cat "$OUT_WAV"
  exit 1
fi

echo "Output wav: $OUT_WAV"
file "$OUT_WAV"
ffprobe -hide_banner -loglevel error -show_entries format=duration -of default=nw=1:nk=1 "$OUT_WAV"
