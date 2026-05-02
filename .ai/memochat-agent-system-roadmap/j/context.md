# Stage 9 Context: Multi-Agent Handoffs

Task request: complete the next roadmap step.

Stage 9 goal: add internal `AgentMessage`, handoff nodes, and first controlled crew/flow templates after single-agent reliability is proven.

Relevant files:
- `apps/server/core/AIOrchestrator/harness/contracts.py`: owns additive harness contracts.
- `apps/server/core/AIOrchestrator/harness/skills/specs.py`: Stage 8 `AgentSpec` templates for researcher, writer, reviewer, support, and knowledge curator.
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`: existing single-agent run boundary that handoff flows can reuse.
- `apps/server/core/AIOrchestrator/harness/runtime/container.py`: wires concrete harness services.
- `apps/server/core/AIOrchestrator/api/agent_router.py`: exposes harness management endpoints.
- `apps/server/core/AIOrchestrator/schemas/api.py`: Pydantic API models.
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`: focused harness tests.

Data and service dependencies:
- This stage is Python harness/API only.
- It does not change Docker ports, databases, queues, Qdrant collections, Neo4j schema, or client bridge code.
- Flow execution reuses the existing single-agent service and trace store; no new persistence schema is introduced.

Docker/MCP checks:
- Not required for this narrow Python change.

Planned verification:
- `apps/server/core/AIOrchestrator/.venv/Scripts/python.exe -m py_compile harness/handoffs/service.py harness/contracts.py harness/runtime/container.py api/agent_router.py schemas/api.py tests/test_harness_structure.py`
- `apps/server/core/AIOrchestrator/.venv/Scripts/python.exe -m unittest tests.test_harness_structure`
- `git diff --check`

Open risks:
- Stage 9 implements controlled sequential handoffs. Parallel crews, persistent flow state, and external agent interoperability are intentionally deferred.
