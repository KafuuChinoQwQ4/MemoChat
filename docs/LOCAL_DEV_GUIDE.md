# MemoChat 本地开发环境启动指南

> 当前版本：2026-04-26
> 根目录：`<repo-root>`

本地开发采用“Windows 原生业务进程 + Docker 基础设施”模式。也就是说：GateServer、ChatServer、StatusServer、VarifyServer、MemoChatQml 等进程在 Windows 上运行；PostgreSQL、MongoDB、Redis、MinIO、Redpanda、RabbitMQ、Ollama、Qdrant、Neo4j 和可观测性组件都在 Docker 里运行。

不要在 Windows 本机或 WSL 中再单独安装/启动数据库服务。当前项目不使用 MySQL。

## 1. 前置条件

| 组件 | 要求 | 说明 |
|------|------|------|
| Windows | Windows 10/11 | 推荐 Windows 11 |
| Docker Desktop | 4.x+ | 所有数据库和基础设施依赖都通过 Docker 启动 |
| Visual Studio/MSVC | VS 2022 或当前 MSVC 工具链 | C++ 编译 |
| CMake | 3.29+ 推荐 | 顶层 CMake 工程 |
| Ninja | 推荐 | 当前构建日志多为 Ninja |
| vcpkg | 可跨平台，不是 Windows 专属 | Windows/Linux 都可用，triplet 不同 |
| Qt | 6.8+ | QML 客户端和 MemoOps |
| Git | 2.x+ | 版本管理 |
| Node.js | 仅部分脚本/兼容工具需要 | 当前 VarifyServer 主体已是 C++ 目标 |
| Python | 3.10+ 推荐 | AIOrchestrator、部分工具脚本和压测场景 |

## 2. 当前目录速查

| 内容 | 当前路径 |
|------|----------|
| C++ 服务端 | `apps/server/core` |
| Qt/QML 客户端 | `apps/client/desktop` |
| Docker Compose | `infra/deploy/local` |
| 本地运行时服务目录 | `infra/Memo_ops/runtime/services` |
| 本地服务日志 | `infra/Memo_ops/artifacts/logs/services` |
| 启停脚本 | `tools/scripts/status` |
| 初始化脚本 | `tools/scripts` |
| 压测工具 | `tools/loadtest/local-loadtest-cpp` |
| PostgreSQL 迁移 | `apps/server/migrations/postgresql` |

旧文档或旧命令里的 `server/`、`client/`、`deploy/`、`scripts/` 分别对应当前的 `apps/server/core/`、`apps/client/desktop/`、`infra/deploy/`、`tools/scripts/`。

## 3. 启动 Docker 基础设施

从仓库根目录运行：

```powershell
cd <repo-root>
docker compose -f infra\deploy\local\docker-compose.yml up -d
```

只启动数据存储时：

```powershell
docker compose -f infra\deploy\local\compose\datastores.yml up -d
```

常用组件也可以按需分组启动：

```powershell
docker compose -f infra\deploy\local\compose\kafka.yml up -d
docker compose -f infra\deploy\local\compose\rabbitmq.yml up -d
docker compose -f infra\deploy\local\compose\observability.yml up -d
```

当前推荐以 `infra\deploy\local\docker-compose.yml` 作为完整本地依赖入口，因为它和当前运行状态最接近。

## 4. 固定端口

| 服务 | 宿主机端口 | 容器/进程 |
|------|------------|-----------|
| GateServer HTTP | `8080` | Windows 进程 |
| GateServer HTTP/2 | `8082` | Windows 进程/可选目标 |
| VarifyServer gRPC/Health | `50051`, `8081` | Windows 进程 |
| StatusServer gRPC | `50052` | Windows 进程 |
| ChatServer TCP | `8090-8093` | Windows 进程 |
| ChatServer QUIC | `8190-8193` | Windows 进程 |
| ChatServer gRPC | `50055-50058` | Windows 进程 |
| PostgreSQL | `15432` | Docker `memochat-postgres` |
| Redis | `6379` | Docker `memochat-redis` |
| MongoDB | `27017` | Docker `memochat-mongo` |
| MinIO | `9000`, `9001` | Docker `memochat-minio` |
| Redpanda | `19092`, `18082` | Docker `memochat-redpanda` |
| RabbitMQ | `5672`, `15672` | Docker `memochat-rabbitmq` |
| Ollama | `11434` | Docker `memochat-ollama` |
| Qdrant | `6333`, `6334` | Docker `memochat-qdrant` |
| Neo4j | `7474`, `7687` | Docker `memochat-neo4j` |
| Grafana | `3000` | Docker `memochat-grafana` |
| Prometheus | `9090` | Docker `memochat-prometheus` |
| Loki | `3100` | Docker `memochat-loki` |
| Tempo | `3200` | Docker `memochat-tempo` |
| InfluxDB | `8086` | Docker `memochat-influxdb` |

PostgreSQL 的宿主机端口是 `15432`，不是 `5432`。`5432` 只表示容器内部端口或 Kubernetes 集群内部端口。

## 5. 检查 Docker 依赖

```powershell
docker ps --format "table {{.Names}}\t{{.Image}}\t{{.Ports}}\t{{.Status}}"
```

关键检查：

```powershell
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
docker exec memochat-redis redis-cli -a 123456 ping
docker exec memochat-mongo mongosh -u root -p 123456 --authenticationDatabase admin --quiet --eval "db.adminCommand({ ping: 1 })"
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
```

端口归属检查：

```powershell
netstat -ano | findstr /R /C:":15432 .*LISTENING" /C:":27017 .*LISTENING" /C:":6379 .*LISTENING" /C:":19092 .*LISTENING"
```

期望这些端口由 Docker Desktop 后端占用。WSL 里看到端口监听通常是 Docker Desktop 的转发，不代表 WSL 内运行了服务。

## 6. 初始化数据和队列

初始化脚本位于 `tools\scripts`：

```powershell
powershell -ExecutionPolicy Bypass -File tools\scripts\init_postgresql_schema.ps1
powershell -ExecutionPolicy Bypass -File tools\scripts\init_kafka_topics.ps1
powershell -ExecutionPolicy Bypass -File tools\scripts\init_rabbitmq_topology.ps1
```

PostgreSQL schema 的长期来源在：

```text
apps/server/migrations/postgresql/business
apps/server/migrations/postgresql/memo_ops
infra/deploy/local/init/postgresql
```

MongoDB 初始化脚本在：

```text
infra/deploy/local/init/mongo
```

## 7. 构建

常用构建方式：

```powershell
cmake --preset msvc2022-all
cmake --build --preset msvc2022-all-release
```

通用 Ninja 方式：

```powershell
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

可按需关闭部分目标：

```powershell
cmake -B build -S . -G Ninja -DBUILD_TESTS=OFF -DBUILD_LOADTEST=OFF
cmake --build build --config Release --target GateServer ChatServer StatusServer VarifyServer
```

## 8. 部署和启动本地服务

构建完成后，先把可执行文件和配置复制到运行时目录：

```powershell
tools\scripts\status\deploy_services.bat
```

启动所有本地服务窗口：

```powershell
tools\scripts\status\start-all-services.bat
```

该脚本会为 GateServer、StatusServer、ChatServer1-4、VarifyServer 等进程打开独立窗口，并把输出同步写入：

```text
infra/Memo_ops/artifacts/logs/services
```

停止所有本地服务：

```powershell
tools\scripts\status\stop-all-services.bat
```

## 9. 启动客户端

构建后的客户端通常在：

```text
build/bin/Release/MemoChatQml.exe
```

运行时部署目录中也可能存在：

```text
infra/Memo_ops/runtime/services/MemoChatQml/MemoChatQml.exe
```

PowerShell 运行绝对路径时要写完整盘符和反斜杠：

```powershell
& "<repo-root>\build\bin\Release\MemoChatQml.exe"
```

不要写成 `D:MemoChat...`，那是当前 D 盘工作目录下的相对路径。

## 10. 登录自测

GateServer 健康检查：

```powershell
Invoke-WebRequest -UseBasicParsing http://127.0.0.1:8080/healthz
```

登录接口：

```powershell
curl.exe --% -sS -H "Content-Type: application/json" -d "{\"email\":\"1220743777@qq.com\",\"passwd\":\"745230\",\"client_ver\":\"3.0.0\"}" http://127.0.0.1:8080/user_login
```

成功时响应包含：

- `error:0`
- `uid`
- `login_ticket`
- `host`
- `port`

## 11. 常见问题

### 11.1 登录提示网络错误

先确认 GateServer 是否活着：

```powershell
Invoke-WebRequest -UseBasicParsing http://127.0.0.1:8080/healthz
```

再确认业务服务进程：

```powershell
Get-Process GateServer,StatusServer,ChatServer,VarifyServer -ErrorAction SilentlyContinue
```

最后检查 Docker 数据库：

```powershell
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select count(*) from memo.\"user\";"
docker exec memochat-redis redis-cli -a 123456 DBSIZE
docker exec memochat-mongo mongosh "mongodb://memochat_app:123456@127.0.0.1:27017/memochat" --quiet --eval "db.getCollectionNames()"
```

### 11.2 Postgres 连接失败

当前宿主机端口是 `15432`：

```powershell
docker port memochat-postgres
netstat -ano | findstr ":15432"
```

如果配置里还写 `Port=5432`，需要改为 `15432`。Kubernetes values 或容器内部配置除外。

### 11.3 Docker Desktop 已启动但 WSL 里也看到端口

这是 Docker Desktop 的 WSL 转发，不代表 WSL 内跑了数据库。用下面命令区分：

```powershell
wsl -d Ubuntu-24.04 -- bash -lc "ps -ef | grep -Ei 'postgres|mongod|redis-server|qdrant|neo4j|redpanda|influx|rabbitmq|ollama' | grep -v grep || true"
wsl -d Ubuntu-24.04 -- bash -lc "dpkg -l | grep -Ei 'postgres|mongo|redis|mysql|qdrant|neo4j|redpanda|influx|rabbitmq|ollama' || true"
```

没有进程、包和 systemd 服务时，就不是 WSL 本机服务。

### 11.4 start-all-services 窗口没日志

当前脚本会通过 `tools\scripts\status\run-service-console.ps1` 拉起进程并同步输出。日志也会落盘到：

```text
infra/Memo_ops/artifacts/logs/services
```

如果窗口立即退出，优先看对应服务的 `_err.log` 和 `_out.log`。

### 11.5 Docker 容器启动后端口冲突

检查占用：

```powershell
netstat -ano | findstr "<port>"
Get-Process -Id <pid>
```

当前不应有 Windows/WSL 本机数据库占用 `5432`、`3306`、`27017`、`6379`。如果有，先停掉本机服务，再启动 Docker。

## 12. 本地数据和备份

Docker 数据目录：

```text
D:\docker-data\memochat
```

手动备份建议放：

```text
<repo-root>\backups
```

备份命令示例：

```powershell
docker exec memochat-postgres pg_dump -U memochat -d memo_pg -Fc > <repo-root>\backups\memo_pg.dump
docker exec memochat-mongo mongodump --archive --gzip --uri "mongodb://memochat_app:123456@127.0.0.1:27017/memochat" > <repo-root>\backups\memochat_mongo.archive.gz
docker exec memochat-redis redis-cli -a 123456 SAVE
```
