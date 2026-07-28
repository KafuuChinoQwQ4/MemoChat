# Local Runtime Assets

This folder holds the local Docker dependencies and init payloads used by MemoChat on Arch Linux. Docker Desktop/Windows usage is legacy fallback only.

## Release-mode C++ backend

`compose/backend-services.yml` adds the 11 core and 2 optional supported C++
runtime targets to the base infrastructure stack. It is a release overlay, not
a development-default file: core credentials and credentials for each selected
profile are required, and public INI files are mounted read-only. HTTP plus the
client chat TCP/QUIC transports are published
only on `127.0.0.1` (`80`, `8443/tcp`, `8443/udp`, and `8190/udp`); other C++ business
ports stay private. Exposing any of them to a LAN or the Internet requires an
explicit operator-owned override and TLS policy.

The overlay shares the Envoy network namespace with the C++ services. This
keeps the existing Envoy upstream ports stable while resolving
`host.docker.internal` to the private loopback inside that namespace. Databases
and queues remain reachable by their service names on `memochat_default`.

### 1. Build images

Use the server-only release preset and the fresh bundle workflow documented in
[`../images/README.md`](../images/README.md). Each image is built from one
`<TARGET>/{bin,lib,MANIFEST.txt,SHA256SUMS}` directory. Run
`tools/scripts/release/verify_release_tree.sh` before any Docker build.

### 2. Provision private runtime values

Create a private mode-0600 env file from `.env.release.example`. Replace every
base `REPLACE_WITH_*` value and every value belonging to a profile you enable;
inactive profile placeholders may remain or be removed. Never pass the example
directly to Compose or bypass the preflight script for `config`/`up`. Generate
independent random values, for example:

```bash
openssl rand -hex 32
```

Generate `MEMOCHAT_AUTH_REFRESH_PEPPER` independently from the JWT and chat
HMAC secrets. Production refresh-token hashing requires at least 32 bytes; the
hex command above produces a 64-character value representing 32 random bytes.
Generate `MEMOCHAT_RELATION_COMMAND_AUTH_TOKEN`,
`MEMOCHAT_RELATION_CHAT_QUERY_AUTH_TOKEN`, `MEMOCHAT_RELATION_CALL_AUTH_TOKEN`,
and `MEMOCHAT_RELATION_MOMENTS_AUTH_TOKEN` independently as well. Each token
must contain at least 32 printable ASCII bytes. The relation services use these
role-specific values to prevent a query consumer from issuing write commands or
one backend domain from impersonating another; never reuse one token for more
than one role.

`MEMOCHAT_BACKEND_CONFIG_ROOT` is the absolute path to
`apps/server/core`. `MEMOCHAT_TLS_CERT_FILE` and `MEMOCHAT_TLS_KEY_FILE` are
absolute host paths to the ChatServer certificate and private key. The key is a
read-only runtime mount and is not part of an image. Keep the private key owned
by the deployment user with mode `0400` or `0600`; configuration and certificate
mounts must be regular files and must not be group- or world-writable.

The desktop package never contains the TLS private key. When the local Envoy
certificate is signed by a private CA, pass that public CA explicitly while
packaging the client:

```bash
tools/scripts/release/package_linux_client.sh \
  --binary build-linux-client-release-gcc16/bin/MemoChatQml \
  --config build-linux-client-release-gcc16/bin/config.ini \
  --ca-cert /absolute/path/to/local-ca.crt \
  --output-dir artifacts/release
```

The packager validates a single PEM X.509 certificate and writes its relative
path into the packaged `config.ini`. Without `--ca-cert`, the client uses the
system trust store; `MEMOCHAT_CLIENT_CA_FILE` can override the public CA path at
launch without rebuilding the package.

Create the writable Media upload, Envoy log, and Redpanda directories before
`up`. Create the R18 directory only when enabling `--profile r18`, and create
the seven observability runtime directories only when enabling
`--profile observability`. Compose deliberately refuses to create these paths
as root. Every directory must be owned by
`MEMOCHAT_RUNTIME_UID:MEMOCHAT_RUNTIME_GID` with mode
`0700`; they are mutable runtime state, not release content:

```bash
install -d -m 0700 -o 10001 -g 10001 \
  /data/docker-data/memochat/media/uploads \
  /data/docker-data/memochat/envoy/logs \
  /data/docker-data/memochat/redpanda/data
install -d -m 0700 -o 10001 -g 10001 \
  /data/docker-data/memochat/r18  # only with --profile r18
# Only with --profile observability.
install -d -m 0700 -o 10001 -g 10001 \
  /data/docker-data/memochat/observability/{credentials,influxdb,grafana,prometheus,alertmanager,loki,tempo}
```

Use the configured `MEMOCHAT_DOCKER_DATA_ROOT` instead of `/data/docker-data/memochat`
when the private env file selects another data root. The container entrypoint
uses `umask 077`; credential and upload/session files default to owner-only.

The release wrapper provisions Postgres schemas, isolated least-privilege
databases/roles, the Mongo application user/collections, MinIO buckets, and the
MinIO media account. With `--profile observability`, it also creates or updates
an InfluxDB scraper that persists the current Prometheus federation into the
`metrics` bucket. Before provisioning, the wrapper atomically materializes the
Influx username, password, and administrator Token as runtime-owned mode `0400`
files under `observability/credentials`. InfluxDB uses its native `_FILE`
inputs, and the short-lived provision job receives only the administrator token
file path; these credentials are absent from `docker inspect` container
environment output and command arguments. Keep both the private env file and
runtime credential directory owner-only, restrict Docker socket access, and
rotate the files and source values after suspected host exposure. The job
creates or reuses one authorization with only read access to the
`metrics` bucket and atomically writes that token as mode `0600` under
`observability/credentials`; Grafana reads the file at startup and exports the
scoped value to its own process environment. The wrapper compares token hashes
without printing them and recreates only an already-running Grafana container
when provisioning rotates this token.
The jobs are idempotent and use only values from the validated private env file.
They also migrate legacy main-database rows into an empty split database and
synchronize identity sequences. Run provisioning explicitly when preparing a
host:

```bash
RELEASE_ENV=/absolute/private/path/memochat-release.env
tools/scripts/release/run_release_compose.sh \
  --env-file "${RELEASE_ENV}" provision
```

`up` runs the same provisioning step before it starts business services. Mongo
and MinIO application credentials are generated independently from their root
credentials and injected only at runtime.

### 3. Validate and start

From the repository root, with the private env file outside source control:

```bash
RELEASE_ENV=/absolute/private/path/memochat-release.env
tools/scripts/release/run_release_compose.sh --env-file "${RELEASE_ENV}" check
tools/scripts/release/run_release_compose.sh --env-file "${RELEASE_ENV}" config
tools/scripts/release/run_release_compose.sh --env-file "${RELEASE_ENV}" up
```

`run_release_compose.sh` is the only supported release validation/start entry.
Before Docker is invoked it requires the private env file to be a current-user-
owned regular file with mode `0600`; rejects unknown/duplicate variables,
interpolation, command syntax, placeholders, common weak values, short or reused
secrets; and checks the 11 core INI mounts plus selected profile mounts, the TLS
certificate, and private key.
It also clears all ambient `MEMOCHAT_*` variables so an exported shell value
cannot override or extend the values that were validated from the private file. The
`check` action performs only these local checks and does not contact Docker;
`config` additionally executes Compose's fixed base, release-overlay, and
LiveKit `config --quiet` check.

The default profile starts the core messaging services without requiring
credentials or mutable paths for optional service groups. Add `--profile calls`
for CallGateway, its Postgres database, and local LiveKit, or `--profile r18`
for the R18 gateway. Add `--profile observability` for the InfluxDB/Grafana
credentials and observability services. Each selected profile is validated
fail-closed before Docker runs. This
C++ backend release does not include AIOrchestrator, Ollama, Qdrant, or Neo4j,
so `AIGatewayServer` and `AIServer` are intentionally absent from the release
Compose model and the wrapper rejects an `ai` profile instead of starting a
known-incomplete stack.

The bundled LiveKit profile is loopback-only and intended for one-host local
deployment. Internet-facing calls require a separately reviewed LiveKit
deployment with trusted TLS, public IP/firewall configuration, and TURN where
the target network requires it; do not publish the local profile directly.

The release overlay runs InfluxDB, Grafana, Prometheus, Alertmanager, Loki,
Tempo, and OTel Collector as
`MEMOCHAT_RUNTIME_UID:MEMOCHAT_RUNTIME_GID` with read-only root
filesystems, all Linux capabilities dropped, `no-new-privileges`, explicit
health checks, and only their preflight-owned runtime directories writable. OTel
Collector can read only its configuration and the Envoy log directory; it does
not mount source or build trees. Its distroless-compatible health check loads the
live internal `/healthz` response through the collector's own HTTP config
provider, rather than merely validating the static file. InfluxDB writes its CLI
setup file to a tmpfs; its administrator token remains only in the owner-only
runtime credential file, not under `/etc/influxdb2` or in container metadata.
Grafana receives only the generated bucket-scoped read token, never the Influx
administrator token.

cAdvisor is the documented exception: Docker/container discovery requires it
to remain root and privileged with `/dev/kmsg` plus read-only host namespace
mounts. Its own root filesystem is still read-only and
`no-new-privileges` remains enabled. cAdvisor and all other observability
services are disabled by default; add `--profile observability` only when the
operator accepts that host-level visibility:

```bash
tools/scripts/release/run_release_compose.sh --env-file "${RELEASE_ENV}" \
  --profile observability up
```

### 4. Health checks and stop

```bash
RELEASE_PROFILE_ARGS=()
# Add exactly the profiles used for the corresponding `up` command:
# RELEASE_PROFILE_ARGS+=(--profile calls)
# RELEASE_PROFILE_ARGS+=(--profile r18)
# RELEASE_PROFILE_ARGS+=(--profile observability)

docker compose --env-file "${RELEASE_ENV}" \
  -f infra/deploy/local/docker-compose.yml \
  -f infra/deploy/local/compose/backend-services.yml \
  -f infra/deploy/local/compose/livekit.yml \
  "${RELEASE_PROFILE_ARGS[@]}" ps

docker compose --env-file "${RELEASE_ENV}" \
  -f infra/deploy/local/docker-compose.yml \
  -f infra/deploy/local/compose/backend-services.yml \
  -f infra/deploy/local/compose/livekit.yml \
  "${RELEASE_PROFILE_ARGS[@]}" \
  exec memochat-envoy-gateway sh -ec \
  'wget -qO- http://127.0.0.1:8101/healthz && wget -qO- http://127.0.0.1:8102/healthz'

curl -fsS http://127.0.0.1/health

docker compose --env-file "${RELEASE_ENV}" \
  -f infra/deploy/local/docker-compose.yml \
  -f infra/deploy/local/compose/backend-services.yml \
  -f infra/deploy/local/compose/livekit.yml \
  "${RELEASE_PROFILE_ARGS[@]}" \
  down
```

Use the same profile list that was passed to the wrapper. An empty array is the
base-only deployment; omitting a previously enabled profile can leave its
optional containers outside the intended status or shutdown operation.

Registration email, password hashes, chat records, uploaded objects, and all
other account data live in the configured database/object-store volumes under
`MEMOCHAT_DOCKER_DATA_ROOT`; they are never copied into the C++ images. The
email address in the desktop settings is likewise runtime user state, not a
release artifact.

## Compose Files

- `docker-compose.yml`
  - default local Docker architecture, including Envoy gateway `80` and `8443`
- `compose/datastores.yml`
  - Redis `6379`
  - PostgreSQL `15432`
  - MongoDB `27017`
- `compose/kafka.yml`
  - Redpanda Kafka-compatible broker `19092`
  - Redpanda admin proxy `18082`
- `compose/rabbitmq.yml`
  - RabbitMQ AMQP `5672`
  - RabbitMQ management `15672`
- `compose/livekit.yml`
  - loopback-only LiveKit signaling `7880`, RTC/TCP `7881`, and RTC/UDP `7882`
  - enabled only by the `calls` profile; no bundled public TURN service
- `compose/envoy-lb.yml`
  - containerized Gate/Chat cluster variant with Envoy LB `80` (HTTP), `8090-8091` (TCP Stream)
  - do not use this fragment with the default Windows-process local runtime because ChatServer ports would collide
- `compose/observability.yml`
  - standalone observability-only stack; do not combine it with `docker-compose.yml`
  - fails closed unless the InfluxDB and Grafana credentials are supplied by a private env file
  - Prometheus `9090` (短期存储)
  - InfluxDB `8086` (长期存储)
  - Grafana `3000`
  - Loki `3100`
  - Tempo `3200`
  - OTel Collector metrics `9464`
  - cAdvisor `8088`
  - optional NVIDIA DCGM exporter `9400` via `--profile nvidia`

## Recommended Startup Order

From the repository root:

```bash
source /root/.memochat-linux-env
LOCAL_ENV=/absolute/private/path/memochat-local.env
docker compose --env-file "${LOCAL_ENV}" \
  -f infra/deploy/local/docker-compose.yml up -d
```

The private local env file must contain the datastore and observability values
referenced by `docker-compose.yml`; do not use `.env.release.example` directly.
The supported release path remains `run_release_compose.sh`, which additionally
validates ownership, permissions, secret strength, and enabled profiles.

If you need local call/invite or media-call debugging, also start:

```bash
docker compose --env-file "${LOCAL_ENV}" \
  -f infra/deploy/local/docker-compose.yml \
  -f infra/deploy/local/compose/livekit.yml \
  --profile calls up -d
```

`compose/observability.yml` is a standalone alternative for observability-only
debugging. It requires explicit InfluxDB and Grafana credentials, including an
already-provisioned bucket-scoped `MEMOCHAT_INFLUXDB_GRAFANA_TOKEN`; never reuse
the Influx administrator token for Grafana. Do not layer this file onto the main
compose file because both define the same containers. Start it on its own, and
add `--profile nvidia` only when Docker GPU passthrough is configured:

```bash
docker compose --env-file "${LOCAL_ENV}" \
  -f infra/deploy/local/compose/observability.yml up -d
# NVIDIA hosts only:
docker compose --env-file "${LOCAL_ENV}" \
  -f infra/deploy/local/compose/observability.yml \
  --profile nvidia up -d
```

## Fresh Data Reset

All local Docker data is stored under `${MEMOCHAT_DOCKER_DATA_ROOT:-/data/docker-data/memochat}`:

- `/data/docker-data/memochat/redis`
- `/data/docker-data/memochat/postgres`
- `/data/docker-data/memochat/mongo`
- `/data/docker-data/memochat/minio`
- `/data/docker-data/memochat/redpanda`
- `/data/docker-data/memochat/rabbitmq`
- `/data/docker-data/memochat/observability/prometheus`
- `/data/docker-data/memochat/observability/alertmanager`
- `/data/docker-data/memochat/observability/credentials`
- `/data/docker-data/memochat/observability/influxdb`
- `/data/docker-data/memochat/observability/grafana`
- `/data/docker-data/memochat/observability/loki`
- `/data/docker-data/memochat/observability/tempo`

To rebuild from a clean state:

1. Stop MemoChat services.
2. Stop the compose stacks.
3. Delete the matching `/data/docker-data/memochat/...` folder.
4. Start the compose stacks again.

The init scripts under `init/` will recreate the required schemas and seed objects.

## Connectivity Checks

```bash
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
docker exec memochat-redis redis-cli -a "${MEMOCHAT_REDIS_PASSWORD:?}" ping
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
docker exec memochat-mongo mongosh -u "${MEMOCHAT_MONGO_ROOT_USERNAME:?}" \
  -p "${MEMOCHAT_MONGO_ROOT_PASSWORD:?}" --authenticationDatabase admin \
  --quiet --eval "db.adminCommand({ ping: 1 })"
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
curl -fsS http://127.0.0.1/health
curl -fsS http://127.0.0.1:9090/-/ready
curl -fsS http://127.0.0.1:3000/api/health
curl -fsS http://127.0.0.1:3100/ready
curl -fsS http://127.0.0.1:3200/ready
curl -fsS http://127.0.0.1:8088/metrics
```

## Envoy Gateway Check

The default local architecture now uses `memochat-envoy-gateway` for both the HTTP edge on host port `80` and the HTTPS/HTTP3 edge on host port `8443`. It proxies to the host GateServer instances on `8080` and `8084`, keeps Envoy admin on `127.0.0.1:9901` inside the Envoy container, and writes JSON access logs to the Envoy log mount under `/data/docker-data/memochat/envoy/logs`.

Validate the local gateway wiring after changing `infra/deploy/local/docker-compose.yml` or `infra/deploy/local/compose/envoy.yaml`:

```powershell
docker compose -f infra/deploy/local/docker-compose.yml config
docker compose -f infra/deploy/local/docker-compose.yml ps memochat-envoy-gateway
curl -fsS http://127.0.0.1/health
curl.exe -k --http2 https://127.0.0.1:8443/health
```

When HTTP/3-capable curl is available, the same health route should answer over QUIC as well:

```powershell
curl.exe -k --http3-only https://127.0.0.1:8443/health
```

## Gateway Policy Verification

Use these checks after changing `compose/envoy.yaml` or the Envoy service in `docker-compose.yml`. They keep host ports `80` and `8443` stable and do not start or stop the broader MemoChat service stack.

```powershell
docker compose -f infra\deploy\local\docker-compose.yml config --quiet
docker compose -f infra\deploy\local\docker-compose.yml ps memochat-envoy-gateway
tools\scripts\test_envoy_gateway.ps1
tools\scripts\test_envoy_gateway.ps1 -ProbePolicyRoutes
tools\scripts\test_envoy_gateway.ps1 -ProbeResponseHeaders -ShowHeaders
```

`-ProbePolicyRoutes` exercises the auth, AI stream, media upload, and media download Envoy routes. It accepts normal upstream/business failures such as `400`, `401`, `404`, `405`, `429`, `500`, `502`, `503`, and `504`, so the probe remains useful even when GateServer is not running. It also checks gateway-owned method restrictions that should return `405`: `GET /user_login`, `GET /ai/chat/stream`, and `POST /media/download`.

Unknown host traffic is rejected by Envoy with `421` instead of being proxied:

```powershell
curl.exe -i -H "Host: unknown.local" http://127.0.0.1/health
```

Auth-sensitive routes use Envoy local rate limiting. The protected exact routes are `/get_varifycode`, `/user_login`, `/user_register`, and `/reset_pwd`.

```powershell
tools\scripts\test_envoy_auth_limit.ps1
```

The auth-limit smoke is intentionally bursty. Wait briefly after running it from the same local IP before normal login/register/auth smoke.

For full auth smoke, prefer the runtime scripts:

```powershell
tools\scripts\test_login.ps1
tools\scripts\test_register_login.ps1
tools\scripts\full_flow_test.ps1
```

## Envoy Metrics Check

Envoy admin stays bound to loopback inside the Envoy container. It is not published on the host, and the default Prometheus config does not scrape `memochat-envoy-gateway:9901` from the shared Docker network. Use container-local probes for admin readiness or stats; add a dedicated authenticated sidecar/exporter before enabling scrape-based metrics.

```powershell
tools\scripts\test_envoy_metrics.ps1
```

For manual checks:

```powershell
docker exec memochat-envoy-gateway sh -ec "wget -qO- http://127.0.0.1:9901/ready"
docker exec memochat-envoy-gateway sh -ec "wget -qO- http://127.0.0.1:9901/stats/prometheus | head"
```

## Envoy Upstream Routing Check

The default Linux local Envoy edge routes each business prefix directly to its service backend (`login_backend`, `mediagateway_backend`, `momentsgateway_backend`, and so on). There is no local `gate_backend` catch-all; unknown paths return Envoy `404`.

The old PowerShell failover script below is for the legacy Windows GateServer topology only.

```powershell
tools\scripts\test_envoy_failover.ps1
```

Only use fault injection when the local service windows can be interrupted:

```powershell
tools\scripts\test_envoy_failover.ps1 -StopBackend -StopBackendPort 8080
```

`-StopBackend` temporarily stops one local GateServer process in that legacy topology so Envoy can route through the remaining backend. By default, the script restores only the stopped GateServer instance from `infra\Memo_ops\runtime\services\GateServer1` or `GateServer2`; pass `-RestoreScript tools/scripts/status/start-all-services.bat` only when you intentionally want the broader runtime startup script.

## Envoy Structured Log Correlation

Envoy writes JSON access logs to `/var/log/envoy/access.json` in the container. The same file is mounted from `/data/docker-data/memochat/envoy/logs/access.json`; the OTel Collector tails that mount and exports gateway logs to Loki with `service="memochat-envoy-gateway"` and `component="envoy_gateway"`.

Use explicit IDs when you want to compare Envoy, GateServer logs, the mounted access log file, and Loki:

```powershell
$requestId = "manual-request-$([guid]::NewGuid().ToString('N'))"
$traceId = "manual-trace-$([guid]::NewGuid().ToString('N'))"
tools\scripts\test_envoy_gateway.ps1 -RequestId $requestId -TraceId $traceId -CheckDockerLogs
tools\scripts\docker\arch-docker.cmd exec memochat-envoy-gateway sh -c "tail -n 50 /var/log/envoy/access.json" | Select-String $requestId,$traceId
Get-Content \\wsl.localhost\archlinux\data\docker-data\memochat\envoy\logs\access.json -Tail 50 | Select-String $requestId
```

For a one-command generated trace:

```powershell
tools\scripts\test_envoy_gateway.ps1 -GenerateIds -CheckDockerLogs
tools\scripts\test_envoy_loki.ps1
```

The Envoy JSON body stores the normalized request path as `uri_path`; it does not log the raw request URI or query string. Query strings are represented only by `query_present=true|false`. The body also includes low-cardinality `route_family` and `status_class` fields for Loki filtering and dashboards. `request_id` and `trace_id` remain searchable in the JSON body but are intentionally not Loki labels.

For manual Loki checks, use LogQL with the stable service label and a text filter for the request ID:

```powershell
$requestId = "manual-request-$([guid]::NewGuid().ToString('N'))"
$traceId = "manual-trace-$([guid]::NewGuid().ToString('N'))"
tools\scripts\test_envoy_loki.ps1 -RequestId $requestId -TraceId $traceId
$query = '{service="memochat-envoy-gateway"} |= "' + $requestId + '"'
Invoke-RestMethod "http://127.0.0.1:3100/loki/api/v1/query_range?query=$([uri]::EscapeDataString($query))&limit=20" -UseBasicParsing
```

For manual one-off checks:

```powershell
Invoke-WebRequest http://127.0.0.1/health -UseBasicParsing | Select-Object StatusCode,Content
curl.exe -i -H "X-Request-Id: manual-envoy-check" -H "X-Trace-Id: manual-envoy-check" http://127.0.0.1/ai/chat/stream
curl.exe -i -H "X-Request-Id: manual-envoy-check" -H "X-Trace-Id: manual-envoy-check" http://127.0.0.1/media/download
```

The common forwarding headers (`X-Request-Id`, `X-Trace-Id`, `X-Real-IP`, `X-Forwarded-For`, `X-Forwarded-Proto`) are set on proxied requests and are best confirmed from GateServer access logs or request logging when the upstream service is running.

## Starting MemoChat Services

After the Docker dependencies are healthy:

```powershell
scripts\windows\start_test_services.bat --no-client --skip-ops
```

If you also want the local ops platform:

```powershell
scripts\windows\start_test_services.bat --no-client
```

The startup script will:

- refresh runtime service folders under `Memo_ops\runtime\services`
- copy the latest server binaries and configs
- stop stale local processes first
- warn if RabbitMQ, Kafka, or Zipkin are missing

## Stopping Everything

Stop MemoChat services first:

```powershell
scripts\windows\stop_test_services.bat
```

Then stop Docker dependencies:

```bash
LOCAL_PROFILE_ARGS=()
# Use this when the calls profile was started:
# LOCAL_PROFILE_ARGS+=(--profile calls)
docker compose --env-file "${LOCAL_ENV}" \
  -f infra/deploy/local/docker-compose.yml \
  -f infra/deploy/local/compose/livekit.yml \
  "${LOCAL_PROFILE_ARGS[@]}" down
```

If the standalone observability stack was started, stop it with the same
`--env-file` and optional `--profile nvidia` arguments used for startup.

## Notes

- `group_ops` and `history_ack` are stateful TCP scenarios. Do not run them in parallel against the same small local account pool.
- `windows_exporter` runs on the Windows host, not inside Docker. Use `scripts\windows\start_windows_exporter.ps1` before starting Prometheus.
- Grafana has no committed default login; provide private
  `MEMOCHAT_GRAFANA_ADMIN_USER` and `MEMOCHAT_GRAFANA_ADMIN_PASSWORD` values.
  Dashboards are provisioned automatically under the `MemoChat` folder.
- The local chat cluster currently uses synchronous PostgreSQL persistence by default:
  - `chat_private_kafka_primary=false`
  - `chat_group_kafka_primary=false`
- Kafka and RabbitMQ should still be started locally because async side effects, outbox relay, and notification workers depend on them.
