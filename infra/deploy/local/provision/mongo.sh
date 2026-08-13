#!/bin/sh
set -eu

umask 077

for name in MEMOCHAT_MONGO_ROOT_USERNAME MEMOCHAT_MONGO_ROOT_PASSWORD MEMOCHAT_MONGO_APP_USER MEMOCHAT_MONGO_APP_PASSWORD; do
  eval "value=\${${name}:-}"
  [ -n "$value" ] || {
    printf '[mongo-provision] missing %s\n' "$name" >&2
    exit 64
  }
done

attempt=0
until mongosh --quiet --host memochat-mongo --eval 'db.runCommand({ping: 1}).ok' 2>/dev/null | grep -q 1; do
  attempt=$((attempt + 1))
  [ "$attempt" -le 60 ] || {
    printf '[mongo-provision] database did not become ready\n' >&2
    exit 1
  }
  sleep 1
done

mongosh --quiet \
  --host memochat-mongo \
  --authenticationDatabase admin \
  --username "$MEMOCHAT_MONGO_ROOT_USERNAME" \
  --password "$MEMOCHAT_MONGO_ROOT_PASSWORD" \
  /provision/mongo.js >/dev/null

printf '[mongo-provision] application user and collections are ready\n'
