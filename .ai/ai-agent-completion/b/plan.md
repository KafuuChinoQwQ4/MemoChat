# Plan

Assessed: yes

Task summary:
- Make AI Agent harness layers easier to find and reduce orchestration coupling without changing API contracts incompatibly.

Approach:
1. Add a harness layer catalog and expose it through `/agent/layers`.
2. Add lightweight Protocol ports for orchestration dependencies and update `AgentHarnessService` annotations to depend on ports.
3. Add persistent trace fallback read for `/agent/runs/{trace_id}`.
4. Add README files at the harness root and each layer folder.
5. Add a small Python unit test for catalog/skill/trace hydration logic.
6. Verify with Python tests, compileall, API smoke where current runtime allows, and diff review.

Files to modify/create:
- `apps/server/core/AIOrchestrator/harness/layers.py`
- `apps/server/core/AIOrchestrator/harness/ports.py`
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/server/core/AIOrchestrator/harness/feedback/trace_store.py`
- `apps/server/core/AIOrchestrator/api/agent_router.py`
- `apps/server/core/AIOrchestrator/schemas/api.py`
- `apps/server/core/AIOrchestrator/harness/**/README.md`
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`

Status checklist:
- [x] Gather context
- [x] Assess plan against code
- [x] Add layer catalog and docs
- [x] Decouple orchestration dependency annotations
- [x] Add persisted trace readback
- [x] Add focused tests
- [x] Run verification
- [x] Review diff
