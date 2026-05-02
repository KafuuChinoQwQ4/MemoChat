# Stage 9 Plan: Multi-Agent Handoffs

Assessed: yes

## Summary

Add a lightweight multi-agent handoff layer on top of the existing single-agent harness. The first version should be deterministic, traceable, and reusable by API clients.

## Approach

- Add contracts for `AgentMessage`, `HandoffStep`, and `AgentFlow`.
- Add `harness/handoffs` with built-in flow templates and sequential run execution.
- Reuse `AgentHarnessService.run_turn` for each step so guardrails, memory, tools, traces, and model policy remain consistent.
- Return flow messages and a `RunGraph` handoff graph to make each agent transition inspectable.
- Expose additive `/agent/flows`, `/agent/flows/{flow_name}`, and `/agent/flows/run` APIs.
- Add Python unit tests for templates, graph shape, message handoff, and missing flow handling.

## Files

- Create `apps/server/core/AIOrchestrator/harness/handoffs/__init__.py`.
- Create `apps/server/core/AIOrchestrator/harness/handoffs/service.py`.
- Update `apps/server/core/AIOrchestrator/harness/contracts.py`.
- Update `apps/server/core/AIOrchestrator/harness/runtime/container.py`.
- Update `apps/server/core/AIOrchestrator/api/agent_router.py`.
- Update `apps/server/core/AIOrchestrator/schemas/api.py`.
- Update `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`.
- Update `.ai/memochat-agent-system-roadmap/about.md`.
- Update `.ai/memochat-agent-system-roadmap/tasks.json`.

## Checklist

- [x] Gather context and assess plan.
- [x] Implement contracts and handoff service.
- [x] Wire container and API schemas/endpoints.
- [x] Add unit tests.
- [x] Run focused Python verification.
- [x] Review diff and record results.
