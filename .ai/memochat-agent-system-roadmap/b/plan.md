# Plan

Assessed: yes

Task summary:
Expose a `RunGraph` snapshot for current linear AI Agent traces while preserving the existing trace endpoint and runtime path.

Approach:
- Add Pydantic response models for `RunNode`, `RunGraph`, and graph endpoint response.
- Add a trace-store projection method that maps `AgentTrace.events` into a sequential graph.
- Add `GET /agent/runs/{trace_id}/graph` in `agent_router.py`.
- Add focused unit tests for graph projection.
- Mark roadmap Stage 2 complete after verification.

Files to modify:
- `apps/server/core/AIOrchestrator/harness/feedback/trace_store.py`
- `apps/server/core/AIOrchestrator/harness/ports.py`
- `apps/server/core/AIOrchestrator/api/agent_router.py`
- `apps/server/core/AIOrchestrator/schemas/api.py`
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`
- `.ai/memochat-agent-system-roadmap/tasks.json`

Checklist:
- [x] Select Stage 2 and mark it started.
- [x] Add RunGraph projection in trace store.
- [x] Add graph API schema and endpoint.
- [x] Add focused tests.
- [x] Run Python verification.
- [x] Mark roadmap task complete and record verification.
- [x] Review diff.
