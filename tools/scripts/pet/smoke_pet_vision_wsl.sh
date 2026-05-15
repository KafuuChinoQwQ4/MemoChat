#!/usr/bin/env bash
set -euo pipefail

BASE_URL="http://127.0.0.1:8096/pet"
IMAGE_FILE=""
UID_VALUE="7"
PROFILE_ID="default"
PERSONA="memo-pet"
PROVIDER="scripted"

usage() {
  cat <<'EOF'
Usage: smoke_pet_vision_wsl.sh [--base-url URL] [--image-file PATH]

Checks AIOrchestrator pet vision diagnostics. When --image-file is provided,
creates a pet session and uploads one PNG/JPEG frame to /pet/sessions/{id}/capture.

Options:
  --base-url URL      Pet API base URL, default http://127.0.0.1:8096/pet
  --image-file PATH   PNG/JPEG file to base64 upload for a single capture smoke
  --uid UID           Session uid, default 7
  --help              Show this help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --base-url)
      BASE_URL="${2:?missing --base-url value}"
      shift 2
      ;;
    --image-file)
      IMAGE_FILE="${2:?missing --image-file value}"
      shift 2
      ;;
    --uid)
      UID_VALUE="${2:?missing --uid value}"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required command: $1" >&2
    exit 2
  fi
}

require_cmd curl
require_cmd python3

BASE_URL="${BASE_URL%/}"

echo "[pet-vision-smoke] GET ${BASE_URL}/diagnostics/vision"
diagnostics_body="$(curl -fsS "${BASE_URL}/diagnostics/vision")"
printf '%s\n' "$diagnostics_body" | python3 -m json.tool

if [[ -z "$IMAGE_FILE" ]]; then
  echo "[pet-vision-smoke] no --image-file supplied; diagnostics-only smoke complete"
  exit 0
fi

if [[ ! -f "$IMAGE_FILE" ]]; then
  echo "Image file not found: $IMAGE_FILE" >&2
  exit 2
fi

create_body="$(
  python3 - "$UID_VALUE" "$PROFILE_ID" "$PERSONA" "$PROVIDER" <<'PY'
import json
import sys

uid, profile_id, persona, provider = sys.argv[1:5]
print(json.dumps({
    "uid": int(uid),
    "profile_id": profile_id,
    "persona": persona,
    "provider": provider,
}))
PY
)"

echo "[pet-vision-smoke] POST ${BASE_URL}/sessions"
session_response="$(curl -fsS -H 'Content-Type: application/json' -d "$create_body" "${BASE_URL}/sessions")"
printf '%s\n' "$session_response" | python3 -m json.tool
session_id="$(printf '%s\n' "$session_response" | python3 -c 'import json,sys; print(json.load(sys.stdin)["session"]["session_id"])')"

capture_body="$(
  python3 - "$IMAGE_FILE" <<'PY'
import base64
import json
import struct
import sys
from pathlib import Path

path = Path(sys.argv[1])
data = path.read_bytes()
mime = "application/octet-stream"
width = 0
height = 0
if data.startswith(b"\x89PNG\r\n\x1a\n") and len(data) >= 24:
    mime = "image/png"
    width, height = struct.unpack(">II", data[16:24])
elif data.startswith(b"\xff\xd8"):
    mime = "image/jpeg"
    i = 2
    while i + 9 < len(data):
        while i < len(data) and data[i] == 0xff:
            i += 1
        marker = data[i] if i < len(data) else 0
        i += 1
        if marker in {0xc0, 0xc1, 0xc2, 0xc3, 0xc5, 0xc6, 0xc7, 0xc9, 0xca, 0xcb, 0xcd, 0xce, 0xcf}:
            height, width = struct.unpack(">HH", data[i + 3:i + 7])
            break
        if i + 2 > len(data):
            break
        segment_len = struct.unpack(">H", data[i:i + 2])[0]
        i += max(segment_len, 2)
print(json.dumps({
    "frame_base64": base64.b64encode(data).decode("ascii"),
    "frame_mime": mime,
    "frame_width": width,
    "frame_height": height,
    "include_frame": False,
    "metadata": {"source": "uploaded_frame", "file_name": path.name},
}))
PY
)"

echo "[pet-vision-smoke] POST ${BASE_URL}/sessions/${session_id}/capture (--image-file)"
capture_response="$(
  curl -fsS \
    -H 'Content-Type: application/json' \
    -d "$capture_body" \
    "${BASE_URL}/sessions/${session_id}/capture"
)"
printf '%s\n' "$capture_response" | python3 -m json.tool

printf '%s\n' "$capture_response" | python3 -c '
import json
import sys

payload = json.load(sys.stdin)
event_privacy = payload["event"]["privacy"]
observation_privacy = payload["observation"]["privacy"]
if event_privacy.get("raw_frame_sent") is not False:
    raise SystemExit("event privacy.raw_frame_sent was not false")
if observation_privacy.get("retention") != "none":
    raise SystemExit("observation privacy.retention was not none")
print("[pet-vision-smoke] upload capture privacy contract OK")
'
