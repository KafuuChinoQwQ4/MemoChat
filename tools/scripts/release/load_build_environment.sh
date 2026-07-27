#!/usr/bin/env bash

# Source this file with the private self-hosted runner environment as its only
# argument. It deliberately keeps toolchain settings in the caller shell while
# removing runtime credentials before any build command starts.

_memochat_load_build_environment() {
    if [[ "$#" -ne 1 ]]; then
        echo "[FAIL] Usage: source tools/scripts/release/load_build_environment.sh ENV_FILE" >&2
        return 2
    fi

    local _memochat_build_env_file="$1"
    if [[ ! -f "$_memochat_build_env_file" || -L "$_memochat_build_env_file" ]]; then
        echo "[FAIL] Build environment must be a regular, non-symlink file" >&2
        return 1
    fi

    local _memochat_build_env_mode _memochat_build_env_owner
    _memochat_build_env_mode="$(stat -c '%a' -- "$_memochat_build_env_file")" || return 1
    _memochat_build_env_owner="$(stat -c '%u' -- "$_memochat_build_env_file")" || return 1
    if [[ "$_memochat_build_env_mode" != "600" ]]; then
        echo "[FAIL] Build environment file must have mode 0600" >&2
        return 1
    fi
    if [[ "$_memochat_build_env_owner" != "$(id -u)" ]]; then
        echo "[FAIL] Build environment file must be owned by the current runner user" >&2
        return 1
    fi

    local _memochat_restore_xtrace=0
    if [[ "$-" == *x* ]]; then
        _memochat_restore_xtrace=1
        set +x
    fi

    local -A _memochat_loaded_names=()
    local _memochat_env_line _memochat_variable_name _memochat_assignment_value
    local _memochat_single_quoted_value_re="^'[^']*'[[:space:]]*(#.*)?$"
    local _memochat_double_quoted_value_re='^"([^"\\]|\\.)*"[[:space:]]*(#.*)?$'
    local _memochat_unquoted_value_re='^[^[:space:];&|<>()`]+[[:space:]]*(#.*)?$'
    local _memochat_empty_value_re='^[[:space:]]*(#.*)?$'
    while IFS= read -r _memochat_env_line || [[ -n "$_memochat_env_line" ]]; do
        _memochat_env_line="${_memochat_env_line%$'\r'}"
        [[ "$_memochat_env_line" =~ ^[[:space:]]*($|#) ]] && continue
        if [[ "$_memochat_env_line" =~ ^[[:space:]]*(export[[:space:]]+)?([A-Za-z_][A-Za-z0-9_]*)= ]]; then
            _memochat_loaded_names["${BASH_REMATCH[2]}"]=1
            _memochat_assignment_value="${_memochat_env_line#*=}"
            if [[ "$_memochat_assignment_value" =~ $_memochat_single_quoted_value_re ]]; then
                continue
            fi
            if [[ "$_memochat_assignment_value" == *'`'* || "$_memochat_assignment_value" == *'$('* ]]; then
                echo "[FAIL] Build environment assignments cannot execute commands" >&2
                [[ "$_memochat_restore_xtrace" -eq 1 ]] && set -x
                return 1
            fi
            if [[ "$_memochat_assignment_value" =~ $_memochat_double_quoted_value_re \
                || "$_memochat_assignment_value" =~ $_memochat_unquoted_value_re \
                || "$_memochat_assignment_value" =~ $_memochat_empty_value_re ]]; then
                continue
            fi
            echo "[FAIL] Build environment assignments must contain exactly one shell value" >&2
            [[ "$_memochat_restore_xtrace" -eq 1 ]] && set -x
            return 1
        elif [[ "$_memochat_env_line" =~ ^[[:space:]]*unset[[:space:]]+[A-Za-z_][A-Za-z0-9_]*([[:space:]]+[A-Za-z_][A-Za-z0-9_]*)*[[:space:]]*$ ]]; then
            continue
        else
            echo "[FAIL] Build environment may contain only variable assignments and simple unset directives" >&2
            [[ "$_memochat_restore_xtrace" -eq 1 ]] && set -x
            return 1
        fi
    done < "$_memochat_build_env_file"

    # shellcheck disable=SC1090
    if ! source "$_memochat_build_env_file" >/dev/null 2>&1; then
        echo "[FAIL] Could not load the private build environment" >&2
        [[ "$_memochat_restore_xtrace" -eq 1 ]] && set -x
        return 1
    fi

    while IFS= read -r _memochat_variable_name; do
        case "$_memochat_variable_name" in
            MEMOCHAT_*|AWS_*|AZURE_*|GCP_*|GOOGLE_APPLICATION_CREDENTIALS|PGPASSWORD|PGPASSFILE|DATABASE_URL|REDIS_URL|MONGODB_URI|MONGO_URI|RABBITMQ_URL|SMTP_URL|*_PASSWORD|*_PASSWD|*_SECRET|*_SECRET_KEY|*_ACCESS_KEY|*_ACCESSKEY|*_API_KEY|*_HMAC_KEY|*_PRIVATE_KEY|*_TOKEN|*_CREDENTIAL|*_CREDENTIALS|*_AUTHORIZATION|*_COOKIE)
                if ! unset "$_memochat_variable_name"; then
                    echo "[FAIL] Could not remove a sensitive build environment variable" >&2
                    [[ "$_memochat_restore_xtrace" -eq 1 ]] && set -x
                    return 1
                fi
                ;;
        esac
    done < <(compgen -A variable)

    for _memochat_variable_name in "${!_memochat_loaded_names[@]}"; do
        case "$_memochat_variable_name" in
            PATH|LD_LIBRARY_PATH|LIBRARY_PATH|CPATH|CPLUS_INCLUDE_PATH|PKG_CONFIG_PATH|CC|CXX|CPP|AR|AS|LD|NM|OBJCOPY|OBJDUMP|RANLIB|READELF|STRIP|CMAKE_*|CTEST_*|NINJA_*|CCACHE_*|VCPKG_*|QT_*|Qt*|QML_*|GTK_IM_MODULE|IBUS_ENABLE_SYNC_MODE|XMODIFIERS|LANG|LANGUAGE|LC_*)
                ;;
            *)
                if ! unset "$_memochat_variable_name"; then
                    echo "[FAIL] Could not remove a non-toolchain build environment variable" >&2
                    [[ "$_memochat_restore_xtrace" -eq 1 ]] && set -x
                    return 1
                fi
                ;;
        esac
    done

    # Parallelism is release policy, not private machine configuration. Leaving
    # either variable set can override the audited jobs value in CMakePresets.json.
    if ! unset CMAKE_BUILD_PARALLEL_LEVEL VCPKG_MAX_CONCURRENCY; then
        echo "[FAIL] Could not remove build parallelism overrides" >&2
        [[ "$_memochat_restore_xtrace" -eq 1 ]] && set -x
        return 1
    fi

    [[ "$_memochat_restore_xtrace" -eq 1 ]] && set -x
    return 0
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    echo "[FAIL] This helper must be sourced so toolchain variables reach the caller" >&2
    exit 2
fi

_memochat_load_build_environment_status=0
_memochat_load_build_environment "$@" || _memochat_load_build_environment_status=$?
unset -f _memochat_load_build_environment
return "$_memochat_load_build_environment_status"
