# provision

Release-only, idempotent datastore provisioning assets. These scripts run in
short-lived `docker compose run --rm` containers before business services start.

| File | Responsibility |
|---|---|
| `postgres.sh` | Apply migrations, provision isolated least-privilege databases/roles, conditionally provision the calls database, run the one-time split migration, and synchronize identity sequences. |
| `postgres_legacy_split.sh` | Copy legacy main-database tables once per destination table, atomically persist anti-resurrection markers, and skip call data unless the calls profile is selected. |
| `mongo.sh` / `mongo.js` | Provision or rotate the MongoDB application user and required collections without hard-coded credentials. |
| `minio.sh` | Provision required buckets and create or rotate the scoped media service account. |
| `minio-media-policy.json` | Bucket-scoped policy attached to the media service account. |
| `influxdb_observability.sh` | Idempotently create or update the InfluxDB scraper that persists Prometheus federation metrics without exposing its token in command arguments. |
| `grafana_entrypoint.sh` | Read the generated bucket-scoped InfluxDB token from the owner-only runtime secret and inject it into Grafana without exposing the InfluxDB administrator token. |
