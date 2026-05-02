# Plan

Assessed: yes

Task summary:
Add project-owned AI Agent contracts as additive dataclasses in `harness/contracts.py` and mark the roadmap item status.

Approach:
- Add policy/helper contracts first so `AgentSpec` remains structured.
- Add the required roadmap contracts: `AgentSpec`, `ToolSpec`, `GuardrailResult`, `RunNode`, `RunGraph`, `ContextPack`, `AgentTask`.
- Add `to_dict()` helpers for future API serialization.
- Do not alter existing contracts or execution paths.

Files to modify:
- `apps/server/core/AIOrchestrator/harness/contracts.py`
- `.ai/memochat-agent-system-roadmap/tasks.json`

Checklist:
- [x] Mark `stage-0-stabilize-current-agent` complete and `stage-1-contracts` started.
- [x] Add additive contracts to `contracts.py`.
- [x] Run Python compile verification.
- [x] Mark roadmap task complete and record verification.
- [x] Review diff.
