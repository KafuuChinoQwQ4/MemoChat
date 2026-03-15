# Messaging Architecture

MemoChat 现在采用双总线协同：

- Kafka 负责事实流和顺序流
- RabbitMQ 负责任务流和重试流
- Redis 继续负责在线态、缓存和快速路由

## Kafka

Kafka 当前承载：

- `chat.private.v1`
- `chat.group.v1`
- `dialog.sync.v1`
- `relation.state.v1`
- `user.profile.changed.v1`
- `audit.login.v1`

职责：

- `ChatServer` 私聊和群聊主消息流
- dialog runtime 更新投影
- 关系事实变更
- Gate/Status 登录和资料变更审计事件

## RabbitMQ

RabbitMQ 当前承载：

- `chat.delivery.retry`
- `chat.notify.offline`
- `relation.notify`
- `chat.outbox.relay.retry`
- `verify.email.delivery`
- `gate.cache.invalidate`
- `status.presence.refresh`

职责：

- `ChatServer` 消息投递失败重试和离线补投
- 关系通知任务
- outbox 修复调度
- `VarifyServer` 验证码邮件可靠投递
- `GateServer` 缓存失效任务
- `StatusServer` presence 修复任务

## 本地启动

Kafka:

```powershell
docker compose -f .\deploy\local\compose\kafka.yml up -d
powershell -ExecutionPolicy Bypass -File .\scripts\init_kafka_topics.ps1
```

RabbitMQ:

```powershell
docker compose -f .\deploy\local\compose\rabbitmq.yml up -d
powershell -ExecutionPolicy Bypass -File .\scripts\init_rabbitmq_topology.ps1
```

## 运行约束

- 聊天事实源不能回退到 RabbitMQ
- SMTP、补偿、离线补投不能挤进 Kafka 主消费路径
- 同步 HTTP/gRPC 控制面继续同步返回，异步副作用只做 best effort 发布
