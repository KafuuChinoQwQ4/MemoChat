Task: follow-up polish for AI conversation streaming UI.

User feedback:
- The "正在生成回复" control should not sit on the user side; it should belong to the AI message area.
- Thinking collapse/final-answer behavior still looked unchanged.

Relevant files:
- apps/client/desktop/MemoChat-qml/qml/agent/AgentConversationPane.qml
- apps/client/desktop/MemoChat-qml/qml/agent/AgentMessageDelegate.qml

Implementation:
- Removed the global floating "正在生成回复/处理中" badge from AgentConversationPane.
- Restored the message list top margin to the normal value.
- Kept the streaming status inside AgentMessageDelegate so it appears within the assistant message bubble.
- Added an `onIsStreamingChanged` reset so thinking content auto-collapses when streaming finishes.
- Changed the streaming label text inside the assistant bubble to "正在生成回复".

Verification:
- `cmake --build --preset msvc2022-full`
