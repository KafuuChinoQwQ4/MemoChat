#!/bin/sh
set -eu

umask 077

readonly token_file=/run/secrets/memochat-influxdb/grafana-reader.token

[ -f "$token_file" ] && [ ! -L "$token_file" ] && [ -r "$token_file" ] || {
  printf '[grafana-entrypoint] InfluxDB reader token is missing or unsafe\n' >&2
  exit 1
}
[ "$(stat -c '%a' "$token_file")" = 600 ] || {
  printf '[grafana-entrypoint] InfluxDB reader token must have mode 0600\n' >&2
  exit 1
}
[ "$(stat -c '%u:%g' "$token_file")" = "$(id -u):$(id -g)" ] || {
  printf '[grafana-entrypoint] InfluxDB reader token has the wrong owner\n' >&2
  exit 1
}
[ "$(wc -l <"$token_file")" -eq 1 ] || {
  printf '[grafana-entrypoint] InfluxDB reader token must contain one line\n' >&2
  exit 1
}

IFS= read -r MEMOCHAT_INFLUXDB_GRAFANA_TOKEN <"$token_file"
[ "${#MEMOCHAT_INFLUXDB_GRAFANA_TOKEN}" -ge 32 ] || {
  printf '[grafana-entrypoint] InfluxDB reader token is invalid\n' >&2
  exit 1
}
case "$MEMOCHAT_INFLUXDB_GRAFANA_TOKEN" in
  *[!A-Za-z0-9._~+/=-]*)
    printf '[grafana-entrypoint] InfluxDB reader token contains invalid characters\n' >&2
    exit 1
    ;;
esac
export MEMOCHAT_INFLUXDB_GRAFANA_TOKEN

exec /run.sh
