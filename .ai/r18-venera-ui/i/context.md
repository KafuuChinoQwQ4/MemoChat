# Context

Task date: 2026-05-02.

User request: the R18/chat conversion buttons should be the same, specifically the conversion button from the chat interface.

Relevant code:
- `apps/client/desktop/MemoChat-qml/qml/chat/ChatSideBar.qml` defines the chat sidebar R18 conversion button with `qrc:/icons/r18.png`, 52px hit area, 28px icon, 110ms scale/color animation, and 120ms tooltip delay.
- `apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml` defines the R18 sidebar return button.

Decision:
- Keep the R18 button behavior as `root.toggleR18Mode()` so it returns to chat.
- Match its visible structure and icon to the chat sidebar conversion button.

No Docker services or data stores are touched.
