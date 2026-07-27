#!/usr/bin/env bash
set -Eeuo pipefail
set +x

umask 077

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd -P)"
DEPLOY_ROOT="${REPO_ROOT}/infra/deploy/local"
BASE_COMPOSE="${DEPLOY_ROOT}/docker-compose.yml"
RELEASE_COMPOSE="${DEPLOY_ROOT}/compose/backend-services.yml"
LIVEKIT_COMPOSE="${DEPLOY_ROOT}/compose/livekit.yml"

ENV_FILE=""
ACTION=""
declare -a PROFILES=()
declare -a ACTION_ARGS=()
declare -A ENV_VALUES=()
declare -A SECRET_OWNERS=()
declare -A SELECTED_PROFILES=()
readonly -a BACKEND_IMAGE_SLUGS=(
    ai-gateway
    ai-server
    account-server
    call-gateway
    chat-delivery-worker
    chat-message-service
    chat-relation-query-service
    chat-relation-service-worker
    chat-server
    login-server
    media-gateway
    moments-gateway
    r18-gateway
    register-server
    varify-server
)

usage() {
    cat <<'USAGE'
Usage:
  run_release_compose.sh --env-file ABSOLUTE_PATH [--profile calls|r18|observability] check
  run_release_compose.sh --env-file ABSOLUTE_PATH [--profile calls|r18|observability] config
  run_release_compose.sh --env-file ABSOLUTE_PATH [--profile calls|r18|observability] provision
  run_release_compose.sh --env-file ABSOLUTE_PATH [--profile calls|r18|observability] up [SERVICE ...]

Actions:
  check   Validate the private env file, secret strength, and bind mounts only.
  config  Run the fixed release Compose config validation after preflight.
  provision  Start datastores and run idempotent short-lived provision jobs.
  up      Provision datastores, then start the fixed release stack detached.

The private env file must be a current-user-owned, regular non-symlink file
with mode 0600. Values use literal Docker env-file assignments; interpolation,
command syntax, duplicate names, and unrecognized variables are rejected.
USAGE
}

fail() {
    printf '[release-compose] FAIL: %s\n' "$*" >&2
    exit 1
}

while [[ "$#" -gt 0 ]]; do
    case "$1" in
        --env-file)
            [[ "$#" -ge 2 ]] || fail "--env-file requires a path"
            [[ -z "$ENV_FILE" ]] || fail "--env-file may be specified only once"
            ENV_FILE="$2"
            shift 2
            ;;
        --profile)
            [[ "$#" -ge 2 ]] || fail "--profile requires a name"
            case "$2" in
                calls|r18|observability) ;;
                ai) fail "The AI profile is unavailable: AIOrchestrator and its data services are not included" ;;
                *) fail "Unsupported release Compose profile: $2" ;;
            esac
            if [[ -z "${SELECTED_PROFILES[$2]:-}" ]]; then
                PROFILES+=("$2")
                SELECTED_PROFILES["$2"]=1
            fi
            shift 2
            ;;
        check|config|provision|up)
            ACTION="$1"
            shift
            ACTION_ARGS=("$@")
            break
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            fail "Unknown option or action"
            ;;
    esac
done

[[ -n "$ENV_FILE" ]] || fail "--env-file is required"
[[ -n "$ACTION" ]] || fail "One action is required: check, config, provision, or up"
if [[ "$ACTION" != "up" && "${#ACTION_ARGS[@]}" -ne 0 ]]; then
    fail "Only the up action accepts service arguments"
fi
if [[ "$ACTION" == "up" ]]; then
    for service_name in "${ACTION_ARGS[@]}"; do
        [[ "$service_name" =~ ^[a-z0-9][a-z0-9_.-]*$ ]] \
            || fail "Invalid release Compose service name"
    done
fi
[[ "$ENV_FILE" == /* ]] || fail "Release environment path must be absolute"
[[ -f "$ENV_FILE" && ! -L "$ENV_FILE" ]] \
    || fail "Release environment must be a regular, non-symlink file"

env_mode="$(stat -c '%a' -- "$ENV_FILE")" || fail "Could not inspect release environment permissions"
env_owner="$(stat -c '%u' -- "$ENV_FILE")" || fail "Could not inspect release environment owner"
[[ "$env_mode" == "600" ]] || fail "Release environment file must have mode 0600"
[[ "$env_owner" == "$(id -u)" ]] || fail "Release environment file must be owned by the current user"
ENV_FILE="$(realpath -e -- "$ENV_FILE")" || fail "Could not resolve release environment file"

readonly -a BASE_REQUIRED_VARIABLES=(
    MEMOCHAT_BACKEND_CONFIG_ROOT
    MEMOCHAT_TLS_CERT_FILE
    MEMOCHAT_TLS_KEY_FILE
    MEMOCHAT_CPP_IMAGE_PREFIX
    MEMOCHAT_IMAGE_TAG
    MEMOCHAT_DOCKER_DATA_ROOT
    MEMOCHAT_RUNTIME_UID
    MEMOCHAT_RUNTIME_GID
    MEMOCHAT_REDIS_PASSWORD
    MEMOCHAT_POSTGRES_USER
    MEMOCHAT_POSTGRES_DATABASE
    MEMOCHAT_POSTGRES_PASSWORD
    MEMOCHAT_CHAT_POSTGRES_PASSWORD
    MEMOCHAT_ACCOUNT_POSTGRES_PASSWORD
    MEMOCHAT_MEDIA_POSTGRES_PASSWORD
    MEMOCHAT_MOMENTS_POSTGRES_PASSWORD
    MEMOCHAT_MONGO_ROOT_USERNAME
    MEMOCHAT_MONGO_ROOT_PASSWORD
    MEMOCHAT_MONGO_APP_USER
    MEMOCHAT_MONGO_APP_PASSWORD
    MEMOCHAT_RABBITMQ_USER
    MEMOCHAT_RABBITMQ_PASSWORD
    MEMOCHAT_MINIO_ROOT_USER
    MEMOCHAT_MINIO_ROOT_PASSWORD
    MEMOCHAT_MINIO_APP_ACCESS_KEY
    MEMOCHAT_MINIO_APP_SECRET_KEY
    MEMOCHAT_MINIO_PUBLIC_URL
    MEMOCHAT_CHATAUTH_HMACSECRET
    MEMOCHAT_AUTHTOKEN_JWTSECRET
    MEMOCHAT_AUTH_REFRESH_PEPPER
    MEMOCHAT_EMAIL_SMTPUSER
    MEMOCHAT_EMAIL_SMTPPASS
    MEMOCHAT_EMAIL_FROM
)
readonly -a CALLS_REQUIRED_VARIABLES=(
    MEMOCHAT_CALL_POSTGRES_PASSWORD
    MEMOCHAT_LIVEKIT_API_KEY
    MEMOCHAT_LIVEKIT_API_SECRET
    MEMOCHAT_LIVEKIT_URL
)
readonly -a R18_REQUIRED_VARIABLES=(
    MEMOCHAT_R18_SOURCE_ADMIN_KEY
    MEMOCHAT_R18_PICACG_API_KEY
    MEMOCHAT_R18_PICACG_HMAC_KEY
    MEMOCHAT_R18_CREDENTIAL_MASTER_KEY
)
readonly -a OBSERVABILITY_REQUIRED_VARIABLES=(
    MEMOCHAT_INFLUXDB_USERNAME
    MEMOCHAT_INFLUXDB_PASSWORD
    MEMOCHAT_INFLUXDB_ADMIN_TOKEN
    MEMOCHAT_GRAFANA_ADMIN_USER
    MEMOCHAT_GRAFANA_ADMIN_PASSWORD
)

declare -a REQUIRED_VARIABLES=("${BASE_REQUIRED_VARIABLES[@]}")
for profile_name in "${PROFILES[@]}"; do
    case "$profile_name" in
        calls) REQUIRED_VARIABLES+=("${CALLS_REQUIRED_VARIABLES[@]}") ;;
        r18) REQUIRED_VARIABLES+=("${R18_REQUIRED_VARIABLES[@]}") ;;
        observability) REQUIRED_VARIABLES+=("${OBSERVABILITY_REQUIRED_VARIABLES[@]}") ;;
    esac
done

declare -A ALLOWED_VARIABLES=()
for variable_name in \
    "${BASE_REQUIRED_VARIABLES[@]}" \
    "${CALLS_REQUIRED_VARIABLES[@]}" \
    "${R18_REQUIRED_VARIABLES[@]}" \
    "${OBSERVABILITY_REQUIRED_VARIABLES[@]}"; do
    ALLOWED_VARIABLES["$variable_name"]=1
done

line_number=0
while IFS= read -r env_line || [[ -n "$env_line" ]]; do
    line_number=$((line_number + 1))
    env_line="${env_line%$'\r'}"
    [[ "$env_line" =~ ^[[:space:]]*($|#) ]] && continue
    if [[ ! "$env_line" =~ ^([A-Za-z_][A-Za-z0-9_]*)=(.*)$ ]]; then
        fail "Invalid literal assignment at env line ${line_number}"
    fi

    variable_name="${BASH_REMATCH[1]}"
    raw_value="${BASH_REMATCH[2]}"
    [[ -n "${ALLOWED_VARIABLES[$variable_name]:-}" ]] \
        || fail "Unrecognized release variable at env line ${line_number}: ${variable_name}"
    [[ -z "${ENV_VALUES[$variable_name]+present}" ]] \
        || fail "Duplicate release variable: ${variable_name}"

    if [[ "$raw_value" =~ ^\'([^\']*)\'$ ]]; then
        variable_value="${BASH_REMATCH[1]}"
    elif [[ "$raw_value" =~ ^\"([^\"\\]*)\"$ ]]; then
        variable_value="${BASH_REMATCH[1]}"
    else
        [[ "$raw_value" != *[[:space:]]* ]] \
            || fail "Unquoted release value contains whitespace: ${variable_name}"
        variable_value="$raw_value"
    fi
    [[ "$variable_value" != *'$'* && "$variable_value" != *'`'* && "$variable_value" != *'\\'* ]] \
        || fail "Release values must be literal and cannot expand or execute: ${variable_name}"
    [[ -n "$variable_value" ]] || fail "Required release variable is empty: ${variable_name}"
    ENV_VALUES["$variable_name"]="$variable_value"
done < "$ENV_FILE"

for variable_name in "${REQUIRED_VARIABLES[@]}"; do
    [[ -n "${ENV_VALUES[$variable_name]+present}" ]] \
        || fail "Required release variable is missing: ${variable_name}"
done

is_weak_value() {
    local normalized="${1^^}"
    case "$normalized" in
        *REPLACE_WITH_*|*CHANGE_ME*|*CHANGEME*|TODO|PASSWORD|PASSWORD123|ADMIN|SECRET|123456|12345678|QWERTY|LETMEIN|MEMOCHAT-DEV-*)
            return 0
            ;;
    esac
    return 1
}

validate_secret() {
    local variable_name="$1"
    local minimum_length="$2"
    local variable_value="${ENV_VALUES[$variable_name]}"
    is_weak_value "$variable_value" && fail "Placeholder or weak value rejected: ${variable_name}"
    [[ "${#variable_value}" -ge "$minimum_length" ]] \
        || fail "Secret is shorter than ${minimum_length} characters: ${variable_name}"
    if [[ -n "${SECRET_OWNERS[$variable_value]:-}" ]]; then
        fail "Release secrets must not be reused: ${variable_name}"
    fi
    SECRET_OWNERS["$variable_value"]="$variable_name"
}

declare -a SECRETS_32=(
    MEMOCHAT_CHATAUTH_HMACSECRET
    MEMOCHAT_AUTHTOKEN_JWTSECRET
    MEMOCHAT_AUTH_REFRESH_PEPPER
)
declare -a SECRETS_16=(
    MEMOCHAT_REDIS_PASSWORD
    MEMOCHAT_POSTGRES_PASSWORD
    MEMOCHAT_CHAT_POSTGRES_PASSWORD
    MEMOCHAT_ACCOUNT_POSTGRES_PASSWORD
    MEMOCHAT_MEDIA_POSTGRES_PASSWORD
    MEMOCHAT_MOMENTS_POSTGRES_PASSWORD
    MEMOCHAT_MONGO_ROOT_PASSWORD
    MEMOCHAT_MONGO_APP_PASSWORD
    MEMOCHAT_RABBITMQ_PASSWORD
    MEMOCHAT_MINIO_ROOT_USER
    MEMOCHAT_MINIO_ROOT_PASSWORD
    MEMOCHAT_MINIO_APP_ACCESS_KEY
    MEMOCHAT_MINIO_APP_SECRET_KEY
    MEMOCHAT_EMAIL_SMTPPASS
)
if [[ -n "${SELECTED_PROFILES[calls]:-}" ]]; then
    SECRETS_16+=(
        MEMOCHAT_CALL_POSTGRES_PASSWORD
        MEMOCHAT_LIVEKIT_API_KEY
        MEMOCHAT_LIVEKIT_API_SECRET
    )
fi
if [[ -n "${SELECTED_PROFILES[r18]:-}" ]]; then
    SECRETS_32+=(MEMOCHAT_R18_PICACG_HMAC_KEY)
    SECRETS_16+=(
        MEMOCHAT_R18_SOURCE_ADMIN_KEY
        MEMOCHAT_R18_PICACG_API_KEY
    )
fi
if [[ -n "${SELECTED_PROFILES[observability]:-}" ]]; then
    SECRETS_16+=(
        MEMOCHAT_INFLUXDB_PASSWORD
        MEMOCHAT_INFLUXDB_ADMIN_TOKEN
        MEMOCHAT_GRAFANA_ADMIN_PASSWORD
    )
fi
for variable_name in "${SECRETS_32[@]}"; do
    validate_secret "$variable_name" 32
done
for variable_name in "${SECRETS_16[@]}"; do
    validate_secret "$variable_name" 16
done
if [[ -n "${SELECTED_PROFILES[r18]:-}" ]]; then
    validate_secret MEMOCHAT_R18_CREDENTIAL_MASTER_KEY 64
    [[ "${ENV_VALUES[MEMOCHAT_R18_CREDENTIAL_MASTER_KEY]}" =~ ^[0-9A-Fa-f]{64}$ ]] \
        || fail "MEMOCHAT_R18_CREDENTIAL_MASTER_KEY must contain exactly 64 hexadecimal characters"
fi

for variable_name in "${REQUIRED_VARIABLES[@]}"; do
    is_weak_value "${ENV_VALUES[$variable_name]}" \
        && fail "Placeholder or weak value rejected: ${variable_name}"
done

case "${ENV_VALUES[MEMOCHAT_EMAIL_FROM]}" in
    *@*.*) ;;
    *) fail "MEMOCHAT_EMAIL_FROM must be a sender email address" ;;
esac
case "${ENV_VALUES[MEMOCHAT_MINIO_PUBLIC_URL]}" in
    http://*|https://*) ;;
    *) fail "MEMOCHAT_MINIO_PUBLIC_URL must be an HTTP(S) URL" ;;
esac
if [[ -n "${SELECTED_PROFILES[calls]:-}" ]]; then
    case "${ENV_VALUES[MEMOCHAT_LIVEKIT_URL]}" in
        ws://*|wss://*) ;;
        *) fail "MEMOCHAT_LIVEKIT_URL must be a WebSocket URL" ;;
    esac
fi
[[ "${ENV_VALUES[MEMOCHAT_CPP_IMAGE_PREFIX]}" =~ ^[a-z0-9]+([._/-][a-z0-9]+)*$ ]] \
    || fail "MEMOCHAT_CPP_IMAGE_PREFIX is not a safe image prefix"
[[ "${ENV_VALUES[MEMOCHAT_IMAGE_TAG]}" =~ ^[A-Za-z0-9_][A-Za-z0-9_.-]*$ ]] \
    || fail "MEMOCHAT_IMAGE_TAG is not a safe image tag"

for variable_name in \
    MEMOCHAT_POSTGRES_USER \
    MEMOCHAT_POSTGRES_DATABASE \
    MEMOCHAT_MONGO_ROOT_USERNAME \
    MEMOCHAT_MONGO_APP_USER; do
    [[ "${ENV_VALUES[$variable_name]}" =~ ^[A-Za-z_][A-Za-z0-9_.-]*$ ]] \
        || fail "Identifier contains unsupported characters: ${variable_name}"
done
[[ "${ENV_VALUES[MEMOCHAT_MONGO_APP_PASSWORD]}" =~ ^[A-Za-z0-9._~-]+$ ]] \
    || fail "MEMOCHAT_MONGO_APP_PASSWORD must use URI-unreserved characters"
for variable_name in MEMOCHAT_RUNTIME_UID MEMOCHAT_RUNTIME_GID; do
    [[ "${ENV_VALUES[$variable_name]}" =~ ^[0-9]+$ ]] \
        || fail "${variable_name} must be a numeric id"
    (( 10#${ENV_VALUES[$variable_name]} > 0 )) \
        || fail "${variable_name} must not be root"
done

validate_canonical_absolute_directory() {
    local variable_name="$1"
    local directory_path="${ENV_VALUES[$variable_name]}"
    [[ "$directory_path" == /* ]] || fail "Path must be absolute: ${variable_name}"
    [[ -d "$directory_path" && ! -L "$directory_path" ]] \
        || fail "Path must be a real, non-symlink directory: ${variable_name}"
    [[ "$(realpath -e -- "$directory_path")" == "$directory_path" ]] \
        || fail "Path must be canonical and contain no symlink components: ${variable_name}"
}

validate_regular_mount() {
    local file_path="$1"
    local display_name="$2"
    [[ -f "$file_path" && ! -L "$file_path" ]] \
        || fail "Mount must be a regular, non-symlink file: ${display_name}"
    [[ -r "$file_path" ]] || fail "Mount must be readable: ${display_name}"
    local file_mode file_mode_octal
    file_mode="$(stat -c '%a' -- "$file_path")" || fail "Could not inspect mount permissions: ${display_name}"
    file_mode_octal=$((8#$file_mode))
    (( (file_mode_octal & 0022) == 0 )) \
        || fail "Mount must not be group- or world-writable: ${display_name}"
}

validate_canonical_absolute_directory MEMOCHAT_BACKEND_CONFIG_ROOT
validate_canonical_absolute_directory MEMOCHAT_DOCKER_DATA_ROOT

data_root_mode="$(stat -c '%a' -- "${ENV_VALUES[MEMOCHAT_DOCKER_DATA_ROOT]}")" \
    || fail "Could not inspect MEMOCHAT_DOCKER_DATA_ROOT permissions"
data_root_mode_octal=$((8#$data_root_mode))
(( (data_root_mode_octal & 0022) == 0 )) \
    || fail "MEMOCHAT_DOCKER_DATA_ROOT must not be group- or world-writable"

declare -a RUNTIME_DIRECTORIES=(media/uploads envoy/logs)
if [[ -n "${SELECTED_PROFILES[r18]:-}" ]]; then
    RUNTIME_DIRECTORIES+=(r18)
fi
for relative_runtime_directory in "${RUNTIME_DIRECTORIES[@]}"; do
    runtime_directory="${ENV_VALUES[MEMOCHAT_DOCKER_DATA_ROOT]}/${relative_runtime_directory}"
    [[ -d "$runtime_directory" && ! -L "$runtime_directory" ]] \
        || fail "Required runtime directory is missing or is a symlink: ${relative_runtime_directory}"
    [[ "$(realpath -e -- "$runtime_directory")" == "$runtime_directory" ]] \
        || fail "Runtime directory path contains a symlink component: ${relative_runtime_directory}"
    runtime_mode="$(stat -c '%a' -- "$runtime_directory")" \
        || fail "Could not inspect runtime directory mode: ${relative_runtime_directory}"
    runtime_uid="$(stat -c '%u' -- "$runtime_directory")" \
        || fail "Could not inspect runtime directory owner: ${relative_runtime_directory}"
    runtime_gid="$(stat -c '%g' -- "$runtime_directory")" \
        || fail "Could not inspect runtime directory group: ${relative_runtime_directory}"
    [[ "$runtime_mode" == "700" ]] \
        || fail "Runtime directory must have mode 0700: ${relative_runtime_directory}"
    [[ "$runtime_uid" == "${ENV_VALUES[MEMOCHAT_RUNTIME_UID]}" ]] \
        || fail "Runtime directory owner does not match MEMOCHAT_RUNTIME_UID: ${relative_runtime_directory}"
    [[ "$runtime_gid" == "${ENV_VALUES[MEMOCHAT_RUNTIME_GID]}" ]] \
        || fail "Runtime directory group does not match MEMOCHAT_RUNTIME_GID: ${relative_runtime_directory}"
done

readonly -a BASE_CONFIG_FILES=(
    AccountService/account.ini
    ChatServer/chatdeliveryworker1.ini
    ChatServer/chatmessageservice1.ini
    ChatServer/chatrelationquery1.ini
    ChatServer/chatrelationservice1.ini
    ChatServer/chatserver1.ini
    LoginService/login.ini
    MediaService/mediagateway.ini
    MomentsService/momentsgateway.ini
    RegisterService/register.ini
    VarifyServer/config.ini
)
readonly -a CALLS_CONFIG_FILES=(
    CallService/callgateway.ini
)
readonly -a R18_CONFIG_FILES=(
    R18Service/r18gateway.ini
)
declare -a CONFIG_FILES=("${BASE_CONFIG_FILES[@]}")
if [[ -n "${SELECTED_PROFILES[calls]:-}" ]]; then
    CONFIG_FILES+=("${CALLS_CONFIG_FILES[@]}")
fi
if [[ -n "${SELECTED_PROFILES[r18]:-}" ]]; then
    CONFIG_FILES+=("${R18_CONFIG_FILES[@]}")
fi
for relative_config in "${CONFIG_FILES[@]}"; do
    config_path="${ENV_VALUES[MEMOCHAT_BACKEND_CONFIG_ROOT]}/${relative_config}"
    validate_regular_mount "$config_path" "$relative_config"
    [[ "$(realpath -e -- "$config_path")" == "$config_path" ]] \
        || fail "Config mount path contains a symlink component: ${relative_config}"
done

for variable_name in MEMOCHAT_TLS_CERT_FILE MEMOCHAT_TLS_KEY_FILE; do
    file_path="${ENV_VALUES[$variable_name]}"
    [[ "$file_path" == /* ]] || fail "Path must be absolute: ${variable_name}"
    validate_regular_mount "$file_path" "$variable_name"
    [[ "$(realpath -e -- "$file_path")" == "$file_path" ]] \
        || fail "Path must be canonical and contain no symlink components: ${variable_name}"
done

key_file="${ENV_VALUES[MEMOCHAT_TLS_KEY_FILE]}"
key_mode="$(stat -c '%a' -- "$key_file")" || fail "Could not inspect MEMOCHAT_TLS_KEY_FILE permissions"
key_owner="$(stat -c '%u' -- "$key_file")" || fail "Could not inspect MEMOCHAT_TLS_KEY_FILE owner"
[[ "$key_mode" == "400" || "$key_mode" == "600" ]] \
    || fail "MEMOCHAT_TLS_KEY_FILE must have mode 0400 or 0600"
[[ "$key_owner" == "$(id -u)" ]] \
    || fail "MEMOCHAT_TLS_KEY_FILE must be owned by the current user"

command -v openssl >/dev/null 2>&1 || fail "openssl is required to validate the TLS certificate and key"
cert_file="${ENV_VALUES[MEMOCHAT_TLS_CERT_FILE]}"
openssl x509 -in "$cert_file" -noout -checkend 86400 >/dev/null 2>&1 \
    || fail "MEMOCHAT_TLS_CERT_FILE is invalid or expires within 24 hours"
cert_public_key="$(openssl x509 -in "$cert_file" -pubkey -noout 2>/dev/null |
    openssl pkey -pubin -outform DER 2>/dev/null | sha256sum | cut -d' ' -f1)"
key_public_key="$(openssl pkey -in "$key_file" -pubout -outform DER </dev/null 2>/dev/null |
    sha256sum | cut -d' ' -f1)"
[[ -n "$cert_public_key" && "$cert_public_key" == "$key_public_key" ]] \
    || fail "MEMOCHAT_TLS_CERT_FILE and MEMOCHAT_TLS_KEY_FILE do not form a matching key pair"

printf '[release-compose] Preflight passed: private env, secret policy, and bind mounts validated.\n'

if [[ "$ACTION" == "check" ]]; then
    exit 0
fi
command -v docker >/dev/null 2>&1 || fail "docker is required for the ${ACTION} action"

while IFS= read -r variable_name; do
    case "$variable_name" in
        MEMOCHAT_*) unset "$variable_name" ;;
    esac
done < <(compgen -A variable)
unset COMPOSE_FILE COMPOSE_PATH_SEPARATOR COMPOSE_ENV_FILES COMPOSE_PROFILES COMPOSE_PROJECT_NAME
unset COMPOSE_MEMOCHAT_ENABLE_CALLS
if [[ -n "${SELECTED_PROFILES[calls]:-}" ]]; then
    export COMPOSE_MEMOCHAT_ENABLE_CALLS=1
else
    export COMPOSE_MEMOCHAT_ENABLE_CALLS=0
fi

declare -a COMPOSE_ARGS=(
    compose
    --env-file "$ENV_FILE"
    --project-directory "$DEPLOY_ROOT"
    -f "$BASE_COMPOSE"
    -f "$RELEASE_COMPOSE"
    -f "$LIVEKIT_COMPOSE"
)
for profile_name in "${PROFILES[@]}"; do
    COMPOSE_ARGS+=(--profile "$profile_name")
done

if [[ "$ACTION" == "config" ]]; then
    exec docker "${COMPOSE_ARGS[@]}" config --quiet
fi

run_provision() {
    docker "${COMPOSE_ARGS[@]}" up -d memochat-postgres memochat-mongo memochat-minio
    local provision_service
    for provision_service in \
        memochat-release-provision-postgres \
        memochat-release-provision-mongo \
        memochat-release-provision-minio; do
        docker "${COMPOSE_ARGS[@]}" --profile provision run --rm --no-deps "$provision_service"
    done
}

if [[ "$ACTION" == "provision" ]]; then
    run_provision
    exit 0
fi

verify_backend_images() {
    local image_prefix="${ENV_VALUES[MEMOCHAT_CPP_IMAGE_PREFIX]}"
    local image_tag="${ENV_VALUES[MEMOCHAT_IMAGE_TAG]}"
    local image_slug image
    for image_slug in "${BACKEND_IMAGE_SLUGS[@]}"; do
        image="${image_prefix}/${image_slug}:${image_tag}"
        docker image inspect -- "$image" >/dev/null 2>&1 \
            || fail "Required backend image is not present locally: ${image}"
    done
}

verify_backend_images
run_provision
exec docker "${COMPOSE_ARGS[@]}" up -d "${ACTION_ARGS[@]}"
