#!/usr/bin/env bash
set -euo pipefail

CONTAINER="${MEMOCHAT_REDIS_CONTAINER:-memochat-redis}"
REDIS_PASSWORD="${MEMOCHAT_REDIS_PASSWORD:-}"
DRY_RUN="${MEMOCHAT_REDIS_PASSWORD_CLEANUP_DRY_RUN:-0}"

if [[ -z "${REDIS_PASSWORD}" ]]; then
    echo "Missing MEMOCHAT_REDIS_PASSWORD" >&2
    exit 1
fi

if [[ -n "${MEMOCHAT_REDIS_PASSWORD_CLEANUP_PATTERNS:-}" ]]; then
    read -r -a PATTERNS <<<"${MEMOCHAT_REDIS_PASSWORD_CLEANUP_PATTERNS}"
else
    PATTERNS=(ulogin_profile_\* ubaseinfo_\* nameinfo_\*)
fi

if ! docker inspect "${CONTAINER}" >/dev/null 2>&1; then
    echo "Redis container not found: ${CONTAINER}" >&2
    exit 1
fi

redis_cli()
{
    docker exec -e REDISCLI_AUTH="${REDIS_PASSWORD}" "${CONTAINER}" redis-cli --raw "$@"
}

key_payload()
{
    local key="$1"
    local type
    type="$(redis_cli type "${key}" | tr -d '\r')"
    case "${type}" in
        string)
            redis_cli get "${key}" || true
            ;;
        hash)
            redis_cli hgetall "${key}" || true
            ;;
        *)
            return 1
            ;;
    esac
}

contains_password_field()
{
    local key="$1"
    key_payload "${key}" |
        grep -Eiq '(^|["{,[:space:]])(pwd|passwd|password)($|"|[[:space:]]*:|=)'
}

deleted=0
scanned=0

for pattern in "${PATTERNS[@]}"; do
    while IFS= read -r key; do
        [[ -n "${key}" ]] || continue
        scanned=$((scanned + 1))
        if ! contains_password_field "${key}"; then
            continue
        fi
        if [[ "${DRY_RUN}" == "1" || "${DRY_RUN,,}" == "true" ]]; then
            echo "[DRY-RUN] would delete ${key}"
        else
            redis_cli del "${key}" >/dev/null
            echo "[DELETE] ${key}"
        fi
        deleted=$((deleted + 1))
    done < <(redis_cli --scan --pattern "${pattern}")
done

echo "[OK] scanned ${scanned} keys; matched ${deleted} password-bearing cache keys"
