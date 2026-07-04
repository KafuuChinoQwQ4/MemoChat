#!/usr/bin/env bash
set -Eeuo pipefail

# gateserver split Phase 2 (slice 2b): split the account aggregate (user +
# user_id) into a dedicated memo_account database in the shared Docker Postgres.
# Prerequisite (done in code): ChatServer's cross-table `JOIN "user"` queries
# were replaced with a batch GetUsersByUids resolver, and the user/user_id read
# /write methods now use the [AccountPostgres] connection — so the user table can
# live in its own database. Idempotent + reversible (memo_pg never dropped).
#
#   migrate_phase2_account_split.sh           # create db+role, schema, copy data
#   migrate_phase2_account_split.sh --verify  # row parity + isolation
#   migrate_phase2_account_split.sh --rollback

CONTAINER="${MEMOCHAT_PG_CONTAINER:-memochat-postgres}"
PG_USER="${MEMOCHAT_PG_USER:-memochat}"
SRC_DB="${MEMOCHAT_PG_SRC_DB:-memo_pg}"
DST_DB="memo_account"
ROLE="memo_account_app"
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd 2>/dev/null || echo /root/code/MemoChat)"
MIG_DIR="${PROJECT_ROOT}/apps/server/migrations/postgresql/business"
TABLES=("user" "user_id")

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

require_value()
{
    local label="$1"
    local value="$2"
    if [[ -z "${value}" ]]; then
        echo "[ERROR] Missing ${label}; export MEMOCHAT_ACCOUNT_DB_ROLE_PASSWORD." >&2
        exit 1
    fi
}

sql_quote_literal()
{
    printf "%s" "$1" | sed "s/'/''/g"
}

required_account_role_password()
{
    local value
    value="$(first_env MEMOCHAT_ACCOUNT_DB_ROLE_PASSWORD MEMOCHAT_PHASE2_SERVICE_ROLE_PASSWORD MEMOCHAT_POSTGRES_SERVICE_ROLE_PASSWORD || true)"
    require_value "account role password" "${value}"
    printf '%s' "${value}"
}

psql_db() { docker exec -i "$CONTAINER" psql -v ON_ERROR_STOP=1 -U "$PG_USER" -d "$1" "${@:2}"; }
db_exists() { docker exec "$CONTAINER" psql -U "$PG_USER" -d "$SRC_DB" -tAc "SELECT 1 FROM pg_database WHERE datname='$1'" 2>/dev/null | grep -q 1; }
src_count() { docker exec "$CONTAINER" psql -U "$PG_USER" -d "$SRC_DB" -tAc "SELECT count(*) FROM memo.\"$1\"" 2>/dev/null | tr -d '[:space:]'; }
dst_count() { docker exec "$CONTAINER" psql -U "$PG_USER" -d "$DST_DB" -tAc "SELECT count(*) FROM memo.\"$1\"" 2>/dev/null | tr -d '[:space:]'; }

do_migrate() {
    local role_password role_password_sql
    role_password="$(required_account_role_password)"
    role_password_sql="$(sql_quote_literal "${role_password}")"

    echo "[STEP] role + db"
    docker exec "$CONTAINER" psql -v ON_ERROR_STOP=1 -U "$PG_USER" -d "$SRC_DB" -c \
        "DO \$\$ BEGIN IF NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname='$ROLE') THEN CREATE ROLE $ROLE LOGIN; END IF; END \$\$;" >/dev/null
    docker exec "$CONTAINER" psql -v ON_ERROR_STOP=1 -U "$PG_USER" -d "$SRC_DB" \
        -c "ALTER ROLE $ROLE PASSWORD '${role_password_sql}'" >/dev/null
    if ! db_exists "$DST_DB"; then
        docker exec "$CONTAINER" psql -v ON_ERROR_STOP=1 -U "$PG_USER" -d "$SRC_DB" -c "CREATE DATABASE $DST_DB" >/dev/null
        echo "  [OK] created $DST_DB"
    else
        echo "  [SKIP] $DST_DB exists"
    fi
    docker exec "$CONTAINER" psql -v ON_ERROR_STOP=1 -U "$PG_USER" -d "$SRC_DB" \
        -c "REVOKE CONNECT ON DATABASE $DST_DB FROM PUBLIC" \
        -c "GRANT CONNECT ON DATABASE $DST_DB TO $ROLE" \
        -c "GRANT CONNECT ON DATABASE $DST_DB TO $PG_USER" >/dev/null
    # account role must not reach the shared maintenance db.
    docker exec "$CONTAINER" psql -v ON_ERROR_STOP=1 -U "$PG_USER" -d "$SRC_DB" \
        -c "REVOKE CONNECT ON DATABASE $SRC_DB FROM PUBLIC" \
        -c "GRANT CONNECT ON DATABASE $SRC_DB TO $PG_USER" >/dev/null 2>&1 || true

    echo "[STEP] schema"
    psql_db "$DST_DB" < "${MIG_DIR}/009_memo_account_schema.sql" >/dev/null
    echo "[STEP] copy data"
    for t in "${TABLES[@]}"; do
        local n; n="$(src_count "$t")"
        docker exec "$CONTAINER" psql -v ON_ERROR_STOP=1 -U "$PG_USER" -d "$DST_DB" -c "TRUNCATE memo.\"$t\"" >/dev/null
        docker exec "$CONTAINER" bash -c \
            "psql -v ON_ERROR_STOP=1 -U '$PG_USER' -d '$SRC_DB' -c \"\\copy memo.\\\"$t\\\" TO STDOUT\" | psql -v ON_ERROR_STOP=1 -U '$PG_USER' -d '$DST_DB' -c \"\\copy memo.\\\"$t\\\" FROM STDIN\"" >/dev/null
        echo "  [OK] copied memo.$t ($n rows)"
    done
    # bump user identity sequence so new INSERTs don't collide.
    psql_db "$DST_DB" -c "SELECT setval(pg_get_serial_sequence('memo.\"user\"','id'), GREATEST((SELECT COALESCE(MAX(id),1) FROM memo.\"user\"),1))" >/dev/null
    echo "[SUCCESS] memo_account created + data migrated"
}

do_verify() {
    local fail=0
    local role_password
    role_password="$(required_account_role_password)"
    if ! db_exists "$DST_DB"; then echo "  [FAIL] $DST_DB missing"; return 1; fi
    for t in "${TABLES[@]}"; do
        local a b; a="$(src_count "$t")"; b="$(dst_count "$t")"
        if [[ "$a" == "$b" ]]; then echo "  [OK] $DST_DB.memo.$t = $b (matches source)"; else echo "  [FAIL] $DST_DB.memo.$t=$b != $a"; fail=1; fi
    done
    # isolation: account db must not contain chat tables.
    if psql_db "$DST_DB" -tAc "SELECT to_regclass('memo.friend')" 2>/dev/null | grep -q '^memo'; then
        echo "  [FAIL] $DST_DB can see memo.friend (isolation breach)"; fail=1
    else
        echo "  [OK] $DST_DB has no chat (friend) table"
    fi
    if docker exec "$CONTAINER" env PGPASSWORD="${role_password}" psql -U "$ROLE" -d "$SRC_DB" -tAc "SELECT 1" >/dev/null 2>&1; then
        echo "  [FAIL] $ROLE could connect to $SRC_DB"; fail=1
    else
        echo "  [OK] $ROLE cannot connect to $SRC_DB"
    fi
    # account role CAN read its own user table.
    if docker exec "$CONTAINER" env PGPASSWORD="${role_password}" psql -U "$ROLE" -d "$DST_DB" -tAc "SELECT count(*) FROM memo.\"user\"" >/dev/null 2>&1; then
        echo "  [OK] $ROLE can read its own memo.user"
    else
        echo "  [FAIL] $ROLE cannot read memo.user in $DST_DB"; fail=1
    fi
    [[ "$fail" -eq 0 ]] && echo "[SUCCESS] account split verification passed" || { echo "[FAIL]"; return 1; }
}

do_rollback() {
    if db_exists "$DST_DB"; then
        docker exec "$CONTAINER" psql -U "$PG_USER" -d "$SRC_DB" -c "DROP DATABASE $DST_DB WITH (FORCE)" >/dev/null 2>&1 \
            || docker exec "$CONTAINER" psql -U "$PG_USER" -d "$SRC_DB" -c "DROP DATABASE $DST_DB" >/dev/null
        echo "  [OK] dropped $DST_DB"
    fi
    docker exec "$CONTAINER" psql -U "$PG_USER" -d "$SRC_DB" -c "DROP ROLE IF EXISTS $ROLE" >/dev/null 2>&1 || true
    docker exec "$CONTAINER" psql -U "$PG_USER" -d "$SRC_DB" -c "GRANT CONNECT ON DATABASE $SRC_DB TO PUBLIC" >/dev/null 2>&1 || true
    echo "[SUCCESS] account rollback complete (memo_pg intact)"
}

case "${1:-migrate}" in
    migrate) do_migrate ;;
    --verify|verify) do_verify ;;
    --rollback|rollback) do_rollback ;;
    *) echo "Usage: $0 [migrate|--verify|--rollback]" >&2; exit 2 ;;
esac
