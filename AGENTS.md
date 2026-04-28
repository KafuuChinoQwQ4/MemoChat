# MemoChat-Qml-Drogon Agent Instructions

## Project Rules

- All infrastructure containers are in Docker.
- Docker containers must keep stable published ports.
- Project work must use the Docker containers for databases, queues, object storage, observability, and AI/RAG dependencies.
- When changing code, checking MCP, or inspecting databases, first look for the relevant Docker container or configured MCP tool.
- Prefer downloading dependencies, caches, models, and large generated artifacts to `D:`.

## Skill-First Workflow

Before working on this project, read the project skills first:

1. Always read `skills/SKILL.md` for the main MemoChat task workflow.
2. Then read only the relevant focused skill files:
   - `skills/docker-diagnose.md` for Docker, fixed ports, health, or MCP startup issues.
   - `skills/db-migration.md` for Postgres, Redis, MongoDB, MinIO metadata, Neo4j, or Qdrant data changes.
   - `skills/runtime-smoke.md` for deploy/start scripts, service smoke tests, login/register/full-flow checks.
   - `skills/observability.md` for Prometheus, Loki, Tempo, Grafana, InfluxDB, cAdvisor, logs, metrics, and traces.
   - `skills/ai-rag.md` for AIOrchestrator, Ollama, Qdrant, Neo4j, MCP bridge, knowledge base, and RAG work.
   - `skills/qml-ui.md` for MemoChat QML, MemoOps QML, icons, resources, and client UI checks.
   - `skills/task.md` for normal implementation workflow.
   - `skills/withtest.md` for implementation plus iterative runtime testing.
   - `skills/planner.md` for reusable `.ai/<name>/prompt.md` and `tasks.json` automation plans.
   - `skills/reflect.md` for learning from user corrections.
   - `skills/release.md` for release preparation and verification.
   - `skills/icon.md` for SVG/icon asset work.
3. Use `skills/PROMPTS.md` only when constructing phase prompts for delegated or artifact-based work.

Keep skill usage selective: do not load every skill file unless the task genuinely spans all of them.
