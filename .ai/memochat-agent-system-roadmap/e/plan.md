# Plan

Assessed: yes

Task summary:
Introduce structured ContextPack construction and first server-side visible memory APIs.

Approach:
- Add `context_pack` to `MemorySnapshot`.
- Build `ContextPack` sections for short-term chat, episodic memory, semantic memory, and graph context.
- Add visible memory list/delete helpers in `MemoryService`.
- Add `/agent/memory` GET and DELETE endpoints.
- Add schema models for context/memory responses.
- Add focused tests for ContextPack construction and memory deletion helpers.

Files to modify:
- `apps/server/core/AIOrchestrator/harness/contracts.py`
- `apps/server/core/AIOrchestrator/harness/memory/service.py`
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/server/core/AIOrchestrator/api/agent_router.py`
- `apps/server/core/AIOrchestrator/schemas/api.py`
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`
- `.ai/memochat-agent-system-roadmap/tasks.json`

Checklist:
- [x] Select Stage 5 and mark it started.
- [x] Add ContextPack to MemorySnapshot and MemoryService.
- [x] Trace context pack metadata during runs.
- [x] Add visible memory list/delete APIs.
- [x] Add focused tests.
- [x] Run Python verification.
- [x] Mark roadmap task complete and record verification.
- [x] Review diff.
