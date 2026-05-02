# Context

Follow-up bug report:
- Switching account from the settings/more page often does not respond on the first click.
- The AI page does not show the conversation correctly on first entry; switching to another page and back makes it appear.

Relevant existing changes from task `a`:
- `AgentController::ensureInitialized()` was added and called from `ChatShellPage.qml` and `ChatLeftPanel.qml` Loader completion paths.
- That solved one async Loader timing path, but the controller still keeps user-scoped AI state across logout/account switches.

Relevant files:
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`
- `apps/client/desktop/MemoChat-qml/AgentController.h`
- `apps/client/desktop/MemoChat-qml/AppController.cpp`
- `apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentConversationPane.qml`
- `apps/client/desktop/MemoChat-qml/qml/chat/ChatLeftPanel.qml`
- `apps/client/desktop/MemoChat-qml/qml/Main.qml`

Findings:
- `AgentController::ensureInitialized()` is keyed only by uid. It is not reset during `AppController::switchToLogin()`, so same-user relogin or fast account switching can skip the initial AI load.
- `AgentController` keeps `_sessions`, `_current_session_id`, model messages, busy flags, pending requests, knowledge state, trace state, and stream state through logout.
- Session list handling only loads history when `_selectNewestSessionAfterList` is true. A normal first AI entry can receive sessions without selecting/loading a conversation.
- `ChatShellPage.switchAccountToLogin()` uses `Qt.callLater()`, adding an unnecessary deferred hop while the source Loader/window may be about to hide.

Infrastructure:
- No Docker/MCP data checks required; this is desktop client state/QML lifecycle only.

Verification target:
- `cmake --preset msvc2022-client-verify`
- `cmake --build --preset msvc2022-client-verify`
