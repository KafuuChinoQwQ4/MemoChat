# RabbitMQ 本地开发指南

> 当前版本：2026-04-26

RabbitMQ 是 MemoChat 的可靠任务总线，用于需要 ACK/NACK、重试、死信和补偿调度的场景。它不替代 Redpanda/Kafka 的事件流职责。

## 1. 启动

推荐随完整本地依赖一起启动：

```powershell
docker compose -f infra\deploy\local\docker-compose.yml up -d memochat-rabbitmq
```

也可以使用拆分 compose：

```powershell
docker compose -f infra\deploy\local\compose\rabbitmq.yml up -d
```

## 2. 默认地址

| 项 | 值 |
|----|----|
| AMQP | `127.0.0.1:5672` |
| 管理后台 | `http://127.0.0.1:15672` |
| 用户名 | `memochat` |
| 密码 | `123456` |
| vhost | `/` |

如果拆分 compose 文件和当前完整 compose 不一致，以当前运行容器和 `infra\deploy\local\docker-compose.yml` 为准。业务配置应使用 `memochat/123456`。

## 3. 初始化 topology

```powershell
powershell -ExecutionPolicy Bypass -File tools\scripts\init_rabbitmq_topology.ps1
```

建议初始化内容包括：

- direct exchange：`memochat.direct`
- dead letter exchange：`memochat.dlx`
- 验证码邮件队列：`verify.email.delivery.q`
- 验证码邮件重试队列：`verify.email.delivery.retry.q`
- 验证码邮件死信队列：`verify.email.delivery.dlq.q`
- 关系通知、离线补投、状态刷新、缓存失效等任务队列

## 4. 服务配置

相关配置文件：

```text
apps/server/core/ChatServer/config.ini
apps/server/core/GateServer/config.ini
apps/server/core/StatusServer/config.ini
apps/server/core/VarifyServer/config.ini
```

关键项：

```ini
[RabbitMQ]
Host=127.0.0.1
Port=5672
Username=memochat
Password=123456
VHost=/
ExchangeDirect=memochat.direct
ExchangeDlx=memochat.dlx
PrefetchCount=50
RetryDelayMs=5000
MaxRetries=5
```

`VarifyServer` 还包含验证码邮件相关队列：

```ini
VerifyDeliveryQueue=verify.email.delivery.q
VerifyDeliveryRetryQueue=verify.email.delivery.retry.q
VerifyDeliveryDlqQueue=verify.email.delivery.dlq.q
RetryRoutingKey=verify.email.delivery.retry
DlqRoutingKey=verify.email.delivery.dlq
```

## 5. 当前职责

| 场景 | 使用 RabbitMQ 的原因 |
|------|----------------------|
| 验证码邮件发送 | SMTP 不稳定，需要重试和死信 |
| 离线补投 | 在线投递失败后可延迟重试 |
| 关系通知 | 好友申请、通过、拒绝等可异步通知 |
| outbox 补偿 | DB 已落库但边效应失败时补偿 |
| 缓存失效 | 资料、关系、会话缓存变更通知 |

## 6. 验证

```powershell
docker exec memochat-rabbitmq rabbitmq-diagnostics -q ping
docker exec memochat-rabbitmq rabbitmqctl list_queues name messages consumers
```

管理后台：

```text
http://127.0.0.1:15672
```

## 7. 排障

| 问题 | 检查 |
|------|------|
| 连接失败 | 容器是否运行、端口 `5672` 是否 Docker 占用 |
| 认证失败 | 用户是否为 `memochat/123456` |
| 队列不存在 | 是否运行 `tools\scripts\init_rabbitmq_topology.ps1` |
| 消息堆积 | consumer 是否启动、prefetch 是否过大、业务异常是否进入 DLQ |
