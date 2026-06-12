#!/usr/bin/env bash
set -euo pipefail

MINIO_ENDPOINT="${MINIO_ENDPOINT:-http://127.0.0.1:9000}"
MINIO_ACCESS_KEY="${MINIO_ACCESS_KEY:-memochat_admin}"
MINIO_SECRET_KEY="${MINIO_SECRET_KEY:-MinioPass2026!}"

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
