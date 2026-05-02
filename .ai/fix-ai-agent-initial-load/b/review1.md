# Review 1

Findings:
- No correctness blockers found in the edited lifecycle path.
- `AgentController::resetForLogout()` clears user-scoped AI state, aborts active streams, drops pending request tracking, resets `_initialized_uid`, and emits the relevant QML notifications only when state was present.
- `handleSessionRsp()` now deterministically selects a valid session and loads history on the first list response, so the AI page no longer depends on leaving and re-entering the tab to populate the conversation.
- `ChatShellPage.switchAccountToLogin()` now calls `controller.switchToLogin()` directly, removing a deferred callback from a Loader subtree that is about to be hidden.

Residual risk:
- There is still no automated UI runner for the exact click/first-render flow, so verification is build-level plus code review. Manual runtime check should cover: Settings -> 切换账号 first click, login, first AI tab entry with existing AI sessions, and same-account relogin.
