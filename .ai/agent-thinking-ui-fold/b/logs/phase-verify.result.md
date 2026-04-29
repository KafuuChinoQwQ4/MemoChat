Verification completed for follow-up.

Commands:
- `cmake --build --preset msvc2022-full`
  - Passed. Rebuilt QML resources and relinked `bin\Release\MemoChatQml.exe`.
- `rg -n "正在生成回复|statusLabel|自动展开|onIsStreamingChanged|anchors.topMargin" ...`
  - Confirmed "正在生成回复" only remains in `AgentMessageDelegate.qml`; `statusLabel` was removed from `AgentConversationPane.qml`.
- `git diff --check apps\client\desktop\MemoChat-qml\qml\agent\AgentConversationPane.qml apps\client\desktop\MemoChat-qml\qml\agent\AgentMessageDelegate.qml`
  - Passed with Git CRLF warnings only.
