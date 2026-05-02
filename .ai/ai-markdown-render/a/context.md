# Context

Task: add Markdown rendering when AI outputs Markdown.

Relevant files:
- `apps/client/desktop/MemoChat-qml/AgentMessageModel.h`
- `apps/client/desktop/MemoChat-qml/AgentMessageModel.cpp`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentConversationPane.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentMessageDelegate.qml`
- `apps/client/desktop/MemoChat-qml/CMakeLists.txt`

Current behavior:
- AI/user messages are stored in `AgentMessageModel`.
- QML reads `content`, `streamingContent`, and renders `AgentMessageDelegate` with `TextEdit` using `Text.PlainText`.
- Thinking content remains plain text.

Approach:
- Keep raw content unchanged.
- Add a sanitized Markdown-to-rich-text helper in the client.
- Add a `renderedContent` model role for assistant display.
- Render only non-error assistant messages as `TextEdit.RichText`; keep user and error messages as plain text.

Dependencies:
- No Docker or database changes.
- No new third-party dependency in this pass.

Verification:
- `cmake --preset msvc2022-client-verify`
- `cmake --build --preset msvc2022-client-verify`
