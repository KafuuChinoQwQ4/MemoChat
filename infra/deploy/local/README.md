# Local Runtime Assets

This folder holds the local Docker dependencies and init payloads used by MemoChat on Arch Linux. Docker Desktop/Windows usage is legacy fallback only.

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
  - LiveKit `7880`
  - TURN `3478`
- `compose/envoy-lb.yml`
  - containerized Gate/Chat cluster variant with Envoy LB `80` (HTTP), `8090-8091` (TCP Stream)
  - do not use this fragment with the default Windows-process local runtime because ChatServer ports would collide
- `compose/observability.yml`
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
docker compose -f infra/deploy/local/docker-compose.yml up -d
```

If you need local call/invite or media-call debugging, also start:

```bash
docker compose -f infra/deploy/local/compose/livekit.yml up -d
```

If the machine has an NVIDIA GPU and Docker GPU passthrough is configured, add:

```bash
docker compose -f infra/deploy/local/compose/observability.yml --profile nvidia up -d
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
docker exec memochat-redis redis-cli -a 123456 ping
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
docker exec memochat-mongo mongosh -u root -p 123456 --authenticationDatabase admin --quiet --eval "db.adminCommand({ ping: 1 })"
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

The default local architecture now uses `memochat-envoy-gateway` for both the HTTP edge on host port `80` and the HTTPS/HTTP3 edge on host port `8443`. It proxies to the host GateServer instances on `8080` and `8084`, publishes Envoy admin stats on container port `9901`, and writes JSON access logs to the Envoy log mount under `/data/docker-data/memochat/envoy/logs`.

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

Envoy admin and Prometheus stats stay inside the Docker network. `memochat-envoy-gateway` exposes admin port `9901` in the container only, and Prometheus scrapes `/stats/prometheus` through the `envoy_gateway` job.

```powershell
tools\scripts\test_envoy_metrics.ps1
```

For manual checks:

```powershell
docker run --rm --network memochat_default curlimages/curl:8.10.1 -fsS http://memochat-envoy-gateway:9901/ready
docker run --rm --network memochat_default curlimages/curl:8.10.1 -fsS http://memochat-envoy-gateway:9901/stats/prometheus
Invoke-WebRequest "http://127.0.0.1:9090/api/v1/query?query=envoy_server_live%7Bjob%3D%22envoy_gateway%22%7D" -UseBasicParsing
```

## Envoy Upstream Failover Check

The default local Envoy upstream sends client HTTP traffic to host GateServer instances on `8080` and `8084` using the `gate_backend` cluster. The non-destructive failover script checks those ports, sends bounded probes through port `80`, and prints recent upstream evidence from `/var/log/envoy/access.json` inside the container.

```powershell
tools\scripts\test_envoy_failover.ps1
```

Only use fault injection when the local service windows can be interrupted:

```powershell
tools\scripts\test_envoy_failover.ps1 -StopBackend -StopBackendPort 8080
```

`-StopBackend` temporarily stops one local GateServer process so Envoy can route through the remaining backend. By default, the script restores only the stopped GateServer instance from `infra\Memo_ops\runtime\services\GateServer1` or `GateServer2`; pass `-RestoreScript tools/scripts/status/start-all-services.bat` only when you intentionally want the broader runtime startup script.

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

```powershell
docker compose -f infra/deploy/local/docker-compose.yml down
docker compose -f infra/deploy/local/compose/livekit.yml down
```

## Notes

- `group_ops` and `history_ack` are stateful TCP scenarios. Do not run them in parallel against the same small local account pool.
- `windows_exporter` runs on the Windows host, not inside Docker. Use `scripts\windows\start_windows_exporter.ps1` before starting Prometheus.
- Grafana defaults to `admin/admin`. Dashboards are provisioned automatically under the `MemoChat` folder.
- The local chat cluster currently uses synchronous PostgreSQL persistence by default:
  - `chat_private_kafka_primary=false`
  - `chat_group_kafka_primary=false`
- Kafka and RabbitMQ should still be started locally because async side effects, outbox relay, and notification workers depend on them.
