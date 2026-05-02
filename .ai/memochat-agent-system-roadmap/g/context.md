# Stage 6 AgentTask Durable Execution Context

Task: continue MemoChat AI Agent evolution by adding the first durable AgentTask lifecycle.

Current state:
- `harness/contracts.py` already defines `AgentTask`, but it is not wired to storage or API.
- `AgentHarnessService.run_turn(request)` is the synchronous single-turn execution path to reuse for task execution.
- `agent_router.py` exposes `/agent/run`, traces, run graph, memory, layers, skills, tools, and feedback.
- Desktop client reaches AIOrchestrator through Gate `/ai/...` routes and AIServer gRPC.
- Stage 5 added visible memory bridge and QML panel.

Implementation direction:
- Add an AIOrchestrator-local task service that persists JSON task snapshots under `apps/server/core/AIOrchestrator/.runtime/agent_tasks`.
- Add `/agent/tasks` create/list/get/cancel/resume endpoints.
- Keep this stage independent of Docker port and database schema changes.
- Later stages can replace the JSON store with Postgres + queue workers while preserving API shape.

Relevant files:
- `apps/server/core/AIOrchestrator/harness/contracts.py`
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/server/core/AIOrchestrator/harness/runtime/container.py`
- `apps/server/core/AIOrchestrator/api/agent_router.py`
- `apps/server/core/AIOrchestrator/schemas/api.py`
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`

Docker/MCP:
- No Docker services or ports changed for this MVP.
- No database mutation required.

Verification planned:
- `.venv\Scripts\python.exe -m py_compile ...`
- `.venv\Scripts\python.exe -m unittest tests.test_harness_structure`
- If C++/QML bridge is added, run server/client CMake verify builds.
