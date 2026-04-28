# Kubernetes Assets

This folder contains Kubernetes deployment artifacts.

- `charts/memochat/`: MemoChat Helm chart
- `charts/memochat/templates/prod/`: Gate, Status, Chat, Varify, HPA
- `charts/memochat/templates/ops/`: Memo_ops resources
- `charts/memochat/templates/observability/`: collector resources
- `charts/memochat/templates/bootstrap/`: namespaces, config, secrets, init jobs

Use the chart README for install and values details:

- [memochat chart](./charts/memochat/README.md)
