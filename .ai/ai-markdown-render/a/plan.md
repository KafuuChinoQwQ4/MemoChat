# Plan

Task summary: render AI Markdown in the desktop AI assistant message bubble.

Approach:
- Add a small C++ Markdown renderer that escapes raw text and emits Qt-supported rich text.
- Expose rendered assistant content through `AgentMessageModel`.
- Switch QML assistant message text to `RichText` only when appropriate.

Files:
- `apps/client/desktop/MemoChat-qml/MarkdownRenderer.h`
- `apps/client/desktop/MemoChat-qml/MarkdownRenderer.cpp`
- `apps/client/desktop/MemoChat-qml/AgentMessageModel.h`
- `apps/client/desktop/MemoChat-qml/AgentMessageModel.cpp`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentConversationPane.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentMessageDelegate.qml`
- `apps/client/desktop/MemoChat-qml/CMakeLists.txt`

Status:
- [x] Context gathered
- [x] Plan assessed against current files
- [x] Implement renderer and model role
- [x] Update QML delegate
- [x] Verify client build
- [x] Review diff

Assessed: yes
