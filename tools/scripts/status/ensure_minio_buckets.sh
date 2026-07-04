#!/usr/bin/env bash
set -euo pipefail

MINIO_ENDPOINT="${MINIO_ENDPOINT:-http://127.0.0.1:9000}"

first_env()
{
    local name
    for name in "$@"; do
        if [[ -n "${!name:-}" ]]; then
            printf '%s' "${!name}"
            return 0
        fi
    done
    return 1
}

MINIO_ACCESS_KEY="$(first_env MEMOCHAT_MINIO_ROOT_USER MEMOCHAT_MINIO_ACCESSKEY MEMOCHAT_MINIO_ACCESS_KEY MINIO_ROOT_USER MINIO_ACCESS_KEY || true)"
MINIO_SECRET_KEY="$(first_env MEMOCHAT_MINIO_ROOT_PASSWORD MEMOCHAT_MINIO_SECRETKEY MEMOCHAT_MINIO_SECRET_KEY MINIO_ROOT_PASSWORD MINIO_SECRET_KEY || true)"

if [[ -z "${MINIO_ACCESS_KEY}" || -z "${MINIO_SECRET_KEY}" ]]; then
    echo "[ERROR] Missing MinIO credentials. Set MEMOCHAT_MINIO_ROOT_USER and MEMOCHAT_MINIO_ROOT_PASSWORD." >&2
    exit 1
fi

BUCKETS=(
    "${MINIO_BUCKET_AVATAR:-memochat-avatar}"
    "${MINIO_BUCKET_FILE:-memochat-file}"
    "${MINIO_BUCKET_IMAGE:-memochat-image}"
    "${MINIO_BUCKET_VIDEO:-memochat-video}"
    "${MINIO_BUCKET_MOMENTS:-memochat-moments}"
)

echo "  [*] Ensuring MinIO buckets at ${MINIO_ENDPOINT}"
docker exec memochat-minio /usr/bin/mc alias set memochat "${MINIO_ENDPOINT}" "${MINIO_ACCESS_KEY}" "${MINIO_SECRET_KEY}" >/dev/null
for bucket in "${BUCKETS[@]}"; do
    [[ -n "$bucket" ]] || continue
    docker exec memochat-minio /usr/bin/mc mb --ignore-existing "memochat/${bucket}" >/dev/null
    echo "  [OK] MinIO bucket ready: ${bucket}"
done
