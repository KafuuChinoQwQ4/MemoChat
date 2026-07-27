#!/bin/sh
set -eu

umask 077

for name in MEMOCHAT_MINIO_ROOT_USER MEMOCHAT_MINIO_ROOT_PASSWORD MEMOCHAT_MINIO_APP_ACCESS_KEY MEMOCHAT_MINIO_APP_SECRET_KEY; do
  eval "value=\${${name}:-}"
  [ -n "$value" ] || {
    printf '[minio-provision] missing %s\n' "$name" >&2
    exit 64
  }
done

attempt=0
until curl -fsS http://memochat-minio:9000/minio/health/live >/dev/null 2>&1; do
  attempt=$((attempt + 1))
  [ "$attempt" -le 60 ] || {
    printf '[minio-provision] object store did not become ready\n' >&2
    exit 1
  }
  sleep 1
done

mc alias set release http://memochat-minio:9000 "$MEMOCHAT_MINIO_ROOT_USER" "$MEMOCHAT_MINIO_ROOT_PASSWORD" >/dev/null
for bucket in memochat-avatar memochat-file memochat-image memochat-video memochat-moments; do
  mc mb --ignore-existing "release/$bucket" >/dev/null
done

mc admin user add release "$MEMOCHAT_MINIO_APP_ACCESS_KEY" "$MEMOCHAT_MINIO_APP_SECRET_KEY" >/dev/null
mc admin policy create release memochat-media /provision/minio-media-policy.json >/dev/null
mc admin policy attach release memochat-media --user "$MEMOCHAT_MINIO_APP_ACCESS_KEY" >/dev/null

printf '[minio-provision] buckets and media service account are ready\n'
