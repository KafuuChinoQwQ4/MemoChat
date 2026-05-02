# Context

Task:
Continue MemoChat AI Agent roadmap with `stage-2-trace-tree-rungraph`.

Relevant files:
- `apps/server/core/AIOrchestrator/harness/contracts.py`
- `apps/server/core/AIOrchestrator/harness/feedback/trace_store.py`
- `apps/server/core/AIOrchestrator/harness/ports.py`
- `apps/server/core/AIOrchestrator/api/agent_router.py`
- `apps/server/core/AIOrchestrator/schemas/api.py`
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`
- `.ai/memochat-agent-system-roadmap/tasks.json`

Current state:
- Stage 1 added `RunNode` and `RunGraph` contracts.
- Existing trace API is `GET /agent/runs/{trace_id}` and returns `AgentTraceRsp`.
- `AgentTraceStore` stores traces in memory and can reload trace rows/events from Postgres when persistence is enabled.
- Current run execution is still linear and records `TraceEvent` entries for orchestration, memory, execution, model, feedback, and persistence layers.

Implementation decision:
- Project current `AgentTrace` data into a `RunGraph` snapshot on demand.
- Keep the old trace endpoint and persistence schema unchanged.
- Add `GET /agent/runs/{trace_id}/graph` for the new graph response.

Infrastructure:
- No Docker data mutation is needed.
- No fixed Docker ports or runtime service configs are changed.

Verification target:
- `python -m py_compile apps/server/core/AIOrchestrator/harness/contracts.py apps/server/core/AIOrchestrator/harness/feedback/trace_store.py apps/server/core/AIOrchestrator/harness/ports.py apps/server/core/AIOrchestrator/api/agent_router.py apps/server/core/AIOrchestrator/schemas/api.py`
- `python -m unittest apps.server.core.AIOrchestrator.tests.test_harness_structure`

Open risks:
- The graph is a projection, not a separate durable graph table yet.
- Status and edge semantics are intentionally simple until the execution engine becomes graph-native.
