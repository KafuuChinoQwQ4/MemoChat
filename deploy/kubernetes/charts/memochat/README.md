# MemoChat Helm Chart

This chart deploys:

- `GateServer` as a `Deployment`
- `StatusServer` as a `Deployment`
- `ChatServer` as a `StatefulSet`
- `VarifyServer` as a `Deployment`
- `Memo_ops` server and collector
- `otel-collector`

It assumes PostgreSQL, Redis, MongoDB, Kafka, RabbitMQ, MySQL, and LiveKit are external services.

## Build images

Examples:

```bash
docker build -f deploy/images/services/cpp-service.Dockerfile --build-arg TARGET=GateServer -t memochat/gateserver:latest .
docker build -f deploy/images/services/cpp-service.Dockerfile --build-arg TARGET=StatusServer -t memochat/statusserver:latest .
docker build -f deploy/images/services/cpp-service.Dockerfile --build-arg TARGET=ChatServer -t memochat/chatserver:latest .
docker build -f deploy/images/services/varifyserver.Dockerfile -t memochat/varifyserver:latest .
docker build -f deploy/images/services/memo-ops.Dockerfile -t memochat/memo-ops:latest .
```

## Install

```bash
helm upgrade --install memochat deploy/kubernetes/charts/memochat \
  -f deploy/kubernetes/charts/memochat/values/prod.yaml
```

## Notes

- `ChatServer` discovery uses `k8s-statefulset`.
- `GateServer` now exposes `/healthz` and `/readyz`.
- `VarifyServer` exposes HTTP health checks on a separate port.
- The chart uses hook Jobs for PostgreSQL schema bootstrap and Kafka/RabbitMQ topology initialization.
- `templates/` is grouped into `prod`, `ops`, `observability`, `bootstrap`, and `shared`.
