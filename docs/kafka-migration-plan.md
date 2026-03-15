# MemoChat Kafka Migration Plan

## Current state

- `ChatServer` already has an async message path for private and group chat.
- `PrivateMessageService` can run in `shadow` or `primary` async mode.
- `AsyncEventDispatcher` is now decoupled from Redis queue operations through `IAsyncEventBus`.
- The current runtime backend is still `redis`, configured by `Runtime.AsyncEventBus`.

## Phase 1

- Keep `RedisAsyncEventBus` as the production-safe baseline.
- Verify that private and group async flows still work with the new abstraction.
- Add metrics around publish failure, consume lag, and handler latency.

## Phase 2

- Add `KafkaAsyncEventBus` implementing `IAsyncEventBus`.
- Keep `AsyncEventDispatcher`, `PrivateMessageService`, and `GroupMessageService` unchanged at the business layer.
- Select backend by config:
  - `Runtime.AsyncEventBus=redis`
  - `Runtime.AsyncEventBus=kafka`

## Phase 3

- Move `relation.friend_apply` and `relation.friend_auth` into event workers.
- Add outbox-based delivery for message persistence and relation changes.
- Add retry and DLQ topics for poison messages.

## Topic plan

- `chat.private.v1`
- `chat.group.v1`
- `relation.friend_apply.v1`
- `relation.friend_auth.v1`

## Partition key plan

- Private chat: `min_uid:max_uid`
- Group chat: `group_id`
- Friend events: `touid`
