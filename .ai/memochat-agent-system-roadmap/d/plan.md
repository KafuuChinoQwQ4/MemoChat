# Plan

Assessed: yes

Task summary:
Introduce first-class input/tool/output guardrail stages with trace-visible pass/warn/block results.

Approach:
- Add `harness/guardrails/service.py` with deterministic local checks.
- Add helper functions in `AgentHarnessService` to record guardrail trace events and return blocked results.
- Run input guardrails before memory/tools/model work.
- Run tool guardrails after tool execution and before model completion.
- Run output guardrails after model completion and before memory save.
- Add focused tests for pass trace, blocked input, blocked tool, and blocked output.

Files to modify/create:
- `apps/server/core/AIOrchestrator/harness/guardrails/__init__.py`
- `apps/server/core/AIOrchestrator/harness/guardrails/service.py`
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`
- `.ai/memochat-agent-system-roadmap/tasks.json`

Checklist:
- [x] Select Stage 4 and mark it started.
- [x] Add guardrail service.
- [x] Wire guardrail stages into non-stream run path.
- [x] Wire guardrail stages into stream path where possible.
- [x] Add focused tests.
- [x] Run Python verification.
- [x] Mark roadmap task complete and record verification.
- [x] Review diff.
