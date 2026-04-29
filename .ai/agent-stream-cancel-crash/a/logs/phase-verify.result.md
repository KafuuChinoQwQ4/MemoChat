Verification completed 2026-04-29T08:36:55.5002964-07:00.

Commands:
- `cmake --build --preset msvc2022-full`
  - Passed. Rebuilt `AgentController.cpp` and relinked `bin\Release\MemoChatQml.exe`.
- `git diff --check apps\client\desktop\MemoChat-qml\AgentController.cpp`
  - Passed with Git CRLF warning only.

Runtime expectation:
- Clicking stop while a stream is active should no longer crash, even if Qt emits `finished` synchronously from `abort()`.
- The current AI message is finalized immediately and keeps partial text, or shows "已停止生成" if no chunks arrived yet.
