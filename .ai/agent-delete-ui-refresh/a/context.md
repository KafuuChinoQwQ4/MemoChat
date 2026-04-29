Task: restyle the AI session delete confirmation UI to match the app's rounded glass style, and fix AI answers not appearing immediately after sending.

Relevant files:
- apps/client/desktop/MemoChat-qml/qml/chat/ChatLeftPanel.qml
- apps/client/desktop/MemoChat-qml/qml/agent/AgentConversationPane.qml
- apps/client/desktop/MemoChat-qml/AgentMessageModel.cpp/.h

Findings:
- The AI delete confirmation is a native Dialog with standardButtons, producing a square white dialog unlike the rest of the UI.
- AgentConversationPane only scrolls after count changes when currentSessionId is non-empty. For new/first streamed sessions, the session id may not yet be available.
- Streaming content changes do not change the ListView count, so growing answers can remain off screen until the view is recreated.
- AgentMessageModel updates rows by taking a pointer and then searching for the whole entry via QVector::indexOf. It works in common cases but is fragile and less direct than resolving the row by msgId.

Verification:
- Use git diff --check and cmake --build --preset msvc2022-full.
