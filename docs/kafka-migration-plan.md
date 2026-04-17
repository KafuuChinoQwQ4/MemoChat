# MemoChat Kafka 迁移计划

## 当前状态

- `ChatServer` 已经有一条私聊和群聊的异步消息路径。
- `PrivateMessageService` 可以运行在 `shadow` 或 `primary` 异步模式。
- `AsyncEventDispatcher` 现在通过 `IAsyncEventBus` 解耦了 Redis 队列操作。
- 当前的运行时后端仍然是 `redis`，通过 `Runtime.AsyncEventBus` 配置。

## 第一阶段

- 保持 `RedisAsyncEventBus` 作为生产安全基线。
- 验证私聊和群聊异步流程在新抽象层下仍能正常工作。
- 在发布失败、消费延迟和处理程序延迟周围添加指标。

## 第二阶段

- 添加实现 `IAsyncEventBus` 的 `KafkaAsyncEventBus`。
- 保持 `AsyncEventDispatcher`、`PrivateMessageService` 和 `GroupMessageService` 在业务层不变。
- 通过配置选择后端：
  - `Runtime.AsyncEventBus=redis`
  - `Runtime.AsyncEventBus=kafka`

## 第三阶段

- 将 `relation.friend_apply` 和 `relation.friend_auth` 移入事件工作器。
- 为消息持久化和关系变更添加基于 outbox 的投递。
- 为毒消息添加重试和 DLQ Topic。

## Topic 计划

- `chat.private.v1`
- `chat.group.v1`
- `relation.friend_apply.v1`
- `relation.friend_auth.v1`

## 分区键计划

- 私聊：`min_uid:max_uid`
- 群聊：`group_id`
- 好友事件：`touid`
