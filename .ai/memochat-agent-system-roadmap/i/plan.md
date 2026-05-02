# Stage 8 Plan: AgentSpec Templates

Assessed: yes

## Summary

Add project-owned `AgentSpec` templates and expose them through API. Specs should be reusable for single-agent runs now and form the base for future multi-agent handoffs.

## Approach

- Add `AgentSpecRegistry` with five built-in templates: researcher, writer, reviewer, support, and knowledge curator.
- Convert specs to `AgentSkill` objects so the existing planner and `AgentHarnessService` can run them without a new runtime.
- Let model policy defaults influence model call parameters when request fields do not override them.
- Expose `/agent/specs`, `/agent/specs/{spec_name}`, and `/agent/specs/reload`.
- Add tests for template list, skill resolution, planner actions, and model policy defaults.

## Files

- Create `apps/server/core/AIOrchestrator/harness/skills/specs.py`.
- Update `apps/server/core/AIOrchestrator/harness/contracts.py`.
- Update `apps/server/core/AIOrchestrator/harness/skills/registry.py`.
- Update `apps/server/core/AIOrchestrator/harness/orchestration/planner.py`.
- Update `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`.
- Update `apps/server/core/AIOrchestrator/harness/runtime/container.py`.
- Update `apps/server/core/AIOrchestrator/api/agent_router.py`.
- Update `apps/server/core/AIOrchestrator/schemas/api.py`.
- Update `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`.
- Update `.ai/memochat-agent-system-roadmap/about.md`.
- Update `.ai/memochat-agent-system-roadmap/tasks.json`.

## Checklist

- [x] Gather context and assess plan.
- [x] Implement spec registry and runtime wiring.
- [x] Add API schemas and endpoints.
- [x] Add tests.
- [x] Run focused Python verification.
- [x] Review diff and record results.
