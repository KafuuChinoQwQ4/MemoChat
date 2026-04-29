# Context

Task: move AI Agent work from source-level verification to runtime verification.

Relevant state:
- Source-level layer work from task `b` passed Python unit tests and handler smoke.
- Current `memochat-ai-orchestrator` container is running image `sha256:3510bff4...`, created on 2026-04-26.
- A newer local image `memochat-memochat-ai-orchestrator:latest` exists as `sha256:267452b...`, created on 2026-04-28.
- Compose service `memochat-ai-orchestrator` keeps host port `8096:8096` and connects to Ollama, Qdrant, Neo4j, and host Postgres via environment variables.
- `config.py` supports nested `MEMOCHAT_AI_*__*` environment overrides, including `MEMOCHAT_AI_LLM__OLLAMA__BASE_URL=http://memochat-ollama:11434`.

Relevant files:
- `apps/server/core/AIOrchestrator/docker-compose.yml`
- `apps/server/core/AIOrchestrator/Dockerfile`
- `apps/server/core/AIOrchestrator/config.py`
- `apps/server/core/AIOrchestrator/main.py`
- `apps/server/core/AIOrchestrator/api/agent_router.py`
- `apps/server/core/AIOrchestrator/harness/*`

Docker/runtime dependencies:
- AIOrchestrator `8096`
- Ollama `11434`
- Qdrant `6333/6334`
- Neo4j `7474/7687`
- Postgres host `15432`

Open risks:
- Recreating the AIOrchestrator container briefly interrupts port `8096`.
- Full `/agent/run` may still fail if Postgres trace tables are missing or if model/RAG startup pulls heavy local embedding dependencies.
