# Plan

Assessed: yes

Task summary:
Implement the first ToolSpec/permission layer for AI Agent tools without breaking existing chat/tool flows.

Approach:
- Add ToolSpec generation in `ToolExecutor`.
- Add parameter schema extraction from LangChain tool metadata.
- Extend request schemas with `confirmed_tools`.
- Validate explicit requested tools for existence and confirmation.
- Extend `/agent/tools` response model.
- Add focused tests for ToolSpec metadata and confirmation denial.

Files to modify:
- `apps/server/core/AIOrchestrator/harness/execution/tool_executor.py`
- `apps/server/core/AIOrchestrator/harness/ports.py`
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/server/core/AIOrchestrator/api/agent_router.py`
- `apps/server/core/AIOrchestrator/schemas/api.py`
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`
- `.ai/memochat-agent-system-roadmap/tasks.json`

Checklist:
- [x] Select Stage 3 and mark it started.
- [x] Add ToolSpec metadata and schema extraction.
- [x] Add explicit tool validation and confirmation checks.
- [x] Extend API schemas and router response.
- [x] Add focused tests.
- [x] Run Python verification.
- [x] Mark roadmap task complete and record verification.
- [x] Review diff.
