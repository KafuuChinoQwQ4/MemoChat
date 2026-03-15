# Local Runtime Assets

This folder holds the local Docker dependencies and init payloads used by MemoChat on Windows.

## Compose Files

- `compose/datastores.yml`
  - MySQL `3306`
  - Redis `6379`
  - PostgreSQL `5432`
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
- `compose/observability.yml`
  - Prometheus `9090`
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
docker compose -f deploy/local/compose/datastores.yml up -d
docker compose -f deploy/local/compose/kafka.yml up -d
docker compose -f deploy/local/compose/rabbitmq.yml up -d
docker compose -f deploy/local/compose/observability.yml up -d
```

If you need local call/invite or media-call debugging, also start:

```powershell
docker compose -f deploy/local/compose/livekit.yml up -d
```

If the machine has an NVIDIA GPU and Docker GPU passthrough is configured, add:

```powershell
docker compose -f deploy/local/compose/observability.yml --profile nvidia up -d
```

## Fresh Data Reset

All local Docker data is stored on `D:`:

- `D:\docker-data\memochat\mysql`
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
docker exec memochat-mysql mysqladmin -uroot -p123456 ping
docker exec memochat-redis redis-cli -a 123456 ping
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
docker exec memochat-mongo mongosh -u root -p 123456 --authenticationDatabase admin --quiet --eval "db.adminCommand({ ping: 1 })"
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
Invoke-WebRequest http://127.0.0.1:9182/metrics -UseBasicParsing | Select-Object -ExpandProperty StatusCode
Invoke-WebRequest http://127.0.0.1:9090/-/ready -UseBasicParsing | Select-Object -ExpandProperty StatusCode
Invoke-WebRequest http://127.0.0.1:3000/api/health -UseBasicParsing | Select-Object -ExpandProperty StatusCode
Invoke-WebRequest http://127.0.0.1:3100/ready -UseBasicParsing | Select-Object -ExpandProperty StatusCode
Invoke-WebRequest http://127.0.0.1:3200/ready -UseBasicParsing | Select-Object -ExpandProperty StatusCode
Invoke-WebRequest http://127.0.0.1:19100/metrics -UseBasicParsing | Select-Object -ExpandProperty StatusCode
Invoke-WebRequest http://127.0.0.1:8088/metrics -UseBasicParsing | Select-Object -ExpandProperty StatusCode
```

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
docker compose -f deploy/local/compose/rabbitmq.yml down
docker compose -f deploy/local/compose/kafka.yml down
docker compose -f deploy/local/compose/datastores.yml down
docker compose -f deploy/local/compose/observability.yml down
docker compose -f deploy/local/compose/livekit.yml down
```

## Notes

- `group_ops` and `history_ack` are stateful TCP scenarios. Do not run them in parallel against the same small local account pool.
- `windows_exporter` runs on the Windows host, not inside Docker. Use `scripts\windows\start_windows_exporter.ps1` before starting Prometheus.
- Grafana defaults to `admin/admin`. Dashboards are provisioned automatically under the `MemoChat` folder.
- The local chat cluster currently uses synchronous PostgreSQL persistence by default:
  - `chat_private_kafka_primary=false`
  - `chat_group_kafka_primary=false`
- Kafka and RabbitMQ should still be started locally because async side effects, outbox relay, and notification workers depend on them.
