# Context

Task date: 2026-05-02.

User request: when switching to the R18 face, keep a button that can return to the chat face at any time. Make the chat and R18 sidebars visually consistent by using the R18-style sidebar everywhere, and preserve the ability to drag the whole window by pressing the sidebar.

Relevant files:
- `apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml`
- `apps/client/desktop/MemoChat-qml/qml/chat/ChatSideBar.qml`
- `apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`
- `apps/client/desktop/MemoChat-qml/qml.qrc`

Implementation context:
- `ChatShellPage.qml` owns the normal chat face and the R18 face flip state through `r18Mode` and `toggleR18Mode()`.
- The R18 face already uses four left rail pages: 主页, 本地, 导航, 数据源.
- `ChatSideBar.qml` already contains `Window.window.startSystemMove()` for dragging the window from the normal sidebar.
- R18 content is now white acrylic/glass with gray/dark typography, so sidebar text/icon contrast should avoid the previous pink-heavy palette.

No Docker services, databases, queues, object storage, or MCP data stores are touched by this task.

Verification target:
- QML syntax/static check for touched QML files.
- Client build through `cmake --build --preset msvc2022-client-verify`.

Open risks:
- Runtime visual confirmation still depends on launching the desktop app and checking the actual acrylic rendering on the user's machine.
