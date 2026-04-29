# Plan

Task summary:
Fix AI Agent output, `error=2` during questions, and session delete UX.

Approach:
- Use streaming send from the composer to avoid the normal 10 second HTTP timeout.
- Update the local streaming message regardless of server `msg_id`; the server id does not match the client placeholder id.
- Treat `RemoteHostClosedError` after useful SSE content as a normal stream close.
- Refresh sessions after stream completion and auto-select the newest session when a stream created one implicitly.
- Clear the current session and message model after deleting the selected session.
- Bind `sessions` into `AgentPane` so header/session display sees updated session data.

Files modified:
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`
- `apps/client/desktop/MemoChat-qml/AgentController.h`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`
- `apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml`

Checklist:
- [x] Inspect Docker AI dependencies and relevant code paths.
- [x] Implement client streaming/error/session fixes.
- [x] Run `git diff --check`.
- [ ] Build with CMake.
- [ ] Runtime smoke Gate/AIServer AI path if services can be started.
- [ ] Review final diff and record results.

Assessed: yes
