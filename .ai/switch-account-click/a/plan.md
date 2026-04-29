# Plan

Task summary: Fix first-click no-op behavior for chat settings account switching.

Approach:
- Move the login page transition to the start of `AppController::switchToLogin()`.
- Force window sync when `switchToLogin()` is called while `_page` is already `LoginPage`.
- Defer the QML action until after click delivery using `Qt.callLater()`.

Files modified:
- apps/client/desktop/MemoChat-qml/AppController.cpp
- apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml

Verification:
- `cmake --preset msvc2022-client-verify`
- `cmake --build --preset msvc2022-client-verify`

Status:
- [x] Implementation complete
- [x] Client build verification
- [x] Review complete

Assessed: yes
