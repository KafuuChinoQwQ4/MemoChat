#!/usr/bin/env bash
set -euo pipefail

CONTAINER="${MEMOCHAT_POSTGRES_CONTAINER:-memochat-postgres}"
PGUSER="${MEMOCHAT_POSTGRES_USER:-memochat}"
CXX="${CXX:-g++}"

if [[ -n "${MEMOCHAT_PASSWORD_HASH_DATABASES:-}" ]]; then
    read -r -a DATABASES <<< "${MEMOCHAT_PASSWORD_HASH_DATABASES}"
else
    DATABASES=(memo_account memo_pg)
fi

if ! docker inspect "${CONTAINER}" >/dev/null 2>&1; then
    echo "Postgres container not found: ${CONTAINER}" >&2
    exit 1
fi

tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

helper_cpp="${tmpdir}/hash_password.cpp"
helper_bin="${tmpdir}/hash_password"

cat > "${helper_cpp}" <<'CPP'
#include <sodium.h>

#include <iostream>
#include <iterator>
#include <string>

int main()
{
    if (sodium_init() < 0)
    {
        return 1;
    }

    const std::string password((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
    if (password.empty())
    {
        return 2;
    }

    char hash[crypto_pwhash_STRBYTES] = {};
    if (crypto_pwhash_str(hash,
                          password.data(),
                          static_cast<unsigned long long>(password.size()),
                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
    {
        return 3;
    }

    std::cout << hash << '\n';
    return 0;
}
CPP

"${CXX}" -std=c++17 "${helper_cpp}" -o "${helper_bin}" $(pkg-config --cflags --libs libsodium)

psql_db()
{
    local db="$1"
    shift
    docker exec -i "${CONTAINER}" psql -v ON_ERROR_STOP=1 -U "${PGUSER}" -d "${db}" "$@"
}

database_exists()
{
    local db="$1"
    local exists
    exists="$(docker exec "${CONTAINER}" psql -v ON_ERROR_STOP=1 -U "${PGUSER}" -d postgres -Atqc \
        "SELECT EXISTS (SELECT 1 FROM pg_database WHERE datname = '${db}');")"
    [[ "${exists}" == "t" ]]
}

user_table_exists()
{
    local db="$1"
    local exists
    exists="$(docker exec "${CONTAINER}" psql -v ON_ERROR_STOP=1 -U "${PGUSER}" -d "${db}" -Atqc \
        "SELECT to_regclass('memo.\"user\"') IS NOT NULL;")"
    [[ "${exists}" == "t" ]]
}

migrate_database()
{
    local db="$1"

    if ! database_exists "${db}"; then
        echo "[SKIP] ${db}: database not found"
        return
    fi
    if ! user_table_exists "${db}"; then
        echo "[SKIP] ${db}: memo.user table not found"
        return
    fi

    psql_db "${db}" <<'SQL'
ALTER TABLE memo."user"
    ADD COLUMN IF NOT EXISTS password_hash text NOT NULL DEFAULT '';
SQL

    local migrated=0
    while IFS=$'\t' read -r uid password_b64; do
        if [[ -z "${uid}" || -z "${password_b64}" ]]; then
            continue
        fi

        local password
        password="$(printf '%s' "${password_b64}" | base64 --decode)"

        local password_hash
        password_hash="$(printf '%s' "${password}" | "${helper_bin}")"
        unset password

        psql_db "${db}" -v uid="${uid}" -v password_hash="${password_hash}" <<'SQL'
UPDATE memo."user"
SET password_hash = :'password_hash',
    pwd = ''
WHERE uid = :uid;
SQL
        migrated=$((migrated + 1))
    done < <(docker exec -i "${CONTAINER}" psql -v ON_ERROR_STOP=1 -U "${PGUSER}" -d "${db}" -At -F $'\t' -c \
        "SELECT uid, encode(convert_to(pwd, 'UTF8'), 'base64') FROM memo.\"user\" WHERE coalesce(password_hash, '') = '' AND coalesce(pwd, '') <> '' ORDER BY uid;")

    psql_db "${db}" -qc "UPDATE memo.\"user\" SET pwd = '' WHERE coalesce(password_hash, '') <> '' AND coalesce(pwd, '') <> '';"

    local summary
    summary="$(docker exec "${CONTAINER}" psql -v ON_ERROR_STOP=1 -U "${PGUSER}" -d "${db}" -Atqc \
        "SELECT count(*) FILTER (WHERE coalesce(password_hash, '') <> '') || ' hashes, ' || count(*) FILTER (WHERE coalesce(pwd, '') <> '') || ' non-empty pwd' FROM memo.\"user\";")"
    echo "[OK] ${db}: migrated ${migrated} rows; ${summary}"
}

for db in "${DATABASES[@]}"; do
    migrate_database "${db}"
done
