# Stage 6 AgentTask Durable Execution Plan

Task summary:
- Add the first durable AgentTask lifecycle for long-running AI work.

Approach:
- Use additive Python contracts and FastAPI models.
- Store task snapshots as JSON files under `.runtime/agent_tasks`.
- Execute tasks in-process with `asyncio.create_task`, reusing `AgentHarnessService.run_turn`.
- Support list/get/cancel/resume endpoints.
- Keep database/queue migration for a later hardening phase.

Implementation phases:
- [x] Gather context and assess existing symbols.
- [x] Extend AgentTask contract and API schemas with checkpoints/result/error.
- [x] Add `AgentTaskService` for file persistence and background execution.
- [x] Register task service in `HarnessContainer`.
- [x] Add `/agent/tasks` endpoints.
- [x] Add unit tests for lifecycle and persistence.
- [ ] Verify Python compile/unit tests.
- [x] Python API stability verified.
- [x] Bridge AgentTask lifecycle through AIServer/Gate.
- [x] Add AgentController task state and QML task inbox.
- [x] Verify Python, server, and client builds.

Assessed: yes
