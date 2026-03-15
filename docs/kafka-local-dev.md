# Kafka Local Dev

## Start broker

```powershell
docker compose -f .\deploy\local\compose\kafka.yml up -d
```

## Create topics

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\init_kafka_topics.ps1
```

扩展 topic 现在还包括：

- `dialog.sync.v1`
- `relation.state.v1`
- `user.profile.changed.v1`
- `audit.login.v1`

## ChatServer config

- `Runtime.AsyncEventBus=kafka`
- `Runtime.Role=hybrid` for local single-machine validation
- `Kafka.Brokers=127.0.0.1:19092`
- `Kafka.ConsumerGroup=memochat-chat-workers`

## Verify chat flow

```powershell
python .\scripts\verify_kafka_chat_flow.py
```

The verification passes only if private and group chat both complete:

- response status `accepted`
- async status notify `persisted`
- receiver notify delivered
- history query contains the message

## Rollback

- Change `Runtime.AsyncEventBus=redis`
- Restart ChatServer nodes
