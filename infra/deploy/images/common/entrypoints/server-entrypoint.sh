#!/bin/sh
set -eu

fail() {
  printf '[entrypoint] %s\n' "$*" >&2
  exit 64
}

if [ "${1:-}" = "--healthcheck" ]; then
  kill -0 1 2>/dev/null || exit 1
  exit 0
fi

umask 077

service="${MEMOCHAT_SERVICE:-}"
case "${service}" in
  AIGatewayServer|AIServer|AccountServer|CallGatewayServer|ChatDeliveryWorker|ChatMessageService|ChatRelationQueryService|ChatRelationServiceWorker|ChatServer|LoginServer|MediaGatewayServer|MomentsGatewayServer|R18GatewayServer|RegisterServer|VarifyServer)
    ;;
  *)
    fail "Unsupported MEMOCHAT_SERVICE: ${service:-<empty>}"
    ;;
esac

if [ "${MEMOCHAT_RELEASE_MODE:-1}" = "1" ] && [ "${MEMOCHAT_ALLOW_DEV_SECRETS:-0}" != "0" ]; then
  fail "MEMOCHAT_ALLOW_DEV_SECRETS must remain 0 in release mode"
fi

config_path="${CONFIG_PATH:-/run/memochat/config.ini}"
case "${config_path}" in
  /*) ;;
  *) fail "CONFIG_PATH must be absolute" ;;
esac
[ -f "${config_path}" ] && [ -r "${config_path}" ] \
  || fail "Config file is required and must be readable: ${config_path}"

for variable_name in ${REQUIRED_ENV_VARS:-}; do
  case "${variable_name}" in
    ''|*[!A-Z0-9_]*) fail "Invalid name in REQUIRED_ENV_VARS" ;;
  esac

  eval "variable_value=\${${variable_name}:-}"
  [ -n "${variable_value}" ] || fail "Required environment variable is empty: ${variable_name}"

  normalized_value="$(printf '%s' "${variable_value}" | tr '[:lower:]' '[:upper:]')"
  case "${normalized_value}" in
    REPLACE_WITH_*|CHANGE_ME*|CHANGEME*|TODO|PASSWORD|ADMIN|SECRET|123456|MEMOCHAT-DEV-*)
      fail "Required environment variable has a placeholder or weak value: ${variable_name}"
      ;;
  esac

  case "${variable_name}" in
    *PEPPER*)
      [ "${#variable_value}" -ge 32 ] \
        || fail "Required pepper environment variable is too short: ${variable_name}"
      ;;
    *HMAC*|*JWT*)
      [ "${#variable_value}" -ge 32 ] \
        || fail "Required signing secret environment variable is too short: ${variable_name}"
      ;;
    *PASSWORD*|*PASSWD*|*SECRET*|*TOKEN*|*APIKEY*|*API_KEY*|*MONGO_URI*)
      [ "${#variable_value}" -ge 16 ] \
        || fail "Required secret environment variable is too short: ${variable_name}"
      ;;
  esac
done

target_bin="/app/bin/${service}"
[ -x "${target_bin}" ] || fail "Service executable is missing or not executable: ${target_bin}"

config_dir="$(dirname -- "${config_path}")"
cd "${config_dir}"
exec "${target_bin}" --config "${config_path}" "$@"
