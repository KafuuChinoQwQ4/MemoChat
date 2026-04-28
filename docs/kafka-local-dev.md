# Redpanda/Kafka 本地开发指南

> 当前版本：2026-04-26

MemoChat 本地使用 Redpanda 作为 Kafka 兼容事件总线。Redpanda 运行在 Docker 中，业务服务通过宿主机端口 `127.0.0.1:19092` 连接。

## 1. 启动

推荐随完整本地依赖一起启动：

```powershell
docker compose -f infra\deploy\local\docker-compose.yml up -d memochat-redpanda
```

也可以使用拆分 compose：

```powershell
docker compose -f infra\deploy\local\compose\kafka.yml up -d
```

## 2. 初始化 topics

```powershell
powershell -ExecutionPolicy Bypass -File tools\scripts\init_kafka_topics.ps1
```

当前 topic 包括：

- `chat.private.v1`
- `chat.group.v1`
- `chat.private.dlq.v1`
- `chat.group.dlq.v1`
- `dialog.sync.v1`
- `relation.state.v1`
- `user.profile.changed.v1`
- `audit.login.v1`

## 3. ChatServer 配置

配置文件：

```text
apps/server/core/ChatServer/config.ini
apps/server/core/ChatServer/chatserver1.ini
apps/server/core/ChatServer/chatserver2.ini
apps/server/core/ChatServer/chatserver3.ini
apps/server/core/ChatServer/chatserver4.ini
```

关键项：

```ini
[Runtime]
Role=hybrid
AsyncEventBus=kafka
TaskBus=rabbitmq

[Kafka]
Brokers=127.0.0.1:19092
ClientId=memochat-chatserver
ConsumerGroup=memochat-chat-workers
TopicPrivate=chat.private.v1
TopicGroup=chat.group.v1
TopicPrivateDlq=chat.private.dlq.v1
TopicGroupDlq=chat.group.dlq.v1
```

## 4. 验证

容器检查：

```powershell
docker ps | findstr memochat-redpanda
docker exec memochat-redpanda rpk cluster info --brokers 127.0.0.1:19092
```

Topic 检查：

```powershell
docker exec memochat-redpanda rpk topic list --brokers 127.0.0.1:19092
```

消息链路检查：

1. 客户端登录。
2. 发送私聊或群聊。
3. ChatServer 返回 accepted。
4. Redpanda consumer 持久化或触发异步边效应。
5. MongoDB/PostgreSQL 可查到消息正文/索引。
6. 接收方在线时通过 Redis 路由 + gRPC/TCP 推送。

## 5. 回滚和降级

本地可以把 `Runtime.AsyncEventBus` 改回 `redis` 作为降级验证，但当前默认目标是 Redpanda/Kafka。修改后必须重启 ChatServer 节点。

不要用 MySQL 做消息或事件存储；当前最终数据源是 PostgreSQL、MongoDB、Redis 和对象存储。
