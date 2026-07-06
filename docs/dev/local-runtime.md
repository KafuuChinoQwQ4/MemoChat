# 本地运行时

## 基础设施

本地 Docker 栈在 `infra/deploy/local/docker-compose.yml`。主要服务包括：

| 服务 | 默认本地端口 | 作用 |
| --- | --- | --- |
| Envoy | `80`、`8443/tcp`、`8443/udp` | HTTP/HTTPS/HTTP3 边缘入口 |
| Redis | `6379` | 缓存、验证码、在线状态、AI 语义缓存等 |
| Postgres | `15432 -> 5432` | 账号、聊天、媒体、朋友圈等关系数据 |
| MongoDB | `27017` | 消息、朋友圈内容等文档数据 |
| MinIO | `9000`、`9001` | 媒体对象存储与控制台 |
| Redpanda | `19092`、`18082` | Kafka 兼容事件流 |
| RabbitMQ | `5672`、`15672` | 任务队列和管理界面 |
| Prometheus/Grafana/Loki/Tempo | `9090`、`3000`、`3100`、`3200` | 指标、日志和 trace |

端口是开发契约，除非任务明确要求，不要改 compose 发布端口。

## 服务启动

典型 Linux 本地运行：

```bash
source /root/.memochat-linux-env
tools/scripts/status/deploy_services.sh
tools/scripts/status/start-all-services.sh
```

停止：

```bash
tools/scripts/status/stop-all-services.sh
```

`start-all-services.sh` 默认会启动 Envoy 和核心 Docker 依赖，并从 `infra/Memo_ops/runtime/services` 启动服务进程。观测和更完整 AI/RAG 依赖需要时再启动更广的 Docker 栈。

## 客户端启动

QML 客户端：

```bash
tools/scripts/status/start-memochat-qml-wslg.sh --diagnose
tools/scripts/status/start-memochat-qml-wslg.sh
```

Web 前端：

```bash
tools/scripts/status/start-memochat-web.sh --diagnose
tools/scripts/status/start-memochat-web.sh --real
```

`--mock` 可绕过真实聊天长连接，适合只调 UI；`--real` 需要后端聊天入口可用。
