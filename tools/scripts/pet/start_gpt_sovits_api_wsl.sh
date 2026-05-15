#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="${GPT_SOVITS_ROOT:-/data/third_party/GPT-SoVITS}"
ENV_DIR="${GPT_SOVITS_ENV:-/data/micromamba/envs/gpt-sovits}"
MAMBA_ROOT="${MAMBA_ROOT_PREFIX:-/data/micromamba}"
HOST="${GPT_SOVITS_HOST:-0.0.0.0}"
PORT="${GPT_SOVITS_PORT:-9880}"
CONFIG="${GPT_SOVITS_CONFIG:-GPT_SoVITS/configs/tts_infer.yaml}"
LOG_DIR="${GPT_SOVITS_LOG_DIR:-/data/logs/gpt-sovits}"
LOG_FILE="${GPT_SOVITS_LOG_FILE:-$LOG_DIR/api-$PORT.log}"

mkdir -p "$LOG_DIR"
cd "$ROOT"

if [ ! -x "$ENV_DIR/bin/python" ]; then
  echo "Missing GPT-SoVITS Python env: $ENV_DIR" >&2
  exit 1
fi

NVIDIA_LIBS=""
if [ -d "$ENV_DIR/lib/python3.10/site-packages/nvidia" ]; then
  NVIDIA_LIBS="$(find "$ENV_DIR/lib/python3.10/site-packages/nvidia" -maxdepth 3 -type d -name lib | paste -sd: -)"
fi

export MAMBA_ROOT_PREFIX="$MAMBA_ROOT"
export PATH="/usr/lib/wsl/lib:$ENV_DIR/bin:/data/tools/conda-wrapper:/data/tools/micromamba/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export LD_LIBRARY_PATH="${NVIDIA_LIBS:+$NVIDIA_LIBS:}/usr/lib/wsl/lib:${LD_LIBRARY_PATH:-}"
export HF_HOME="${HF_HOME:-/data/hf-cache}"
export TRANSFORMERS_CACHE="${TRANSFORMERS_CACHE:-/data/hf-cache/transformers}"
export XDG_CACHE_HOME="${XDG_CACHE_HOME:-/data/cache}"

echo "GPT-SoVITS root: $ROOT"
echo "Python env: $ENV_DIR"
echo "API: http://$HOST:$PORT"
echo "Config: $CONFIG"
echo "Log: $LOG_FILE"
echo

"$ENV_DIR/bin/python" - <<'PY'
import torch
import torchaudio
import torchcodec
print("torch", torch.__version__, "cuda", torch.version.cuda, "available", torch.cuda.is_available())
print("gpu", torch.cuda.get_device_name(0) if torch.cuda.is_available() else "none")
print("torchaudio", torchaudio.__version__)
print("torchcodec", torchcodec.__version__)
PY

exec "$ENV_DIR/bin/python" api_v2.py -a "$HOST" -p "$PORT" -c "$CONFIG" >>"$LOG_FILE" 2>&1
