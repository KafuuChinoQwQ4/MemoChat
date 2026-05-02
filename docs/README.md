# MemoChat 文档索引

> 当前文档基准日期：2026-04-26
> 当前仓库根目录：`<repo-root>`

本文档目录用于说明 MemoChat 当前可运行架构。旧文档中如果仍出现 `server/`、`client/`、`deploy/`、`scripts/` 等根目录路径，应按当前目录映射理解：

| 旧路径 | 当前路径 | 说明 |
|--------|----------|------|
| `server/` | `apps/server/core/` | C++ 服务端核心服务 |
| `client/` | `apps/client/desktop/` | Qt/QML 桌面客户端 |
| `Memo_ops/` | `infra/Memo_ops/` | 运营平台与本地运行时目录 |
| `deploy/` | `infra/deploy/` | Docker Compose、Kubernetes、镜像配置 |
| `scripts/` | `tools/scripts/` | Windows/运维/初始化脚本 |
| `local-loadtest-cpp/` | `tools/loadtest/local-loadtest-cpp/` | C++ 压测工具 |

## 当前原则

- 本地依赖统一运行在 Docker 中，不在 Windows 本机或 WSL 中单独运行数据库和基础设施服务。
- Docker 容器必须固定占用宿主机端口，项目配置通过 `127.0.0.1:<端口>` 访问这些容器。
- Postgres 宿主机端口是 `15432`，容器内仍是 `5432`。文档中单独写 `5432` 时通常表示容器内端口或 Kubernetes 集群内部端口。
- 当前本地开发不使用 MySQL。用户、关系、群组、会话索引和运营数据都落在 PostgreSQL。
- MongoDB 保存消息正文和动态内容，Redis 保存临时状态、验证码、在线路由、缓存和部分 Pub/Sub。
- Redpanda 作为 Kafka 兼容事件总线，RabbitMQ 作为可靠任务和重试队列。
- MinIO 提供本地 S3 兼容对象存储，Qdrant/Neo4j/Ollama 支撑 AI/RAG/图谱能力。

## 推荐阅读顺序

1. [当前架构基准](当前架构基准.md)：先读这个，里面是当前目录、端口、服务和数据流的事实来源。
2. [本地开发环境启动指南](LOCAL_DEV_GUIDE.md)：本机开发、Docker-only 依赖、服务启动和排障。
3. [部署指南](部署指南.md)：本地 Docker、Windows 原生服务、Kubernetes/生产部署边界。
4. [架构文档](架构文档.md)：组件级架构、客户端/服务端模块说明。
5. [数据库架构](数据库架构.md)：PostgreSQL/MongoDB/Redis 的职责划分、表/集合/Key 设计。
6. [消息架构](messaging-architecture.md)：TCP/QUIC、gRPC、Kafka/Redpanda、RabbitMQ 与 Redis 路由。
7. [AI Agent 端到端测试指南](ai_agent_e2e_test.md)：AIOrchestrator、Ollama、Qdrant、Neo4j 相关测试。

## 当前本地固定端口

| 类别 | 服务 | 容器/进程 | 宿主机端口 |
|------|------|-----------|------------|
| 网关 | GateServer | Windows 原生进程 | `8080` |
| 网关 HTTP/2 | GateServer HTTP/2 | Windows 原生进程或可选目标 | `8082` |
| 验证码 | VarifyServer | Windows 原生进程 | `50051`, `8081` |
| 状态服务 | StatusServer | Windows 原生进程 | `50052` |
| 聊天 TCP | ChatServer 1-4 | Windows 原生进程 | `8090-8093` |
| 聊天 QUIC | ChatServer 1-4 | Windows 原生进程 | `8190-8193` |
| Chat gRPC | ChatServer 1-4 | Windows 原生进程 | `50055-50058` |
| 数据库 | PostgreSQL | `memochat-postgres` | `15432` |
| 数据库 | MongoDB | `memochat-mongo` | `27017` |
| 缓存 | Redis | `memochat-redis` | `6379` |
| 对象存储 | MinIO | `memochat-minio` | `9000`, `9001` |
| 事件总线 | Redpanda | `memochat-redpanda` | `19092`, `18082` |
| 任务队列 | RabbitMQ | `memochat-rabbitmq` | `5672`, `15672` |
| AI LLM | Ollama | `memochat-ollama` | `11434` |
| 向量库 | Qdrant | `memochat-qdrant` | `6333`, `6334` |
| 图数据库 | Neo4j | `memochat-neo4j` | `7474`, `7687` |
| 指标 | Prometheus | `memochat-prometheus` | `9090` |
| 指标长期存储 | InfluxDB | `memochat-influxdb` | `8086` |
| 可视化 | Grafana | `memochat-grafana` | `3000` |
| 日志 | Loki | `memochat-loki` | `3100` |
| 链路追踪 | Tempo | `memochat-tempo` | `3200` |
| 遥测 Collector | OTel Collector | `memochat-otel-collector` | `4317`, `4318`, `9411`, `9464` |

## 文档维护规则

- 新文档优先引用当前路径，不再新增根目录 `server/`、`client/`、`deploy/`、`scripts/`。
- 新本地运行说明必须默认 Docker-only 依赖，不写“本机安装 Postgres/Mongo/Redis/MySQL”。
- 新数据库说明禁止把 MySQL 写入当前架构。历史迁移脚本可以提到，但必须标注为历史兼容或归档用途。
- 本地 Postgres 连接端口统一写 `15432`；Kubernetes 或容器内部通信才写 `5432`。
- 下载、备份和持久化目录优先写到 `D:`，例如 `D:\docker-data\memochat` 和 `<repo-root>\backups`。
