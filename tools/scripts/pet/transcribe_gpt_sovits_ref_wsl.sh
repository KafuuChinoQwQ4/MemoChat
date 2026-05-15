#!/usr/bin/env bash
set -Eeuo pipefail

ENV_DIR="${GPT_SOVITS_ENV:-/data/micromamba/envs/gpt-sovits}"
REF_AUDIO="${GPT_SOVITS_REF_AUDIO:-/data/gpt-sovits/refs/kafuu-chino-ref.wav}"
OUT_TEXT="${GPT_SOVITS_REF_TEXT_OUT:-/data/gpt-sovits/refs/kafuu-chino-ref.ja.txt}"
MODEL="${WHISPER_MODEL:-base}"
CACHE_DIR="${WHISPER_CACHE_DIR:-/data/whisper-cache}"

mkdir -p "$(dirname "$OUT_TEXT")" "$CACHE_DIR"

NVIDIA_LIBS=""
if [ -d "$ENV_DIR/lib/python3.10/site-packages/nvidia" ]; then
  NVIDIA_LIBS="$(find "$ENV_DIR/lib/python3.10/site-packages/nvidia" -maxdepth 3 -type d -name lib | paste -sd: -)"
fi

export MAMBA_ROOT_PREFIX="${MAMBA_ROOT_PREFIX:-/data/micromamba}"
export PATH="/usr/lib/wsl/lib:$ENV_DIR/bin:/data/tools/conda-wrapper:/data/tools/micromamba/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export LD_LIBRARY_PATH="${NVIDIA_LIBS:+$NVIDIA_LIBS:}/usr/lib/wsl/lib:${LD_LIBRARY_PATH:-}"

"$ENV_DIR/bin/python" - "$REF_AUDIO" "$OUT_TEXT" "$MODEL" "$CACHE_DIR" <<'PY'
import sys
from pathlib import Path

audio, out_text, model_name, cache_dir = sys.argv[1:]

try:
    import torch
    import whisper
except Exception as exc:
    raise SystemExit(f"ASR import failed: {type(exc).__name__}: {exc}")

device = "cuda" if torch.cuda.is_available() else "cpu"
model = whisper.load_model(model_name, device=device, download_root=cache_dir)
result = model.transcribe(
    audio,
    language="ja",
    task="transcribe",
    fp16=(device == "cuda"),
    temperature=0,
    condition_on_previous_text=False,
)
text = " ".join(result.get("text", "").strip().split())
Path(out_text).write_text(text + "\n", encoding="utf-8")
print(text)
PY

echo "Wrote: $OUT_TEXT"
