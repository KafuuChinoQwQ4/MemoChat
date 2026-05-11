---
description: Diagnose MemoChat Docker services, fixed ports, container health, and MCP connectivity without changing runtime mappings.
---

# MemoChat Docker Diagnose

Use when containers fail to start, MCP cannot connect, ports conflict, health checks fail, or a service cannot reach Redis/Postgres/Mongo/Redpanda/RabbitMQ/MinIO/observability/AI dependencies.

## Rules

- Docker is the source of truth for infrastructure.
- Do not install local database or queue services outside Docker.
- Do not change published ports unless the user explicitly asks.
- Arch native Docker is the default runtime. Source `/root/.memochat-linux-env` so `DOCKER_HOST` is unset and Docker uses `/var/run/docker.sock`.
- Prefer `/data` for Linux caches/downloads and `/data/docker-data/memochat` for Docker bind data. Windows Docker Desktop data under `D:\docker-data` is legacy migration/backup data only.
- Use MCP tools when available; otherwise use `docker exec` and container logs.

## Baseline Inventory

Run:

```bash
docker ps --format "table {{.Names}}\t{{.Image}}\t{{.Status}}\t{{.Ports}}"
docker network ls
docker compose -f infra/deploy/local/docker-compose.yml ps
```

If Docker is unavailable in Arch, check `systemctl is-active docker`, then start it with `systemctl enable --now docker`. Do not switch to Docker Desktop unless the task is explicitly a legacy migration or backup check.

Expected fixed host ports:

- Redis `6379`
- Postgres `15432`
- Mongo `27017`
- MinIO `9000/9001`
- Redpanda `19092/18082`
- RabbitMQ `5672/15672`
- AI Orchestrator `8096`
- Ollama `11434`
- Neo4j `7474/7687`
- Qdrant `6333/6334`
- Grafana `3000`
- Prometheus `9090`
- InfluxDB `8086`
- Loki `3100`
- Tempo `3200`
- OTel `4317/4318/9411/9464`
- cAdvisor `8088`

## Health Checks

Use targeted checks:

```bash
docker exec memochat-redis redis-cli -a 123456 ping
docker exec memochat-postgres pg_isready -U memochat -d memo_pg
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select current_database(), current_user;"
docker exec memochat-mongo mongosh -u root -p 123456 --authenticationDatabase admin --quiet --eval "db.adminCommand({ ping: 1 })"
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
```

For HTTP services:

```bash
curl -fsS http://127.0.0.1:3000/api/health
curl -fsS http://127.0.0.1:9090/-/ready
curl -fsS http://127.0.0.1:3100/ready
curl -fsS http://127.0.0.1:3200/ready
curl -fsS http://127.0.0.1:6333/
curl -fsS http://127.0.0.1:7474/
curl -fsS http://127.0.0.1:11434/api/tags
```

## Logs And Root Cause

Use:

```bash
docker logs --tail 200 <container>
docker inspect <container>
```

Classify the failure:

- container not running
- unhealthy healthcheck
- port conflict
- bad credentials
- volume/data corruption
- missing Docker network
- service config points to host port from inside Docker
- MCP startup timeout or wrong host/port

## Safe Fixes

- Restart one container when logs show transient startup failure:
  `docker restart <container>`
- Recreate only the relevant compose service when config changed:
  `docker compose -f <compose-file> up -d <service>`
- For data reset, stop first and only delete the specific approved Docker volume or host data path.

## Report

Return:

- affected containers
- exact failing check
- root cause
- commands run
- safe next action
- whether any port/config drift was found
