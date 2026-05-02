# Stage 7 Context: Replay And Regression Evals

Task request: continue the MemoChat AI Agent roadmap with the next step.

Stage 7 goal: add replay/eval infrastructure that verifies trace shape, guardrail behavior, tool calls, and latency without depending on exact model wording.

Relevant files:
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`: records guardrail, orchestration, memory, execution, and feedback trace events for each agent turn.
- `apps/server/core/AIOrchestrator/harness/feedback/trace_store.py`: stores and loads `AgentTrace` and projects traces into `RunGraph`.
- `apps/server/core/AIOrchestrator/harness/runtime/container.py`: wires harness services.
- `apps/server/core/AIOrchestrator/api/agent_router.py`: exposes harness APIs under `/agent`.
- `apps/server/core/AIOrchestrator/schemas/api.py`: Pydantic request/response contracts.
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`: existing Python harness structure tests and fake services.

Data and service dependencies:
- This stage adds Python-only eval service code. It does not change Docker ports, databases, queues, object storage, or vector/graph schemas.
- Evals can run against the existing in-memory or persisted trace store through the existing trace-store boundary.

Docker/MCP checks:
- No Docker or MCP query is required for this narrow Python harness change.

Planned verification:
- `apps/server/core/AIOrchestrator/.venv/Scripts/python.exe -m py_compile harness/evals/service.py api/agent_router.py schemas/api.py tests/test_harness_structure.py`
- `apps/server/core/AIOrchestrator/.venv/Scripts/python.exe -m unittest tests.test_harness_structure`
- `git diff --check`

Open risks:
- Live evals that call the real model can still fail due to provider availability; Stage 7 therefore focuses on structural assertions and supports trace replay separately from live run evaluation.
