---
description: Create a reusable MemoChat automation plan with prompt.md and tasks.json under .ai/<name>/.
---

# MemoChat Planner

Use this to create a reusable automation package for repeated project work.

## Output

Create:

```text
.ai/<name>/
  prompt.md
  tasks.json
```

Use short kebab-case names. Check `.ai/` first to avoid collisions.

## prompt.md Must Include

- goal and scope
- relevant MemoChat modules
- Docker containers and fixed ports involved
- MCP tools or Docker commands to inspect state
- exact build/test/runtime commands
- data setup and cleanup rules
- failure handling, especially Windows file locks and Docker health
- commit guidance if the automation will commit

## tasks.json Format

```json
{
  "tasks": [
    {
      "id": "short-id",
      "title": "Short title",
      "description": "Concrete work to perform",
      "started": false,
      "completed": false,
      "dependencies": []
    }
  ]
}
```

## MemoChat Defaults

- Repository root: `D:\MemoChat-Qml-Drogon`.
- Downloads and large generated assets: prefer `D:`.
- Infrastructure: Docker only.
- Datastores: Redis `6379`, Postgres host `15432`, Mongo `27017`, MinIO `9000/9001`.
- Queues: Redpanda `19092/18082`, RabbitMQ `5672/15672`.
- AI/RAG: AI Orchestrator `8096`, Ollama `11434`, Neo4j `7474/7687`, Qdrant `6333/6334`.
- Observability: Grafana `3000`, Prometheus `9090`, InfluxDB `8086`, Loki `3100`, Tempo `3200`, OTel `4317/4318/9411/9464`, cAdvisor `8088`.
- Build/test preset: `msvc2022-full`, which writes to `build\bin\Release` for `deploy_services.bat`.

## Validation

After writing the files, read them once and verify:

- every task is actionable
- dependencies are acyclic
- commands match actual repo paths
- Docker/MCP assumptions are explicit
- `.ai/` files are not included in any commit instructions
