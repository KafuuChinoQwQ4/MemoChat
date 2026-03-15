# Local Runtime Assets

This folder contains local-only dependency orchestration files.

- `compose/kafka.yml`
- `compose/rabbitmq.yml`
- `compose/livekit.yml`

Examples:

```bash
docker compose -f deploy/local/compose/kafka.yml up -d
docker compose -f deploy/local/compose/rabbitmq.yml up -d
docker compose -f deploy/local/compose/livekit.yml up -d
```
