# Context

Task: 第一次点切换账号会没反应，第二次才能退出切换，修复 bug。

Relevant files:
- apps/client/desktop/MemoChat-qml/qml/chat/ChatMorePane.qml: account operation buttons emit `switchAccountRequested` and `logoutRequested`.
- apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml: handles those signals and calls controller logout/switch flow.
- apps/client/desktop/MemoChat-qml/qml/Main.qml: listens for `controller.pageChanged()` and syncs login/chat windows.
- apps/client/desktop/MemoChat-qml/AppController.cpp: `switchToLogin()` clears chat state and switches page.
- apps/client/desktop/MemoChat-qml/AppControllerSession.cpp: `onConnectionClosed()` ignores the expected disconnect when `_ignore_next_login_disconnect` is set.

Finding:
- The account buttons called `controller.switchToLogin()` directly from the button click handler inside an asynchronously loaded chat pane.
- `switchToLogin()` did several teardown calls before page/window sync, so the visible transition could be delayed or miss a boundary case where the login page was already set but the chat window remained visible.

Plan:
- [x] Make `switchToLogin()` switch/sync the page before teardown and force a `pageChanged` sync if already on LoginPage.
- [x] Defer the QML signal handler with `Qt.callLater()` so the MouseArea click finishes before the chat pane/window teardown begins.
- [ ] Verify with client build.
- [ ] Review diff and record result.
