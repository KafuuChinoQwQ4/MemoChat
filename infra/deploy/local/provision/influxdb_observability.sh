#!/bin/sh
set -eu

umask 077

payload_file=
token_temp_file=
cleanup() {
  [ -z "$payload_file" ] || rm -f "$payload_file"
  [ -z "$token_temp_file" ] || rm -f "$token_temp_file"
}
trap cleanup EXIT HUP INT TERM

for name in INFLUX_HOST INFLUX_ORG INFLUX_TOKEN_FILE INFLUX_BUCKET; do
  eval "value=\${${name}:-}"
  [ -n "$value" ] || {
    printf '[influxdb-provision] missing %s\n' "$name" >&2
    exit 64
  }
done

[ -f "$INFLUX_TOKEN_FILE" ] && [ ! -L "$INFLUX_TOKEN_FILE" ] && [ -r "$INFLUX_TOKEN_FILE" ] || {
  printf '[influxdb-provision] administrator token file is missing or unsafe\n' >&2
  exit 64
}
[ "$(stat -c '%a' "$INFLUX_TOKEN_FILE")" = 400 ] || {
  printf '[influxdb-provision] administrator token file must have mode 0400\n' >&2
  exit 64
}
[ "$(stat -c '%u:%g' "$INFLUX_TOKEN_FILE")" = "$(id -u):$(id -g)" ] || {
  printf '[influxdb-provision] administrator token file has the wrong owner\n' >&2
  exit 64
}
[ "$(wc -l <"$INFLUX_TOKEN_FILE")" -eq 1 ] || {
  printf '[influxdb-provision] administrator token file must contain one line\n' >&2
  exit 64
}
IFS= read -r INFLUX_TOKEN <"$INFLUX_TOKEN_FILE"
[ "${#INFLUX_TOKEN}" -ge 32 ] || {
  printf '[influxdb-provision] administrator token is invalid\n' >&2
  exit 64
}
case "$INFLUX_TOKEN" in
  *[!A-Za-z0-9._~+/=-]*)
    printf '[influxdb-provision] administrator token contains invalid characters\n' >&2
    exit 64
    ;;
esac
export INFLUX_TOKEN

attempt=0
until curl -fsS "${INFLUX_HOST}/health" >/dev/null 2>&1; do
  attempt=$((attempt + 1))
  [ "$attempt" -le 60 ] || {
    printf '[influxdb-provision] database did not become ready\n' >&2
    exit 1
  }
  sleep 1
done

bucket_id="$(influx bucket list --name "$INFLUX_BUCKET" --json | dasel -r json -w plain '.[0].id')"
org_id="$(influx org list --name "$INFLUX_ORG" --json | dasel -r json -w plain '.[0].id')"
for resource_id in "$bucket_id" "$org_id"; do
  case "$resource_id" in
    ''|*[!0-9a-f]*)
      printf '[influxdb-provision] API returned an invalid resource id\n' >&2
      exit 1
      ;;
  esac
  [ "${#resource_id}" -eq 16 ] || {
    printf '[influxdb-provision] API returned an invalid resource id length\n' >&2
    exit 1
  }
done

readonly scraper_name=memochat-prometheus-federation
readonly scraper_url='http://memochat-prometheus:9090/federate?match%5B%5D=%7B__name__%3D~%22.%2B%22%7D'
readonly auth_header_name=Authorization

scraper_response="$(
  curl --fail-with-body --silent --show-error --config - \
    "${INFLUX_HOST}/api/v2/scrapers?orgID=${org_id}&name=${scraper_name}" <<EOF
header = "${auth_header_name}: Token ${INFLUX_TOKEN}"
EOF
)"
existing_id="$(
  printf '%s' "$scraper_response" |
    dasel -r json -w plain '.configurations.[0].id' 2>/dev/null || true
)"

payload_file="$(mktemp /tmp/memochat-influx-scraper.XXXXXX)"
printf '%s\n' \
  "{\"allowInsecure\":false,\"bucketID\":\"${bucket_id}\",\"name\":\"${scraper_name}\",\"orgID\":\"${org_id}\",\"type\":\"prometheus\",\"url\":\"${scraper_url}\"}" \
  >"$payload_file"

if [ -n "$existing_id" ]; then
  case "$existing_id" in
    *[!0-9a-f]*)
      printf '[influxdb-provision] API returned an invalid scraper id\n' >&2
      exit 1
      ;;
  esac
  [ "${#existing_id}" -eq 16 ] || {
    printf '[influxdb-provision] API returned an invalid scraper id length\n' >&2
    exit 1
  }
  method=PATCH
  endpoint="${INFLUX_HOST}/api/v2/scrapers/${existing_id}"
else
  method=POST
  endpoint="${INFLUX_HOST}/api/v2/scrapers"
fi

curl --fail-with-body --silent --show-error \
  --request "$method" \
  --header 'Content-Type: application/json' \
  --data-binary "@${payload_file}" \
  --config - \
  "$endpoint" \
  >/dev/null <<EOF
header = "${auth_header_name}: Token ${INFLUX_TOKEN}"
EOF

readonly grafana_token_directory=/run/secrets/memochat-influxdb
readonly grafana_token_file="${grafana_token_directory}/grafana-reader.token"
readonly grafana_auth_description=memochat-grafana-metrics-reader
readonly grafana_permission="read:orgs/${org_id}/buckets/${bucket_id}"

[ -d "$grafana_token_directory" ] && [ ! -L "$grafana_token_directory" ] || {
  printf '[influxdb-provision] Grafana secret directory is missing or unsafe\n' >&2
  exit 1
}

valid_api_token() {
  [ "${#1}" -ge 32 ] || return 1
  case "$1" in
    *[!A-Za-z0-9._~+/=-]*) return 1 ;;
  esac
  return 0
}

authorizations="$(influx auth list --json)"
reader_token=
reader_auth_id=
stale_auth_ids=
auth_index=0
while description="$(
  printf '%s' "$authorizations" |
    dasel -r json -w plain ".[${auth_index}].description" 2>/dev/null
)"; do
  if [ "$description" = "$grafana_auth_description" ]; then
    candidate_id="$(
      printf '%s' "$authorizations" |
        dasel -r json -w plain ".[${auth_index}].id"
    )"
    candidate_status="$(
      printf '%s' "$authorizations" |
        dasel -r json -w plain ".[${auth_index}].status"
    )"
    candidate_permission="$(
      printf '%s' "$authorizations" |
        dasel -r json -w plain ".[${auth_index}].permissions.[0]" 2>/dev/null || true
    )"
    candidate_token="$(
      printf '%s' "$authorizations" |
        dasel -r json -w plain ".[${auth_index}].token" 2>/dev/null || true
    )"
    case "$candidate_id" in
      ''|*[!0-9a-f]*)
        printf '[influxdb-provision] API returned an invalid authorization id\n' >&2
        exit 1
        ;;
    esac
    [ "${#candidate_id}" -eq 16 ] || {
      printf '[influxdb-provision] API returned an invalid authorization id length\n' >&2
      exit 1
    }
    has_extra_permission=false
    if printf '%s' "$authorizations" |
      dasel -r json -w plain ".[${auth_index}].permissions.[1]" >/dev/null 2>&1; then
      has_extra_permission=true
    fi

    candidate_valid=true
    [ "$candidate_status" = active ] || candidate_valid=false
    [ "$candidate_permission" = "$grafana_permission" ] || candidate_valid=false
    [ "$has_extra_permission" = false ] || candidate_valid=false
    valid_api_token "$candidate_token" || candidate_valid=false

    if [ "$candidate_valid" = true ] && [ -z "$reader_token" ]; then
      reader_token="$candidate_token"
      reader_auth_id="$candidate_id"
    else
      stale_auth_ids="${stale_auth_ids} ${candidate_id}"
    fi
  fi
  auth_index=$((auth_index + 1))
done

if [ -z "$reader_token" ]; then
  authorization_response="$(
    influx auth create \
      --org "$INFLUX_ORG" \
      --read-bucket "$bucket_id" \
      --description "$grafana_auth_description" \
      --json
  )"
  reader_auth_id="$(
    printf '%s' "$authorization_response" | dasel -r json -w plain '.id'
  )"
  reader_token="$(
    printf '%s' "$authorization_response" | dasel -r json -w plain '.token'
  )"
  created_permission="$(
    printf '%s' "$authorization_response" | dasel -r json -w plain '.permissions.[0]'
  )"
  [ "$created_permission" = "$grafana_permission" ] || {
    printf '[influxdb-provision] created Grafana token has unexpected permissions\n' >&2
    exit 1
  }
  if printf '%s' "$authorization_response" |
    dasel -r json -w plain '.permissions.[1]' >/dev/null 2>&1; then
    printf '[influxdb-provision] created Grafana token is not read-only\n' >&2
    exit 1
  fi
  valid_api_token "$reader_token" || {
    printf '[influxdb-provision] created Grafana token is invalid\n' >&2
    exit 1
  }
fi

case "$reader_auth_id" in
  ''|*[!0-9a-f]*)
    printf '[influxdb-provision] API returned an invalid Grafana authorization id\n' >&2
    exit 1
    ;;
esac
[ "${#reader_auth_id}" -eq 16 ] || {
  printf '[influxdb-provision] API returned an invalid Grafana authorization id length\n' >&2
  exit 1
}

for stale_auth_id in $stale_auth_ids; do
  [ "$stale_auth_id" = "$reader_auth_id" ] || influx auth delete --id "$stale_auth_id" >/dev/null
done

token_temp_file="$(mktemp "${grafana_token_file}.tmp.XXXXXX")"
printf '%s\n' "$reader_token" >"$token_temp_file"
chmod 0600 "$token_temp_file"
mv -f "$token_temp_file" "$grafana_token_file"
token_temp_file=

printf '[influxdb-provision] Prometheus scraper and Grafana read token are ready\n'
