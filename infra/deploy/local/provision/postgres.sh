#!/bin/sh
set -eu

umask 077

required() {
  name="$1"
  eval "value=\${${name}:-}"
  [ -n "$value" ] || {
    printf '[postgres-provision] missing %s\n' "$name" >&2
    exit 64
  }
}

for name in \
  MEMOCHAT_POSTGRES_USER \
  MEMOCHAT_POSTGRES_DATABASE \
  MEMOCHAT_POSTGRES_PASSWORD \
  MEMOCHAT_CHAT_POSTGRES_PASSWORD \
  MEMOCHAT_ACCOUNT_POSTGRES_PASSWORD \
  MEMOCHAT_MEDIA_POSTGRES_PASSWORD \
  MEMOCHAT_MOMENTS_POSTGRES_PASSWORD \
  MEMOCHAT_PROVISION_CALLS; do
  required "$name"
done

case "$MEMOCHAT_PROVISION_CALLS" in
  0) ;;
  1) required MEMOCHAT_CALL_POSTGRES_PASSWORD ;;
  *)
    printf '[postgres-provision] MEMOCHAT_PROVISION_CALLS must be 0 or 1\n' >&2
    exit 64
    ;;
esac

export PGHOST=memochat-postgres
export PGPORT=5432
export PGUSER="$MEMOCHAT_POSTGRES_USER"
export PGPASSWORD="$MEMOCHAT_POSTGRES_PASSWORD"
export PGDATABASE="$MEMOCHAT_POSTGRES_DATABASE"

attempt=0
until psql -X -v ON_ERROR_STOP=1 -tAc 'SELECT 1' >/dev/null 2>&1; do
  attempt=$((attempt + 1))
  [ "$attempt" -le 60 ] || {
    printf '[postgres-provision] database did not become ready\n' >&2
    exit 1
  }
  sleep 1
done

psql -X -v ON_ERROR_STOP=1 \
  -v chat_password="$MEMOCHAT_CHAT_POSTGRES_PASSWORD" \
  -v account_password="$MEMOCHAT_ACCOUNT_POSTGRES_PASSWORD" \
  -v media_password="$MEMOCHAT_MEDIA_POSTGRES_PASSWORD" \
  -v moments_password="$MEMOCHAT_MOMENTS_POSTGRES_PASSWORD" >/dev/null <<'SQL'
SELECT 'CREATE ROLE memo_chat_app LOGIN NOSUPERUSER NOCREATEDB NOCREATEROLE NOREPLICATION'
WHERE NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'memo_chat_app') \gexec
SELECT 'CREATE ROLE memo_account_app LOGIN NOSUPERUSER NOCREATEDB NOCREATEROLE NOREPLICATION'
WHERE NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'memo_account_app') \gexec
SELECT 'CREATE ROLE memo_media_app LOGIN NOSUPERUSER NOCREATEDB NOCREATEROLE NOREPLICATION'
WHERE NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'memo_media_app') \gexec
SELECT 'CREATE ROLE memo_moments_app LOGIN NOSUPERUSER NOCREATEDB NOCREATEROLE NOREPLICATION'
WHERE NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'memo_moments_app') \gexec
SELECT format('ALTER ROLE memo_chat_app PASSWORD %L', :'chat_password') \gexec
SELECT format('ALTER ROLE memo_account_app PASSWORD %L', :'account_password') \gexec
SELECT format('ALTER ROLE memo_media_app PASSWORD %L', :'media_password') \gexec
SELECT format('ALTER ROLE memo_moments_app PASSWORD %L', :'moments_password') \gexec
SQL

if [ "$MEMOCHAT_PROVISION_CALLS" = 1 ]; then
  psql -X -v ON_ERROR_STOP=1 \
    -v call_password="$MEMOCHAT_CALL_POSTGRES_PASSWORD" >/dev/null <<'SQL'
SELECT 'CREATE ROLE memo_call_app LOGIN NOSUPERUSER NOCREATEDB NOCREATEROLE NOREPLICATION'
WHERE NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'memo_call_app') \gexec
SELECT format('ALTER ROLE memo_call_app PASSWORD %L', :'call_password') \gexec
SQL
fi

apply_main_migration() {
  migration="$1"
  {
    printf 'SET search_path TO memo, public;\n'
    sed 's/__APP_SCHEMA__/memo/g' "/migrations/$migration"
  } | psql -X -v ON_ERROR_STOP=1 >/dev/null
}

for migration in \
  001_baseline.sql \
  002_chat_event_outbox.sql \
  003_moments.sql \
  004_ai_agent.sql \
  005_ai_agent_harness.sql \
  006_a2a_game_persistence.sql \
  007_a2a_game_templates.sql \
  010_password_hash.sql \
  011_auth_refresh_tokens.sql \
  012_media_access_grants.sql \
  013_r18_access_policy.sql; do
  apply_main_migration "$migration"
done

database_exists() {
  psql -X -tAc "SELECT 1 FROM pg_database WHERE datname = '$1'" | grep -q 1
}

create_database() {
  database="$1"
  if database_exists "$database"; then
    return 1
  fi
  psql -X -v ON_ERROR_STOP=1 -c "CREATE DATABASE $database" >/dev/null
  return 0
}

create_database memo_account || true
create_database memo_media || true
create_database memo_moments || true
if [ "$MEMOCHAT_PROVISION_CALLS" = 1 ]; then
  create_database memo_call || true
fi

psql -X -v ON_ERROR_STOP=1 >/dev/null <<SQL
REVOKE CONNECT ON DATABASE "$MEMOCHAT_POSTGRES_DATABASE" FROM PUBLIC;
GRANT CONNECT ON DATABASE "$MEMOCHAT_POSTGRES_DATABASE" TO "$MEMOCHAT_POSTGRES_USER", memo_chat_app;
REVOKE CONNECT ON DATABASE memo_account FROM PUBLIC;
GRANT CONNECT ON DATABASE memo_account TO "$MEMOCHAT_POSTGRES_USER", memo_account_app;
REVOKE CONNECT ON DATABASE memo_media FROM PUBLIC;
GRANT CONNECT ON DATABASE memo_media TO "$MEMOCHAT_POSTGRES_USER", memo_media_app;
REVOKE CONNECT ON DATABASE memo_moments FROM PUBLIC;
GRANT CONNECT ON DATABASE memo_moments TO "$MEMOCHAT_POSTGRES_USER", memo_moments_app;
SQL

if [ "$MEMOCHAT_PROVISION_CALLS" = 1 ]; then
  psql -X -v ON_ERROR_STOP=1 >/dev/null <<SQL
REVOKE CONNECT ON DATABASE memo_call FROM PUBLIC;
GRANT CONNECT ON DATABASE memo_call TO "$MEMOCHAT_POSTGRES_USER", memo_call_app;
SQL
fi

psql -X -v ON_ERROR_STOP=1 -d memo_account -f /migrations/009_memo_account_schema.sql >/dev/null
psql -X -v ON_ERROR_STOP=1 -d memo_account -f /migrations/010_password_hash.sql >/dev/null
psql -X -v ON_ERROR_STOP=1 -d memo_account -f /migrations/011_auth_refresh_tokens.sql >/dev/null
psql -X -v ON_ERROR_STOP=1 -d memo_account -f /migrations/013_r18_access_policy.sql >/dev/null
psql -X -v ON_ERROR_STOP=1 -d memo_media -f /migrations/008_memo_media_schema.sql >/dev/null
psql -X -v ON_ERROR_STOP=1 -d memo_moments -f /migrations/008_memo_moments_schema.sql >/dev/null
if [ "$MEMOCHAT_PROVISION_CALLS" = 1 ]; then
  psql -X -v ON_ERROR_STOP=1 -d memo_call -f /migrations/008_memo_call_schema.sql >/dev/null
fi

sh /provision/postgres_legacy_split.sh

sync_identity_sequence() {
  database="$1"
  relation="$2"
  column="$3"
  psql -X -v ON_ERROR_STOP=1 -d "$database" -tAc \
    "SELECT setval(pg_get_serial_sequence('$relation', '$column'), COALESCE(MAX($column), 1), COUNT(*) > 0) FROM $relation" \
    >/dev/null
}

sync_identity_sequence memo_account 'memo."user"' id
sync_identity_sequence memo_account memo.auth_refresh_token id
sync_identity_sequence memo_media memo.chat_media_asset media_id
sync_identity_sequence memo_media memo.chat_media_access_grant grant_id
sync_identity_sequence memo_moments memo.moments moment_id
sync_identity_sequence memo_moments memo.moments_comment id
sync_identity_sequence memo_moments memo.moments_comment_like id
sync_identity_sequence memo_moments memo.moments_like id

psql -X -v ON_ERROR_STOP=1 >/dev/null <<'SQL'
GRANT USAGE ON SCHEMA memo TO memo_chat_app;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE
  memo.friend,
  memo.friend_apply,
  memo.friend_apply_tag,
  memo.friend_tag,
  memo.chat_group,
  memo.chat_group_member,
  memo.chat_group_apply,
  memo.chat_group_msg,
  memo.chat_group_msg_ext,
  memo.chat_private_msg,
  memo.chat_dialog,
  memo.chat_dialog_meta,
  memo.chat_private_read_state,
  memo.chat_group_read_state,
  memo.chat_group_admin_permission,
  memo.chat_event_outbox
TO memo_chat_app;
GRANT USAGE, SELECT, UPDATE ON ALL SEQUENCES IN SCHEMA memo TO memo_chat_app;
SQL

printf '[postgres-provision] schemas, isolated databases, and application roles are ready\n'
