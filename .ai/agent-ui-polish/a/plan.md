# Plan

Task summary:
Polish the AI Agent and R18 QML surfaces, fix AI GET endpoints in the client, and verify the local model list path returns `qwen3:4b`.

Approach:
- Match the existing private-chat bubble style for AI Agent messages.
- Replace AI Agent GET-like POST calls with real GET requests.
- Keep knowledge-base and model dialogs clipped and bounded by the parent.
- Reduce glass backdrop opacity and remove the R18 white haze.
- Prefer existing `GlassButton`, `GlassSurface`, and scrollbar components.

Files modified:
- `apps/client/desktop/MemoChatShared/httpmgr.h`
- `apps/client/desktop/MemoChatShared/httpmgr.cpp`
- `apps/client/desktop/MemoChat-qml/AgentController.h`
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`
- `apps/client/desktop/MemoChat-qml/AgentMessageModel.cpp`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentMessageDelegate.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentConversationPane.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/KnowledgeBasePane.qml`
- `apps/client/desktop/MemoChat-qml/qml/components/GlassBackdrop.qml`
- `apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml`
- `apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`
- `tools/scripts/status/start-all-services.bat`
- `tools/scripts/status/stop-all-services.bat`

Checklist:
- [x] Add shared HTTP GET support.
- [x] Route session/history/model/kb list through GET.
- [x] Default AI model to `qwen3:4b` and preserve server-provided model list.
- [x] Render Agent messages as left/right bubbles.
- [x] Prevent knowledge/model/trace panels from exceeding parent bounds.
- [x] Make base glass and R18 pink surfaces more transparent.
- [x] Replace R18 default white buttons/inputs with rounded glass styling.
- [x] Add AIServer to local start/stop scripts so Gate AI routes work after one-command startup.
- [x] Build `MemoChatQml`.
- [x] Start/check Docker AI dependencies.
- [x] Verify Gate model list returns `qwen3:4b`.

Assessed: yes
