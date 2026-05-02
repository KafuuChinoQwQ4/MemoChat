# Stage 10 Context: A2A-Ready Interoperability

Task request: complete the next roadmap step.

Stage 10 goal: add `AgentCard` and remote-agent placeholder contracts so MemoChat can later participate in agent-to-agent ecosystems safely.

Relevant files:
- `apps/server/core/AIOrchestrator/harness/contracts.py`: additive harness contracts.
- `apps/server/core/AIOrchestrator/harness/skills/specs.py`: local `AgentSpec` templates that can become card skills.
- `apps/server/core/AIOrchestrator/harness/handoffs/service.py`: controlled local handoff flows.
- `apps/server/core/AIOrchestrator/harness/runtime/container.py`: concrete harness wiring.
- `apps/server/core/AIOrchestrator/api/agent_router.py`: harness management API.
- `apps/server/core/AIOrchestrator/schemas/api.py`: Pydantic API models.
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`: focused harness tests.

External reference checked:
- A2A public specification describes Agent Cards as metadata for identity, URL, capabilities, authentication, and skills. Stage 10 implements a compatible local metadata boundary, not full remote execution.

Data and service dependencies:
- This stage is Python harness/API only.
- No Docker port changes, database migrations, Qdrant collection changes, Neo4j schema changes, or client bridge changes are required.
- Remote agents are placeholders held in memory by the Python service for now.

Docker/MCP checks:
- Not required for this narrow Python change.

Planned verification:
- `apps/server/core/AIOrchestrator/.venv/Scripts/python.exe -m py_compile harness/interop/service.py harness/contracts.py harness/runtime/container.py api/agent_router.py schemas/api.py tests/test_harness_structure.py`
- `apps/server/core/AIOrchestrator/.venv/Scripts/python.exe -m unittest tests.test_harness_structure`
- `git diff --check`

Open risks:
- This is A2A-ready metadata and registry scaffolding, not a full A2A JSON-RPC task runner.
