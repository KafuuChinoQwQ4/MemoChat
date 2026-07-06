# 基础设施与数据组件

本地基础设施配置位于 `infra/deploy/local/`。基础设施容器是开发事实来源，不在 Docker 外启动同类依赖。

## 本地 Docker 组件

| 组件 | 容器 | 职责 |
| --- | --- | --- |
| Envoy | `memochat-envoy-gateway` | 本地 HTTP/HTTPS/HTTP3 边缘入口 |
| Postgres | `memochat-postgres` | 关系数据，默认库 `memo_pg`，拆分服务另有独立库 |
| Redis | `memochat-redis` | 缓存、验证码、在线状态、AI 语义缓存 |
| MongoDB | `memochat-mongo` | 消息、朋友圈等文档数据 |
| MinIO | `memochat-minio` | 媒体对象存储 |
| Redpanda | `memochat-redpanda` | Kafka 兼容事件流 |
| RabbitMQ | `memochat-rabbitmq` | 必须处理的任务队列 |
| Prometheus/Grafana/Loki/Tempo/OTel/cAdvisor | 多容器 | 指标、日志、trace 和容器观测 |

## 数据所有权

服务数据库归属以 `apps/server/core/docs/database-ownership.md` 为准。当前 Media/Moments/Call 和账号域已经有独立数据库/角色；ChatServer、R18、AIServer 仍保留部分 legacy 共享路径。

## 运维入口

- 本地服务部署和启动：`tools/scripts/status/`
- Docker compose：`infra/deploy/local/docker-compose.yml`
- 可观测性配置：`infra/deploy/local/observability/` 和 `infra/observability/`
- K8s/Helm 相关配置：`infra/Memo_ops/k8s/`、`infra/Memo_ops/helm/`
