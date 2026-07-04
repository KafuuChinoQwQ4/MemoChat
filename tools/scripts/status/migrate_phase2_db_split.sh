#!/usr/bin/env bash
set -Eeuo pipefail

# gateserver microservice split Phase 2 (slice 2a): orchestrate the per-service
# PostgreSQL database split for the safely-splittable domains (media/moments/
# call) in the shared local Docker Postgres instance. Idempotent and reversible.
#
#   migrate_phase2_db_split.sh           # create dbs+roles, load schema, copy data
#   migrate_phase2_db_split.sh --verify  # verify row counts match + cross-db isolation
#   migrate_phase2_db_split.sh --rollback # drop the new dbs+roles (memo_pg untouched)
#
# memo_pg.memo.* source tables are NEVER dropped by this script — the split makes
# COPIES so a rollback is a clean discard. account is intentionally NOT handled
# here (slice 2b: ChatServer still JOINs the user table). r18 has no PG tables.

CONTAINER="${MEMOCHAT_PG_CONTAINER:-memochat-postgres}"
PG_USER="${MEMOCHAT_PG_USER:-memochat}"
SRC_DB="${MEMOCHAT_PG_SRC_DB:-memo_pg}"
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd 2>/dev/null || echo /root/code/MemoChat)"
MIG_DIR="${PROJECT_ROOT}/apps/server/migrations/postgresql/business"

# domain -> "database|schema_file|table list"
declare -A DB_OF=( [media]=memo_media [moments]=memo_moments [call]=memo_call )
declare -A SCHEMA_OF=( [media]=008_memo_media_schema.sql [moments]=008_memo_moments_schema.sql [call]=008_memo_call_schema.sql )
declare -A TABLES_OF=(
    [media]="chat_media_asset"
    [moments]="moments moments_comment moments_comment_like moments_like"
    [call]="chat_call_session"
)
DOMAINS=(media moments call)

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
        echo "[ERROR] Missing ${label}; export MEMOCHAT_PHASE2_SERVICE_ROLE_PASSWORD." >&2
        exit 1
    fi
}

sql_quote_literal()
{
    printf "%s" "$1" | sed "s/'/''/g"
}

required_service_role_password()
{
    local value
    value="$(first_env MEMOCHAT_PHASE2_SERVICE_ROLE_PASSWORD MEMOCHAT_POSTGRES_SERVICE_ROLE_PASSWORD || true)"
    require_value "service role password" "${value}"
    printf '%s' "${value}"
}

psql_db() { docker exec -i "$CONTAINER" psql -v ON_ERROR_STOP=1 -U "$PG_USER" -d "$1" "${@:2}"; }
psql_src() { psql_db "$SRC_DB" "$@"; }

db_exists() { docker exec "$CONTAINER" psql -U "$PG_USER" -d "$SRC_DB" -tAc "SELECT 1 FROM pg_database WHERE datname='$1'" 2>/dev/null | grep -q 1; }

create_database_if_absent() {
    local db="$1"
    if db_exists "$db"; then
        echo "  [SKIP] database $db already exists"
    else
        docker exec "$CONTAINER" psql -v ON_ERROR_STOP=1 -U "$PG_USER" -d "$SRC_DB" -c "CREATE DATABASE $db" >/dev/null
        echo "  [OK] created database $db"
    fi
}

src_count() { psql_src -tAc "SELECT count(*) FROM memo.$1" 2>/dev/null | tr -d '[:space:]'; }
dst_count() { psql_db "$1" -tAc "SELECT count(*) FROM memo.$2" 2>/dev/null | tr -d '[:space:]'; }

do_migrate() {
    local service_password service_password_sql
    service_password="$(required_service_role_password)"
    service_password_sql="$(sql_quote_literal "${service_password}")"

    echo "[STEP] Create roles (008_db_split_media_moments_call.sql)"
    psql_src < "${MIG_DIR}/008_db_split_media_moments_call.sql" >/dev/null
    echo "  [OK] roles ensured"
    for d in "${DOMAINS[@]}"; do
        docker exec "$CONTAINER" psql -v ON_ERROR_STOP=1 -U "$PG_USER" -d "$SRC_DB" \
            -c "ALTER ROLE memo_${d}_app PASSWORD '${service_password_sql}'" >/dev/null
    done
    echo "  [OK] role passwords injected from environment"

    # A service role must not be able to connect to the shared maintenance db
    # memo_pg (it owns only its own per-service db). PUBLIC holds implicit CONNECT
    # on memo_pg, so revoke it from PUBLIC and grant it back to the owner role
    # (memochat) that every other service uses. The owner can always connect
    # regardless, but the explicit GRANT keeps intent clear and is idempotent.
    docker exec "$CONTAINER" psql -v ON_ERROR_STOP=1 -U "$PG_USER" -d "$SRC_DB" \
        -c "REVOKE CONNECT ON DATABASE $SRC_DB FROM PUBLIC" \
        -c "GRANT CONNECT ON DATABASE $SRC_DB TO $PG_USER" >/dev/null 2>&1 || true

    for d in "${DOMAINS[@]}"; do
        local db="${DB_OF[$d]}"
        echo "[STEP] ${d} -> ${db}"
        create_database_if_absent "$db"
        # Lock down DB-level CONNECT: revoke from PUBLIC, grant only to the owning
        # service role, so a foreign service role cannot even connect (DB-per-
        # service isolation, D-DATA). memochat (superuser/maintenance) keeps access.
        local role="memo_${d}_app"
        docker exec "$CONTAINER" psql -v ON_ERROR_STOP=1 -U "$PG_USER" -d "$SRC_DB" \
            -c "REVOKE CONNECT ON DATABASE $db FROM PUBLIC" \
            -c "GRANT CONNECT ON DATABASE $db TO $role" >/dev/null
        echo "  [*] load schema ${SCHEMA_OF[$d]}"
        psql_db "$db" < "${MIG_DIR}/${SCHEMA_OF[$d]}" >/dev/null
        for t in ${TABLES_OF[$d]}; do
            # copy data memo_pg.memo.<t> -> <db>.memo.<t> via COPY through a pipe.
            local n; n="$(src_count "$t")"
            docker exec "$CONTAINER" psql -v ON_ERROR_STOP=1 -U "$PG_USER" -d "$db" -c "TRUNCATE memo.$t" >/dev/null
            docker exec "$CONTAINER" bash -c \
                "psql -v ON_ERROR_STOP=1 -U '$PG_USER' -d '$SRC_DB' -c \"\\copy memo.$t TO STDOUT\" | psql -v ON_ERROR_STOP=1 -U '$PG_USER' -d '$db' -c \"\\copy memo.$t FROM STDIN\"" >/dev/null
            # fix identity sequence high-water so new inserts do not collide.
            local pk
            pk="$(psql_db "$db" -tAc "SELECT a.attname FROM pg_index i JOIN pg_attribute a ON a.attrelid=i.indrelid AND a.attnum=ANY(i.indkey) WHERE i.indrelid='memo.$t'::regclass AND i.indisprimary LIMIT 1")"
            if psql_db "$db" -tAc "SELECT pg_get_serial_sequence('memo.$t','$pk')" | grep -q .; then
                psql_db "$db" -c "SELECT setval(pg_get_serial_sequence('memo.$t','$pk'), GREATEST((SELECT COALESCE(MAX($pk),1) FROM memo.$t),1))" >/dev/null
            fi
            echo "  [OK] copied memo.$t ($n rows)"
        done
    done
    echo "[SUCCESS] media/moments/call databases created + data migrated"
}

do_verify() {
    local fail=0
    local service_password
    service_password="$(required_service_role_password)"
    echo "[STEP] Verify row-count parity (source memo_pg vs per-service db)"
    echo "  [NOTE] parity is a point-in-time check: run right after migrate. If the"
    echo "         GateServer monolith (memo_pg) served writes since migration, re-run"
    echo "         'migrate' to re-sync — the Envoy path routes these domains to the"
    echo "         gateways (per-service db), so production writes do not hit memo_pg."
    for d in "${DOMAINS[@]}"; do
        local db="${DB_OF[$d]}"
        if ! db_exists "$db"; then echo "  [FAIL] $db missing"; fail=1; continue; fi
        for t in ${TABLES_OF[$d]}; do
            local a b; a="$(src_count "$t")"; b="$(dst_count "$db" "$t")"
            if [[ "$a" == "$b" ]]; then echo "  [OK] $db.memo.$t = $b (matches source)";
            else echo "  [FAIL] $db.memo.$t=$b != source=$a"; fail=1; fi
        done
    done
    echo "[STEP] Verify cross-database isolation (a service db must NOT see foreign tables)"
    # memo_media must not contain moments/call tables.
    if psql_db memo_media -tAc "SELECT to_regclass('memo.moments')" 2>/dev/null | grep -q '^memo'; then
        echo "  [FAIL] memo_media can see memo.moments (isolation breach)"; fail=1
    else
        echo "  [OK] memo_media has no moments table"
    fi
    if psql_db memo_moments -tAc "SELECT to_regclass('memo.chat_media_asset')" 2>/dev/null | grep -q '^memo'; then
        echo "  [FAIL] memo_moments can see memo.chat_media_asset"; fail=1
    else
        echo "  [OK] memo_moments has no media table"
    fi
    # role isolation: media role cannot connect to moments db.
    if docker exec "$CONTAINER" env PGPASSWORD="${service_password}" psql -U memo_media_app -d memo_moments -tAc "SELECT 1" >/dev/null 2>&1; then
        echo "  [FAIL] memo_media_app could connect to memo_moments (role breach)"; fail=1
    else
        echo "  [OK] memo_media_app cannot connect to memo_moments"
    fi
    # service roles must not connect to the shared maintenance db memo_pg.
    for d in "${DOMAINS[@]}"; do
        if docker exec "$CONTAINER" env PGPASSWORD="${service_password}" psql -U "memo_${d}_app" -d "$SRC_DB" -tAc "SELECT 1" >/dev/null 2>&1; then
            echo "  [FAIL] memo_${d}_app could connect to $SRC_DB (must be isolated)"; fail=1
        else
            echo "  [OK] memo_${d}_app cannot connect to $SRC_DB"
        fi
    done
    [[ "$fail" -eq 0 ]] && echo "[SUCCESS] Phase 2a verification passed" || { echo "[FAIL] verification failed"; return 1; }
}

do_rollback() {
    echo "[STEP] Rollback: drop per-service databases (memo_pg untouched)"
    for d in "${DOMAINS[@]}"; do
        local db="${DB_OF[$d]}"
        if db_exists "$db"; then
            docker exec "$CONTAINER" psql -U "$PG_USER" -d "$SRC_DB" -c "DROP DATABASE $db WITH (FORCE)" >/dev/null 2>&1 \
                || docker exec "$CONTAINER" psql -U "$PG_USER" -d "$SRC_DB" -c "DROP DATABASE $db" >/dev/null
            echo "  [OK] dropped $db"
        else
            echo "  [SKIP] $db absent"
        fi
    done
    psql_src < "${MIG_DIR}/008_db_split_media_moments_call_rollback.sql" >/dev/null
    # Restore the original PUBLIC CONNECT on memo_pg (forward migration revoked it).
    docker exec "$CONTAINER" psql -U "$PG_USER" -d "$SRC_DB" \
        -c "GRANT CONNECT ON DATABASE $SRC_DB TO PUBLIC" >/dev/null 2>&1 || true
    echo "[SUCCESS] rollback complete (databases + roles dropped; memo_pg intact)"
}

case "${1:-migrate}" in
    migrate) do_migrate ;;
    --verify|verify) do_verify ;;
    --rollback|rollback) do_rollback ;;
    *) echo "Usage: $0 [migrate|--verify|--rollback]" >&2; exit 2 ;;
esac
