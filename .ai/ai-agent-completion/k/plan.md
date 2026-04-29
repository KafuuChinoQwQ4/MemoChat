# Plan

Task summary: finish AI Agent delivery by making AIOrchestrator runtime config deterministic across rebuilds and completing Agent / KnowledgeBase UI state handling.

Approach:

- Make the AIOrchestrator config file source explicit via env-configurable config path plus compose-mounted runtime config, and tighten compose startup behavior with health-aware dependencies.
- Keep runtime model-data mount stable to avoid accidental Ollama cold-start or lost local models.
- Fix client-side Agent defaults and add explicit busy/error/status handling for model refresh and knowledge-base operations.
- Remove premature KB refresh calls from QML and let refresh follow confirmed backend completion.

Files to modify:

- `apps/server/core/AIOrchestrator/config.py`
- `apps/server/core/AIOrchestrator/docker-compose.yml`
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`
- `apps/client/desktop/MemoChat-qml/AgentController.h`
- `apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/KnowledgeBasePane.qml`
- `.ai/ai-agent-completion/k/*`

Implementation phases:

- [x] Create follow-up task artifacts and gather targeted context.
- [x] Make AIOrchestrator runtime config source explicit and compose behavior stable across rebuilds.
- [x] Finish Agent / KnowledgeBase state propagation and UI exception handling.
- [x] Run targeted build/runtime verification.
- [x] Review diff and record results.

Docker / MCP checks required:

- Preserve published ports for `memochat-ai-orchestrator` `8096` and `memochat-ollama` `11434`.
- Validate compose rendering after changes before any container recreation.

Assessed: yes
