# Context

Task:
Continue MemoChat AI Agent roadmap with `stage-4-guardrails`.

Relevant files:
- `apps/server/core/AIOrchestrator/harness/contracts.py`
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/server/core/AIOrchestrator/harness/execution/tool_executor.py`
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`
- `.ai/memochat-agent-system-roadmap/tasks.json`

Current state:
- Stage 1 added `GuardrailResult`.
- Stage 2 added `RunGraph` projection, so guardrail trace events will appear as graph nodes.
- Stage 3 added ToolSpec metadata, timeout wrapping, and explicit tool confirmation/argument blocking as `ToolObservation.metadata.status = "blocked"`.
- `AgentHarnessService` currently records plan, memory, execution, model, save, and feedback trace events.

Implementation decision:
- Add a small `harness/guardrails` service.
- Input guardrails run after trace start and before memory/tool/model work.
- Tool guardrails convert Stage 3 blocked tool observations into `guardrail` trace events and stop the run before model completion.
- Output guardrails run after model completion and before memory save/feedback.
- Keep API response models compatible by returning blocked messages through existing `HarnessRunResult` and trace events.

Infrastructure:
- No Docker data mutation is needed.
- No stable Docker ports or runtime service configs are changed.

Verification target:
- Python compile for touched AIOrchestrator modules.
- `python -m unittest tests.test_harness_structure` in the AIOrchestrator `.venv`.

Open risks:
- Streaming output guardrail runs after accumulation, so already-sent chunks cannot be recalled in this stage.
- Guardrail rule set is intentionally conservative and can be expanded in Stage 7 evals.
