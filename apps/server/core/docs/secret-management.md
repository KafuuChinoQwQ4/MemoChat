# 密钥管理

本文档记录 MemoChat 后端服务的密钥配置与生产注入方式,覆盖登录票据签名、SMTP、
对象存储、音视频、AI 依赖、Docker 本地依赖和观测系统凭据。

## ChatAuth HmacSecret

登录票据(ChatLoginTicket)由 HMAC 签名,签名密钥配置在各服务 ini 的
`[ChatAuth] HmacSecret`。本地开发所有服务共用同一个众所周知的 dev 值
(`memochat-dev-chat-secret`),方便联调。

### 生产环境:环境变量注入

`IniConfig` 支持用环境变量覆盖任意 ini 键,覆盖键名由 `EnvKeyFor(section, key)` 生成,
规则是 `MEMOCHAT_<SECTION>_<KEY>`(大写、非字母数字转下划线)。因此 `[ChatAuth] HmacSecret`
的覆盖环境变量是:

```
MEMOCHAT_CHATAUTH_HMACSECRET
```

生产部署**必须**通过该环境变量注入强随机密钥(由 k8s Secret / 部署管线提供),
不得使用 ini 里的 dev 默认值。

Kubernetes chart 的 `secrets.chatAuthSecret` 默认留空,模板使用 Helm `required` 强制部署时显式
提供该值。

### 运行时防呆

服务启动读取 HmacSecret 时,会用 `memochat::auth::IsWellKnownDevHmacSecret()` 检测是否仍是
dev 默认值;若是,发出一次性 WARN 日志提示"set env MEMOCHAT_CHATAUTH_HMACSECRET"。
此防呆已接入 `AuthLoginSupport`(Account/Register/Login 系)与 `ChatSessionConfig`(ChatServer)
的密钥读取点。

## 注入约定

| 密钥 | 配置位置 | 生产环境变量 |
|------|----------|--------------|
| 登录票据签名 | `[ChatAuth] HmacSecret` | `MEMOCHAT_CHATAUTH_HMACSECRET` |
| Varify SMTP 用户 | `[Email] SMTPUser` | `MEMOCHAT_EMAIL_SMTPUSER` |
| Varify SMTP 授权码 | `[Email] SMTPPass` | `MEMOCHAT_EMAIL_SMTPPASS` |
| Varify SMTP 发件人 | `[Email] From` | `MEMOCHAT_EMAIL_FROM` |
| Media MinIO access key | `[MinIO] AccessKey` 或进程环境 | `MEMOCHAT_MINIO_ACCESSKEY` / `MINIO_ACCESS_KEY` |
| Media MinIO secret key | `[MinIO] SecretKey` 或进程环境 | `MEMOCHAT_MINIO_SECRETKEY` / `MINIO_SECRET_KEY` |
| Call LiveKit API key | `[Call] ApiKey` | `MEMOCHAT_CALL_APIKEY` |
| Call LiveKit API secret | `[Call] ApiSecret` | `MEMOCHAT_CALL_APISECRET` |
| R18 Picacg API key | 进程环境 | `MEMOCHAT_R18_PICACG_API_KEY` |
| R18 Picacg HMAC key | 进程环境 | `MEMOCHAT_R18_PICACG_HMAC_KEY` |
| AI Postgres 密码 | AI compose/env override | `MEMOCHAT_AI_POSTGRES__PASSWORD` |
| AI RabbitMQ 密码 | AI compose/env override | `MEMOCHAT_AI_AGENT_QUEUE__RABBITMQ__PASSWORD` |
| AI Redis 密码 | AI compose/env override | `MEMOCHAT_AI_SEMANTIC_CACHE__REDIS__PASSWORD` |
| AI Neo4j 密码 | AI compose/env override | `MEMOCHAT_AI_NEO4J__PASSWORD` |
| Grafana MCP admin 密码 | 开发工具进程环境 | `MEMOCHAT_GRAFANA_ADMIN_PASSWORD` |
| RabbitMQ MCP admin 密码 | 开发工具进程环境 | `MEMOCHAT_RABBITMQ_PASSWORD` |
| InfluxDB MCP token | 开发工具进程环境 | `MEMOCHAT_INFLUXDB_ADMIN_TOKEN` |
| MinIO MCP root secret | 开发工具进程环境 | `MEMOCHAT_MINIO_ROOT_PASSWORD` / `MEMOCHAT_MINIO_SECRETKEY` |
| Redis 压测密码 | 开发工具进程环境 | `MEMOCHAT_REDIS_PASSWORD` |
| Postgres 压测密码 | 开发工具进程环境 | `MEMOCHAT_POSTGRES_PASSWORD` |
| MongoDB 工具 URI/密码 | 开发工具进程环境 | `MEMOCHAT_MONGODB_URI` / `MEMOCHAT_MONGO_APP_PASSWORD` |
| Neo4j MCP 密码 | 开发工具进程环境 | `MEMOCHAT_NEO4J_PASSWORD` / `MEMOCHAT_AI_NEO4J__PASSWORD` |
| Phase2 服务库角色密码 | 本地迁移脚本进程环境 | `MEMOCHAT_PHASE2_SERVICE_ROLE_PASSWORD` / `MEMOCHAT_ACCOUNT_DB_ROLE_PASSWORD` |

## SMTP

`VarifyServer/config.ini` 和 `varify2.ini` 不提交真实 SMTP 用户授权码。默认 `SMTPUser`
与 `SMTPPass` 为空;未注入凭据时邮件发送快速失败并记录缺失字段,不打印凭据值。生产必须用
`MEMOCHAT_EMAIL_SMTPUSER`、`MEMOCHAT_EMAIL_SMTPPASS` 和 `MEMOCHAT_EMAIL_FROM`
注入。

## Media / Call

`MediaService/mediagateway.ini` 不提交可用 MinIO secret;MinIO 凭据通过
`MEMOCHAT_MINIO_ACCESSKEY` / `MEMOCHAT_MINIO_SECRETKEY` 或 `MINIO_ACCESS_KEY` /
`MINIO_SECRET_KEY` 注入。`CallService/callgateway.ini` 不提交可用 LiveKit secret;`ApiKey`
和 `ApiSecret` 必须通过 `MEMOCHAT_CALL_APIKEY` / `MEMOCHAT_CALL_APISECRET` 注入。

## R18 Picacg

Picacg adapter 不提交 API key 或 HMAC key。启用官方源时,通过
`MEMOCHAT_R18_PICACG_API_KEY` 和 `MEMOCHAT_R18_PICACG_HMAC_KEY` 注入;未注入时请求快速失败。

## Docker Local Defaults

`infra/deploy/local/**` 是本地 Docker 开发栈,可保留众所周知的 dev 默认值,但所有密码、token
和 API secret 必须使用 `${ENV:-dev_default}` 形式,保证生产/staging 能通过环境变量覆盖而不改
端口和 volume。不要把这些 local compose 默认值复制到生产部署。

## AI Orchestrator

`apps/server/core/AIOrchestrator/config.yaml` 不保存可用依赖密码;本地 Docker 由
`apps/server/core/AIOrchestrator/docker-compose.yml` 注入 dev 默认值。生产必须覆盖 AI 相关
`MEMOCHAT_AI_*__PASSWORD` 环境变量,尤其是 Postgres、RabbitMQ、Redis 和 Neo4j。

dev 值保留在 ini 中无妨(仅本地用);所有 ini 的 `HmacSecret=` 行附有注释指向上述环境变量。

## Developer Tools / MCP

`tools/mcps/**` 和 `tools/loadtest/python-loadtest/system_bench.py` 不提交可用基础设施凭据。
这些工具从调用进程环境读取本地 Docker 依赖凭据;缺少对应变量时快速失败并只提示变量名。

- Docker services MCP: `MEMOCHAT_GRAFANA_ADMIN_USER`、`MEMOCHAT_GRAFANA_ADMIN_PASSWORD`、
  `MEMOCHAT_RABBITMQ_USER`、`MEMOCHAT_RABBITMQ_PASSWORD`、`MEMOCHAT_INFLUXDB_ADMIN_TOKEN`、
  `MEMOCHAT_MINIO_ROOT_USER`、`MEMOCHAT_MINIO_ROOT_PASSWORD`。
- MongoDB MCP: 优先读取 `MEMOCHAT_MONGODB_URI` 或 `MEMOCHAT_MONGO_URI`;未提供完整 URI 时,
  使用 `MEMOCHAT_MONGO_APP_USER`、`MEMOCHAT_MONGO_APP_PASSWORD`、`MEMOCHAT_MONGO_HOST`、
  `MEMOCHAT_MONGO_PORT` 和 `MEMOCHAT_MONGO_DATABASE` 组装连接串。
- System bench: Redis/Postgres/Mongo 连接分别读取 `MEMOCHAT_REDIS_PASSWORD`、
  `MEMOCHAT_POSTGRES_PASSWORD`、`MEMOCHAT_MONGODB_URI` 或 `MEMOCHAT_MONGO_APP_PASSWORD`。
- Neo4j MCP: 连接地址和用户读取 `MEMOCHAT_NEO4J_URI`、`MEMOCHAT_NEO4J_USER`,密码读取
  `MEMOCHAT_NEO4J_PASSWORD` 或 AI 栈同名密码 `MEMOCHAT_AI_NEO4J__PASSWORD`。
- MinIO helper 和运行时 smoke: 读取 `MEMOCHAT_MINIO_ROOT_USER`、
  `MEMOCHAT_MINIO_ROOT_PASSWORD` 及其 `MEMOCHAT_MINIO_ACCESSKEY` /
  `MEMOCHAT_MINIO_SECRETKEY` 别名;不要在脚本源码里写入可用对象存储凭据。
- RabbitMQ topology 初始化和 Redis 清理/验证码脚本分别读取 `MEMOCHAT_RABBITMQ_PASSWORD`
  与 `MEMOCHAT_REDIS_PASSWORD`,缺失时失败。
- Phase2 数据库拆分脚本使用 `MEMOCHAT_PHASE2_SERVICE_ROLE_PASSWORD` 和
  `MEMOCHAT_ACCOUNT_DB_ROLE_PASSWORD` 注入新建服务角色密码;SQL 迁移文件不得保存角色密码。
