# Deploy Layout

This directory is split by deployment concern instead of by tool name.

## Structure

- `images/`
  - Container build inputs
  - `services/` contains service-specific Dockerfiles
  - `common/entrypoints/` contains shared runtime entrypoints
- `local/`
  - Local Docker Compose files for development dependencies
- `kubernetes/`
  - Kubernetes-facing assets
  - `charts/` contains Helm charts
  - Chart templates are grouped by subsystem

## Common Entry Points

Build images:

```bash
docker build -f deploy/images/services/cpp-service.Dockerfile --build-arg TARGET=GateServer -t memochat/gateserver:latest .
docker build -f deploy/images/services/cpp-service.Dockerfile --build-arg TARGET=StatusServer -t memochat/statusserver:latest .
docker build -f deploy/images/services/cpp-service.Dockerfile --build-arg TARGET=ChatServer -t memochat/chatserver:latest .
docker build -f deploy/images/services/varifyserver.Dockerfile -t memochat/varifyserver:latest .
docker build -f deploy/images/services/memo-ops.Dockerfile -t memochat/memo-ops:latest .
```

Install chart:

```bash
helm upgrade --install memochat deploy/kubernetes/charts/memochat \
  -f deploy/kubernetes/charts/memochat/values/prod.yaml
```

Start local dependency containers:

```bash
docker compose -f deploy/local/compose/kafka.yml up -d
docker compose -f deploy/local/compose/rabbitmq.yml up -d
docker compose -f deploy/local/compose/livekit.yml up -d
```
