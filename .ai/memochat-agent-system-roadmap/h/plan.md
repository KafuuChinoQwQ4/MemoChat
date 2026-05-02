# Stage 7 Plan: Replay And Regression Evals

Assessed: yes

## Summary

Create a small project-owned replay/eval service for AIOrchestrator. It should validate run traces by structure and behavior: final status, required layers/events, forbidden events/tools, required tools, and latency.

## Approach

- Keep implementation Python-side in AIOrchestrator first.
- Add a reusable eval service under `harness/evals`.
- Wire the service into `HarnessContainer`.
- Add Pydantic models and `/agent/evals` plus `/agent/evals/run`.
- Add unit tests using the existing fake harness services.

## Files

- Create `apps/server/core/AIOrchestrator/harness/evals/__init__.py`.
- Create `apps/server/core/AIOrchestrator/harness/evals/service.py`.
- Update `apps/server/core/AIOrchestrator/harness/runtime/container.py`.
- Update `apps/server/core/AIOrchestrator/api/agent_router.py`.
- Update `apps/server/core/AIOrchestrator/schemas/api.py`.
- Update `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`.
- Update `.ai/memochat-agent-system-roadmap/about.md`.
- Update `.ai/memochat-agent-system-roadmap/tasks.json`.

## Checklist

- [x] Gather context and verify symbols.
- [x] Write assessed plan.
- [x] Implement eval service and API schema.
- [x] Add API endpoints and container wiring.
- [x] Add unit tests for pass, blocked, and failing expectation cases.
- [x] Run focused Python verification.
- [x] Review diff and record results.
