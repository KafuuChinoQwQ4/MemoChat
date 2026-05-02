# Stage 10 Plan: A2A-Ready Interoperability

Assessed: yes

## Summary

Add local Agent Card metadata and remote-agent placeholder registration so MemoChat can expose its local agent capabilities and safely catalog remote agents later.

## Approach

- Add additive `AgentCard`, `AgentCardSkill`, `AgentCapabilities`, and `RemoteAgentRef` contracts.
- Add `harness/interop` service that builds MemoChat's local Agent Card from AgentSpec and AgentFlow registries.
- Add in-memory remote agent registration/list/delete with disabled-by-default invocation placeholders.
- Expose `/agent/card`, `/agent/remote-agents`, and related endpoints.
- Add tests for local card shape, remote registration, duplicate update, and delete.

## Files

- Create `apps/server/core/AIOrchestrator/harness/interop/__init__.py`.
- Create `apps/server/core/AIOrchestrator/harness/interop/service.py`.
- Update `apps/server/core/AIOrchestrator/harness/contracts.py`.
- Update `apps/server/core/AIOrchestrator/harness/runtime/container.py`.
- Update `apps/server/core/AIOrchestrator/api/agent_router.py`.
- Update `apps/server/core/AIOrchestrator/schemas/api.py`.
- Update `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`.
- Update `.ai/memochat-agent-system-roadmap/about.md`.
- Update `.ai/memochat-agent-system-roadmap/tasks.json`.

## Checklist

- [x] Gather context and assess plan.
- [x] Implement interop contracts and service.
- [x] Wire container and API schemas/endpoints.
- [x] Add tests.
- [x] Run focused Python verification.
- [x] Review diff and record results.
