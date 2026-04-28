---
description: Investigate MemoChat metrics, logs, traces, dashboards, and container resource usage through Prometheus, Loki, Tempo, Grafana, InfluxDB, cAdvisor, and Docker.
---

# MemoChat Observability

Use when diagnosing latency, errors, missing telemetry, service health, dashboards, logs, traces, or resource usage.

## Data Sources

Prefer MCP tools when available:

- Prometheus: `prometheus_query`, `prometheus_targets`
- Loki: `loki_query`
- Tempo: `tempo_services`, `tempo_search_traces`
- Grafana: `grafana_datasources`, `grafana_search_dashboards`
- InfluxDB: `influx_query`
- cAdvisor: `cadvisor_metrics`

Fallback HTTP endpoints:

```powershell
Invoke-WebRequest http://127.0.0.1:9090/-/ready -UseBasicParsing
Invoke-WebRequest http://127.0.0.1:3100/ready -UseBasicParsing
Invoke-WebRequest http://127.0.0.1:3200/ready -UseBasicParsing
Invoke-WebRequest http://127.0.0.1:3000/api/health -UseBasicParsing
Invoke-WebRequest http://127.0.0.1:8088/metrics -UseBasicParsing
```

## Investigation Flow

1. Identify the service and time window.
2. Check Docker status and restart history.
3. Check Prometheus targets for scrape failures.
4. Query Loki for service errors.
5. Search Tempo for traces by service or endpoint.
6. Inspect cAdvisor/container CPU/memory when latency or crashes are involved.
7. Check Grafana datasource/dashboard provisioning only if dashboards are wrong.

## Useful Query Patterns

Prometheus:

```text
up
rate(http_server_duration_count[5m])
process_resident_memory_bytes
container_cpu_usage_seconds_total
```

Loki:

```text
{service="GateServer"} |= "error"
{service="ChatServer"} |= "warn"
{service=~"GateServer|ChatServer|StatusServer|VarifyServer"}
```

Tempo:

- search by service name first
- then narrow by route, status, or trace id from logs

## Code/Config Areas

Check:

- service `config.ini` telemetry sections
- `apps/server/core/common/logging`
- `apps/server/core/common/observability` if present
- `infra/deploy/local/observability`
- `infra/deploy/local/docker-compose.yml`
- `infra/Memo_ops/runtime/services/*/config.ini`

## Report

Include:

- exact time window
- datasource queried
- query strings
- key log/metric/trace findings
- likely root cause
- recommended code/config/runtime fix
