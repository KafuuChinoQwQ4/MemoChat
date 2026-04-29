Verification completed 2026-04-29T08:12:26.0975572-07:00.

Commands:
- `cmake --build --preset msvc2022-full`
  - Passed. Rebuilt QML resource and relinked `bin\Release\MemoChatQml.exe`.
- `git diff --check apps\client\desktop\MemoChat-qml\qml\agent\AgentConversationPane.qml apps\client\desktop\MemoChat-qml\qml\agent\AgentMessageDelegate.qml .ai\agent-thinking-ui-fold`
  - Passed with Git line-ending warnings only.

Manual visual expectation:
- The top-right "正在生成回复" badge no longer overlaps messages because the ListView reserves top space while loading/streaming.
- Thinking is shown expanded while streaming and collapses to a rounded "思考过程" header once the assistant message is finalized.
