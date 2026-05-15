#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="/data/third_party/GPT-SoVITS"
MAMBA_BIN="/data/tools/micromamba/bin/micromamba"
MAMBA_ROOT="/data/micromamba"
CONDA_WRAPPER="/data/tools/conda-wrapper/conda"
LOG_DIR="/data/logs/gpt-sovits"
SOURCE="${GPT_SOVITS_SOURCE:-ModelScope}"
DEVICE="${GPT_SOVITS_DEVICE:-CU128}"

mkdir -p /data/third_party /data/tools/micromamba/bin "$MAMBA_ROOT" "$LOG_DIR"

echo "[1/6] Checking GPU and tools"
command -v git >/dev/null
command -v ffmpeg >/dev/null
command -v ffprobe >/dev/null
nvidia-smi || true

echo "[2/6] Installing micromamba if missing"
if [ ! -x "$MAMBA_BIN" ]; then
  tmp="/data/tools/micromamba/micromamba.tar.bz2"
  curl -L --retry 5 --connect-timeout 20 \
    https://micro.mamba.pm/api/micromamba/linux-64/latest \
    -o "$tmp"
  tar -xjf "$tmp" -C /data/tools/micromamba bin/micromamba
fi
"$MAMBA_BIN" --version

echo "[3/6] Cloning/updating GPT-SoVITS"
if [ ! -d "$ROOT/.git" ]; then
  git clone --depth 1 https://github.com/RVC-Boss/GPT-SoVITS.git "$ROOT"
else
  git -C "$ROOT" pull --ff-only || true
fi

echo "[4/6] Creating Python 3.10 environment"
export MAMBA_ROOT_PREFIX="$MAMBA_ROOT"
if ! "$MAMBA_BIN" env list | awk '{print $1}' | grep -qx "gpt-sovits"; then
  "$MAMBA_BIN" create -y -n gpt-sovits -c conda-forge python=3.10 pip
fi
"$MAMBA_BIN" run -n gpt-sovits python --version

echo "[5/6] Preparing conda wrapper and Arch GCC16 patch"
mkdir -p "$(dirname "$CONDA_WRAPPER")"
cat > "$CONDA_WRAPPER" <<'SH'
#!/usr/bin/env bash
export MAMBA_ROOT_PREFIX=/data/micromamba
exec /data/tools/micromamba/bin/micromamba "$@"
SH
chmod +x "$CONDA_WRAPPER"

cat > "$ROOT/install.memochat.sh" <<'SH'
#!/usr/bin/env bash
set -Eeuo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

python - <<'PY'
from pathlib import Path
source = Path("install.sh").read_text(encoding="utf-8")
source = source.replace(
    'run_conda_quiet "libstdcxx-ng>=$gcc_major_version"',
    'run_conda_quiet "libstdcxx-ng>=15"',
)
source = source.replace('tput cuu1 && tput el', '(tput cuu1 && tput el) || true')
Path(".install.memochat.generated.sh").write_text(source, encoding="utf-8")
PY

exec bash ./.install.memochat.generated.sh "$@"
SH
chmod +x "$ROOT/install.memochat.sh"

echo "[6/6] Running GPT-SoVITS installer: device=$DEVICE source=$SOURCE"
cd "$ROOT"
log="$LOG_DIR/install-$(date +%Y%m%d-%H%M%S).log"
echo "Log: $log"
env -i \
  HOME=/root \
  MAMBA_ROOT_PREFIX="$MAMBA_ROOT" \
  WORKFLOW=false \
  TERM="${TERM:-xterm}" \
  PATH="/data/tools/conda-wrapper:/data/tools/micromamba/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
  "$MAMBA_BIN" run -n gpt-sovits bash "$ROOT/install.memochat.sh" --device "$DEVICE" --source "$SOURCE" \
  2>&1 | tee "$log"

echo
echo "GPT-SoVITS install finished."
echo "Source: $ROOT"
echo "Env: $MAMBA_ROOT/envs/gpt-sovits"
echo "Log: $log"
echo
echo "Next command to verify imports:"
echo "env MAMBA_ROOT_PREFIX=$MAMBA_ROOT $MAMBA_BIN run -n gpt-sovits python -c 'import torch; print(torch.__version__, torch.cuda.is_available())'"
