# Context

Task: improve AI Agent harness layering and completion while keeping coupling low.

Existing state:
- Harness code already lives under `apps/server/core/AIOrchestrator/harness`.
- Main layers already exist as folders: `orchestration`, `execution`, `feedback`, `memory`, `knowledge`, `llm`, `mcp`, `skills`, `runtime`.
- `AgentHarnessService` currently imports concrete implementation classes for planner, LLM registry, tool executor, memory service, trace store, and feedback evaluator.
- `/agent/runs/{trace_id}` only checks in-memory trace state, so persisted traces are not readable after process restart.
- There is no machine-readable layer catalog endpoint, and folders do not contain local README files for quick discovery.

Relevant files:
- `apps/server/core/AIOrchestrator/harness/contracts.py`
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/server/core/AIOrchestrator/harness/runtime/container.py`
- `apps/server/core/AIOrchestrator/harness/feedback/trace_store.py`
- `apps/server/core/AIOrchestrator/api/agent_router.py`
- `apps/server/core/AIOrchestrator/schemas/api.py`

Docker/MCP status:
- AI/RAG containers are running: AIOrchestrator `8096`, Ollama `11434`, Qdrant `6333/6334`, Neo4j `7474/7687`, Postgres `15432`.
- Previous task found running AIOrchestrator image is stale and rebuild is blocked by Docker Hub/network; source-level Python checks remain possible.

Open risks:
- This pass should avoid broad functional rewrites until existing client changes are merged or committed.
- Runtime image may not reflect source changes until Docker build succeeds.
