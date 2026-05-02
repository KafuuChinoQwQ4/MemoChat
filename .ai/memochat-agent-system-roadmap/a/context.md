# Context

Task:
Continue building MemoChat AI Agent according to `.ai/memochat-agent-system-roadmap`.

Selected roadmap item:
- `stage-1-contracts`

Relevant files:
- `apps/server/core/AIOrchestrator/harness/contracts.py`
- `.ai/memochat-agent-system-roadmap/tasks.json`
- `.ai/memochat-agent-system-roadmap/prompt.md`
- `apps/server/core/AIOrchestrator/docs/harness_engineering_plan.md`
- `apps/server/core/AIOrchestrator/harness/README.md`

Current state:
- Existing contracts include `AgentSkill`, `PlanStep`, `ToolObservation`, `TraceEvent`, `AgentTrace`, `ProviderEndpoint`, `MemorySnapshot`, and `HarnessRunResult`.
- `AgentHarnessService` still uses the old linear path. Stage 1 must not change runtime behavior.
- The new contracts should be additive and safe for later Stage 2+ implementation.

Infrastructure:
- No Docker data mutation is needed for this stage.
- AIOrchestrator code should be verified with Python compileall.

Verification target:
- `python -m compileall apps/server/core/AIOrchestrator`

Open risks:
- Avoid overfitting contracts to one external framework.
- Keep API compatibility with current QML/AIServer responses.
