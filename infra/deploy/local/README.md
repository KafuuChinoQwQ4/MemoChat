# Local Runtime Assets

This folder holds the local Docker dependencies and init payloads used by MemoChat on Windows.

## Compose Files

- `docker-compose.yml`
  - default local Docker architecture, including Nginx HTTP gateway `80`
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
- `compose/nginx-lb.yml`
  - containerized Gate/Chat cluster variant with Nginx LB `80` (HTTP), `8090-8094` and `8097` (TCP Stream)
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

```powershell
scripts\windows\start_windows_exporter.ps1
docker compose -f infra/deploy/local/docker-compose.yml up -d
```

If you need local call/invite or media-call debugging, also start:

```powershell
docker compose -f infra/deploy/local/compose/livekit.yml up -d
```

If the machine has an NVIDIA GPU and Docker GPU passthrough is configured, add:

```powershell
docker compose -f infra/deploy/local/compose/observability.yml --profile nvidia up -d
```

## Fresh Data Reset

All local Docker data is stored on `D:`:

- `D:\docker-data\memochat\redis`
- `D:\docker-data\memochat\postgres`
- `D:\docker-data\memochat\mongo`
- `D:\docker-data\memochat\observability\prometheus`
- `D:\docker-data\memochat\observability\grafana`
- `D:\docker-data\memochat\observability\loki`
- `D:\docker-data\memochat\observability\tempo`

To rebuild from a clean state:

1. Stop MemoChat services.
2. Stop the compose stacks.
3. Delete the matching `D:\docker-data\memochat\...` folder.
4. Start the compose stacks again.

The init scripts under `init/` will recreate the required schemas and seed objects.

## Connectivity Checks

```powershell
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
docker exec memochat-redis redis-cli -a 123456 ping
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
docker exec memochat-mongo mongosh -u root -p 123456 --authenticationDatabase admin --quiet --eval "db.adminCommand({ ping: 1 })"
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
Invoke-WebRequest http://127.0.0.1/health -UseBasicParsing | Select-Object -ExpandProperty StatusCode
Invoke-WebRequest http://127.0.0.1:9182/metrics -UseBasicParsing | Select-Object -ExpandProperty StatusCode
Invoke-WebRequest http://127.0.0.1:9090/-/ready -UseBasicParsing | Select-Object -ExpandProperty StatusCode
Invoke-WebRequest http://127.0.0.1:3000/api/health -UseBasicParsing | Select-Object -ExpandProperty StatusCode
Invoke-WebRequest http://127.0.0.1:3100/ready -UseBasicParsing | Select-Object -ExpandProperty StatusCode
Invoke-WebRequest http://127.0.0.1:3200/ready -UseBasicParsing | Select-Object -ExpandProperty StatusCode
Invoke-WebRequest http://127.0.0.1:19100/metrics -UseBasicParsing | Select-Object -ExpandProperty StatusCode
Invoke-WebRequest http://127.0.0.1:8088/metrics -UseBasicParsing | Select-Object -ExpandProperty StatusCode
```

## Nginx Gateway Check

The default local architecture includes Nginx as the client-facing HTTP entrypoint on stable host port `80`. It proxies to local Windows GateServer instances on `8080` and `8084` through a container-startup alias named `memochat-host-gateway-ipv4`. That alias is derived from Docker's `host-gateway` entry but keeps Nginx upstream resolution on IPv4 only, avoiding Docker Desktop host IPv6 retries. Existing direct GateServer smoke scripts still target `http://127.0.0.1:8080`; use the narrow gateway script when you specifically want to verify the Nginx route without changing the full runtime flow.

The local gateway only proxies recognized hostnames: `localhost`, `127.0.0.1`, and `host.docker.internal`. Unknown `Host` traffic is handled by the default Nginx server and is not proxied to GateServer. Host-facing traffic stays on port `80`; Nginx status `8081` and exporter `9113` remain internal Docker-network ports.

```powershell
tools\scripts\test_nginx_gateway.ps1
tools\scripts\test_nginx_gateway.ps1 -Login
tools\scripts\test_nginx_gateway.ps1 -GenerateIds -CheckDockerLogs
```

The default check probes `http://127.0.0.1/health`. The optional `-Login` check posts the same lightweight login payload style used by `tools\scripts\test_login.ps1` through `http://127.0.0.1/user_login`. If Nginx is not already running, the script reports the connection failure and exits nonzero; it does not start or stop Docker stacks.

For full auth smoke, prefer the runtime scripts:

```powershell
tools\scripts\test_login.ps1
tools\scripts\test_register_login.ps1
tools\scripts\full_flow_test.ps1
```

These scripts default to the Nginx entrypoint `http://127.0.0.1`, mirror the client-side XOR password encoding, and use unique email/user values for registration flows to avoid fixed-account collisions and `1005 UserExist` false failures.

### Nginx Policy Verification

Use these checks after changing `compose/nginx-local.conf` or when confirming the local gateway policy. They keep port `80` stable and do not start or stop the broader MemoChat service stack.

```powershell
docker compose -f infra\deploy\local\docker-compose.yml config --quiet
docker run --rm -v "${PWD}\infra\deploy\local\compose\nginx-local.conf:/etc/nginx/nginx.conf:ro" nginx:1.26.2-alpine nginx -t
docker compose -f infra\deploy\local\docker-compose.yml ps memochat-nginx-lb
tools\scripts\test_nginx_gateway.ps1
```

When Nginx is already running, the gateway script can also probe route-policy locations that are safe without real business state:

```powershell
tools\scripts\test_nginx_gateway.ps1 -ProbePolicyRoutes
tools\scripts\test_nginx_gateway.ps1 -ProbeResponseHeaders -ShowHeaders
```

`-ProbePolicyRoutes` exercises the auth, AI stream, media upload, and media download Nginx locations. It accepts normal upstream/business failures such as `400`, `401`, `404`, `405`, `429`, `500`, `502`, `503`, and `504`, so the probe remains useful even when GateServer is not running. It also checks Nginx-owned method restrictions that should return `405`: `GET /user_login`, `GET /ai/chat/stream`, and `POST /media/download`.

`-ProbeResponseHeaders` checks common gateway response headers from `/health`: `X-Request-Id`, `X-Content-Type-Options: nosniff`, `X-Frame-Options: SAMEORIGIN`, `Referrer-Policy: no-referrer`, and `Permissions-Policy: geolocation=(), microphone=(), camera=()`. It also keeps the AI stream response header check for `X-Accel-Buffering: no` on `/ai/chat/stream`.

To confirm unknown host traffic is rejected at the Nginx edge instead of proxied, use a manual probe and expect Nginx to close the request rather than return a GateServer response:

```powershell
curl.exe -i -H "Host: unknown.local" http://127.0.0.1/health
```

Auth-sensitive routes use the local Nginx baseline `limit_req_zone $binary_remote_addr zone=auth_per_ip:10m rate=30r/m` with `burst=10 nodelay` and `limit_req_status 429`. The protected exact routes are `/get_varifycode`, `/user_login`, `/user_register`, and `/reset_pwd`.

```powershell
tools\scripts\test_nginx_auth_limit.ps1
```

The auth-limit smoke is intentionally bursty. After running it from the same local IP, wait briefly before normal login/register/auth smoke so the Nginx per-IP bucket can drain.

### Nginx Metrics Check

Local Nginx metrics stay inside the Docker network. `memochat-nginx-lb` exposes the internal `stub_status` endpoint at `http://memochat-nginx-lb:8081/stub_status`, and `memochat-nginx-exporter` runs `nginx/nginx-prometheus-exporter:1.5.1` against that endpoint. The exporter listens on container port `9113` only; neither `8081` nor `9113` is published to the Windows host, so host-facing Nginx traffic remains on stable port `80`.

Prometheus scrapes the exporter through the `nginx_gateway` job at `memochat-nginx-exporter:9113`. Use the metrics smoke after changing Nginx, compose observability wiring, or Prometheus scrape configuration:

```powershell
tools\scripts\test_nginx_metrics.ps1
```

For manual Docker-network checks:

```powershell
docker run --rm --network memochat_default curlimages/curl:8.10.1 -fsS http://memochat-nginx-lb:8081/stub_status
docker run --rm --network memochat_default curlimages/curl:8.10.1 -fsS http://memochat-nginx-exporter:9113/metrics
Invoke-WebRequest "http://127.0.0.1:9090/api/v1/query?query=nginx_up" -UseBasicParsing
```

When updating local architecture docs or verification notes for any Nginx step, keep this metrics path in sync with the compose service name, exporter image tag, internal-only ports, Prometheus job name, and smoke script command.

The local Grafana instance at `http://127.0.0.1:3000` provisions a `MemoChat` folder with the `Nginx Gateway` dashboard from `infra/deploy/local/observability/grafana/dashboards/nginx-gateway.json`. It uses the existing Prometheus datasource uid `prometheus` and Loki datasource uid `loki`; no Docker published ports change for the dashboard. The panels cover exporter health, request throughput, connection state, route/status counts, 5xx log events, and recent access logs.

### Nginx Upstream Failover Check

The default local Nginx upstream sends client HTTP traffic on port `80` to two Windows GateServer instances, `memochat-host-gateway-ipv4:8080` and `memochat-host-gateway-ipv4:8084`. The upstream policy uses `least_conn`; each backend has `max_fails=3` and `fail_timeout=10s`.

Use the failover smoke script in its default diagnostic mode first. This mode is non-destructive: it checks the two GateServer ports, sends bounded probes through Nginx port `80`, and prints recent upstream evidence from the Nginx JSON logs. By default, the script treats gateway errors `502`, `503`, and `504` as failures; add `-AllowGatewayErrors` only when collecting diagnostic evidence while GateServer is intentionally unavailable.

```powershell
tools\scripts\test_nginx_failover.ps1
```

Only use fault injection when the local service windows can be interrupted:

```powershell
tools\scripts\test_nginx_failover.ps1 -StopBackend -StopBackendPort 8080
```

`-StopBackend` temporarily stops one local GateServer process so Nginx can route through the remaining backend. By default, the script restores only the stopped GateServer instance from `infra\Memo_ops\runtime\services\GateServer1` or `GateServer2`; pass `-RestoreScript tools/scripts/status/start-all-services.bat` only when you intentionally want the broader runtime startup script. If restore is disabled or fails, run that start script manually before continuing other smoke tests.

Confirm failover from `docker logs memochat-nginx-lb` by checking the JSON access log fields `upstream_addr`, `upstream_status`, and `upstream_response_time`. During a successful failover check, recent proxied requests should show the healthy backend address and acceptable upstream status/latency values after Nginx marks the stopped backend unavailable.

### Nginx Structured Log Correlation

Nginx writes JSON access logs to the container log stream, so `docker logs memochat-nginx-lb` remains available for quick local inspection. The same JSON access log stream is also written to `D:\docker-data\memochat\nginx\logs\access.json`; Docker mounts that file path into the OTel Collector, which tails it and exports Nginx gateway logs to Loki with low-cardinality labels such as `service="memochat-nginx-lb"` and `component="nginx_gateway"`.

Host-facing ports stay stable: Nginx HTTP is `80` and Loki is `3100`. Nginx status `8081` and exporter `9113` stay internal to the Docker network.

Use explicit IDs when you want to compare Nginx with GateServer logs, the mounted access log file, and Loki, or ask the smoke scripts to generate them:

```powershell
$requestId = "manual-request-$([guid]::NewGuid().ToString('N'))"
$traceId = "manual-trace-$([guid]::NewGuid().ToString('N'))"
tools\scripts\test_nginx_gateway.ps1 -RequestId $requestId -TraceId $traceId -CheckDockerLogs
docker logs memochat-nginx-lb --tail 50 | Select-String $requestId,$traceId
Get-Content D:\docker-data\memochat\nginx\logs\access.json -Tail 50 | Select-String $requestId
```

For a one-command generated trace:

```powershell
tools\scripts\test_nginx_gateway.ps1 -GenerateIds -CheckDockerLogs
tools\scripts\test_nginx_loki.ps1
```

`-CheckDockerLogs` only searches recent Nginx Docker logs for the request/trace IDs. Docker log access failures are reported as unavailable and do not make ordinary gateway health checks depend on Docker log collection.

The Nginx JSON body stores the normalized request path as `uri_path`; it does not log the raw request URI or query string. Query strings are represented only by `query_present=true|false`. The body also includes low-cardinality `route_family` and `status_class` fields for Loki filtering and dashboards. `request_id` and `trace_id` remain searchable in the JSON body for correlation, but they are intentionally not Loki labels.

`tools\scripts\test_nginx_loki.ps1` sends a Nginx-owned `GET /__nginx_loki_probe` probe through `http://127.0.0.1` with generated `X-Request-Id` and `X-Trace-Id` headers, checks the mounted access log file by default, then retries Loki until the request ID appears or the bounded wait expires. It also verifies `route_family` and `status_class` classification and sends a media-style probe with a query token to confirm raw query material is redacted. It does not require GateServer to be running. If you only want the Loki query path, add `-SkipAccessLogFileCheck`.

For manual Loki checks, use LogQL with the stable service label and a text filter for the request ID:

```powershell
$requestId = "manual-request-$([guid]::NewGuid().ToString('N'))"
$traceId = "manual-trace-$([guid]::NewGuid().ToString('N'))"
tools\scripts\test_nginx_loki.ps1 -RequestId $requestId -TraceId $traceId
$query = '{service="memochat-nginx-lb"} |= "' + $requestId + '"'
Invoke-RestMethod "http://127.0.0.1:3100/loki/api/v1/query_range?query=$([uri]::EscapeDataString($query))&limit=20" -UseBasicParsing
```

For manual one-off checks:

```powershell
Invoke-WebRequest http://127.0.0.1/health -UseBasicParsing | Select-Object StatusCode,Content
curl.exe -i -H "X-Request-Id: manual-nginx-check" -H "X-Trace-Id: manual-nginx-check" http://127.0.0.1/ai/chat/stream
curl.exe -i -H "X-Request-Id: manual-nginx-check" -H "X-Trace-Id: manual-nginx-check" http://127.0.0.1/media/download
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
