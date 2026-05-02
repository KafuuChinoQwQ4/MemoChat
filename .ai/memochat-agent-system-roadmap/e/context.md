# Context

Task:
Continue MemoChat AI Agent roadmap with `stage-5-contextpack-memory`.

Relevant files:
- `apps/server/core/AIOrchestrator/harness/contracts.py`
- `apps/server/core/AIOrchestrator/harness/memory/service.py`
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/server/core/AIOrchestrator/api/agent_router.py`
- `apps/server/core/AIOrchestrator/schemas/api.py`
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`
- `apps/server/migrations/postgresql/business/004_ai_agent.sql`
- `.ai/memochat-agent-system-roadmap/tasks.json`

Current state:
- Stage 1 added `ContextPack` and `ContextSection`.
- `MemoryService.load()` currently returns short-term chat, episodic summaries, semantic profile, and optional graph context as `MemorySnapshot`.
- Prompt construction still reads fields directly from `MemorySnapshot`.
- Existing memory tables are `ai_episodic_memory` and `ai_semantic_memory`; no schema change is needed for a first visible-memory API.

Implementation decision:
- Add `context_pack` to `MemorySnapshot` as an additive field.
- Build a structured `ContextPack` during memory load.
- Record context pack section names, sources, and token estimates in memory trace events.
- Add server-side visible memory APIs under `/agent/memory` using existing Postgres tables.
- Support deleting episodic memory rows and semantic memory keys without changing schema.

Infrastructure:
- No Docker data mutation is needed.
- No stable Docker ports or runtime service configs are changed.

Verification target:
- Python compile for touched AIOrchestrator modules.
- `python -m unittest tests.test_harness_structure` in the AIOrchestrator `.venv`.

Open risks:
- QML memory viewer UI is not implemented in this pass.
- Graph memory deletion is not exposed yet because graph memory has separate Neo4j semantics.
