# Plan

Assessed: yes

Task summary:
Fix account-switch and first AI-page render lifecycle bugs by making AI state reset on logout and making session list responses select/load an initial conversation deterministically.

Approach:
- Add a controller reset API used by `AppController::switchToLogin()`.
- Clear user-scoped AI state and abort streams/pending requests during logout.
- After a session list response, select a valid current session or the first returned session and load history when needed.
- Make switch-account QML call `controller.switchToLogin()` directly.

Files to modify:
- `apps/client/desktop/MemoChat-qml/AgentController.h`
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`
- `apps/client/desktop/MemoChat-qml/AppController.cpp`
- `apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml`

Checklist:
- [x] Add AI reset method and call it during account switch/logout.
- [x] Auto-select/load AI session history after first session list.
- [x] Remove deferred switch-account call.
- [x] Build client verify preset.
- [x] Review diff and record result.
