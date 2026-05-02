# Stage 8 Context: AgentSpec Templates

Task request: complete the next roadmap step.

Stage 8 goal: add reusable `AgentSpec` templates for researcher, writer, reviewer, support, and knowledge curator with model, tool, knowledge, memory, and guardrail policies.

Relevant files:
- `apps/server/core/AIOrchestrator/harness/contracts.py`: already defines `AgentSpec`, `ModelPolicy`, `ToolPolicy`, `KnowledgePolicy`, and `MemoryPolicy`.
- `apps/server/core/AIOrchestrator/harness/skills/registry.py`: current hard-coded skill registry and intent routing.
- `apps/server/core/AIOrchestrator/harness/orchestration/planner.py`: resolves skill, builds plan and prompts.
- `apps/server/core/AIOrchestrator/harness/runtime/container.py`: wires concrete harness services.
- `apps/server/core/AIOrchestrator/api/agent_router.py`: exposes harness management endpoints.
- `apps/server/core/AIOrchestrator/schemas/api.py`: Pydantic API models.
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`: focused harness tests.

Data and service dependencies:
- This stage is Python harness/API only.
- No database migrations, Docker port changes, Qdrant collection changes, Neo4j schema changes, or client bridge changes are required.

Docker/MCP checks:
- Not required for this narrow Python change.

Planned verification:
- `apps/server/core/AIOrchestrator/.venv/Scripts/python.exe -m py_compile harness/skills/specs.py harness/skills/registry.py harness/orchestration/planner.py harness/orchestration/agent_service.py harness/runtime/container.py api/agent_router.py schemas/api.py tests/test_harness_structure.py`
- `apps/server/core/AIOrchestrator/.venv/Scripts/python.exe -m unittest tests.test_harness_structure`
- `git diff --check`

Open risks:
- Stage 8 exposes policy templates and planner selection; full multi-agent orchestration is intentionally left for Stage 9.
