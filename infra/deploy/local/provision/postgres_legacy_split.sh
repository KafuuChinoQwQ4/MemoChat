#!/bin/sh
set -eu

umask 077

transfer_file=""
cleanup() {
  [ -z "$transfer_file" ] || rm -f -- "$transfer_file"
}
trap cleanup EXIT HUP INT TERM

required() {
  name="$1"
  eval "value=\${${name}:-}"
  [ -n "$value" ] || {
    printf '[postgres-split-migration] missing %s\n' "$name" >&2
    exit 64
  }
}

for name in PGHOST PGPORT PGUSER PGPASSWORD MEMOCHAT_POSTGRES_DATABASE MEMOCHAT_PROVISION_CALLS; do
  required "$name"
done

case "$MEMOCHAT_PROVISION_CALLS" in
  0|1) ;;
  *)
    printf '[postgres-split-migration] MEMOCHAT_PROVISION_CALLS must be 0 or 1\n' >&2
    exit 64
    ;;
esac

ensure_marker_table() {
  destination="$1"
  psql -X -v ON_ERROR_STOP=1 -d "$destination" >/dev/null <<'SQL'
CREATE SCHEMA IF NOT EXISTS memochat_release;
REVOKE ALL ON SCHEMA memochat_release FROM PUBLIC;
CREATE TABLE IF NOT EXISTS memochat_release.data_migration (
  migration_id text PRIMARY KEY,
  applied_at timestamptz NOT NULL DEFAULT clock_timestamp()
);
REVOKE ALL ON TABLE memochat_release.data_migration FROM PUBLIC;
SQL
}

marker_is_applied() {
  destination="$1"
  migration_id="$2"
  if ! marker_state="$(
    psql -X -v ON_ERROR_STOP=1 -d "$destination" \
      -v migration_id="$migration_id" -tA <<'SQL'
SELECT EXISTS (
  SELECT 1
  FROM memochat_release.data_migration
  WHERE migration_id = :'migration_id'
);
SQL
  )"; then
    printf '[postgres-split-migration] failed to read migration marker %s\n' "$migration_id" >&2
    exit 1
  fi

  case "$marker_state" in
    t) return 0 ;;
    f) return 1 ;;
    *)
      printf '[postgres-split-migration] invalid marker state for %s\n' "$migration_id" >&2
      exit 1
      ;;
  esac
}

copy_legacy_table_once() {
  destination="$1"
  table="$2"
  migration_id="$3"

  ensure_marker_table "$destination"
  if marker_is_applied "$destination" "$migration_id"; then
    return 0
  fi

  transfer_file="$(mktemp /tmp/memochat-postgres-copy.XXXXXX)"
  psql -X -v ON_ERROR_STOP=1 -d "$MEMOCHAT_POSTGRES_DATABASE" \
    -c "\\copy memo.$table TO STDOUT" >"$transfer_file"

  psql -X -v ON_ERROR_STOP=1 -d "$destination" \
    -v migration_id="$migration_id" >/dev/null <<SQL
BEGIN;
SELECT pg_advisory_xact_lock(hashtextextended(:'migration_id', 0));
SELECT EXISTS (
  SELECT 1
  FROM memochat_release.data_migration
  WHERE migration_id = :'migration_id'
) AS migration_already_applied \gset
\if :migration_already_applied
\else
  SELECT EXISTS (SELECT 1 FROM memo.$table) AS destination_has_rows \gset
  \if :destination_has_rows
  \else
    \copy memo.$table FROM '$transfer_file'
  \endif
  INSERT INTO memochat_release.data_migration (migration_id)
  VALUES (:'migration_id');
\endif
COMMIT;
SQL

  rm -f -- "$transfer_file"
  transfer_file=""
}

copy_legacy_table_once memo_account '"user"' 'legacy-split-v1:memo_account:user'
copy_legacy_table_once memo_account user_id 'legacy-split-v1:memo_account:user_id'
copy_legacy_table_once memo_account auth_refresh_token 'legacy-split-v1:memo_account:auth_refresh_token'
copy_legacy_table_once memo_media chat_media_asset 'legacy-split-v1:memo_media:chat_media_asset'
copy_legacy_table_once memo_media chat_media_access_grant 'legacy-split-v1:memo_media:chat_media_access_grant'
copy_legacy_table_once memo_moments moments 'legacy-split-v1:memo_moments:moments'
copy_legacy_table_once memo_moments moments_comment 'legacy-split-v1:memo_moments:moments_comment'
copy_legacy_table_once memo_moments moments_comment_like 'legacy-split-v1:memo_moments:moments_comment_like'
copy_legacy_table_once memo_moments moments_like 'legacy-split-v1:memo_moments:moments_like'
if [ "$MEMOCHAT_PROVISION_CALLS" = 1 ]; then
  copy_legacy_table_once memo_call chat_call_session 'legacy-split-v1:memo_call:chat_call_session'
fi

printf '[postgres-split-migration] legacy split copy is complete\n'
