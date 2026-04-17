# Kafka 本地开发指南

## 启动 Broker

```powershell
docker compose -f .\deploy\local\compose\kafka.yml up -d
```

## 创建 Topics

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\init_kafka_topics.ps1
```

扩展 Topic 现在还包括：

- `dialog.sync.v1`
- `relation.state.v1`
- `user.profile.changed.v1`
- `audit.login.v1`

## ChatServer 配置

- `Runtime.AsyncEventBus=kafka`
- `Runtime.Role=hybrid` 用于本地单机器验证
- `Kafka.Brokers=127.0.0.1:19092`
- `Kafka.ConsumerGroup=memochat-chat-workers`

## 验证聊天流程

```powershell
python .\scripts\verify_kafka_chat_flow.py
```

验证通过的条件是私聊和群聊都完成了以下步骤：

- 响应状态 `accepted`
- 异步状态通知 `persisted`
- 接收方通知已送达
- 历史查询包含该消息

## 回滚

- 修改 `Runtime.AsyncEventBus=redis`
- 重启 ChatServer 节点
