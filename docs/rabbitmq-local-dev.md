# RabbitMQ Local Dev

本地 RabbitMQ 任务总线使用 [rabbitmq.yml](/d:/MemoChat-Qml-Drogon/deploy/local/compose/rabbitmq.yml)。

启动：

```powershell
docker compose -f .\deploy\local\compose\rabbitmq.yml up -d
```

默认地址：

- AMQP: `127.0.0.1:5672`
- 管理后台: `http://127.0.0.1:15672`
- 用户名: `guest`
- 密码: `guest`

`ChatServer` 相关配置在 [config.ini](/d:/MemoChat-Qml-Drogon/server/ChatServer/config.ini)：

- `Runtime.TaskBus=rabbitmq`
- `RabbitMQ.Host=127.0.0.1`
- `RabbitMQ.Port=5672`

当前定位：

- Kafka 继续承载私聊/群聊主消息流
- RabbitMQ 承载异步任务、重试、离线补投、关系通知、outbox 补偿调度
- `VarifyServer` 验证码邮件改为 `verify.email.delivery`
- `StatusServer` 增加 `status.presence.refresh`
- `GateServer` 增加 `gate.cache.invalidate`

初始化 topology：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\init_rabbitmq_topology.ps1
```
