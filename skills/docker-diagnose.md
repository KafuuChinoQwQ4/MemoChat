---
description: 诊断 MemoChat Docker 服务、固定端口、容器健康状态和 MCP 连通性，同时不改变运行时映射。
---

# MemoChat Docker 诊断

当容器启动失败、MCP 无法连接、端口冲突、健康检查失败，或某个服务无法连接 Redis/Postgres/Mongo/Redpanda/RabbitMQ/MinIO/观测/AI 依赖时使用。

## 规则

- Docker 是基础设施的事实来源。
- 不要在 Docker 外安装本地数据库或队列服务。
- 除非用户明确要求，否则不要修改发布端口。
- WSL 发行版名是 `archlinux`；Arch 原生 Docker 是默认运行时。加载 `/root/.memochat-linux-env`，让 `DOCKER_HOST` 取消设置，并使 Docker 使用 `/var/run/docker.sock`。
- 从 Windows/PowerShell 进入默认 Linux 环境时使用：

```powershell
wsl -d archlinux -- bash -lc 'cd /root/code/MemoChat-Qml-Drogon-linux && source /root/.memochat-linux-env && <command>'
```

- Linux 缓存/下载优先使用 `/data`，Docker 绑定数据使用 `/data/docker-data/memochat`。`D:\docker-data` 下的 Windows Docker Desktop 数据只作为旧版迁移/备份数据。
- 可用时使用 MCP 工具；否则使用 `docker exec` 和容器日志。

## 基线清点

运行：

```bash
docker ps --format "table {{.Names}}\t{{.Image}}\t{{.Status}}\t{{.Ports}}"
docker network ls
docker compose -f infra/deploy/local/docker-compose.yml ps
```

如果 `archlinux` 中 Docker 不可用，检查 `systemctl is-active docker`，然后用 `systemctl enable --now docker` 启动。除非任务明确是旧版迁移或备份检查，否则不要切换到 Docker Desktop。

预期固定主机端口：

- Envoy Gateway：`memochat-envoy-gateway`，端口 `80`、`8443/tcp`、`8443/udp`
- Redis `6379`
- Postgres `15432`
- Mongo `27017`
- MinIO `9000/9001`
- Redpanda `19092/18082`
- RabbitMQ `5672/15672`
- AI Orchestrator `8096`
- Ollama `11434`，仅当本地启用 Ollama 时要求存在
- Neo4j `7474/7687`
- Qdrant `6333/6334`
- Grafana `3000`
- Prometheus `9090`
- Alertmanager `9093`
- InfluxDB `8086`
- Loki `3100`
- Tempo `3200`
- OTel `4317/4318/9411/9464`
- cAdvisor `8088`

当前本地容器名基线：

- `memochat-envoy-gateway`
- `memochat-redis`
- `memochat-postgres`
- `memochat-mongo`
- `memochat-rabbitmq`
- `memochat-redpanda`
- `memochat-minio`
- `memochat-qdrant`
- `memochat-neo4j`
- `memochat-ai-orchestrator`
- `memochat-prometheus`
- `memochat-alertmanager`
- `memochat-grafana`
- `memochat-loki`
- `memochat-tempo`
- `memochat-otel-collector`
- `memochat-influxdb`
- `memochat-cadvisor`

## 健康检查

使用定向检查：

```bash
docker exec memochat-redis redis-cli -a 123456 ping
docker exec memochat-postgres pg_isready -U memochat -d memo_pg
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select current_database(), current_user;"
docker exec memochat-mongo mongosh -u root -p 123456 --authenticationDatabase admin --quiet --eval "db.adminCommand({ ping: 1 })"
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
```

对于 HTTP 服务：

```bash
curl -fsS http://127.0.0.1:3000/api/health
curl -fsS http://127.0.0.1:80/health
curl -fsS http://127.0.0.1:9090/-/ready
curl -fsS http://127.0.0.1:3100/ready
curl -fsS http://127.0.0.1:3200/ready
curl -fsS http://127.0.0.1:6333/
curl -fsS http://127.0.0.1:7474/
curl -fsS http://127.0.0.1:8096/health
curl -fsS http://127.0.0.1:11434/api/tags  # only when Ollama is enabled
```

## 日志和根因

使用：

```bash
docker logs --tail 200 <container>
docker inspect <container>
```

分类故障：

- 容器未运行
- healthcheck 不健康
- 端口冲突
- 凭据错误
- volume/数据损坏
- 缺少 Docker network
- Docker 内部服务配置指向了主机端口
- MCP 启动超时或 host/port 错误

## 安全修复

- 日志显示瞬时启动失败时，只重启单个容器：
  `docker restart <container>`
- 配置改变时，只重建相关 compose 服务：
  `docker compose -f <compose-file> up -d <service>`
- 数据重置必须先停止，并且只删除已批准的具体 Docker volume 或主机数据路径。

## 报告

返回：

- 受影响容器
- 准确失败检查
- 根因
- 执行的命令
- 安全下一步
- 是否发现任何端口/配置漂移
